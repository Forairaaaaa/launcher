#include "view_models/compass_view_model.hpp"
#include <spdlog/spdlog.h>

namespace compass {

CompassViewModel::CompassViewModel(CompassRouter& router, CompassModel& model) : ViewModel(router), _model(model)
{
}

void CompassViewModel::onEnter()
{
    spdlog::info("CompassViewModel enter");
    _info_expanded.set(false);
}

void CompassViewModel::onKey(uint32_t key)
{
    switch (key) {
        case '7':
            if (_info_expanded.get()) {
                _router.push(PageId::Calibration);
            }
            break;
        case '8':
            _info_expanded.set(!_info_expanded.get());
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
