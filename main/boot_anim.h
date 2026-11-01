#pragma once

#include "display.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <lvgl.h>

#define BOOT_TAG "CinematicBoot"

inline void RunEmoBootAnimation(Display* display) {
    if (display == nullptr) return;

    ESP_LOGI(BOOT_TAG, "Running Welcome -> Booting -> EMO Eyes...");

    // 1. Black Canvas
    lv_obj_t* boot_scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(boot_scr, lv_color_black(), 0);
    lv_scr_load(boot_scr);

    // 2. Center Text Label (Welcome & Booting Sequence)
    lv_obj_t* status_txt = lv_label_create(boot_scr);
    lv_obj_set_style_text_color(status_txt, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_text_font(status_txt, &lv_font_montserrat_14, 0);

    // Step A: WELCOME
    lv_label_set_text(status_txt, "W E L C O M E");
    lv_obj_center(status_txt);
    lv_refr_now(NULL);
    vTaskDelay(pdMS_TO_TICKS(1200));

    // Step B: Welcome Fade/Transition -> Your device is booting now...
    lv_label_set_text(status_txt, "");
    lv_refr_now(NULL);
    vTaskDelay(pdMS_TO_TICKS(200));

    lv_label_set_text(status_txt, "Your device is booting now...");
    lv_obj_center(status_txt);
    lv_refr_now(NULL);
    vTaskDelay(pdMS_TO_TICKS(1400));

    lv_obj_del(status_txt);

    // 3. Glowing Card - "EMO AI ROBOT"
    lv_obj_t* card = lv_obj_create(boot_scr);
    lv_obj_set_size(card, 180, 90);
    lv_obj_center(card);
    lv_obj_set_style_bg_color(card, lv_color_black(), 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_border_width(card, 3, 0);
    lv_obj_set_style_radius(card, 12, 0);

    lv_obj_t* card_txt = lv_label_create(card);
    lv_label_set_text(card_txt, "EMO\nAI ROBOT");
    lv_obj_set_style_text_color(card_txt, lv_color_white(), 0);
    lv_obj_set_style_text_align(card_txt, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(card_txt);

    lv_obj_t* dheeraj_lbl = lv_label_create(boot_scr);
    lv_label_set_text(dheeraj_lbl, "powered by\nDHEERAJ");
    lv_obj_set_style_text_color(dheeraj_lbl, lv_color_hex(0x80D8FF), 0);
    lv_obj_set_style_text_align(dheeraj_lbl, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(dheeraj_lbl, LV_ALIGN_BOTTOM_RIGHT, -20, -15);

    lv_refr_now(NULL);
    vTaskDelay(pdMS_TO_TICKS(1200));

    lv_obj_del(card);
    lv_obj_del(dheeraj_lbl);

    // 4. Center-Out Curtain Split (Window slide transition)
    lv_obj_t* shutter_left = lv_obj_create(boot_scr);
    lv_obj_set_style_bg_color(shutter_left, lv_color_black(), 0);
    lv_obj_set_style_border_width(shutter_left, 0, 0);
    lv_obj_set_style_radius(shutter_left, 0, 0);

    lv_obj_t* shutter_right = lv_obj_create(boot_scr);
    lv_obj_set_style_bg_color(shutter_right, lv_color_black(), 0);
    lv_obj_set_style_border_width(shutter_right, 0, 0);
    lv_obj_set_style_radius(shutter_right, 0, 0);

    for (int w = 160; w >= 0; w -= 16) {
        lv_obj_set_size(shutter_left, w, 240);
        lv_obj_align(shutter_left, LV_ALIGN_LEFT_MID, 0, 0);

        lv_obj_set_size(shutter_right, w, 240);
        lv_obj_align(shutter_right, LV_ALIGN_RIGHT_MID, 0, 0);

        lv_refr_now(NULL);
        vTaskDelay(pdMS_TO_TICKS(25));
    }

    // 5. Clean up & Directly load big EMO Eyes
    lv_obj_del(boot_scr);

    display->SetEmotion("happy");
    display->SetStatus("EMO ONLINE");
}
