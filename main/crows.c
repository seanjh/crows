
#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void) {
    printf("Hello from printf!\n");
    ESP_LOGI("main", "Hello from ESP_LOGI!");
    
    while(1) {
        printf("Loop %lu\n", xTaskGetTickCount());
        ESP_LOGI("main", "Loop tick %lu", xTaskGetTickCount());
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
