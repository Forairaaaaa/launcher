#include "view_models/compass_view_model.hpp"
#include <spdlog/spdlog.h>

namespace compass {

CompassViewModel::CompassViewModel(CompassRouter& router, CompassModel& model) : ViewModel(router), _model(model)
{
}

void CompassViewModel::onEnter()
{
    spdlog::info("CompassViewModel enter");
}

void CompassViewModel::onKey(uint32_t key)
{
    switch (key) {
        case '8':
        case '\r':
        case '\n':
            _router.push(PageId::Calibration);
            break;
        default:
            break;
    }
}

void CompassViewModel::tick(uint32_t nowMs)
{
    _model.tick(nowMs);
}

}  // namespace compass
