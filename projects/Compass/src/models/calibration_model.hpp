#pragma once

#include "models/compass_model.hpp"
#include <tools/observable/single_observable.hpp>
#include <cstdint>
#include <filesystem>
#include <string>

namespace compass {

enum class CalibrationState {
    Idle,
    Running,
    Done,
};

struct CompassCalibration {
    bool valid = false;
    Axis3 mag_offset;
    Axis3 mag_scale{1.0f, 1.0f, 1.0f};
};

Axis3 applyMagCalibration(const Axis3& mag, const CompassCalibration& calibration);

class CalibrationModel {
public:
    CalibrationModel();

    smooth_ui_toolkit::SingleObservable<CalibrationState>& state()
    {
        return _state;
    }

    smooth_ui_toolkit::SingleObservable<std::string>& status()
    {
        return _status;
    }

    smooth_ui_toolkit::SingleObservable<float>& progress()
    {
        return _progress;
    }

    const CompassCalibration& calibration() const
    {
        return _calibration;
    }

    uint32_t sampleCount() const
    {
        return _sample_count;
    }

    void start();
    bool finish();
    void stop();
    void tick(uint32_t nowMs);
    void updateSample(const CompassSample& sample);

    static std::filesystem::path configPath();
    static bool load(CompassCalibration& calibration);
    static bool loadFrom(const std::filesystem::path& path, CompassCalibration& calibration);
    static bool save(const CompassCalibration& calibration);
    static bool saveTo(const std::filesystem::path& path, const CompassCalibration& calibration);

private:
    smooth_ui_toolkit::SingleObservable<CalibrationState> _state{CalibrationState::Idle};
    smooth_ui_toolkit::SingleObservable<std::string> _status{"Ready"};
    smooth_ui_toolkit::SingleObservable<float> _progress{0.0f};
    CompassCalibration _calibration;
    Axis3 _mag_min;
    Axis3 _mag_max;
    uint32_t _sample_count = 0;
    uint32_t _start_ms     = 0;
    bool _has_sample       = false;

    void resetCapture();
    void captureMag(const Axis3& mag);
    bool buildCalibration(CompassCalibration& calibration) const;
    void updateProgress();
};

}  // namespace compass
