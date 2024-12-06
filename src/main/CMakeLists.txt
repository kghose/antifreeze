idf_component_register(
    SRCS 
        "main.c"
        "state.c"
        "wifi.c"
        "httpserver.c"
    INCLUDE_DIRS "."
    REQUIRES
        "esp32-owb"
        "esp32-ds18b20"
    PRIV_REQUIRES
        "esp_http_server"
        "esp_wifi"
        "nvs_flash"
        "esp_netif"
)
