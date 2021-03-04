set(COMPONENT_REQUIRES "connect" "get-sntp" "ESP8266-RTOS-DHT" "json" "nvs_flash" "esp_http_client")
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS "fridge-sensor.c")
set(COMPONENT_ADD_INCLUDEDIRS "../")

register_component()
