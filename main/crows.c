
#include "esp_err.h"
#include "esp_littlefs.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "portmacro.h"
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

void app_main(void) {
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  printf("Initializing littlefs\n");

  esp_vfs_littlefs_conf_t conf = {
      .base_path = "/data",
      .partition_label = "storage",
      .format_if_mount_failed = true,
      .dont_mount = false,
  };

  esp_err_t ret = esp_vfs_littlefs_register(&conf);

  if (ret != ESP_OK) {
    if (ret == ESP_FAIL) {
      printf("Failed to mount or format filesystem\n");
    } else if (ret == ESP_ERR_NOT_FOUND) {
      printf("Failed to find littlefs partition\n");
    } else {
      printf("Failed to initialize littlefs (%s)\n", esp_err_to_name(ret));
    }
    return;
  }

  size_t total = 0, used = 0;
  ret = esp_littlefs_info(conf.partition_label, &total, &used);
  if (ret != ESP_OK) {
    printf("Failed to get littlefs partition information (%s)\n",
           esp_err_to_name(ret));
    esp_littlefs_format(conf.partition_label);
  } else {
    printf("Partition total: %d, used: %d\n", total, used);
  }
}
