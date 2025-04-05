idf_component_register(SRCS "fast_scan.c" "supabase_client.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_wifi nvs_flash esp_http_client json esp_event esp_timer mbedtls)
