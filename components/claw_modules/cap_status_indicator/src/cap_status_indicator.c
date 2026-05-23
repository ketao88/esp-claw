/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "cap_status_indicator.h"

#include <string.h>
#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "led_strip.h"
#include "led_strip_rmt.h"
#include "led_strip_types.h"
#include "claw_core.h"
#include "wifi_manager.h"

static const char *TAG = "cap_status_indicator";

/* ===== Hardware config ===== */
#define INDICATOR_GPIO          48         /* WS2812 data GPIO (user specified) */
#define INDICATOR_LED_COUNT     48
#define INDICATOR_RMT_RES_HZ    (10 * 1000 * 1000)

/* ===== Brightness ===== */
#define BRIGHTNESS_IDLE         (uint8_t)(255 * 0.40f)   /* 40% - about 102 */
#define BRIGHTNESS_ACTIVE       (uint8_t)(255 * 0.60f)

/* ===== Timing (ms) ===== */
#define BLINK_PERIOD_MS         800        /* Not-connected red blink period */
#define PULSE_PERIOD_MS         1200       /* Active conversation pulse */
#define TASK_INTERVAL_MS        50         /* Main loop tick */

/* ===== Error flash pattern ===== */
#define ERROR_FLASH_COUNT       3          /* Number of quick red flashes */
#define ERROR_FLASH_ON_MS       200        /* Flash on duration */
#define ERROR_FLASH_OFF_MS      150        /* Flash off duration */
#define ERROR_PAUSE_MS          2000       /* Pause between flash cycles */

/* ===== States ===== */
typedef enum {
    STATE_WIFI_DISCONNECTED,    /* Red slow blink */
    STATE_WIFI_CONNECTED,       /* Green solid @ 40% */
    STATE_CONVERSATION_ACTIVE,  /* Cyan slow pulse */
    STATE_LLM_ERROR,            /* Red flash 3x, pause 2s, loop */
} indicator_state_t;

static led_strip_handle_t s_strip = NULL;
static indicator_state_t s_state = STATE_WIFI_DISCONNECTED;
static SemaphoreHandle_t s_state_lock = NULL;
static TaskHandle_t s_task = NULL;
static char s_last_error[256] = {0};  /* Last LLM error message */

/* ===== Color helpers ===== */
static uint8_t scale_brightness(uint8_t channel, uint8_t brightness)
{
    return (uint16_t)channel * brightness / 255;
}

static void set_all_leds(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{
    if (!s_strip) return;
    uint8_t sr = scale_brightness(r, brightness);
    uint8_t sg = scale_brightness(g, brightness);
    uint8_t sb = scale_brightness(b, brightness);
    for (int i = 0; i < INDICATOR_LED_COUNT; i++) {
        led_strip_set_pixel(s_strip, i, sr, sg, sb);
    }
    led_strip_refresh(s_strip);
}

static void clear_all(void)
{
    if (!s_strip) return;
    led_strip_clear(s_strip);
    led_strip_refresh(s_strip);
}

/* ===== Pattern generators ===== */
static void pattern_red_blink(uint32_t tick)
{
    /* Slow blink: on for half period, off for half */
    bool on = (tick % BLINK_PERIOD_MS) < (BLINK_PERIOD_MS / 2);
    if (on) {
        set_all_leds(255, 0, 0, BRIGHTNESS_IDLE);
    } else {
        clear_all();
    }
}

static void pattern_green_solid(void)
{
    set_all_leds(0, 255, 0, BRIGHTNESS_IDLE);
}

static void pattern_cyan_pulse(uint32_t tick)
{
    /* Sine-like pulse using triangle wave approximation */
    uint32_t phase = tick % PULSE_PERIOD_MS;
    uint32_t half = PULSE_PERIOD_MS / 2;
    uint8_t brightness;
    if (phase <= half) {
        brightness = (uint8_t)((uint16_t)phase * BRIGHTNESS_IDLE / half);
    } else {
        brightness = (uint8_t)((uint16_t)(PULSE_PERIOD_MS - phase) * BRIGHTNESS_IDLE / half);
    }
    if (brightness < 10) brightness = 10;   /* minimum glow */
    set_all_leds(0, 255, 255, brightness);
}

static void pattern_llm_error(uint32_t tick)
{
    /* Error pattern: red flash ERROR_FLASH_COUNT times, then pause ERROR_PAUSE_MS, repeat.
     *
     * One cycle: ERROR_FLASH_COUNT * (ON + OFF) + PAUSE
     * Total cycle time: 3 * (200 + 150) + 2000 = 3050 ms
     */
    uint32_t cycle_total = ERROR_FLASH_COUNT * (ERROR_FLASH_ON_MS + ERROR_FLASH_OFF_MS) + ERROR_PAUSE_MS;
    uint32_t phase = tick % cycle_total;
    uint32_t cursor = 0;

    for (int i = 0; i < ERROR_FLASH_COUNT; i++) {
        /* ON */
        if (phase >= cursor && phase < cursor + ERROR_FLASH_ON_MS) {
            set_all_leds(255, 0, 0, BRIGHTNESS_IDLE);
            return;
        }
        cursor += ERROR_FLASH_ON_MS;
        /* OFF */
        if (phase >= cursor && phase < cursor + ERROR_FLASH_OFF_MS) {
            clear_all();
            return;
        }
        cursor += ERROR_FLASH_OFF_MS;
    }

    /* PAUSE - all off */
    clear_all();
}

/* ===== WiFi state callback (via wifi_manager) ===== */
static void wifi_state_callback(bool connected, void *user_ctx)
{
    (void)user_ctx;
    xSemaphoreTake(s_state_lock, portMAX_DELAY);
    if (s_state != STATE_CONVERSATION_ACTIVE && s_state != STATE_LLM_ERROR) {
        if (connected) {
            s_state = STATE_WIFI_CONNECTED;
            ESP_LOGD(TAG, "WiFi connected → green solid");
        } else {
            s_state = STATE_WIFI_DISCONNECTED;
            ESP_LOGD(TAG, "WiFi disconnected → red blink");
        }
    }
    xSemaphoreGive(s_state_lock);
}

/* ===== Claw Core callbacks ===== */
static void on_completion(const claw_core_completion_summary_t *summary, void *user_ctx)
{
    (void)user_ctx;
    wifi_manager_status_t wifi_status;
    wifi_manager_get_status(&wifi_status);

    xSemaphoreTake(s_state_lock, portMAX_DELAY);

    if (summary && summary->error_message) {
        /* LLM error: switch to error pattern and save message */
        s_state = STATE_LLM_ERROR;
        strlcpy(s_last_error, summary->error_message, sizeof(s_last_error));
        ESP_LOGW(TAG, "LLM error: %s", summary->error_message);
    } else {
        /* Success: return to WiFi state */
        s_state = wifi_status.sta_connected ? STATE_WIFI_CONNECTED : STATE_WIFI_DISCONNECTED;
    }

    xSemaphoreGive(s_state_lock);
}

/* ===== Main task ===== */
static void indicator_task(void *pvParameters)
{
    (void)pvParameters;
    TickType_t last_wake = xTaskGetTickCount();
    uint32_t elapsed_ms = 0;

    while (1) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(TASK_INTERVAL_MS));
        elapsed_ms += TASK_INTERVAL_MS;

        indicator_state_t current_state;
        xSemaphoreTake(s_state_lock, portMAX_DELAY);
        current_state = s_state;
        xSemaphoreGive(s_state_lock);

        switch (current_state) {
        case STATE_WIFI_DISCONNECTED:
            pattern_red_blink(elapsed_ms);
            break;
        case STATE_WIFI_CONNECTED:
            pattern_green_solid();
            break;
        case STATE_CONVERSATION_ACTIVE:
            pattern_cyan_pulse(elapsed_ms);
            break;
        case STATE_LLM_ERROR:
            pattern_llm_error(elapsed_ms);
            break;
        }
    }
}

/* ===== Public API ===== */

void cap_status_indicator_conversation_start(void)
{
    if (!s_state_lock) return;
    xSemaphoreTake(s_state_lock, portMAX_DELAY);
    s_state = STATE_CONVERSATION_ACTIVE;
    s_last_error[0] = '\0';
    xSemaphoreGive(s_state_lock);
}

bool cap_status_indicator_is_llm_error(void)
{
    bool err_state = false;
    if (!s_state_lock) return false;
    xSemaphoreTake(s_state_lock, portMAX_DELAY);
    err_state = (s_state == STATE_LLM_ERROR);
    xSemaphoreGive(s_state_lock);
    return err_state;
}

const char *cap_status_indicator_last_error(void)
{
    return s_last_error[0] ? s_last_error : NULL;
}

esp_err_t cap_status_indicator_init(void)
{
    esp_err_t err;

    /* Create RMT-based LED strip for 48 WS2812s */
    led_strip_config_t strip_config = {
        .strip_gpio_num = INDICATOR_GPIO,
        .max_leds = INDICATOR_LED_COUNT,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags = { .invert_out = false },
    };
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = INDICATOR_RMT_RES_HZ,
        .mem_block_symbols = 0,
        .flags = { .with_dma = true },
    };

    err = led_strip_new_rmt_device(&strip_config, &rmt_config, &s_strip);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "led_strip_new_rmt_device failed: %s", esp_err_to_name(err));
        return err;
    }
    clear_all();
    ESP_LOGI(TAG, "WS2812 strip %d LEDs on GPIO %d initialized", INDICATOR_LED_COUNT, INDICATOR_GPIO);

    /* Create state lock */
    s_state_lock = xSemaphoreCreateMutex();
    if (!s_state_lock) {
        led_strip_del(s_strip);
        s_strip = NULL;
        return ESP_ERR_NO_MEM;
    }

    /* Query initial WiFi state via wifi_manager */
    wifi_manager_status_t wifi_status;
    wifi_manager_get_status(&wifi_status);
    s_state = wifi_status.sta_connected ? STATE_WIFI_CONNECTED : STATE_WIFI_DISCONNECTED;

    /* Register with wifi_manager for state change callbacks */
    err = wifi_manager_register_state_callback(wifi_state_callback, NULL);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to register wifi_manager callback: %s", esp_err_to_name(err));
    }

    /* Register claw_core request start and completion observers */
    err = claw_core_add_completion_observer(on_completion, NULL);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to register completion observer: %s", esp_err_to_name(err));
    }

    /* Create indicator task */
    err = xTaskCreate(indicator_task, "indicator", 4096, NULL, 5, &s_task);
    if (err != pdPASS) {
        vSemaphoreDelete(s_state_lock);
        led_strip_del(s_strip);
        s_strip = NULL;
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Status indicator started");
    return ESP_OK;
}
