idf_component_register(SRCS "esp_ibeacon_api.c"
                            "main.c"
                    PRIV_REQUIRES bt nvs_flash esp_wifi esp_netif esp_event esp_http_client esp_https_ota app_update esp_driver_gpio mbedtls
                    INCLUDE_DIRS ".")
