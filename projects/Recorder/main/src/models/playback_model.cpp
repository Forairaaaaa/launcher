#include "models/playback_model.hpp"
#include <algorithm>

namespace recorder {

void PlaybackModel::load(const RecordingFile& file)
{
    _file.set(file);
    _progress_sec.set(0.0f);
    _state.set(PlaybackState::Paused);
    _last_tick_ms = 0;
}

void PlaybackModel::togglePlayPause()
{
    if (_state.get() == PlaybackState::Playing) {
        _state.set(PlaybackState::Paused);
    } else {
        _state.set(PlaybackState::Playing);
    }
}

void PlaybackModel::seek(float offsetSec)
{
    float duration = static_cast<float>(_file.get().durationSec);
    float next     = std::clamp(_progress_sec.get() + offsetSec, 0.0f, duration);
    _progress_sec.set(next);
}

void PlaybackModel::toggleSpeed()
{
    float current = _speed.get();
    if (current < 2.0f) {
        _speed.set(2.0f);
    } else if (current < 5.0f) {
        _speed.set(5.0f);
    } else {
        _speed.set(1.0f);
    }
}

void PlaybackModel::tick(uint32_t nowMs)
{
    if (_state.get() != PlaybackState::Playing) {
        _last_tick_ms = nowMs;
        return;
    }

    if (_last_tick_ms == 0) {
        _last_tick_ms = nowMs;
        return;
    }

    float dt      = static_cast<float>(nowMs - _last_tick_ms) / 1000.0f;
    _last_tick_ms = nowMs;
    seek(dt * _speed.get());

    if (_progress_sec.get() >= static_cast<float>(_file.get().durationSec)) {
        _state.set(PlaybackState::Stopped);
    }
}

}  // namespace recorder
