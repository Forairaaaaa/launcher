#pragma once

#include "view_models/playback_view_model.hpp"
#include "views/bottom_key_bar.hpp"
#include "views/view.hpp"
#include <lvgl/lvgl_cpp/label.hpp>
#include <lvgl/lvgl_cpp/obj.hpp>
#include <memory>

namespace recorder {

class PlaybackView : public View {
public:
    explicit PlaybackView(PlaybackViewModel& view_model);
    ~PlaybackView() override;

    PlaybackView(const PlaybackView&)            = delete;
    PlaybackView& operator=(const PlaybackView&) = delete;

    void onEnter(lv_obj_t* parent) override;
    void onExit() override;
    void tick(uint32_t nowMs) override;

private:
    class WaveformView;
    class ProgressBar;

    PlaybackViewModel& _view_model;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Container> _root;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Label> _title_label;
    std::unique_ptr<WaveformView> _waveform;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Container> _playhead;
    std::unique_ptr<ProgressBar> _progress_bar;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Label> _elapsed_label;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Label> _remaining_label;
    std::unique_ptr<BottomKeyBar> _key_bar;

    void renderState(PlaybackState state);
    void renderFile(const RecordingFile& file);
    void renderProgress(float progressSec);
    void renderSpeed(float speed);
    void updateKeyBar();
    static void onStateChanged(void* context, const PlaybackState& state);
    static void onFileChanged(void* context, const RecordingFile& file);
    static void onProgressChanged(void* context, const float& progressSec);
    static void onSpeedChanged(void* context, const float& speed);
};

}  // namespace recorder
