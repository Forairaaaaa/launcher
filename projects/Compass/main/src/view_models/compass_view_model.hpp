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

    smooth_ui_toolkit::SingleObservable<CompassSample>& sample()
    {
        return _model.sample();
    }

    smooth_ui_toolkit::SingleObservable<bool>& infoExpanded()
    {
        return _info_expanded;
    }

    smooth_ui_toolkit::SingleObservable<uint32_t>& magic()
    {
        return _magic;
    }

private:
    CompassModel& _model;
    smooth_ui_toolkit::SingleObservable<bool> _info_expanded{false};
    smooth_ui_toolkit::SingleObservable<uint32_t> _magic{0};
    uint32_t _magic_count = 0;

    bool canGenerateMagic() const;
    void generateMagic();
};

}  // namespace compass
