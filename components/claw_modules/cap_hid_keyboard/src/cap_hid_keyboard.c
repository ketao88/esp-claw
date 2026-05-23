/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * USB HID Keyboard implementation using esp_tinyusb.
 * Registers as a standard USB HID keyboard.
 */
#include "cap_hid_keyboard.h"

#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "class/hid/hid_device.h"

static const char *TAG = "cap_hid_keyboard";

/* ===== HID keyboard report buffer ===== */
/* The hid_keyboard_report_t is already defined in tusb_hid.h */
typedef struct __attribute__((packed)) {
    uint8_t modifier;
    uint8_t reserved;
    uint8_t keycodes[6];
} keyboard_report_t;

static bool s_hid_ready = false;

/* ===== HID Keyboard configuration descriptor ===== */
enum {
    ITF_NUM_HID,
    ITF_NUM_TOTAL
};

#define TUD_HID_REPORT_DESC_KEYBOARD_SIZE  TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(1))

static const uint8_t s_hid_report_desc[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(1))
};

static const uint8_t s_config_desc[] = {
    /* Config descriptor */
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN, 0, 100),

    /* HID interface */
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_KEYBOARD,
                       sizeof(s_hid_report_desc), 0x00, 16, 0)
};

/* ===== TinyUSB event callback ===== */
static void tusb_event_cb(tinyusb_event_t *event, void *arg)
{
    (void)arg;
    switch (event->id) {
    case TINYUSB_EVENT_ATTACHED:
        s_hid_ready = true;
        ESP_LOGI(TAG, "USB attached (host connected)");
        break;
    case TINYUSB_EVENT_DETACHED:
        s_hid_ready = false;
        ESP_LOGI(TAG, "USB detached");
        break;
        s_hid_ready = true;
        break;
    default:
        break;
    }
}

/* HID callbacks (not provided by esp_tinyusb, must be defined by app) */
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const *report, uint16_t len)
{
    (void)instance;
    (void)report;
    (void)len;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                                hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                            hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)bufsize;
}

/* ===== Send a HID key press/release ===== */
static esp_err_t send_keys(uint8_t modifier, const uint8_t *keycodes, size_t keycode_count)
{
    keyboard_report_t report;

    if (!s_hid_ready) {
        return ESP_ERR_INVALID_STATE;
    }

    /* Press */
    memset(&report, 0, sizeof(report));
    report.modifier = modifier;
    for (size_t i = 0; i < keycode_count && i < 6; i++) {
        report.keycodes[i] = keycodes[i];
    }
    tud_hid_n_report(0, 1, &report, sizeof(report));

    /* Small delay then release */
    vTaskDelay(pdMS_TO_TICKS(5));
    memset(&report, 0, sizeof(report));
    tud_hid_n_report(0, 1, &report, sizeof(report));

    return ESP_OK;
}

/* ===== ASCII to HID keycode map ===== */
typedef struct {
    char ascii;
    uint8_t modifier;
    uint8_t keycode;
} hid_key_map_t;

static const hid_key_map_t s_keymap[] = {
    /* Letters a-z */
    {'a', 0x00, 0x04}, {'b', 0x00, 0x05}, {'c', 0x00, 0x06}, {'d', 0x00, 0x07},
    {'e', 0x00, 0x08}, {'f', 0x00, 0x09}, {'g', 0x00, 0x0A}, {'h', 0x00, 0x0B},
    {'i', 0x00, 0x0C}, {'j', 0x00, 0x0D}, {'k', 0x00, 0x0E}, {'l', 0x00, 0x0F},
    {'m', 0x00, 0x10}, {'n', 0x00, 0x11}, {'o', 0x00, 0x12}, {'p', 0x00, 0x13},
    {'q', 0x00, 0x14}, {'r', 0x00, 0x15}, {'s', 0x00, 0x16}, {'t', 0x00, 0x17},
    {'u', 0x00, 0x18}, {'v', 0x00, 0x19}, {'w', 0x00, 0x1A}, {'x', 0x00, 0x1B},
    {'y', 0x00, 0x1C}, {'z', 0x00, 0x1D},
    /* Uppercase A-Z (same + shift) */
    {'A', 0x02, 0x04}, {'B', 0x02, 0x05}, {'C', 0x02, 0x06}, {'D', 0x02, 0x07},
    {'E', 0x02, 0x08}, {'F', 0x02, 0x09}, {'G', 0x02, 0x0A}, {'H', 0x02, 0x0B},
    {'I', 0x02, 0x0C}, {'J', 0x02, 0x0D}, {'K', 0x02, 0x0E}, {'L', 0x02, 0x0F},
    {'M', 0x02, 0x10}, {'N', 0x02, 0x11}, {'O', 0x02, 0x12}, {'P', 0x02, 0x13},
    {'Q', 0x02, 0x14}, {'R', 0x02, 0x15}, {'S', 0x02, 0x16}, {'T', 0x02, 0x17},
    {'U', 0x02, 0x18}, {'V', 0x02, 0x19}, {'W', 0x02, 0x1A}, {'X', 0x02, 0x1B},
    {'Y', 0x02, 0x1C}, {'Z', 0x02, 0x1D},
    /* Numbers */
    {'1', 0x00, 0x1E}, {'2', 0x00, 0x1F}, {'3', 0x00, 0x20}, {'4', 0x00, 0x21},
    {'5', 0x00, 0x22}, {'6', 0x00, 0x23}, {'7', 0x00, 0x24}, {'8', 0x00, 0x25},
    {'9', 0x00, 0x26}, {'0', 0x00, 0x27},
    /* Control */
    {'\n', 0x00, 0x28}, {'\r', 0x00, 0x28}, {'\b', 0x00, 0x2A},
    {'\t', 0x00, 0x2B}, {' ',  0x00, 0x2C},
    /* Symbols (unshifted) */
    {'-', 0x00, 0x2D}, {'=', 0x00, 0x2E},
    {'[', 0x00, 0x2F}, {']', 0x00, 0x30},
    {'\\', 0x00, 0x31}, {';', 0x00, 0x33},
    {'\'', 0x00, 0x34}, {'`', 0x00, 0x35},
    {',', 0x00, 0x36}, {'.', 0x00, 0x37},
    {'/', 0x00, 0x38},
    /* Symbols (shifted) */
    {'!', 0x02, 0x1E}, {'@', 0x02, 0x1F}, {'#', 0x02, 0x20}, {'$', 0x02, 0x21},
    {'%', 0x02, 0x22}, {'^', 0x02, 0x23}, {'&', 0x02, 0x24}, {'*', 0x02, 0x25},
    {'(', 0x02, 0x26}, {')', 0x02, 0x27},
    {'_', 0x02, 0x2D}, {'+', 0x02, 0x2E},
    {'{', 0x02, 0x2F}, {'}', 0x02, 0x30},
    {'|', 0x02, 0x31}, {':', 0x02, 0x33},
    {'"', 0x02, 0x34}, {'~', 0x02, 0x35},
    {'<', 0x02, 0x36}, {'>', 0x02, 0x37},
    {'?', 0x02, 0x38},
};

#define KEYMAP_SIZE (sizeof(s_keymap) / sizeof(s_keymap[0]))

/* ===== Public API ===== */

esp_err_t cap_hid_keyboard_type_text(const char *text)
{
    size_t len;

    if (!text) {
        return ESP_ERR_INVALID_ARG;
    }

    len = strlen(text);
    if (!s_hid_ready) {
        ESP_LOGW(TAG, "USB not ready, cannot type %zu chars", len);
        return ESP_ERR_INVALID_STATE;
    }

    for (size_t i = 0; i < len; i++) {
        char c = text[i];
        uint8_t modifier = 0;
        uint8_t keycode = 0;
        bool found = false;

        for (size_t k = 0; k < KEYMAP_SIZE; k++) {
            if (s_keymap[k].ascii == c) {
                modifier = s_keymap[k].modifier;
                keycode = s_keymap[k].keycode;
                found = true;
                break;
            }
        }

        if (!found) {
            ESP_LOGW(TAG, "Unsupported char: 0x%02x", (unsigned char)c);
            continue;
        }

        if (send_keys(modifier, &keycode, 1) != ESP_OK) {
            return ESP_FAIL;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    ESP_LOGD(TAG, "Typed %zu chars", len);
    return ESP_OK;
}

esp_err_t cap_hid_keyboard_init(void)
{
    const tinyusb_config_t tusb_cfg = {
        .port = TINYUSB_PORT_FULL_SPEED_0,
        .phy = {
            .skip_setup = false,
            .self_powered = false,
            .vbus_monitor_io = -1,
        },
        .task = {
            .size = 4096,
            .priority = 5,
            .xCoreID = tskNO_AFFINITY,
        },
        .descriptor = {
            .device = NULL,
            .qualifier = NULL,
            .string = NULL,
            .string_count = 0,
            .full_speed_config = s_config_desc,
            .high_speed_config = NULL,
        },
        .event_cb = tusb_event_cb,
        .event_arg = NULL,
    };

    esp_err_t err = tinyusb_driver_install(&tusb_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "tinyusb_driver_install failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "USB HID keyboard ready (GPIO 19/20)");
    ESP_LOGI(TAG, "Plug USB into host to type via cap_hid_keyboard_type_text()");
    return ESP_OK;
}
