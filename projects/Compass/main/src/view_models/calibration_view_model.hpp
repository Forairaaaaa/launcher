#pragma once

#include "models/calibration_model.hpp"
#include "view_models/view_model.hpp"

namespace compass {

class CalibrationViewModel : public ViewModel {
public:
    CalibrationViewModel(CompassRouter& router, CalibrationModel& model);

    PageId pageId() const override
    {
        return PageId::Calibration;
    }

    void onEnter() override;
    void onExit() override;
    void onKey(uint32_t key) override;
    void tick(uint32_t nowMs) override;

    smooth_ui_toolkit::SingleObservable<CalibrationState>& state()
    {
        return _model.state();
    }

private:
    CalibrationModel& _model;
};

}  // namespace compass
