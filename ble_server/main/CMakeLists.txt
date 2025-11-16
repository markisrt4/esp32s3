idf_component_register(
    SRCS "ble_server.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash bt
)
