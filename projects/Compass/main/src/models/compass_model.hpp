#pragma once

#include <tools/observable/single_observable.hpp>
#include <cstdint>

namespace compass {

struct CompassReading {
    float headingDeg = 0.0f;
};

class CompassModel {
public:
    smooth_ui_toolkit::SingleObservable<CompassReading>& reading()
    {
        return _reading;
    }

    void tick(uint32_t nowMs);

private:
    smooth_ui_toolkit::SingleObservable<CompassReading> _reading{CompassReading{}};
};

}  // namespace compass
