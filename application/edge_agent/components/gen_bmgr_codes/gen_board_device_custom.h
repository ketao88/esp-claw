/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * Auto-generated custom device structure definitions
 * DO NOT MODIFY THIS FILE MANUALLY
 *
 * See LICENSE file for details.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "dev_custom.h"

// Custom device structure definitions
// These structures are dynamically generated based on YAML configuration

// Structure definition for led_strip
typedef struct {
    const char *name;           /*!< Custom device name */
    const char *type;           /*!< Device type: "custom" */
    const char *chip;           /*!< Chip name */
    int8_t       max_leds;
    uint8_t     peripheral_count;
    const char *peripheral_name;
} dev_custom_led_strip_config_t;

