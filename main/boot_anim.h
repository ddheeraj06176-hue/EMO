#pragma once

#include "display.h"
#include <esp_log.h>

#define BOOT_TAG "CyberBoot"

#if defined(CONFIG_DISPLAY_TYPE) || defined(CONFIG_LV_COLOR_DEPTH)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <lvgl.h>
#include <esp_lvgl_port.h>

inline void RunEmoBootAnimation(Display* display) {
    if (display == nullptr) return;

    ESP_LOGI(BOOT_TAG, "Running Clean Cyber Lines & Window Split Boot...");

    lv_obj_t* prev_scr = lv_scr_act();

    // 1. Pitch Black Canvas
    lvgl_port_lock(0);
    lv_obj_t* boot_scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(boot_scr, lv_color_black(), 0);
    lv_scr_load(boot_scr);

    // 2. Expanding Neon Cyan Lines (Center se bahar nikalne wali do vertical lines)
    lv_obj_t* line_l = lv_obj_create(boot_scr);
    lv_obj_t* line_r = lv_obj_create(boot_scr);
    lv_obj_set_style_bg_color(line_l, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_bg_color(line_r, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_border_width(line_l, 0, 0);
    lv_obj_set_style_border_width(line_r, 0, 0);
    lvgl_port_unlock();

    // Lines ka expansion
    for (int dist = 0; dist <= 95; dist += 5) {
        lvgl_port_lock(0);
        lv_obj_set_size(line_l, 3, 75);
        lv_obj_align(line_l, LV_ALIGN_CENTER, -dist, 0);

        lv_obj_set_size(line_r, 3, 75);
        lv_obj_align(line_r, LV_ALIGN_CENTER, dist, 0);
        lvgl_port_unlock();

        vTaskDelay(pdMS_TO_TICKS(25));
    }

    // Lines remove
    lvgl_port_lock(0);
    lv_obj_del(line_l);
    lv_obj_del(line_r);

    // 3. EMO AI ROBOT Central Card Box
    lv_obj_t* card = lv_obj_create(boot_scr);
    lv_obj_set_size(card, 210, 95);
    lv_obj_center(card);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x08101E), 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_radius(card, 14, 0);

    lv_obj_t* emo_main = lv_label_create(card);
    lv_label_set_text(emo_main, "EMO");
    lv_obj_set_style_text_color(emo_main, lv_color_hex(0x00E5FF), 0);
    lv_obj_align(emo_main, LV_ALIGN_TOP_MID, 0, 6);

    lv_obj_t* emo_sub = lv_label_create(card);
    lv_label_set_text(emo_sub, "AI ROBOT");
    lv_obj_set_style_text_color(emo_sub, lv_color_white(), 0);
    lv_obj_align(emo_sub, LV_ALIGN_CENTER, 0, 8);

    lv_obj_t* author = lv_label_create(boot_scr);
    lv_label_set_text(author, "POWERED BY DHEERAJ");
    lv_obj_set_style_text_color(author, lv_color_hex(0x00E5FF), 0);
    lv_obj_align(author, LV_ALIGN_BOTTOM_RIGHT, -20, -15);
    lvgl_port_unlock();

    // Smooth Neon Glow Pulse
    for (int p = 0; p < 2; p++) {
        lvgl_port_lock(0);
        lv_obj_set_style_border_color(card, lv_color_white(), 0);
        lv_obj_set_style_border_width(card, 4, 0);
        lvgl_port_unlock();
        vTaskDelay(pdMS_TO_TICKS(220));

        lvgl_port_lock(0);
        lv_obj_set_style_border_color(card, lv_color_hex(0x00E5FF), 0);
        lv_obj_set_style_border_width(card, 2, 0);
        lvgl_port_unlock();
        vTaskDelay(pdMS_TO_TICKS(280));
    }

    // Steady freeze
    vTaskDelay(pdMS_TO_TICKS(1800));

    // Card remove
    lvgl_port_lock(0);
    lv_obj_del(card);
    lv_obj_del(author);

    // 4. Vertical Side-by-Side Curtain Split (Left-Right Window Open)
    lv_obj_t* shutter_left = lv_obj_create(boot_scr);
    lv_obj_set_style_bg_color(shutter_left, lv_color_black(), 0);
    lv_obj_set_style_border_width(shutter_left, 0, 0);
    lv_obj_set_style_radius(shutter_left, 0, 0);

    lv_obj_t* shutter_right = lv_obj_create(boot_scr);
    lv_obj_set_style_bg_color(shutter_right, lv_color_black(), 0);
    lv_obj_set_style_border_width(shutter_right, 0, 0);
    lv_obj_set_style_radius(shutter_right, 0, 0);
    lvgl_port_unlock();

    // Dono taraf parde ki tarah khidki khulegi (160px se 0px)
    for (int w = 160; w >= 0; w -= 16) {
        lvgl_port_lock(0);
        lv_obj_set_size(shutter_left, w, 240);
        lv_obj_align(shutter_left, LV_ALIGN_LEFT_MID, 0, 0);

        lv_obj_set_size(shutter_right, w, 240);
        lv_obj_align(shutter_right, LV_ALIGN_RIGHT_MID, 0, 0);
        lvgl_port_unlock();

        vTaskDelay(pdMS_TO_TICKS(35));
    }

    // 5. Clean Screen & UI Handover
    lvgl_port_lock(0);
    if (prev_scr != nullptr) {
        lv_scr_load(prev_scr);
    }
    lv_obj_del(boot_scr);
    lvgl_port_unlock();

    vTaskDelay(pdMS_TO_TICKS(100));

    display->SetEmotion("neutral");
    display->SetStatus("EMO ONLINE");
}
#else
inline void RunEmoBootAnimation(Display* display) {}
#endif
