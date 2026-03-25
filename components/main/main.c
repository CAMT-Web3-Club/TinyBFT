#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    printf("TinyBFT library loaded successfully.\n");
    printf("This is a library component. Implement your application on top of this library.\n");
    
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    for (int i = 0; i < 3; i++) {
        printf("TinyBFT v0.1 - Library mode\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    printf("Build your application by creating a custom app_main() in your project.\n");
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
