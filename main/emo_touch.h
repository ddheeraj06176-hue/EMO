#pragma once

#include "display.h"
#include <esp_log.h>

#define PET_TAG "EmoTouch"

#if defined(CONFIG_IDF_TARGET_ESP32S3)
#include "driver/gpio.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define TOUCH_IRQ_PIN GPIO_NUM_40

static void EmoPettingTask(void* pvParameters) {
    Display* display = (Display*)pvParameters;

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << TOUCH_IRQ_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    ESP_LOGI(PET_TAG, "Screen Pet Interaction active on GPIO %d", TOUCH_IRQ_PIN);

    while (true) {
        if (gpio_get_level(TOUCH_IRQ_PIN) == 0) {
            ESP_LOGI(PET_TAG, "Petting detected!");
            if (display != nullptr) {
                display->SetEmotion("happy");
                display->SetStatus("LOVES PETS <3");
            }
            vTaskDelay(pdMS_TO_TICKS(1500));
            if (display != nullptr) {
                display->SetStatus("EMO ONLINE");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

inline void StartEmoTouchService(Display* display) {
    if (display != nullptr) {
        xTaskCreatePinnedToCore(EmoPettingTask, "EmoPetTask", 3072, display, 1, NULL, 1);
    }
}
#else
// Non-S3 targets par safe stub
inline void StartEmoTouchService(Display* display) {
    // Non-S3 boards par silently pass
}
#endif
