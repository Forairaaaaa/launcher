#pragma once

#include <tools/observable/single_observable.hpp>
#include <cstdint>

namespace compass {

enum class CalibrationState {
    Idle,
    Running,
    Done,
};

class CalibrationModel {
public:
    smooth_ui_toolkit::SingleObservable<CalibrationState>& state()
    {
        return _state;
    }

    void start();
    void stop();
    void tick(uint32_t nowMs);

private:
    smooth_ui_toolkit::SingleObservable<CalibrationState> _state{CalibrationState::Idle};
};

}  // namespace compass
