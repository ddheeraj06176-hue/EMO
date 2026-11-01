#pragma once

#include "display.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <lvgl.h>

#define BOOT_TAG "CinematicBoot"

inline void RunEmoBootAnimation(Display* display) {
    if (display == nullptr) return;

    ESP_LOGI(BOOT_TAG, "Running Clean Cinematic Boot...");

    // 1. Black Screen Canvas
    lv_obj_t* boot_scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(boot_scr, lv_color_black(), 0);
    lv_scr_load(boot_scr);

    // 2. Cyan Glowing Ring (320x240 center)
    lv_obj_t* ring = lv_arc_create(boot_scr);
    lv_obj_set_size(ring, 120, 120);
    lv_obj_center(ring);
    lv_arc_set_rotation(ring, 270);
    lv_arc_set_bg_angles(ring, 0, 360);
    lv_obj_remove_style(ring, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(ring, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(ring, 4, LV_PART_MAIN);
    lv_obj_set_style_arc_color(ring, lv_color_hex(0x00E5FF), LV_PART_MAIN);

    // 3. Center EMO Logo
    lv_obj_t* logo_label = lv_label_create(boot_scr);
    lv_label_set_text(logo_label, "E  M  O");
    lv_obj_set_style_text_color(logo_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(logo_label, &lv_font_montserrat_14, 0);
    lv_obj_align(logo_label, LV_ALIGN_CENTER, 0, -6);

    // 4. Subtitle Dynamic Text
    lv_obj_t* sub_label = lv_label_create(boot_scr);
    lv_label_set_text(sub_label, "");
    lv_obj_set_style_text_color(sub_label, lv_color_hex(0x80D8FF), 0);
    lv_obj_set_style_text_font(sub_label, &lv_font_montserrat_14, 0);
    lv_obj_align(sub_label, LV_ALIGN_BOTTOM_MID, 0, -20);

    // Initial draw
    lv_refr_now(NULL);

    // Step A: Ring ko 360 degree ghumao
    for (int i = 0; i <= 360; i += 30) {
        lv_arc_set_angles(ring, 0, i);
        lv_refr_now(NULL);
        vTaskDelay(pdMS_TO_TICKS(25));
    }

    // Step B: Text Animation - "by" se lekar "powered by dheeraj" tak
    const char* frames[] = {
        "by",
        "d by d",
        "ed by dh",
        "red by dhee",
        "ered by dheer",
        "powered by dheeraj"
    };

    for (int f = 0; f < 6; f++) {
        lv_label_set_text(sub_label, frames[f]);
        lv_obj_align(sub_label, LV_ALIGN_BOTTOM_MID, 0, -20);
        lv_refr_now(NULL);
        vTaskDelay(pdMS_TO_TICKS(110));
    }

    // Step C: Glow Hold
    lv_obj_set_style_text_color(logo_label, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_text_color(sub_label, lv_color_white(), 0);
    lv_refr_now(NULL);
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Step D: Boot screen delete karke EMO main UI par shift
    lv_obj_del(boot_scr);

    display->SetStatus("EMO ONLINE");
    display->SetEmotion("happy");
}
