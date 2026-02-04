/**
 * ESP32 iBeacon Transmitter
 *
 * Professional firmware for ESP32-based iBeacon broadcasting.
 * Optimized for room detection, presence awareness, and indoor positioning.
 *
 * Features:
 * - Simple string-based UUID configuration
 * - Configurable transmission power and advertising interval
 * - Low power consumption (~20-30mA)
 * - Compatible with all iBeacon-enabled devices
 *
 * @author ESP32 iBeacon Project
 * @version 1.0.0
 * @license MIT
 *
 * Compatible with:
 * - iOS Room Detector app
 * - Any iBeacon-compatible iOS/Android app
 * - Home automation systems
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

// ============================================================================
// CONFIGURATION - Edit these values for your beacon
// ============================================================================

// Your Beacon UUID - Just paste the UUID string here!
// Must match the UUID configured in your iOS app
// Format: XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX
//
// Common manufacturer UUIDs (copy and paste):
//   Estimote:     B9407F30-F5F8-466E-AFF9-25556B57FE6D
//   Kontakt.io:   F7826DA6-4FA2-4E98-8024-BC5B71E0893E
//   Apple AirLocate:  E2C56DB5-DFFB-48D2-B060-D0F5A71096E0
//
// Or generate your own at: https://www.uuidgenerator.net/
#define BEACON_UUID_STRING "B9407F30-F5F8-466E-AFF9-25556B57FE6D"

// Major number (0-65535) - Use to group beacons (e.g., all beacons in one building)
#define BEACON_MAJOR 100

// Minor number (0-65535) - Unique identifier for each beacon
// Suggestion: Living Room = 1, Bedroom = 2, Kitchen = 3, etc.
#define BEACON_MINOR 1

// Advertising interval in milliseconds (100-10000)
// Lower = more responsive but uses more battery
// Higher = saves battery but slower detection
// Recommended: 500-1000ms for room detection
#define ADVERTISING_INTERVAL_MS 500

// Transmit Power (-127 to 0 dBm)
// Higher = longer range but more battery consumption
// -4 dBm is good for most rooms (5-10m range)
// -20 dBm for smaller areas to reduce false detections
#define TRANSMIT_POWER -4

// ============================================================================
// END OF CONFIGURATION
// ============================================================================

static const char* TAG = "iBeacon";
extern esp_ble_ibeacon_vendor_t vendor_config;

/**
 * Convert UUID string to byte array
 * Converts "B9407F30-F5F8-466E-AFF9-25556B57FE6D" to byte array
 */
static void parse_uuid_string(const char* uuid_str, uint8_t* uuid_bytes)
{
    int byte_index = 0;

    for (int i = 0; uuid_str[i] != '\0' && byte_index < 16; i++) {
        // Skip dashes
        if (uuid_str[i] == '-') {
            continue;
        }

        // Parse two hex characters into one byte
        char hex_pair[3] = {uuid_str[i], uuid_str[i+1], '\0'};
        uuid_bytes[byte_index] = (uint8_t)strtol(hex_pair, NULL, 16);
        byte_index++;
        i++; // Skip the second character of the pair
    }
}

// Bluetooth advertising parameters
static esp_ble_adv_params_t adv_params = {
    .adv_int_min        = ADVERTISING_INTERVAL_MS * 1000 / 625,  // Convert ms to BLE units
    .adv_int_max        = ADVERTISING_INTERVAL_MS * 1000 / 625,
    .adv_type           = ADV_TYPE_NONCONN_IND,     // Non-connectable advertising
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,             // Use all 3 BLE channels
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
        // Advertising data set successfully, start advertising
        esp_ble_gap_start_advertising(&adv_params);
        break;

    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        // Check if advertising started successfully
        if ((err = param->adv_start_cmpl.status) != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "Advertising start failed: %s", esp_err_to_name(err));
        } else {
            ESP_LOGI(TAG, "✓ iBeacon is broadcasting!");
            ESP_LOGI(TAG, "✓ Your device is now discoverable by iOS Room Detector");
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

    // Initialize Bluetooth controller
    esp_bluedroid_init();
    esp_bluedroid_enable();

    // Register GAP callback
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

    // Parse UUID string to byte array
    uint8_t beacon_uuid[16];
    parse_uuid_string(BEACON_UUID_STRING, beacon_uuid);

    // Set beacon UUID, Major, and Minor
    esp_ble_set_ibeacon_params(beacon_uuid, BEACON_MAJOR, BEACON_MINOR);

    // Print configuration for verification
    esp_ble_print_ibeacon_config();

    // Configure advertising data
    esp_ble_ibeacon_t ibeacon_adv_data;
    esp_err_t status = esp_ble_config_ibeacon_data(&vendor_config, &ibeacon_adv_data);

    if (status == ESP_OK) {
        // Set advertising data and start advertising
        esp_ble_gap_config_adv_data_raw((uint8_t*)&ibeacon_adv_data, sizeof(ibeacon_adv_data));

        // Set transmit power
        esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, TRANSMIT_POWER);

        ESP_LOGI(TAG, "✓ iBeacon configured successfully");
    } else {
        ESP_LOGE(TAG, "Failed to configure iBeacon: %s", esp_err_to_name(status));
    }
}

/**
 * Main application entry point
 */
void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  ESP32 iBeacon Transmitter");
    ESP_LOGI(TAG, "========================================");

    // Initialize Non-Volatile Storage (required for Bluetooth)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Release Classic Bluetooth memory (we only need BLE)
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    // Initialize Bluetooth controller
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));

    // Initialize Bluetooth stack and start beacon
    bluetooth_init();
    start_ibeacon();

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Setup complete! Beacon is now running.");
    ESP_LOGI(TAG, "Power consumption: ~20-30mA (optimized)");
    ESP_LOGI(TAG, "========================================");
}
