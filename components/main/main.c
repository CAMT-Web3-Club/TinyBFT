/**
 * TinyBFT ESP32 Main Application
 *
 * This initializes the BFT replica on ESP32-C3.
 * Currently a placeholder - add your BFT logic here.
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_netif.h"

static const char* TAG = "tinybft";

// Forward declaration for C++ replica init
extern int tinybft_replica_init(void);

void app_main(void)
{
    ESP_LOGI(TAG, "TinyBFT v0.1 starting...");

    // Initialize NVS (required for WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS initialized");

    // Initialize TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_LOGI(TAG, "Network stack initialized");

    // Initialize WiFi (required for ESP-NOW)
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi initialized (STA mode)");

    // Get MAC address
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    ESP_LOGI(TAG, "MAC: %02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    ESP_LOGI(TAG, "TinyBFT ready. Waiting for configuration...");

    // Main loop
    int count = 0;
    while (1) {
        ESP_LOGI(TAG, "TinyBFT alive - heartbeat %d", count++);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
