#pragma once

#include "core/recorder_types.hpp"
#include <tools/observable/observable.hpp>

namespace recorder {

class PlaybackModel {
public:
    smooth_ui_toolkit::Observable<PlaybackState>& state()
    {
        return _state;
    }

    smooth_ui_toolkit::Observable<RecordingFile>& file()
    {
        return _file;
    }

    smooth_ui_toolkit::Observable<float>& progressSec()
    {
        return _progress_sec;
    }

    smooth_ui_toolkit::Observable<float>& speed()
    {
        return _speed;
    }

    void load(const RecordingFile& file);
    void togglePlayPause();
    void seek(float offsetSec);
    void toggleSpeed();
    void tick(uint32_t nowMs);

private:
    smooth_ui_toolkit::Observable<PlaybackState> _state {PlaybackState::Stopped};
    smooth_ui_toolkit::Observable<RecordingFile> _file {RecordingFile {}};
    smooth_ui_toolkit::Observable<float> _progress_sec {0.0f};
    smooth_ui_toolkit::Observable<float> _speed {1.0f};
    uint32_t _last_tick_ms = 0;
};

} // namespace recorder
