#include "models/calibration_model.hpp"

namespace compass {

void CalibrationModel::start()
{
    _state.set(CalibrationState::Running);
}

void CalibrationModel::stop()
{
    _state.set(CalibrationState::Idle);
}

void CalibrationModel::tick(uint32_t nowMs)
{
    (void)nowMs;
}

}  // namespace compass
