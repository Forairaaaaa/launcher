#include "view_models/playback_view_model.hpp"
#include <spdlog/spdlog.h>

namespace recorder {

PlaybackViewModel::PlaybackViewModel(RecorderRouter& router, PlaybackModel& model) : ViewModel(router), _model(model) {}

void PlaybackViewModel::onEnter()
{
    spdlog::info("PlaybackViewModel enter");
}

void PlaybackViewModel::onKey(uint32_t key)
{
    switch (key) {
    case '4':
        _router.back();
        break;
    case '5':
        _model.seek(-10.0f);
        break;
    case '6':
        _model.togglePlayPause();
        break;
    case '7':
        _model.seek(10.0f);
        break;
    case '8':
        _model.toggleSpeed();
        break;
    default:
        break;
    }
}

void PlaybackViewModel::tick(uint32_t nowMs)
{
    _model.tick(nowMs);
}

} // namespace recorder
