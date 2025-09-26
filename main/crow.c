#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "esp_log.h"
#include "esp_littlefs.h"

// WAV header structure
typedef struct {
    char riff[4];           // "RIFF"
    uint32_t file_size;     // File size - 8
    char wave[4];           // "WAVE"
    char fmt[4];            // "fmt "
    uint32_t fmt_size;      // Format chunk size
    uint16_t audio_format;  // Audio format (1 = PCM)
    uint16_t num_channels;  // Number of channels
    uint32_t sample_rate;   // Sample rate
    uint32_t byte_rate;     // Byte rate
    uint16_t block_align;   // Block align
    uint16_t bits_per_sample; // Bits per sample
    char data[4];           // "data"
    uint32_t data_size;     // Data chunk size
} __attribute__((packed)) wav_header_t;

static const char* TAG = "CROW";

// I2S pins for ESP32-C3
#define I2S_BCK_PIN     6
#define I2S_WS_PIN      7
#define I2S_DATA_PIN    5
#define SAMPLE_RATE     16000

static i2s_chan_handle_t tx_handle = NULL;

void init_littlefs(void) {
    ESP_LOGI(TAG, "Initializing LittleFS...");

    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/data",
        .partition_label = "storage",
        .format_if_mount_failed = true,
        .dont_mount = false,
    };

    esp_err_t ret = esp_vfs_littlefs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find LittleFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_littlefs_info("storage", &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get LittleFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "LittleFS: %d kB total, %d kB used", total / 1024, used / 1024);
    }
}

void init_i2s(void) {
    ESP_LOGI(TAG, "Initializing I2S...");

    // I2S channel configuration
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    esp_err_t ret = i2s_new_channel(&chan_cfg, &tx_handle, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2S channel: %s", esp_err_to_name(ret));
        return;
    }

    // I2S standard configuration with MONO mode
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_BCK_PIN,
            .ws = I2S_WS_PIN,
            .dout = I2S_DATA_PIN,
            .din = I2S_GPIO_UNUSED,
        },
    };

    ret = i2s_channel_init_std_mode(tx_handle, &std_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2S standard mode: %s", esp_err_to_name(ret));
        return;
    }

    ret = i2s_channel_enable(tx_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable I2S channel: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "I2S initialized: %d Hz, mono, pins BCK=%d WS=%d DATA=%d",
             SAMPLE_RATE, I2S_BCK_PIN, I2S_WS_PIN, I2S_DATA_PIN);
}

void play_wav_file(const char* filename) {
    ESP_LOGI(TAG, "Opening WAV file: %s", filename);

    FILE* file = fopen(filename, "rb");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open WAV file: %s", filename);
        return;
    }

    // Read and validate WAV header
    wav_header_t header;
    if (fread(&header, sizeof(wav_header_t), 1, file) != 1) {
        ESP_LOGE(TAG, "Failed to read WAV header");
        fclose(file);
        return;
    }

    // Verify it's a valid WAV file
    if (strncmp(header.riff, "RIFF", 4) != 0 || strncmp(header.wave, "WAVE", 4) != 0) {
        ESP_LOGE(TAG, "Invalid WAV file format");
        fclose(file);
        return;
    }

    ESP_LOGI(TAG, "WAV: %ld Hz, %d channels, %d bits, %ld bytes",
             header.sample_rate, header.num_channels, header.bits_per_sample, header.data_size);

    // Read and play audio data in chunks
    uint8_t buffer[1024];
    size_t bytes_written;
    uint32_t total_read = 0;

    while (!feof(file) && total_read < header.data_size) {
        size_t to_read = (header.data_size - total_read) > sizeof(buffer) ?
                         sizeof(buffer) : (header.data_size - total_read);
        size_t bytes_read = fread(buffer, 1, to_read, file);

        if (bytes_read == 0) break;

        // For mono WAV files, write directly to I2S (native mono support!)
        esp_err_t ret = i2s_channel_write(tx_handle, buffer, bytes_read, &bytes_written, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "I2S write failed: %s", esp_err_to_name(ret));
            break;
        }

        total_read += bytes_read;
    }

    fclose(file);
    ESP_LOGI(TAG, "WAV playback complete: %ld bytes played", total_read);
}

void app_main(void) {
    // Short delay to allow serial monitoring to connect
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGI(TAG, "Crow Audio System Starting...");

    // Initialize filesystem
    init_littlefs();

    // Initialize I2S audio
    init_i2s();

    ESP_LOGI(TAG, "Initialization complete!");

    // Test reading from LittleFS
    FILE* file = fopen("/data/test.txt", "r");
    if (file) {
        char buffer[64];
        if (fgets(buffer, sizeof(buffer), file)) {
            ESP_LOGI(TAG, "Read from LittleFS: %s", buffer);
        }
        fclose(file);
    } else {
        ESP_LOGE(TAG, "Failed to open test file");
    }

    // Test writing to LittleFS
    FILE* write_file = fopen("/data/runtime_test.txt", "w");
    if (write_file) {
        fprintf(write_file, "LittleFS write test at runtime!\n");
        fclose(write_file);
        ESP_LOGI(TAG, "Successfully wrote to LittleFS");
    } else {
        ESP_LOGE(TAG, "Failed to write to LittleFS");
    }

    // Play the StreetChicken.wav file
    ESP_LOGI(TAG, "Playing StreetChicken.wav...");
    play_wav_file("/data/StreetChicken.wav");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));  // Longer delay after playback
        ESP_LOGI(TAG, "Playback complete. System ready.");
    }
}