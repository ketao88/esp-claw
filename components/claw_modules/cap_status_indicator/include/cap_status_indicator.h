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
 * @brief Initialize the WS2812 status indicator.
 *        48 LEDs on GPIO 48 (RMT). Controls patterns based on WiFi and conversation state.
 *
 * Patterns:
 *   - Not connected to WiFi: red slow blink
 *   - WiFi connected / idle: green solid @ 40% brightness (102/255)
 *   - Conversation active: slow cyan pulse
 *   - LLM error (timeout / API error): red flash 3 times, pause 2s, loop
 */
esp_err_t cap_status_indicator_init(void);

/**
 * @brief Signal that a conversation has started (triggers active pulse pattern).
 *        Also clears any active LLM error state.
 */
void cap_status_indicator_conversation_start(void);

/**
 * @brief Query whether the indicator is currently showing an LLM error pattern.
 * @return true if LLM error pattern is active, false otherwise.
 */
bool cap_status_indicator_is_llm_error(void);

/**
 * @brief Get the last LLM error message (may be NULL).
 * @return Pointer to internal error string, valid until next conversation start.
 */
const char *cap_status_indicator_last_error(void);

#ifdef __cplusplus
}
#endif
