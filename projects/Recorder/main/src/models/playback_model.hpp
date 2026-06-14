#pragma once

#include "core/recorder_types.hpp"
#include <tools/observable/single_observable.hpp>
#include <memory>

namespace recorder {

class PlaybackModel {
public:
    PlaybackModel();
    ~PlaybackModel();

    smooth_ui_toolkit::SingleObservable<PlaybackState>& state()
    {
        return _state;
    }

    smooth_ui_toolkit::SingleObservable<RecordingFile>& file()
    {
        return _file;
    }

    smooth_ui_toolkit::SingleObservable<float>& progressSec()
    {
        return _progress_sec;
    }

    smooth_ui_toolkit::SingleObservable<float>& speed()
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

    smooth_ui_toolkit::SingleObservable<PlaybackState> _state{PlaybackState::Stopped};
    smooth_ui_toolkit::SingleObservable<RecordingFile> _file{RecordingFile{}};
    smooth_ui_toolkit::SingleObservable<float> _progress_sec{0.0f};
    smooth_ui_toolkit::SingleObservable<float> _speed{1.0f};
    std::unique_ptr<Impl> _impl;
};

}  // namespace recorder
