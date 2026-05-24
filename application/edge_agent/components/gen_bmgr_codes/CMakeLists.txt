# Board information output
message(STATUS "Selected Board: esp32_S3_DevKitC_1")
message(STATUS "Board Path: /home/ket/.openclaw/workspace/esp-claw/application/edge_agent/boards/espressif/esp32_S3_DevKitC_1")

idf_component_register(
    SRC_DIRS "." "../../boards/espressif/esp32_S3_DevKitC_1"
    INCLUDE_DIRS "."
        "../../boards/espressif/esp32_S3_DevKitC_1"
        "../../managed_components/espressif__esp_board_manager/devices/dev_custom"
        "../../managed_components/espressif__esp_board_manager/peripherals/periph_rmt/idf5"
    REQUIRES esp_board_manager
)

# This is equivalent to adding WHOLE_ARCHIVE option to the idf_component_register call above:
idf_component_set_property(${COMPONENT_NAME} WHOLE_ARCHIVE TRUE)
