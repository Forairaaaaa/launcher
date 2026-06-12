#include "view_models/recording_view_model.hpp"
#include <spdlog/spdlog.h>

namespace recorder {

RecordingViewModel::RecordingViewModel(RecorderRouter& router, RecordingModel& model) : ViewModel(router), _model(model)
{
}

void RecordingViewModel::onEnter()
{
    spdlog::info("RecordingViewModel enter");
}

void RecordingViewModel::onKey(uint32_t key)
{
    switch (key) {
        case '5':
            if (_model.state().get() == RecordingState::Recording) {
                _model.pause();
            } else if (_model.state().get() == RecordingState::Paused) {
                _model.resume();
            }
            break;
        case '6':
            if (_model.state().get() == RecordingState::Idle) {
                _model.start();
            } else {
                _model.stop();
            }
            break;
        case '8':
            _router.push(PageId::Files);
            break;
        default:
            break;
    }
}

void RecordingViewModel::tick(uint32_t nowMs)
{
    _model.tick(nowMs);
}

}  // namespace recorder
