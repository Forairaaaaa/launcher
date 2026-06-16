#include "view_models/calibration_view_model.hpp"
#include <spdlog/spdlog.h>

namespace compass {

CalibrationViewModel::CalibrationViewModel(CompassRouter& router, CalibrationModel& model)
    : ViewModel(router), _model(model)
{
}

void CalibrationViewModel::onEnter()
{
    spdlog::info("CalibrationViewModel enter");
}

void CalibrationViewModel::onExit()
{
    _model.stop();
}

void CalibrationViewModel::onKey(uint32_t key)
{
    switch (key) {
        case '4':
        case '\x1b':
            _router.back();
            break;
        case '6':
        case '\r':
        case '\n':
            _model.start();
            break;
        default:
            break;
    }
}

void CalibrationViewModel::tick(uint32_t nowMs)
{
    _model.tick(nowMs);
}

}  // namespace compass
