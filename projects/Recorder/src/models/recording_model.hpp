#pragma once

#include "core/recorder_types.hpp"
#include <tools/observable/single_observable.hpp>
#include <memory>
#include <string>

namespace recorder {

class RecordingModel {
public:
    RecordingModel();
    explicit RecordingModel(std::string recordings_dir);
    ~RecordingModel();

    smooth_ui_toolkit::SingleObservable<RecordingState>& state()
    {
        return _state;
    }

    smooth_ui_toolkit::SingleObservable<AudioFrame>& frame()
    {
        return _frame;
    }

    smooth_ui_toolkit::SingleObservable<uint32_t>& elapsedSec()
    {
        return _elapsed_sec;
    }

    smooth_ui_toolkit::SingleObservable<PendingRecordingFile>& pendingRecording()
    {
        return _pending_recording;
    }

    void start();
    void stop();
    void pause();
    void resume();
    bool confirmPendingRecording(const std::string& name);
    bool discardPendingRecording();
    void tick(uint32_t nowMs);

private:
    struct Impl;

    smooth_ui_toolkit::SingleObservable<RecordingState> _state{RecordingState::Idle};
    smooth_ui_toolkit::SingleObservable<AudioFrame> _frame{AudioFrame{}};
    smooth_ui_toolkit::SingleObservable<uint32_t> _elapsed_sec{0};
    smooth_ui_toolkit::SingleObservable<PendingRecordingFile> _pending_recording{PendingRecordingFile{}};
    std::unique_ptr<Impl> _impl;
};

}  // namespace recorder
