/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * Auto-generated device configuration file
 * DO NOT MODIFY THIS FILE MANUALLY
 *
 * See LICENSE file for details.
 */

#include <stdlib.h>
#include "esp_board_device.h"
#include "dev_custom.h"
#include "gen_board_device_custom.h"

// Device configuration structures
const static dev_custom_led_strip_config_t esp_bmgr_led_strip_cfg = {
    .name = "led_strip",
    .type = "custom",
    .chip = "ws2812",
    .max_leds = 1,
    .peripheral_count = 1,
    .peripheral_name = "rmt_tx",
};

// Device descriptor array
const esp_board_device_desc_t g_esp_board_devices[] = {
    {
        .next = NULL,
        .name = "led_strip",
        .chip = "ws2812",
        .type = "custom",
        .sub_type = NULL,
        .cfg = &esp_bmgr_led_strip_cfg,
        .cfg_size = sizeof(esp_bmgr_led_strip_cfg),
        .init_skip = true,
    },
};
