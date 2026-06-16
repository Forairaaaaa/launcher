#pragma once

#include <tools/observable/single_observable.hpp>
#include <cstdint>
#include <memory>
#include <string>

namespace compass {

struct Axis3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

enum class CompassDataSource {
    Mock,
    Iio,
};

struct CompassSample {
    CompassDataSource source = CompassDataSource::Mock;
    bool available           = false;
    std::string status;
    float headingDeg = 0.0f;
    float pitchDeg   = 0.0f;
    float rollDeg    = 0.0f;
    float bubbleX    = 0.0f;
    float bubbleY    = 0.0f;
    Axis3 accel;
    Axis3 gyro;
    Axis3 mag;
};

class CompassModel {
public:
    CompassModel();
    ~CompassModel();

    CompassModel(const CompassModel&)            = delete;
    CompassModel& operator=(const CompassModel&) = delete;

    smooth_ui_toolkit::SingleObservable<CompassSample>& sample()
    {
        return _sample;
    }

    void tick(uint32_t nowMs);

private:
    struct Impl;

    smooth_ui_toolkit::SingleObservable<CompassSample> _sample{CompassSample{}};
    std::unique_ptr<Impl> _impl;
};

}  // namespace compass
