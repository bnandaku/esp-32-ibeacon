/**
 * ESP32 iBeacon + Webhook Sender with OTA Support
 *
 * Combined firmware that broadcasts iBeacon signals and sends webhooks.
 *
 * Features:
 * - iBeacon broadcasting at 50ms intervals
 * - WiFi connectivity
 * - HTTPS webhook sending
 * - WiFi OTA (Over-The-Air) updates for remote firmware upgrades
 * - LED status indicators
 *
 * @version 3.1.0
 * @license MIT
 */

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_bt_main.h"
#include "esp_ibeacon_api.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "esp_mac.h"
#include "esp_crt_bundle.h"

// ============================================================================
// CONFIGURATION - Edit these values
// ============================================================================

// WiFi Configuration
#define WIFI_SSID      CONFIG_EXAMPLE_WIFI_SSID
#define WIFI_PASSWORD  CONFIG_EXAMPLE_WIFI_PASSWORD

// OTA Server URL - Where to check for firmware updates (optional)
// Example: "http://192.168.1.100:8080/firmware.bin"
#define OTA_UPDATE_URL CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL

// Webhook URL - Send HTTPS POST requests to this URL
// Change this to your webhook endpoint
#define WEBHOOK_URL "https://discord.com/api/webhooks/1470114757087334411/ZjD8kJmnlqKKyn4oOOm2zjOc233qqK87GsvckmmCmmCxXyis8s0mzxXndH2rQPOCwruB"

// Firmware Version
#define FIRMWARE_VERSION "3.1.0"

// LED Pin (GPIO 2 on most ESP32 dev boards)
#define LED_GPIO 2

// Webhook sending interval (in seconds)
// Set to 0 to send only once on startup
#define WEBHOOK_INTERVAL_SEC 0

// Your Beacon UUID - Just paste the UUID string here!
// Must match the UUID configured in your iOS app
// Format: XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX
#define BEACON_UUID_STRING "ED17A803-D1AC-4F04-A2F0-7802B4C9C70C"

// Default Major/Minor values (used only on first boot if NVS is empty)
#define DEFAULT_BEACON_MAJOR 100
#define DEFAULT_BEACON_MINOR 15

// Advertising interval in milliseconds (50-10000)
// 50ms = 20 broadcasts per second (very responsive)
#define ADVERTISING_INTERVAL_MS 50

// Transmit Power (ESP32 Power Levels)
// ESP_PWR_LVL_P3 = +3 dBm (~10m range) - Good balance for room detection
#define TRANSMIT_POWER ESP_PWR_LVL_P3

// ============================================================================
// END OF CONFIGURATION
// ============================================================================

static const char* TAG = "iBeacon";
static const char* OTA_TAG = "OTA";
static const char* WIFI_TAG = "WiFi";
static const char* NVS_TAG = "NVS";
extern esp_ble_ibeacon_vendor_t vendor_config;

// NVS namespace and keys for persistent storage
#define NVS_NAMESPACE "beacon_cfg"
#define NVS_KEY_MAJOR "major"
#define NVS_KEY_MINOR "minor"

// Actual beacon values (loaded from NVS on boot)
static uint16_t g_beacon_major = DEFAULT_BEACON_MAJOR;
static uint16_t g_beacon_minor = DEFAULT_BEACON_MINOR;

// OTA update check interval (in seconds)
#define OTA_CHECK_INTERVAL_SEC 300  // Check every 5 minutes

// WiFi connection event bits
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;
#define WIFI_MAXIMUM_RETRY 5

// Forward declarations
static void webhook_task(void *pvParameter);

/**
 * Load beacon configuration from NVS
 */
static bool load_beacon_config_from_nvs(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;

    ESP_LOGI(NVS_TAG, "Loading beacon configuration from NVS...");

    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGW(NVS_TAG, "NVS namespace not found, using defaults");
        return false;
    }

    // Try to read major
    err = nvs_get_u16(nvs_handle, NVS_KEY_MAJOR, &g_beacon_major);
    if (err != ESP_OK) {
        ESP_LOGW(NVS_TAG, "Major not found in NVS, using default: %d", DEFAULT_BEACON_MAJOR);
        g_beacon_major = DEFAULT_BEACON_MAJOR;
    } else {
        ESP_LOGI(NVS_TAG, "✓ Loaded Major from NVS: %d", g_beacon_major);
    }

    // Try to read minor
    err = nvs_get_u16(nvs_handle, NVS_KEY_MINOR, &g_beacon_minor);
    if (err != ESP_OK) {
        ESP_LOGW(NVS_TAG, "Minor not found in NVS, using default: %d", DEFAULT_BEACON_MINOR);
        g_beacon_minor = DEFAULT_BEACON_MINOR;
    } else {
        ESP_LOGI(NVS_TAG, "✓ Loaded Minor from NVS: %d", g_beacon_minor);
    }

    nvs_close(nvs_handle);
    return true;
}

/**
 * Save beacon configuration to NVS
 */
static esp_err_t save_beacon_config_to_nvs(uint16_t major, uint16_t minor)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;

    ESP_LOGI(NVS_TAG, "Saving beacon configuration to NVS...");

    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(NVS_TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_u16(nvs_handle, NVS_KEY_MAJOR, major);
    if (err != ESP_OK) {
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_set_u16(nvs_handle, NVS_KEY_MINOR, minor);
    if (err != ESP_OK) {
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_commit(nvs_handle);
    if (err == ESP_OK) {
        ESP_LOGI(NVS_TAG, "✓ Beacon configuration saved to NVS");
        g_beacon_major = major;
        g_beacon_minor = minor;
    }

    nvs_close(nvs_handle);
    return err;
}

/**
 * WiFi event handler
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(WIFI_TAG, "Retry connecting to WiFi (%d/%d)", s_retry_num, WIFI_MAXIMUM_RETRY);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(WIFI_TAG, "Failed to connect to WiFi");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(WIFI_TAG, "✓ Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/**
 * Initialize WiFi connection
 */
static void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(WIFI_TAG, "WiFi initialization finished");
    ESP_LOGI(WIFI_TAG, "Connecting to SSID: %s", WIFI_SSID);

    /* Wait until either connected or failed */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(WIFI_TAG, "✓ Connected to SSID: %s", WIFI_SSID);

        // Blink LED slowly during WiFi connection stabilization
        ESP_LOGI(WIFI_TAG, "Waiting 5 seconds for network to stabilize...");
        for (int i = 0; i < 5; i++) {
            gpio_set_level(LED_GPIO, 1);
            vTaskDelay(250 / portTICK_PERIOD_MS);
            gpio_set_level(LED_GPIO, 0);
            vTaskDelay(250 / portTICK_PERIOD_MS);
        }

        ESP_LOGI(WIFI_TAG, "Network stable, starting webhook task...");
        xTaskCreate(&webhook_task, "webhook", 12288, NULL, 3, NULL);  // 12KB stack (HTTPS with TLS)
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(WIFI_TAG, "✗ Failed to connect to SSID: %s", WIFI_SSID);
    } else {
        ESP_LOGE(WIFI_TAG, "✗ Unexpected WiFi event");
    }
}

/**
 * Webhook task (runs once or periodically, depending on WEBHOOK_INTERVAL_SEC)
 */
static void webhook_task(void *pvParameter)
{
    ESP_LOGI(TAG, "Webhook task started, sending notification...");
    ESP_LOGI(TAG, "Free heap: %lu bytes", esp_get_free_heap_size());

    // Get MAC address for device identification
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    while (1) {
        // Create compact JSON payload
        static char json_payload[512];  // Static allocation to keep off task stack
        snprintf(json_payload, sizeof(json_payload),
            "{\"content\":\"iBeacon Connected\","
            "\"embeds\":[{\"title\":\"ESP32 iBeacon Online\",\"color\":3066993,"
            "\"fields\":["
            "{\"name\":\"Device MAC\",\"value\":\"%s\",\"inline\":true},"
            "{\"name\":\"Major\",\"value\":\"%d\",\"inline\":true},"
            "{\"name\":\"Minor\",\"value\":\"%d\",\"inline\":true},"
            "{\"name\":\"Firmware\",\"value\":\"%s\",\"inline\":true},"
            "{\"name\":\"UUID\",\"value\":\"%s\",\"inline\":false},"
            "{\"name\":\"WiFi SSID\",\"value\":\"%s\",\"inline\":true},"
            "{\"name\":\"Interval\",\"value\":\"%dms\",\"inline\":true}"
            "]}]}",
            mac_str,
            g_beacon_major,
            g_beacon_minor,
            FIRMWARE_VERSION,
            BEACON_UUID_STRING,
            WIFI_SSID,
            ADVERTISING_INTERVAL_MS
        );

        ESP_LOGI(TAG, "JSON payload size: %d bytes", strlen(json_payload));

        // Configure HTTP client for HTTPS
        ESP_LOGI(TAG, "Configuring HTTPS client...");
        esp_http_client_config_t config = {
            .url = WEBHOOK_URL,
            .method = HTTP_METHOD_POST,
            .timeout_ms = 10000,
            .crt_bundle_attach = esp_crt_bundle_attach,
        };

        ESP_LOGI(TAG, "Initializing HTTP client...");
        esp_http_client_handle_t client = esp_http_client_init(&config);
        if (client == NULL) {
            ESP_LOGE(TAG, "Failed to initialize HTTP client");
            gpio_set_level(LED_GPIO, 0);
            if (WEBHOOK_INTERVAL_SEC == 0) {
                vTaskDelete(NULL);
            }
            continue;
        }

        ESP_LOGI(TAG, "Setting headers...");
        esp_http_client_set_header(client, "Content-Type", "application/json");
        esp_http_client_set_post_field(client, json_payload, strlen(json_payload));

        ESP_LOGI(TAG, "Performing HTTP POST to webhook (payload: %d bytes)...", strlen(json_payload));
        esp_err_t err = esp_http_client_perform(client);
        ESP_LOGI(TAG, "HTTP perform completed: %s", esp_err_to_name(err));

        if (err == ESP_OK) {
            int status_code = esp_http_client_get_status_code(client);
            int content_length = esp_http_client_get_content_length(client);

            ESP_LOGI(TAG, "HTTP Status: %d, Content-Length: %d", status_code, content_length);

            if (status_code == 200 || status_code == 204) {
                ESP_LOGI(TAG, "✓ Webhook sent successfully (HTTP %d)", status_code);

                // Blink LED 5 times rapidly to indicate success
                for (int i = 0; i < 5; i++) {
                    gpio_set_level(LED_GPIO, 1);  // On
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                    gpio_set_level(LED_GPIO, 0);  // Off
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }
            } else if (status_code >= 400) {
                ESP_LOGE(TAG, "✗ Webhook error: HTTP %d", status_code);
                gpio_set_level(LED_GPIO, 0);  // Turn off LED
            } else {
                ESP_LOGW(TAG, "⚠️  Webhook returned status: %d", status_code);
                gpio_set_level(LED_GPIO, 0);  // Turn off LED
            }
        } else {
            ESP_LOGE(TAG, "✗ Failed to send webhook: %s", esp_err_to_name(err));
            gpio_set_level(LED_GPIO, 0);  // Turn off LED
        }

        esp_http_client_cleanup(client);

        // If WEBHOOK_INTERVAL_SEC is 0, send once and exit
        // Otherwise, wait and send again
        if (WEBHOOK_INTERVAL_SEC == 0) {
            ESP_LOGI(TAG, "Webhook task complete (one-time mode), deleting task");
            vTaskDelete(NULL);
            return;
        } else {
            ESP_LOGI(TAG, "Waiting %d seconds before next webhook...", WEBHOOK_INTERVAL_SEC);
            vTaskDelay((WEBHOOK_INTERVAL_SEC * 1000) / portTICK_PERIOD_MS);
        }
    }
}

/**
 * Convert UUID string to byte array
 */
static void parse_uuid_string(const char* uuid_str, uint8_t* uuid_bytes)
{
    int byte_index = 0;

    for (int i = 0; uuid_str[i] != '\0' && byte_index < 16; i++) {
        if (uuid_str[i] == '-') {
            continue;
        }
        char hex_pair[3] = {uuid_str[i], uuid_str[i+1], '\0'};
        uuid_bytes[byte_index] = (uint8_t)strtol(hex_pair, NULL, 16);
        byte_index++;
        i++;
    }
}

// Bluetooth advertising parameters
static esp_ble_adv_params_t adv_params = {
    .adv_int_min        = ADVERTISING_INTERVAL_MS * 1000 / 625,
    .adv_int_max        = ADVERTISING_INTERVAL_MS * 1000 / 625,
    .adv_type           = ADV_TYPE_NONCONN_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

/**
 * Bluetooth GAP event handler
 */
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    esp_err_t err;

    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
        esp_ble_gap_start_advertising(&adv_params);
        break;

    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        if ((err = param->adv_start_cmpl.status) != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "Advertising start failed: %s", esp_err_to_name(err));
        } else {
            ESP_LOGI(TAG, "✓ iBeacon is broadcasting at %dms intervals!", ADVERTISING_INTERVAL_MS);
        }
        break;

    default:
        break;
    }
}

/**
 * Initialize Bluetooth and register callbacks
 */
static void bluetooth_init(void)
{
    ESP_LOGI(TAG, "Initializing Bluetooth...");

    esp_bluedroid_init();
    esp_bluedroid_enable();

    char device_name[32];
    snprintf(device_name, sizeof(device_name), "iBeacon-%d-%d", g_beacon_major, g_beacon_minor);
    esp_ble_gap_set_device_name(device_name);
    ESP_LOGI(TAG, "✓ Device name set to: %s", device_name);

    esp_err_t status = esp_ble_gap_register_callback(gap_event_handler);
    if (status != ESP_OK) {
        ESP_LOGE(TAG, "GAP register failed: %s", esp_err_to_name(status));
        return;
    }

    ESP_LOGI(TAG, "✓ Bluetooth initialized");
}

/**
 * Configure and start iBeacon advertising
 */
static void start_ibeacon(void)
{
    ESP_LOGI(TAG, "Configuring iBeacon...");

    uint8_t beacon_uuid[16];
    parse_uuid_string(BEACON_UUID_STRING, beacon_uuid);

    esp_ble_set_ibeacon_params(beacon_uuid, g_beacon_major, g_beacon_minor);
    esp_ble_print_ibeacon_config();

    esp_ble_ibeacon_t ibeacon_adv_data;
    esp_err_t status = esp_ble_config_ibeacon_data(&vendor_config, &ibeacon_adv_data);

    if (status == ESP_OK) {
        esp_ble_gap_config_adv_data_raw((uint8_t*)&ibeacon_adv_data, sizeof(ibeacon_adv_data));
        esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, TRANSMIT_POWER);
        ESP_LOGI(TAG, "✓ iBeacon configured successfully");
    } else {
        ESP_LOGE(TAG, "Failed to configure iBeacon: %s", esp_err_to_name(status));
    }
}

/**
 * Send OTA error webhook notification
 */
static void send_ota_error_webhook(const char* error_message)
{
    ESP_LOGI(OTA_TAG, "Sending OTA error webhook...");

    // Get MAC address
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // Create JSON payload for error
    static char json_payload[512];
    snprintf(json_payload, sizeof(json_payload),
        "{\"content\":\"⚠️ OTA Update Failed\","
        "\"embeds\":[{\"title\":\"ESP32 OTA Error\",\"color\":15158332,"
        "\"fields\":["
        "{\"name\":\"Device MAC\",\"value\":\"%s\",\"inline\":true},"
        "{\"name\":\"Major\",\"value\":\"%d\",\"inline\":true},"
        "{\"name\":\"Minor\",\"value\":\"%d\",\"inline\":true},"
        "{\"name\":\"Error\",\"value\":\"%s\",\"inline\":false},"
        "{\"name\":\"Firmware\",\"value\":\"%s\",\"inline\":true},"
        "{\"name\":\"OTA URL\",\"value\":\"%s\",\"inline\":false}"
        "]}]}",
        mac_str,
        g_beacon_major,
        g_beacon_minor,
        error_message,
        FIRMWARE_VERSION,
        OTA_UPDATE_URL
    );

    // Configure HTTP client for HTTPS
    esp_http_client_config_t config = {
        .url = WEBHOOK_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 10000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(OTA_TAG, "Failed to initialize HTTP client for webhook");
        return;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_payload, strlen(json_payload));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        if (status_code == 200 || status_code == 204) {
            ESP_LOGI(OTA_TAG, "✓ OTA error webhook sent successfully");
        } else {
            ESP_LOGW(OTA_TAG, "⚠️ Webhook returned status: %d", status_code);
        }
    } else {
        ESP_LOGE(OTA_TAG, "✗ Failed to send webhook: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}

/**
 * Send OTA success webhook notification
 */
static void send_ota_success_webhook(const char* status_message)
{
    ESP_LOGI(OTA_TAG, "Sending OTA success webhook...");

    // Get MAC address
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // Create JSON payload for success
    static char json_payload[512];
    snprintf(json_payload, sizeof(json_payload),
        "{\"content\":\"✅ OTA Check Complete\","
        "\"embeds\":[{\"title\":\"ESP32 OTA Status\",\"color\":5763719,"
        "\"fields\":["
        "{\"name\":\"Device MAC\",\"value\":\"%s\",\"inline\":true},"
        "{\"name\":\"Major\",\"value\":\"%d\",\"inline\":true},"
        "{\"name\":\"Minor\",\"value\":\"%d\",\"inline\":true},"
        "{\"name\":\"Status\",\"value\":\"%s\",\"inline\":false},"
        "{\"name\":\"Firmware\",\"value\":\"%s\",\"inline\":true},"
        "{\"name\":\"Free Heap\",\"value\":\"%lu bytes\",\"inline\":true}"
        "]}]}",
        mac_str,
        g_beacon_major,
        g_beacon_minor,
        status_message,
        FIRMWARE_VERSION,
        esp_get_free_heap_size()
    );

    // Configure HTTP client for HTTPS
    esp_http_client_config_t config = {
        .url = WEBHOOK_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 10000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(OTA_TAG, "Failed to initialize HTTP client for webhook");
        return;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_payload, strlen(json_payload));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        if (status_code == 200 || status_code == 204) {
            ESP_LOGI(OTA_TAG, "✓ OTA success webhook sent");
        }
    }

    esp_http_client_cleanup(client);
}

/**
 * Blink LED to indicate OTA error
 */
static void blink_led_ota_error(void)
{
    // Blink LED for 500ms
    gpio_set_level(LED_GPIO, 1);  // On
    vTaskDelay(500 / portTICK_PERIOD_MS);
    gpio_set_level(LED_GPIO, 0);  // Off
}

/**
 * Perform OTA update
 */
static void perform_ota_update(void)
{
    ESP_LOGI(OTA_TAG, "Starting OTA update...");
    ESP_LOGI(OTA_TAG, "Checking for updates at: %s", OTA_UPDATE_URL);

    esp_http_client_config_t config = {
        .url = OTA_UPDATE_URL,
        .timeout_ms = 30000,
        .keep_alive_enable = true,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &config,
    };

    esp_err_t ret = esp_https_ota(&ota_config);

    if (ret == ESP_OK) {
        ESP_LOGI(OTA_TAG, "✓ OTA update successful! Rebooting...");

        // Send success webhook before rebooting
        send_ota_success_webhook("Firmware updated successfully - rebooting");

        vTaskDelay(1000 / portTICK_PERIOD_MS);
        esp_restart();
    } else if (ret == ESP_ERR_NOT_FOUND) {
        // No update available - this is normal
        ESP_LOGI(OTA_TAG, "No update available (already on latest version)");
        send_ota_success_webhook("No update needed - already on latest firmware");
    } else {
        // Actual error occurred
        ESP_LOGE(OTA_TAG, "✗ OTA update failed: %s", esp_err_to_name(ret));

        // Blink LED to indicate error
        blink_led_ota_error();

        // Send webhook notification
        send_ota_error_webhook(esp_err_to_name(ret));
    }
}

/**
 * OTA task - periodically checks for updates
 */
static void ota_task(void *pvParameter)
{
    ESP_LOGI(OTA_TAG, "OTA task started");
    ESP_LOGI(OTA_TAG, "Will check for updates every %d seconds", OTA_CHECK_INTERVAL_SEC);

    // Wait for WiFi to connect first
    vTaskDelay(10000 / portTICK_PERIOD_MS);

    while (1) {
        ESP_LOGI(OTA_TAG, "Checking for firmware updates...");
        perform_ota_update();

        // Wait before next check
        vTaskDelay((OTA_CHECK_INTERVAL_SEC * 1000) / portTICK_PERIOD_MS);
    }
}

/**
 * Main application entry point
 */
void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  ESP32 iBeacon + Webhook with OTA");
    ESP_LOGI(TAG, "  Version %s", FIRMWARE_VERSION);
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");

    // Initialize LED GPIO
    gpio_config_t led_config = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&led_config);
    gpio_set_level(LED_GPIO, 0);  // LED off initially
    ESP_LOGI(TAG, "✓ LED initialized on GPIO %d", LED_GPIO);

    // Initialize Non-Volatile Storage (required for Bluetooth and WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Load beacon configuration from NVS
    bool config_loaded = load_beacon_config_from_nvs();
    if (!config_loaded && g_beacon_minor != DEFAULT_BEACON_MINOR) {
        ESP_LOGI(NVS_TAG, "First boot detected, saving configuration to NVS");
        save_beacon_config_to_nvs(g_beacon_major, g_beacon_minor);
    }

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "** BEACON CONFIGURATION **");
    ESP_LOGI(TAG, "UUID:     %s", BEACON_UUID_STRING);
    ESP_LOGI(TAG, "MAJOR:    %d %s", g_beacon_major, config_loaded ? "(from NVS)" : "(default)");
    ESP_LOGI(TAG, "MINOR:    %d %s", g_beacon_minor, config_loaded ? "(from NVS)" : "(default)");
    ESP_LOGI(TAG, "Interval: %dms (50ms = 20 broadcasts/sec)", ADVERTISING_INTERVAL_MS);
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");

    // Initialize WiFi
    ESP_LOGI(TAG, "Initializing WiFi...");
    wifi_init_sta();

    // Release Classic Bluetooth memory (we only need BLE)
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    // Initialize Bluetooth controller
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));

    // Initialize Bluetooth stack and start beacon
    bluetooth_init();
    start_ibeacon();

    // Start OTA update task
    xTaskCreate(&ota_task, "ota_task", 8192, NULL, 5, NULL);
    ESP_LOGI(OTA_TAG, "✓ OTA task created");

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Setup complete! iBeacon is broadcasting.");
    ESP_LOGI(TAG, "Webhook: ENABLED | OTA: ENABLED");
    ESP_LOGI(TAG, "========================================");
}
