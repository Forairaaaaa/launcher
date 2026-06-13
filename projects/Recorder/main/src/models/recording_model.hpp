#pragma once

#include "core/recorder_types.hpp"
#include <tools/observable/observable.hpp>
#include <memory>

namespace recorder {

class RecordingModel {
public:
    RecordingModel();
    ~RecordingModel();

    smooth_ui_toolkit::Observable<RecordingState>& state()
    {
        return _state;
    }

    smooth_ui_toolkit::Observable<float>& micAmp()
    {
        return _mic_amp;
    }

    smooth_ui_toolkit::Observable<AudioFrame>& frame()
    {
        return _frame;
    }

    smooth_ui_toolkit::Observable<uint32_t>& elapsedSec()
    {
        return _elapsed_sec;
    }

    smooth_ui_toolkit::Observable<PendingRecordingFile>& pendingRecording()
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

    smooth_ui_toolkit::Observable<RecordingState> _state{RecordingState::Idle};
    smooth_ui_toolkit::Observable<float> _mic_amp{0.0f};
    smooth_ui_toolkit::Observable<AudioFrame> _frame{AudioFrame{}};
    smooth_ui_toolkit::Observable<uint32_t> _elapsed_sec{0};
    smooth_ui_toolkit::Observable<PendingRecordingFile> _pending_recording{PendingRecordingFile{}};
    std::unique_ptr<Impl> _impl;
};

}  // namespace recorder
