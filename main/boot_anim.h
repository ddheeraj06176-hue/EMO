#pragma once

#include "display.h"
#include <esp_log.h>

#define BOOT_TAG "CyberBoot"

// कंडीशन: सिर्फ़ LCD / Display वाले बोर्ड्स पर ही LVGL कोड चलेगा
#if defined(CONFIG_DISPLAY_TYPE) || defined(CONFIG_LV_COLOR_DEPTH)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <lvgl.h>
#include <esp_lvgl_port.h>

inline void RunEmoBootAnimation(Display* display) {
    if (display == nullptr) return;

    ESP_LOGI(BOOT_TAG, "Running Modern Cyber Glow Boot Animation...");

    lv_obj_t* prev_scr = lv_scr_act();

    // 1. Deep Space Canvas
    lvgl_port_lock(0);
    lv_obj_t* boot_scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(boot_scr, lv_color_hex(0x050811), 0);
    lv_scr_load(boot_scr);

    // 2. High-Tech Neon Glow Ring
    lv_obj_t* arc = lv_arc_create(boot_scr);
    lv_obj_set_size(arc, 130, 130);
    lv_arc_set_rotation(arc, 270);
    lv_arc_set_bg_angles(arc, 0, 360);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_center(arc);

    lv_obj_set_style_arc_color(arc, lv_color_hex(0x001a33), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 5, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0x00E5FF), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, 5, LV_PART_INDICATOR);

    lv_obj_t* txt_title = lv_label_create(boot_scr);
    lv_label_set_text(txt_title, "W E L C O M E");
    lv_obj_set_style_text_color(txt_title, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_text_font(txt_title, &lv_font_montserrat_14, 0);
    lv_obj_center(txt_title);
    lvgl_port_unlock();

    for (int angle = 0; angle <= 360; angle += 24) {
        lvgl_port_lock(0);
        lv_arc_set_angles(arc, 0, angle);
        lvgl_port_unlock();
        vTaskDelay(pdMS_TO_TICKS(40));
    }

    vTaskDelay(pdMS_TO_TICKS(1200));

    // 3. Subtext Transition
    lvgl_port_lock(0);
    lv_label_set_text(txt_title, "INITIALIZING SYSTEM");
    lv_obj_set_style_text_color(txt_title, lv_color_hex(0x80D8FF), 0);
    lv_obj_center(txt_title);

    lv_obj_t* sub_txt = lv_label_create(boot_scr);
    lv_label_set_text(sub_txt, "Your device is booting now...");
    lv_obj_set_style_text_color(sub_txt, lv_color_hex(0x546E7A), 0);
    lv_obj_align(sub_txt, LV_ALIGN_CENTER, 0, 30);
    lvgl_port_unlock();

    vTaskDelay(pdMS_TO_TICKS(1800));

    lvgl_port_lock(0);
    lv_obj_del(arc);
    lv_obj_del(txt_title);
    lv_obj_del(sub_txt);

    // 4. Futuristic Hologram Card
    lv_obj_t* card = lv_obj_create(boot_scr);
    lv_obj_set_size(card, 210, 100);
    lv_obj_center(card);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x0a1128), 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_radius(card, 16, 0);

    lv_obj_t* emo_main = lv_label_create(card);
    lv_label_set_text(emo_main, "E M O");
    lv_obj_set_style_text_color(emo_main, lv_color_hex(0x00E5FF), 0);
    lv_obj_align(emo_main, LV_ALIGN_TOP_MID, 0, 8);

    lv_obj_t* emo_sub = lv_label_create(card);
    lv_label_set_text(emo_sub, "AI COMPANION");
    lv_obj_set_style_text_color(emo_sub, lv_color_white(), 0);
    lv_obj_align(emo_sub, LV_ALIGN_CENTER, 0, 10);

    lv_obj_t* author = lv_label_create(boot_scr);
    lv_label_set_text(author, "POWERED BY DHEERAJ");
    lv_obj_set_style_text_color(author, lv_color_hex(0x00E5FF), 0);
    lv_obj_align(author, LV_ALIGN_BOTTOM_RIGHT, -20, -15);
    lvgl_port_unlock();

    for (int p = 0; p < 2; p++) {
        lvgl_port_lock(0);
        lv_obj_set_style_border_color(card, lv_color_white(), 0);
        lv_obj_set_style_border_width(card, 4, 0);
        lvgl_port_unlock();
        vTaskDelay(pdMS_TO_TICKS(100));

        lvgl_port_lock(0);
        lv_obj_set_style_border_color(card, lv_color_hex(0x00E5FF), 0);
        lv_obj_set_style_border_width(card, 2, 0);
        lvgl_port_unlock();
        vTaskDelay(pdMS_TO_TICKS(180));
    }

    vTaskDelay(pdMS_TO_TICKS(1500));

    // 5. Center-Out Curtain Split
    lvgl_port_lock(0);
    lv_obj_del(card);
    lv_obj_del(author);

    lv_obj_t* shutter_l = lv_obj_create(boot_scr);
    lv_obj_set_style_bg_color(shutter_l, lv_color_hex(0x050811), 0);
    lv_obj_set_style_border_width(shutter_l, 0, 0);
    lv_obj_set_style_radius(shutter_l, 0, 0);

    lv_obj_t* shutter_r = lv_obj_create(boot_scr);
    lv_obj_set_style_bg_color(shutter_r, lv_color_hex(0x050811), 0);
    lv_obj_set_style_border_width(shutter_r, 0, 0);
    lv_obj_set_style_radius(shutter_r, 0, 0);
    lvgl_port_unlock();

    for (int w = 160; w >= 0; w -= 20) {
        lvgl_port_lock(0);
        lv_obj_set_size(shutter_l, w, 240);
        lv_obj_align(shutter_l, LV_ALIGN_LEFT_MID, 0, 0);

        lv_obj_set_size(shutter_r, w, 240);
        lv_obj_align(shutter_r, LV_ALIGN_RIGHT_MID, 0, 0);
        lvgl_port_unlock();

        vTaskDelay(pdMS_TO_TICKS(30));
    }

    // 6. Restore Original UI Screen
    lvgl_port_lock(0);
    if (prev_scr != nullptr) {
        lv_scr_load(prev_scr);
    }
    lv_obj_del(boot_scr);
    lvgl_port_unlock();

    vTaskDelay(pdMS_TO_TICKS(80));

    display->SetEmotion("neutral");
    display->SetStatus("EMO ONLINE");
}
#else
// Non-display boards ke liye empty safe stub
inline void RunEmoBootAnimation(Display* display) {
    // Bina display wale boards par kuch nahi karega, chupchap pass ho jayega
}
#endif
