#include "models/recording_model.hpp"
#include <cmath>

namespace recorder {

void RecordingModel::start()
{
    _state.set(RecordingState::Recording);
}

void RecordingModel::stop()
{
    _state.set(RecordingState::Idle);
    _mic_amp.set(0.0f);
}

void RecordingModel::pause()
{
    if (_state.get() == RecordingState::Recording) {
        _state.set(RecordingState::Paused);
    }
}

void RecordingModel::resume()
{
    if (_state.get() == RecordingState::Paused) {
        _state.set(RecordingState::Recording);
    }
}

void RecordingModel::tick(uint32_t nowMs)
{
    if (_state.get() != RecordingState::Recording) {
        return;
    }

    AudioFrame next;
    next.samples.resize(24);
    for (size_t i = 0; i < next.samples.size(); ++i) {
        float phase = static_cast<float>((nowMs / 24 + i * 17) % 360) * 0.017453292f;
        next.samples[i] = (std::sin(phase) + 1.0f) * 0.5f;
        next.amp += next.samples[i];
    }
    next.amp /= next.samples.empty() ? 1.0f : static_cast<float>(next.samples.size());

    _frame.set(std::move(next));
    _mic_amp.set(_frame.get().amp);
}

} // namespace recorder
