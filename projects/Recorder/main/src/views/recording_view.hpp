#pragma once

#include "view_models/recording_view_model.hpp"
#include "views/bottom_key_bar.hpp"
#include "views/view.hpp"
#include <lvgl/lvgl_cpp/obj.hpp>
#include <memory>
#include <string>

namespace recorder {

class RecordingWaveformViewBase;

class RecordingView : public View {
public:
    explicit RecordingView(RecordingViewModel& view_model);
    ~RecordingView() override;

    RecordingView(const RecordingView&)            = delete;
    RecordingView& operator=(const RecordingView&) = delete;

    void onEnter(lv_obj_t* parent) override;
    void onExit() override;
    void tick(uint32_t nowMs) override;

private:
    class DurationPanel;
    class FileConfirmDialog;
    class PausedLabel;

    RecordingViewModel& _view_model;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Container> _root;
    std::unique_ptr<RecordingWaveformViewBase> _waveform;
    std::unique_ptr<DurationPanel> _duration_panel;
    std::unique_ptr<FileConfirmDialog> _file_confirm_dialog;
    std::unique_ptr<PausedLabel> _paused_label;
    std::unique_ptr<BottomKeyBar> _key_bar;

    void createWaveform(RecordingWaveformType type);
    void renderState(RecordingState state);
    void renderElapsed(uint32_t elapsed_sec);
    void renderFrame(const AudioFrame& frame);
    void renderWaveformType(RecordingWaveformType type);
    void renderPendingRecording(const PendingRecordingFile& pending);
    void renderPendingRecordingName(const std::string& name);
    static void onStateChanged(void* context, const RecordingState& state);
    static void onElapsedChanged(void* context, const uint32_t& elapsed_sec);
    static void onFrameChanged(void* context, const AudioFrame& frame);
    static void onWaveformTypeChanged(void* context, const RecordingWaveformType& type);
    static void onPendingRecordingChanged(void* context, const PendingRecordingFile& pending);
    static void onPendingRecordingNameChanged(void* context, const std::string& name);
};

}  // namespace recorder
