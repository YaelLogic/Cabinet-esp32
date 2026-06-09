idf_component_register(SRCS "app_main.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES app_config wifi_manager mqtt_manager uart_manager protocol_manager cabinet_manager nvs_flash esp_event esp_netif)
