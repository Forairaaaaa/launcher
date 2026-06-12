#include "ui/recorder.hpp"

#include "lvgl/lvgl.h"

namespace {

struct RecorderUi {
    bool recording = false;
    uint32_t elapsed_seconds = 0;
    lv_timer_t *timer = nullptr;
    lv_obj_t *status_label = nullptr;
    lv_obj_t *time_label = nullptr;
    lv_obj_t *button = nullptr;
    lv_obj_t *button_label = nullptr;
};

RecorderUi g_ui;

void set_plain(lv_obj_t *obj)
{
    lv_obj_remove_style_all(obj);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

void update_ui()
{
    lv_label_set_text(g_ui.status_label, g_ui.recording ? "Recording" : "Ready");
    lv_label_set_text_fmt(g_ui.time_label, "%02lu:%02lu",
                          (unsigned long)(g_ui.elapsed_seconds / 60),
                          (unsigned long)(g_ui.elapsed_seconds % 60));
    lv_label_set_text(g_ui.button_label, g_ui.recording ? "STOP" : "REC");

    lv_obj_set_style_bg_color(g_ui.button,
                              lv_color_hex(g_ui.recording ? 0x333333 : 0xd33f49),
                              0);
}

void timer_cb(lv_timer_t *)
{
    if (!g_ui.recording) {
        return;
    }

    ++g_ui.elapsed_seconds;
    update_ui();
}

void button_cb(lv_event_t *)
{
    g_ui.recording = !g_ui.recording;
    if (g_ui.recording) {
        g_ui.elapsed_seconds = 0;
    }
    update_ui();
}

lv_obj_t *make_label(lv_obj_t *parent, const char *text, const lv_font_t *font)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xf4f4f5), 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    return label;
}

void build_ui()
{
    lv_obj_t *screen = lv_screen_active();
    set_plain(screen);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x101114), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    lv_obj_t *root = lv_obj_create(screen);
    set_plain(root);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(root,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(root, 10, 0);

    lv_obj_t *title = make_label(root, "Recorder", &lv_font_montserrat_18);
    (void)title;

    g_ui.status_label = make_label(root, "Ready", &lv_font_montserrat_14);

    g_ui.time_label = make_label(root, "00:00", &lv_font_montserrat_20);

    g_ui.button = lv_button_create(root);
    lv_obj_set_size(g_ui.button, 96, 36);
    lv_obj_set_style_radius(g_ui.button, 6, 0);
    lv_obj_set_style_border_width(g_ui.button, 0, 0);
    lv_obj_set_style_shadow_width(g_ui.button, 0, 0);
    lv_obj_add_event_cb(g_ui.button, button_cb, LV_EVENT_CLICKED, nullptr);

    g_ui.button_label = make_label(g_ui.button, "REC", &lv_font_montserrat_14);
    lv_obj_center(g_ui.button_label);

    g_ui.timer = lv_timer_create(timer_cb, 1000, nullptr);
    update_ui();
}

} // namespace

void recorder_ui_init(void)
{
    build_ui();
}
