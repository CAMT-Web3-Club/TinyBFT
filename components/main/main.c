#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char* TAG = "main";

extern void tinybft_example_task(void* pvParameters);

void app_main(void)
{
    ESP_LOGI(TAG, "TinyBFT ESP-IDF Application Starting...");

    xTaskCreate(&tinybft_example_task, "tinybft_task", 8192, NULL, 5, NULL);
}
