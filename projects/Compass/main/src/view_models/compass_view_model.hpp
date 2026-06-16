#pragma once

#include "models/compass_model.hpp"
#include "view_models/view_model.hpp"

namespace compass {

class CompassViewModel : public ViewModel {
public:
    CompassViewModel(CompassRouter& router, CompassModel& model);

    PageId pageId() const override
    {
        return PageId::Compass;
    }

    void onEnter() override;
    void onKey(uint32_t key) override;
    void tick(uint32_t nowMs) override;

    smooth_ui_toolkit::SingleObservable<CompassReading>& reading()
    {
        return _model.reading();
    }

private:
    CompassModel& _model;
};

}  // namespace compass
