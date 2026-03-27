/**
 * TinyBFT ESP32 Replica Example
 *
 * This example demonstrates running a BFT replica on ESP32-C3
 * using ESP-NOW transport.
 */

#include <cstdio>
#include <cstring>
#include <map>
#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_now.h"

#include "libbyz.h"

static const char* TAG = "esp_replica";

// Simple in-memory key-value store
static std::map<std::string, std::string> kv_store;

// Execute committed request
static int exec_command(Byz_req* req, Byz_rep* rep, Byz_buffer* ndet,
                        int client, bool read_only) {
    std::string cmd(req->contents, req->size);

    ESP_LOGI(TAG, "Command from client %d: %.*s", client, req->size,
             req->contents);

    if (cmd.substr(0, 4) == "GET ") {
        std::string key = cmd.substr(4);
        auto it = kv_store.find(key);

        if (it != kv_store.end()) {
            std::string value = it->second;
            if ((int)value.size() <= rep->size) {
                memcpy(rep->contents, value.data(), value.size());
                rep->size = value.size();
            } else {
                rep->size = 0;
            }
        } else {
            std::string not_found = "NOT_FOUND";
            memcpy(rep->contents, not_found.data(), not_found.size());
            rep->size = not_found.size();
        }
        return 0;

    } else if (cmd.substr(0, 4) == "SET ") {
        if (read_only) return -1;

        size_t eq_pos = cmd.find('=', 4);
        if (eq_pos != std::string::npos) {
            std::string key = cmd.substr(4, eq_pos - 4);
            std::string value = cmd.substr(eq_pos + 1);
            kv_store[key] = value;

            std::string ok = "OK";
            memcpy(rep->contents, ok.data(), ok.size());
            rep->size = ok.size();

            ESP_LOGI(TAG, "SET %s = %s", key.c_str(), value.c_str());
            return 0;
        }

        std::string error = "INVALID_FORMAT";
        memcpy(rep->contents, error.data(), error.size());
        rep->size = error.size();
        return 0;
    }

    std::string error = "UNKNOWN_COMMAND";
    memcpy(rep->contents, error.data(), error.size());
    rep->size = error.size();
    return 0;
}

static void comp_ndet(Seqno seqno, Byz_buffer* ndet) {
    (void)seqno;
    ndet->size = 0;
}

static int recv_reply(Byz_rep* rep) {
    (void)rep;
    return 0;
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "TinyBFT ESP32 Replica starting...");

    // Initialize NVS (required for WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "NVS initialized");

    // Note: Actual Byz_init_replica would be called here with
    // proper config from SPIFFS or hardcoded values
    // For now, this is a placeholder

    ESP_LOGI(TAG, "Replica initialized, entering main loop");

    while (true) {
        // Call Byz_replica_process() in a loop
        // Byz_replica_process();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
