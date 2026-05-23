idf_component_register(
    SRCS
        "main.c"
        "esp_wifi_stubs.c"
)

idf_component_set_property(${COMPONENT_NAME} WHOLE_ARCHIVE TRUE)
