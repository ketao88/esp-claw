/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * Auto-generated peripheral configuration file
 * DO NOT MODIFY THIS FILE MANUALLY
 *
 * See LICENSE file for details.
 */

#include <stdlib.h>
#include "esp_board_periph.h"
#include "periph_rmt.h"

// Peripheral configuration structures
const static periph_rmt_config_t esp_bmgr_rmt_tx_cfg = {
    .role = ESP_BOARD_PERIPH_ROLE_TX,
    .channel_config = {
        .tx = {
            .gpio_num = 38,
            .clk_src = RMT_CLK_SRC_DEFAULT,
            .resolution_hz = 10000000,
            .mem_block_symbols = 64,
            .trans_queue_depth = 4,
            .intr_priority = 1,
            .flags = {
                .invert_out = false,
                .with_dma = true,
                .io_loop_back = false,
                .io_od_mode = false,
                .allow_pd = false,
                .init_level = 0,
            },
        },
    },
};

// Peripheral descriptor array
const esp_board_periph_desc_t g_esp_board_peripherals[] = {
    {
        .next = NULL,
        .name = "rmt_tx",
        .type = "rmt",
        .format = NULL,
        .role = ESP_BOARD_PERIPH_ROLE_TX,
        .cfg = &esp_bmgr_rmt_tx_cfg,
        .cfg_size = sizeof(esp_bmgr_rmt_tx_cfg),
        .id = 0,
    },
};
