/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "http_server_priv.h"

#include <stdio.h>
#include <string.h>

#include "cJSON.h"
#include "esp_log.h"
#include "wifi_manager.h"

static const char *TAG = "http_wifi_api";

static const char *auth_mode_to_string(wifi_auth_mode_t mode)
{
    switch (mode) {
    case WIFI_AUTH_OPEN:            return "open";
    case WIFI_AUTH_WEP:             return "wep";
    case WIFI_AUTH_WPA_PSK:         return "wpa_psk";
    case WIFI_AUTH_WPA2_PSK:        return "wpa2_psk";
    case WIFI_AUTH_WPA_WPA2_PSK:    return "wpa_wpa2_psk";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "wpa2_enterprise";
    case WIFI_AUTH_WPA3_PSK:        return "wpa3_psk";
    case WIFI_AUTH_WPA2_WPA3_PSK:   return "wpa2_wpa3_psk";
    case WIFI_AUTH_WAPI_PSK:        return "wapi_psk";
    default:                        return "unknown";
    }
}

/* GET /api/wifi/scan  →  trigger a scan and return JSON list of APs */
static esp_err_t wifi_scan_handler(httpd_req_t *req)
{
    wifi_manager_scan_record_t records[30] = {0};
    uint16_t count = 0;
    esp_err_t err = wifi_manager_scan_aps(records, 30, &count);

    if (err != ESP_OK) {
        httpd_resp_set_type(req, "application/json");
        char err_buf[128];
        snprintf(err_buf, sizeof(err_buf),
                 "{\"ok\":false,\"error\":\"scan_failed\",\"message\":\"%s\"}",
                 esp_err_to_name(err));
        return httpd_resp_send(req, err_buf, strlen(err_buf));
    }

    cJSON *root = cJSON_CreateObject();
    if (!root) {
        httpd_resp_send_500(req);
        return ESP_ERR_NO_MEM;
    }

    cJSON_AddBoolToObject(root, "ok", true);
    cJSON_AddNumberToObject(root, "count", count);

    cJSON *aps = cJSON_AddArrayToObject(root, "aps");
    if (aps) {
        for (uint16_t i = 0; i < count; i++) {
            cJSON *ap = cJSON_CreateObject();
            if (!ap) {
                continue;
            }
            cJSON_AddStringToObject(ap, "ssid", records[i].ssid[0] ? records[i].ssid : "(hidden)");
            cJSON_AddNumberToObject(ap, "rssi", records[i].rssi);
            cJSON_AddNumberToObject(ap, "channel", records[i].primary);
            cJSON_AddStringToObject(ap, "auth", auth_mode_to_string(records[i].authmode));
            cJSON_AddItemToArray(aps, ap);
        }
    }

    esp_err_t send_err = http_server_send_json_response(req, root);
    return send_err;
}

esp_err_t http_server_register_wifi_routes(httpd_handle_t server)
{
    const httpd_uri_t handlers[] = {
        { .uri = "/api/wifi/scan", .method = HTTP_GET, .handler = wifi_scan_handler },
    };

    for (size_t i = 0; i < sizeof(handlers) / sizeof(handlers[0]); ++i) {
        esp_err_t err = httpd_register_uri_handler(server, &handlers[i]);
        if (err != ESP_OK) {
            return err;
        }
    }
    return ESP_OK;
}
