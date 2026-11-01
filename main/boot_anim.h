#pragma once

#include "display.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <lvgl.h>
#include <esp_lvgl_port.h>

#define BOOT_TAG "CinematicBoot"

inline void RunEmoBootAnimation(Display* display) {
    if (display == nullptr) return;

    ESP_LOGI(BOOT_TAG, "Running Welcome -> Booting -> EMO Eyes (WDT Safe)...");

    // 1. Create Canvas with Lock
    lvgl_port_lock(0);
    lv_obj_t* boot_scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(boot_scr, lv_color_black(), 0);
    lv_scr_load(boot_scr);

    lv_obj_t* status_txt = lv_label_create(boot_scr);
    lv_obj_set_style_text_color(status_txt, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_text_font(status_txt, &lv_font_montserrat_14, 0);
    lv_label_set_text(status_txt, "W E L C O M E");
    lv_obj_center(status_txt);
    lvgl_port_unlock();

    // Delay outside lock so watchdog and background tasks get CPU time
    vTaskDelay(pdMS_TO_TICKS(1000));

    // 2. Transition text
    lvgl_port_lock(0);
    lv_label_set_text(status_txt, "Your device is booting now...");
    lv_obj_center(status_txt);
    lvgl_port_unlock();

    vTaskDelay(pdMS_TO_TICKS(1200));

    // 3. Card sequence
    lvgl_port_lock(0);
    lv_obj_del(status_txt);

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
    lvgl_port_unlock();

    vTaskDelay(pdMS_TO_TICKS(1200));

    // 4. Center-Out Curtain Split
    lvgl_port_lock(0);
    lv_obj_del(card);
    lv_obj_del(dheeraj_lbl);

    lv_obj_t* shutter_left = lv_obj_create(boot_scr);
    lv_obj_set_style_bg_color(shutter_left, lv_color_black(), 0);
    lv_obj_set_style_border_width(shutter_left, 0, 0);
    lv_obj_set_style_radius(shutter_left, 0, 0);

    lv_obj_t* shutter_right = lv_obj_create(boot_scr);
    lv_obj_set_style_bg_color(shutter_right, lv_color_black(), 0);
    lv_obj_set_style_border_width(shutter_right, 0, 0);
    lv_obj_set_style_radius(shutter_right, 0, 0);
    lvgl_port_unlock();

    for (int w = 160; w >= 0; w -= 20) {
        lvgl_port_lock(0);
        lv_obj_set_size(shutter_left, w, 240);
        lv_obj_align(shutter_left, LV_ALIGN_LEFT_MID, 0, 0);

        lv_obj_set_size(shutter_right, w, 240);
        lv_obj_align(shutter_right, LV_ALIGN_RIGHT_MID, 0, 0);
        lvgl_port_unlock();

        vTaskDelay(pdMS_TO_TICKS(30));
    }

    // 5. Clean up & Load EMO Eyes
    lvgl_port_lock(0);
    lv_obj_del(boot_scr);
    lvgl_port_unlock();

    display->SetEmotion("happy");
    display->SetStatus("EMO ONLINE");
}
