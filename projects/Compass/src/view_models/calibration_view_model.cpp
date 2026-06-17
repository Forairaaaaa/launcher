#include "view_models/calibration_view_model.hpp"
#include <spdlog/spdlog.h>

namespace compass {

CalibrationViewModel::CalibrationViewModel(CompassRouter& router, CalibrationModel& model, CompassModel& compass_model)
    : ViewModel(router), _model(model), _compass_model(compass_model)
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
            if (_model.state().get() == CalibrationState::Running) {
                if (_model.finish()) {
                    _compass_model.reloadCalibration();
                }
            } else {
                _model.start();
            }
            break;
        default:
            break;
    }
}

void CalibrationViewModel::tick(uint32_t nowMs)
{
    _compass_model.tick(nowMs);
    _model.updateSample(_compass_model.sample().get());
    const auto previous_state = _model.state().get();
    _model.tick(nowMs);
    if (previous_state == CalibrationState::Running && _model.state().get() == CalibrationState::Done) {
        _compass_model.reloadCalibration();
    }
}

}  // namespace compass
