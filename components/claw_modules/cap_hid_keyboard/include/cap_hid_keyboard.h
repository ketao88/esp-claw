/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize USB HID keyboard device via TinyUSB.
 *
 * Requires TinyUSB stack enabled in sdkconfig.
 * USB Serial/JTAG must be disabled to free the USB PHY (GPIO 19/20).
 */
esp_err_t cap_hid_keyboard_init(void);

/**
 * @brief Type a string via USB HID keyboard.
 *
 * Converts ASCII text to HID keycodes, handles shift for uppercase,
 * sends key press/release reports for each character.
 * Supports: a-z, A-Z, 0-9, space, enter, backspace, common punctuation.
 *
 * @param text  Null-terminated UTF-8 text to type (ASCII only)
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if text is NULL
 */
esp_err_t cap_hid_keyboard_type_text(const char *text);

#ifdef __cplusplus
}
#endif
