#pragma once

#include "core/recorder_types.hpp"
#include <tools/observable/observable.hpp>
#include <memory>

namespace recorder {

class PlaybackModel {
public:
    PlaybackModel();
    ~PlaybackModel();

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
    void pause();
    void seek(float offsetSec);
    void toggleSpeed();
    void tick(uint32_t nowMs);

private:
    struct Impl;

    smooth_ui_toolkit::Observable<PlaybackState> _state{PlaybackState::Stopped};
    smooth_ui_toolkit::Observable<RecordingFile> _file{RecordingFile{}};
    smooth_ui_toolkit::Observable<float> _progress_sec{0.0f};
    smooth_ui_toolkit::Observable<float> _speed{1.0f};
    std::unique_ptr<Impl> _impl;
};

}  // namespace recorder
