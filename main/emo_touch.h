#pragma once

#include "display.h"
#include "driver/gpio.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#define TOUCH_IRQ_PIN GPIO_NUM_40
#define PET_TAG "EmoTouch"

static void EmoPettingTask(void* pvParameters) {
    Display* display = (Display*)pvParameters;

    // GPIO 40 ko Input + Pull-up configure karo
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << TOUCH_IRQ_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    ESP_LOGI(PET_TAG, "Screen Pet Interaction active on GPIO %d", TOUCH_IRQ_PIN);

    while (true) {
        // T_IRQ touch hote hi LOW (0) hota hai
        if (gpio_get_level(TOUCH_IRQ_PIN) == 0) {
            ESP_LOGI(PET_TAG, "Petting detected! Making EMO happy...");

            if (display != nullptr) {
                display->SetEmotion("happy");
                display->SetStatus("LOVES PETS <3");
            }

            // Hold / Debounce delay taaki continuous smooth react kare
            vTaskDelay(pdMS_TO_TICKS(1500));

            if (display != nullptr) {
                display->SetStatus("EMO ONLINE");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Single one-liner function to start the touch task
inline void StartEmoTouchService(Display* display) {
    xTaskCreatePinnedToCore(EmoPettingTask, "EmoPetTask", 3072, display, 1, NULL, 1);
}
