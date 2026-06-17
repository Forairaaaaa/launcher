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
    _magic_count = 0;
}

void CompassViewModel::onKey(uint32_t key)
{
    switch (key) {
        case ' ':
            if (canGenerateMagic()) {
                ++_magic_count;
                if (_magic_count >= 3) {
                    _magic_count = 0;
                    generateMagic();
                }
            } else {
                _magic_count = 0;
            }
            break;
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

bool CompassViewModel::canGenerateMagic() const
{
    return !_info_expanded.get();
}

void CompassViewModel::generateMagic()
{
    _magic.set(_magic.get() + 1);
}

}  // namespace compass
