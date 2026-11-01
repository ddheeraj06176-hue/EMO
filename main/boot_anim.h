#pragma once

#include "display.h"
#include <esp_log.h>

#define BOOT_TAG "HarmonyBoot"

#if defined(CONFIG_DISPLAY_TYPE) || defined(CONFIG_LV_COLOR_DEPTH)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <lvgl.h>
#include <esp_lvgl_port.h>

inline void RunEmoBootAnimation(Display* display) {
    if (display == nullptr) return;

    ESP_LOGI(BOOT_TAG, "Running Slow HarmonyOS Cinematic Boot Animation...");

    lv_obj_t* prev_scr = lv_scr_act();

    // 1. Pitch Black Canvas
    lvgl_port_lock(0);
    lv_obj_t* boot_scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(boot_scr, lv_color_black(), 0);
    lv_scr_load(boot_scr);

    // 2. Center Harmony Arc Ring
    lv_obj_t* arc = lv_arc_create(boot_scr);
    lv_obj_set_size(arc, 120, 120);
    lv_arc_set_rotation(arc, 270);
    lv_arc_set_bg_angles(arc, 0, 360);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_center(arc);

    lv_obj_set_style_arc_color(arc, lv_color_hex(0x111625), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 4, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0x00E5FF), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, 4, LV_PART_INDICATOR);

    // Minimal Clean Welcome Text
    lv_obj_t* txt_title = lv_label_create(boot_scr);
    lv_label_set_text(txt_title, "W E L C O M E");
    lv_obj_set_style_text_color(txt_title, lv_color_white(), 0);
    lv_obj_set_style_text_font(txt_title, &lv_font_montserrat_14, 0);
    lv_obj_center(txt_title);
    lvgl_port_unlock();

    // SLOW & SMOOTH Ring Sweep (Cinematic 360 Rotation)
    for (int angle = 0; angle <= 360; angle += 10) {
        lvgl_port_lock(0);
        lv_arc_set_angles(arc, 0, angle);
        lvgl_port_unlock();
        vTaskDelay(pdMS_TO_TICKS(45)); // Slow fluid rotation
    }

    // Long Pause taaki 'WELCOME' aaram se padh sakein
    vTaskDelay(pdMS_TO_TICKS(2200));

    // 3. Subtext Transition: Initializing System
    lvgl_port_lock(0);
    lv_label_set_text(txt_title, "E M O  O S");
    lv_obj_set_style_text_color(txt_title, lv_color_hex(0x00E5FF), 0);
    lv_obj_center(txt_title);

    lv_obj_t* sub_txt = lv_label_create(boot_scr);
    lv_label_set_text(sub_txt, "Your device is booting now...");
    lv_obj_set_style_text_color(sub_txt, lv_color_hex(0x90A4AE), 0);
    lv_obj_align(sub_txt, LV_ALIGN_CENTER, 0, 32);
    lvgl_port_unlock();

    // Solid 2.5 second pause text padhne ke liye
    vTaskDelay(pdMS_TO_TICKS(2500));

    // Clean first phase
    lvgl_port_lock(0);
    lv_obj_del(arc);
    lv_obj_del(txt_title);
    lv_obj_del(sub_txt);

    // 4. Harmony Card Flare Presentation
    lv_obj_t* card = lv_obj_create(boot_scr);
    lv_obj_set_size(card, 210, 95);
    lv_obj_center(card);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x070B14), 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_radius(card, 16, 0);

    lv_obj_t* emo_main = lv_label_create(card);
    lv_label_set_text(emo_main, "E M O");
    lv_obj_set_style_text_color(emo_main, lv_color_hex(0x00E5FF), 0);
    lv_obj_align(emo_main, LV_ALIGN_TOP_MID, 0, 6);

    lv_obj_t* emo_sub = lv_label_create(card);
    lv_label_set_text(emo_sub, "AI COMPANION");
    lv_obj_set_style_text_color(emo_sub, lv_color_white(), 0);
    lv_obj_align(emo_sub, LV_ALIGN_CENTER, 0, 8);

    lv_obj_t* author = lv_label_create(boot_scr);
    lv_label_set_text(author, "POWERED BY DHEERAJ");
    lv_obj_set_style_text_color(author, lv_color_hex(0x80D8FF), 0);
    lv_obj_align(author, LV_ALIGN_BOTTOM_RIGHT, -20, -15);
    lvgl_port_unlock();

    // Slow Breath Lighting Pulse
    for (int p = 0; p < 2; p++) {
        lvgl_port_lock(0);
        lv_obj_set_style_border_color(card, lv_color_white(), 0);
        lv_obj_set_style_border_width(card, 3, 0);
        lvgl_port_unlock();
        vTaskDelay(pdMS_TO_TICKS(250)); // Slow pulse in

        lvgl_port_lock(0);
        lv_obj_set_style_border_color(card, lv_color_hex(0x00E5FF), 0);
        lv_obj_set_style_border_width(card, 2, 0);
        lvgl_port_unlock();
        vTaskDelay(pdMS_TO_TICKS(350)); // Slow pulse out
    }

    // Freeze time for EMO + Dheeraj card
    vTaskDelay(pdMS_TO_TICKS(2200));

    // 5. HarmonyOS Cinematic Shutter Reveal
    lvgl_port_lock(0);
    lv_obj_del(card);
    lv_obj_del(author);

    lv_obj_t* shutter_top = lv_obj_create(boot_scr);
    lv_obj_set_style_bg_color(shutter_top, lv_color_black(), 0);
    lv_obj_set_style_border_width(shutter_top, 0, 0);
    lv_obj_set_style_radius(shutter_top, 0, 0);

    lv_obj_t* shutter_bottom = lv_obj_create(boot_scr);
    lv_obj_set_style_bg_color(shutter_bottom, lv_color_black(), 0);
    lv_obj_set_style_border_width(shutter_bottom, 0, 0);
    lv_obj_set_style_radius(shutter_bottom, 0, 0);
    lvgl_port_unlock();

    // Slow eye aperture opening (Vertical split like an eyelid opening)
    for (int h = 120; h >= 0; h -= 12) {
        lvgl_port_lock(0);
        lv_obj_set_size(shutter_top, 320, h);
        lv_obj_align(shutter_top, LV_ALIGN_TOP_MID, 0, 0);

        lv_obj_set_size(shutter_bottom, 320, h);
        lv_obj_align(shutter_bottom, LV_ALIGN_BOTTOM_MID, 0, 0);
        lvgl_port_unlock();

        vTaskDelay(pdMS_TO_TICKS(40)); // Smooth cinematic open
    }

    // 6. Safe UI Handover
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
