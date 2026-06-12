#pragma once

#include "core/recorder_types.hpp"
#include <tools/observable/observable.hpp>

namespace recorder {

class RecordingModel {
public:
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

    void start();
    void stop();
    void pause();
    void resume();
    void tick(uint32_t nowMs);

private:
    smooth_ui_toolkit::Observable<RecordingState> _state {RecordingState::Idle};
    smooth_ui_toolkit::Observable<float> _mic_amp {0.0f};
    smooth_ui_toolkit::Observable<AudioFrame> _frame {AudioFrame {}};
};

} // namespace recorder
