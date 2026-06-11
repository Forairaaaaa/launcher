#pragma once
#include "../ui_app_page.hpp"
#include <unordered_map>
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include "hal/hal_process.h"
#include "compat/input_keys.h"

// ============================================================
//  Audio Recorder
//  Screen: 320 x 170  (top bar 20px, ui_APP_Container 320x150)
//
//  Keys:
//    R          Start recording
//    S          Stop recording / playback
//    P          Play last recording
//    D          Delete selected recording
//    UP/DOWN    Navigate recording list
//    ESC        Stop active process, go back home
// ============================================================

enum class RecState { IDLE, RECORDING, PLAYING };

class RecModel
{
public:
    const std::vector<std::string> &recordings() const { return recordings_; }
    const std::string &current_file() const { return current_file_; }
    int selected_idx() const { return selected_idx_; }
    RecState state() const { return state_; }
    bool is_active() const { return state_ == RecState::RECORDING || state_ == RecState::PLAYING; }

    bool start_recording()
    {
        if (state_ != RecState::IDLE) return false;

        rec_counter_++;
        char fname[64];
        snprintf(fname, sizeof(fname), "/tmp/rec_%03d.wav", rec_counter_);
        current_file_ = fname;

#if defined(HAL_PLATFORM_SDL)
        create_simulated_recording(current_file_.c_str());
        active_pid_ = -1;
#else
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "arecord -f cd -t wav %s", current_file_.c_str());
        active_pid_ = hal_process_spawn(cmd, 0);
#endif

        state_ = RecState::RECORDING;
        printf("[Rec] Start recording: %s  pid=%d\n", current_file_.c_str(), active_pid_);
        return true;
    }

    bool stop_action()
    {
        if (state_ == RecState::RECORDING)
        {
            stop_backend();
            recordings_.push_back(current_file_);
            selected_idx_ = (int)recordings_.size() - 1;
            printf("[Rec] Stopped recording: %s\n", current_file_.c_str());
            state_ = RecState::IDLE;
            return true;
        }

        if (state_ == RecState::PLAYING)
        {
            stop_backend();
            printf("[Rec] Stopped playback\n");
            state_ = RecState::IDLE;
            return true;
        }

        return false;
    }

    bool play_selected()
    {
        if (state_ != RecState::IDLE) return false;
        if (recordings_.empty()) return false;

        const std::string &file = recordings_[selected_idx_];

#if defined(HAL_PLATFORM_SDL)
        active_pid_ = -1;
#else
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "aplay %s", file.c_str());
        active_pid_ = hal_process_spawn(cmd, 0);
#endif

        current_file_ = file;
        state_ = RecState::PLAYING;
        printf("[Rec] Playing: %s  pid=%d\n", file.c_str(), active_pid_);
        return true;
    }

    bool delete_selected()
    {
        if (recordings_.empty()) return false;
        if (state_ != RecState::IDLE) return false;

        const std::string file = recordings_[selected_idx_];
        ::unlink(file.c_str());
        printf("[Rec] Deleted: %s\n", file.c_str());

        recordings_.erase(recordings_.begin() + selected_idx_);
        if (selected_idx_ >= (int)recordings_.size() && selected_idx_ > 0)
            selected_idx_--;
        return true;
    }

    bool move_selection(int delta)
    {
        if (recordings_.empty()) return false;

        int next = selected_idx_ + delta;
        if (next < 0) next = 0;
        if (next >= (int)recordings_.size()) next = (int)recordings_.size() - 1;
        if (next == selected_idx_) return false;

        selected_idx_ = next;
        return true;
    }

    bool cancel_active()
    {
        if (!is_active()) return false;
        stop_backend();
        state_ = RecState::IDLE;
        return true;
    }

    void stop_backend()
    {
        if (active_pid_ > 0)
            hal_process_stop(active_pid_);
        active_pid_ = -1;
    }

private:
    std::vector<std::string> recordings_;
    int         selected_idx_ = 0;
    int         rec_counter_ = 0;
    RecState    state_ = RecState::IDLE;
    hal_pid_t   active_pid_ = -1;
    std::string current_file_;

#if defined(HAL_PLATFORM_SDL)
    void create_simulated_recording(const char *path)
    {
        FILE *fp = fopen(path, "wb");
        if (!fp) return;
        const char payload[] = "Simulated APPLaunch recorder output.\n";
        fwrite(payload, 1, sizeof(payload) - 1, fp);
        fclose(fp);
    }
#endif
};

class RecView
{
public:
    explicit RecView(lv_obj_t *container)
    {
        create_ui(container);
    }

    void rebuild_recordings(const std::vector<std::string> &recordings, int selected_idx)
    {
        lv_obj_t *list_cont = ui_obj_["list_cont"];
        lv_obj_clean(list_cont);

        if (recordings.empty())
        {
            lv_obj_t *lbl = lv_label_create(list_cont);
            lv_label_set_text(lbl, "(no recordings yet)");
            lv_obj_set_pos(lbl, 0, 0);
            lv_obj_set_style_text_color(lbl, lv_color_hex(0x555555), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(lbl, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
            return;
        }

        static constexpr int ROW_H = 14;
        static constexpr int MAX_VISIBLE = 5;

        int count = (int)recordings.size();
        int visible = (count < MAX_VISIBLE) ? count : MAX_VISIBLE;
        int offset = selected_idx - visible / 2;
        if (offset < 0) offset = 0;
        if (offset > count - visible) offset = count - visible;
        if (offset < 0) offset = 0;

        for (int vi = 0; vi < visible; ++vi)
        {
            int ri = vi + offset;
            bool is_sel = (ri == selected_idx);

            lv_obj_t *lbl = lv_label_create(list_cont);
            const std::string &path = recordings[ri];
            std::string display = path;
            size_t slash = path.rfind('/');
            if (slash != std::string::npos)
                display = path.substr(slash + 1);

            char buf[64];
            snprintf(buf, sizeof(buf), "%s %s", is_sel ? ">" : " ", display.c_str());
            lv_label_set_text(lbl, buf);
            lv_obj_set_pos(lbl, 0, vi * ROW_H);
            lv_obj_set_width(lbl, 300);
            lv_label_set_long_mode(lbl, LV_LABEL_LONG_CLIP);
            lv_obj_set_style_text_color(lbl,
                is_sel ? lv_color_hex(0x1F6FEB) : lv_color_hex(0xE6EDF3),
                LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(lbl, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }

    void update_status(RecState state)
    {
        lv_obj_t *lbl_status = ui_obj_["lbl_status"];
        lv_obj_t *red_dot = ui_obj_["red_dot"];

        switch (state)
        {
        case RecState::IDLE:
            lv_label_set_text(lbl_status, "READY");
            lv_obj_set_style_text_color(lbl_status, lv_color_hex(0xE6EDF3), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(red_dot, LV_OBJ_FLAG_HIDDEN);
            break;
        case RecState::RECORDING:
            lv_label_set_text(lbl_status, "RECORDING");
            lv_obj_set_style_text_color(lbl_status, lv_color_hex(0xE74C3C), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_clear_flag(red_dot, LV_OBJ_FLAG_HIDDEN);
            break;
        case RecState::PLAYING:
            lv_label_set_text(lbl_status, "PLAYING");
            lv_obj_set_style_text_color(lbl_status, lv_color_hex(0x2ECC71), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(red_dot, LV_OBJ_FLAG_HIDDEN);
            break;
        }
    }

    void update_elapsed(int elapsed_sec)
    {
        char buf[16];
        int mins = elapsed_sec / 60;
        int secs = elapsed_sec % 60;
        snprintf(buf, sizeof(buf), "%02d:%02d", mins, secs);
        lv_label_set_text(ui_obj_["lbl_time"], buf);
    }

    void update_file(const std::string &file)
    {
        if (file.empty())
        {
            lv_label_set_text(ui_obj_["lbl_file"], "File: (none)");
            return;
        }

        char buf[128];
        snprintf(buf, sizeof(buf), "File: %s", file.c_str());
        lv_label_set_text(ui_obj_["lbl_file"], buf);
    }

    void set_recording_dot_visible(bool visible)
    {
        lv_obj_t *red_dot = ui_obj_["red_dot"];
        if (visible)
            lv_obj_clear_flag(red_dot, LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_add_flag(red_dot, LV_OBJ_FLAG_HIDDEN);
    }

private:
    std::unordered_map<std::string, lv_obj_t *> ui_obj_;

    void create_ui(lv_obj_t *container)
    {
        lv_obj_t *bg = lv_obj_create(container);
        lv_obj_set_size(bg, 320, 150);
        lv_obj_set_pos(bg, 0, 0);
        lv_obj_set_style_radius(bg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(bg, lv_color_hex(0x0D1117), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(bg, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(bg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(bg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(bg, LV_OBJ_FLAG_SCROLLABLE);
        ui_obj_["bg"] = bg;

        lv_obj_t *title_bar = lv_obj_create(bg);
        lv_obj_set_size(title_bar, 320, 22);
        lv_obj_set_pos(title_bar, 0, 0);
        lv_obj_set_style_radius(title_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(title_bar, lv_color_hex(0x1F3A5F), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(title_bar, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(title_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(title_bar, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *lbl_title = lv_label_create(title_bar);
        lv_label_set_text(lbl_title, "Recorder");
        lv_obj_set_align(lbl_title, LV_ALIGN_LEFT_MID);
        lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t *lbl_hint = lv_label_create(title_bar);
        lv_label_set_text(lbl_hint, "R:Rec  S:Stop  P:Play  ESC:Back");
        lv_obj_set_align(lbl_hint, LV_ALIGN_RIGHT_MID);
        lv_obj_set_x(lbl_hint, -4);
        lv_obj_set_style_text_color(lbl_hint, lv_color_hex(0x7EA8D8), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl_hint, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t *content = lv_obj_create(bg);
        lv_obj_set_size(content, 320, 128);
        lv_obj_set_pos(content, 0, 22);
        lv_obj_set_style_radius(content, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(content, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(content, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(content, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
        ui_obj_["content"] = content;

        lv_obj_t *red_dot = lv_obj_create(content);
        lv_obj_set_size(red_dot, 10, 10);
        lv_obj_set_pos(red_dot, 10, 10);
        lv_obj_set_style_radius(red_dot, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(red_dot, lv_color_hex(0xE74C3C), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(red_dot, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(red_dot, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(red_dot, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(red_dot, LV_OBJ_FLAG_HIDDEN);
        ui_obj_["red_dot"] = red_dot;

        lv_obj_t *lbl_status = lv_label_create(content);
        lv_label_set_text(lbl_status, "READY");
        lv_obj_set_pos(lbl_status, 26, 4);
        lv_obj_set_style_text_color(lbl_status, lv_color_hex(0xE6EDF3), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl_status, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
        ui_obj_["lbl_status"] = lbl_status;

        lv_obj_t *lbl_time = lv_label_create(content);
        lv_label_set_text(lbl_time, "00:00");
        lv_obj_set_pos(lbl_time, 120, 4);
        lv_obj_set_style_text_color(lbl_time, lv_color_hex(0xE6EDF3), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl_time, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
        ui_obj_["lbl_time"] = lbl_time;

        lv_obj_t *lbl_file = lv_label_create(content);
        lv_label_set_text(lbl_file, "File: (none)");
        lv_obj_set_pos(lbl_file, 10, 24);
        lv_obj_set_width(lbl_file, 300);
        lv_label_set_long_mode(lbl_file, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_style_text_color(lbl_file, lv_color_hex(0x7EA8D8), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl_file, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
        ui_obj_["lbl_file"] = lbl_file;

        lv_obj_t *sep = lv_obj_create(content);
        lv_obj_set_size(sep, 300, 1);
        lv_obj_set_pos(sep, 10, 38);
        lv_obj_set_style_radius(sep, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(sep, lv_color_hex(0x555555), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(sep, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(sep, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(sep, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *lbl_list_title = lv_label_create(content);
        lv_label_set_text(lbl_list_title, "Recordings:");
        lv_obj_set_pos(lbl_list_title, 10, 42);
        lv_obj_set_style_text_color(lbl_list_title, lv_color_hex(0xE6EDF3), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl_list_title, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t *list_cont = lv_obj_create(content);
        lv_obj_set_size(list_cont, 300, 70);
        lv_obj_set_pos(list_cont, 10, 56);
        lv_obj_set_style_radius(list_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(list_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(list_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(list_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(list_cont, LV_OBJ_FLAG_SCROLLABLE);
        ui_obj_["list_cont"] = list_cont;

        rebuild_recordings({}, 0);
    }
};

class UIRecPage : public app_base
{
public:
    UIRecPage() : app_base(), view_(ui_APP_Container)
    {
        set_page_title("REC");
        refresh_all();
        event_handler_init();
    }

    ~UIRecPage()
    {
        model_.cancel_active();
        stop_elapsed_timer();
        stop_blink_timer();
    }

private:
    RecModel  model_;
    RecView   view_;
    int       elapsed_sec_ = 0;
    lv_timer_t *elapsed_timer_ = nullptr;
    lv_timer_t *blink_timer_ = nullptr;
    bool      blink_visible_ = true;

    void refresh_all()
    {
        view_.update_status(model_.state());
        view_.update_file(model_.current_file());
        view_.update_elapsed(elapsed_sec_);
        view_.rebuild_recordings(model_.recordings(), model_.selected_idx());
    }

    void start_activity_timers(bool blinking)
    {
        elapsed_sec_ = 0;
        view_.update_elapsed(elapsed_sec_);
        if (!elapsed_timer_)
            elapsed_timer_ = lv_timer_create(elapsed_timer_cb, 1000, this);
        else
            lv_timer_reset(elapsed_timer_);

        if (blinking)
            start_blink_timer();
        else
            stop_blink_timer();
    }

    void stop_activity_timers()
    {
        stop_elapsed_timer();
        stop_blink_timer();
    }

    static void elapsed_timer_cb(lv_timer_t *t)
    {
        UIRecPage *self = static_cast<UIRecPage *>(lv_timer_get_user_data(t));
        if (self) self->on_elapsed_tick();
    }

    void on_elapsed_tick()
    {
        if (!model_.is_active()) return;
        elapsed_sec_++;
        view_.update_elapsed(elapsed_sec_);
    }

    void stop_elapsed_timer()
    {
        if (elapsed_timer_)
        {
            lv_timer_delete(elapsed_timer_);
            elapsed_timer_ = nullptr;
        }
    }

    static void blink_timer_cb(lv_timer_t *t)
    {
        UIRecPage *self = static_cast<UIRecPage *>(lv_timer_get_user_data(t));
        if (self) self->on_blink_tick();
    }

    void on_blink_tick()
    {
        blink_visible_ = !blink_visible_;
        view_.set_recording_dot_visible(blink_visible_);
    }

    void start_blink_timer()
    {
        blink_visible_ = true;
        view_.set_recording_dot_visible(true);
        if (!blink_timer_)
            blink_timer_ = lv_timer_create(blink_timer_cb, 500, this);
        else
            lv_timer_reset(blink_timer_);
    }

    void stop_blink_timer()
    {
        if (blink_timer_)
        {
            lv_timer_delete(blink_timer_);
            blink_timer_ = nullptr;
        }
        blink_visible_ = true;
    }

    void event_handler_init()
    {
        lv_obj_add_event_cb(ui_root, UIRecPage::static_lvgl_handler, LV_EVENT_ALL, this);
    }

    static void static_lvgl_handler(lv_event_t *e)
    {
        UIRecPage *self = static_cast<UIRecPage *>(lv_event_get_user_data(e));
        if (self) self->event_handler(e);
    }

    void event_handler(lv_event_t *e)
    {
        if (IS_KEY_RELEASED(e))
        {
            uint32_t key = LV_EVENT_KEYBOARD_GET_KEY(e);
            handle_key(key);
        }
    }

    void handle_key(uint32_t key)
    {
        switch (key)
        {
        case KEY_R:
            if (model_.start_recording())
            {
                start_activity_timers(true);
                refresh_all();
            }
            break;
        case KEY_S:
            if (model_.stop_action())
            {
                stop_activity_timers();
                refresh_all();
            }
            break;
        case KEY_P:
            if (model_.play_selected())
            {
                start_activity_timers(false);
                refresh_all();
            }
            break;
        case KEY_D:
            if (model_.delete_selected())
                refresh_all();
            break;
        case KEY_UP:
            if (model_.move_selection(-1))
                view_.rebuild_recordings(model_.recordings(), model_.selected_idx());
            break;
        case KEY_DOWN:
            if (model_.move_selection(1))
                view_.rebuild_recordings(model_.recordings(), model_.selected_idx());
            break;
        case KEY_ESC:
            if (model_.cancel_active())
            {
                stop_activity_timers();
                refresh_all();
            }
            if (go_back_home) go_back_home();
            break;
        default:
            break;
        }
    }
};
