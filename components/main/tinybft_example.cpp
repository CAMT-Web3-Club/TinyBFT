#include "tinybft_example.h"
#include "Node.h"
#include "Transport.h"
#include <esp_log.h>

static const char* TAG = "tinybft_example";

extern "C" {

void tinybft_example_task(void* pvParameters)
{
    (void)pvParameters;
    
    ESP_LOGI(TAG, "Initializing TinyBFT transport...");
    
    using namespace libbyzea;
    
    // This will be called after Node is initialized
    // The actual initialization happens in Node::init_transport()
    
    ESP_LOGI(TAG, "TinyBFT example task running");
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

} // extern "C"
