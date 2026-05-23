/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Board manager stubs for ESP32-S3 DevKitC board.
 * Removed WiFi stubs - they're now in the fixed esp_wifi library.
 */

#include "esp_err.h"

/* Board manager peripheral/device stubs for ESP32-S3 DevKitC board */
esp_err_t periph_rmt_init(void) { return ESP_OK; }
esp_err_t periph_rmt_deinit(void) { return ESP_OK; }
esp_err_t dev_custom_init(void) { return ESP_OK; }
esp_err_t dev_custom_deinit(void) { return ESP_OK; }
