#include "models/calibration_model.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <unordered_map>

namespace compass {

namespace {

constexpr uint32_t kAutoFinishMs             = 20000;
constexpr uint32_t kMinimumCalibrationSamples = 20;
constexpr float kMinimumAxisSpan             = 0.01f;
constexpr float kCoverageTarget              = 80.0f;
constexpr const char* kCalibrationPathEnv    = "COMPASS_CALIBRATION_PATH";
constexpr const char* kConfigDirEnv          = "COMPASS_CONFIG_DIR";
constexpr const char* kCalibrationFileName   = "calibration.conf";

float axisValue(const Axis3& value, size_t index)
{
    switch (index) {
        case 0:
            return value.x;
        case 1:
            return value.y;
        default:
            return value.z;
    }
}

void setAxisValue(Axis3& value, size_t index, float axis_value)
{
    switch (index) {
        case 0:
            value.x = axis_value;
            break;
        case 1:
            value.y = axis_value;
            break;
        default:
            value.z = axis_value;
            break;
    }
}

std::string trim(std::string value)
{
    auto is_space = [](unsigned char ch) { return std::isspace(ch) != 0; };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(),
                                            [&](char ch) { return !is_space(static_cast<unsigned char>(ch)); }));
    value.erase(
        std::find_if(value.rbegin(), value.rend(), [&](char ch) { return !is_space(static_cast<unsigned char>(ch)); })
            .base(),
        value.end());
    return value;
}

bool envValue(const char* key, std::filesystem::path& path)
{
    const char* value = std::getenv(key);
    if (!value || value[0] == '\0') {
        return false;
    }

    path = value;
    return true;
}

float configFloat(const std::unordered_map<std::string, std::string>& values, const char* key, float fallback)
{
    const auto it = values.find(key);
    if (it == values.end()) {
        return fallback;
    }

    try {
        return std::stof(it->second);
    } catch (...) {
        return fallback;
    }
}

}  // namespace

Axis3 applyMagCalibration(const Axis3& mag, const CompassCalibration& calibration)
{
    if (!calibration.valid) {
        return mag;
    }

    return Axis3{
        (mag.x - calibration.mag_offset.x) * calibration.mag_scale.x,
        (mag.y - calibration.mag_offset.y) * calibration.mag_scale.y,
        (mag.z - calibration.mag_offset.z) * calibration.mag_scale.z,
    };
}

CalibrationModel::CalibrationModel()
{
    if (load(_calibration)) {
        _status.set("Calibration loaded");
    }
}

void CalibrationModel::start()
{
    resetCapture();
    _status.set("Move in a figure eight");
    _progress.set(0.0f);
    _state.set(CalibrationState::Running);
}

bool CalibrationModel::finish()
{
    if (_state.get() != CalibrationState::Running) {
        return false;
    }

    CompassCalibration calibration;
    if (!buildCalibration(calibration)) {
        _status.set("Need more motion");
        return false;
    }

    if (!save(calibration)) {
        _status.set("Failed to save calibration");
        return false;
    }

    _calibration = calibration;
    _progress.set(1.0f);
    _status.set("Saved");
    _state.set(CalibrationState::Done);
    spdlog::info("CalibrationModel: calibration saved to {}", configPath().string());
    return true;
}

void CalibrationModel::stop()
{
    _state.set(CalibrationState::Idle);
}

void CalibrationModel::tick(uint32_t nowMs)
{
    if (_state.get() != CalibrationState::Running) {
        return;
    }

    if (_start_ms == 0) {
        _start_ms = nowMs;
        return;
    }

    if (nowMs - _start_ms >= kAutoFinishMs) {
        finish();
    }
}

void CalibrationModel::updateSample(const CompassSample& sample)
{
    if (_state.get() != CalibrationState::Running || !sample.available) {
        return;
    }

    captureMag(sample.rawMag);
    updateProgress();
}

std::filesystem::path CalibrationModel::configPath()
{
    std::filesystem::path path;
    if (envValue(kCalibrationPathEnv, path)) {
        return path;
    }

    if (envValue(kConfigDirEnv, path)) {
        return path / kCalibrationFileName;
    }

#if COMPASS_USE_MOCK_IMU
    return kCalibrationFileName;
#else
    return std::filesystem::path("/var/lib/Compass") / kCalibrationFileName;
#endif
}

bool CalibrationModel::load(CompassCalibration& calibration)
{
    return loadFrom(configPath(), calibration);
}

bool CalibrationModel::loadFrom(const std::filesystem::path& path, CompassCalibration& calibration)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    std::unordered_map<std::string, std::string> values;
    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }

        const auto separator = line.find('=');
        if (separator == std::string::npos) {
            continue;
        }

        values[trim(line.substr(0, separator))] = trim(line.substr(separator + 1));
    }

    CompassCalibration next;
    next.valid        = true;
    next.mag_offset.x = configFloat(values, "mag_offset_x", 0.0f);
    next.mag_offset.y = configFloat(values, "mag_offset_y", 0.0f);
    next.mag_offset.z = configFloat(values, "mag_offset_z", 0.0f);
    next.mag_scale.x  = configFloat(values, "mag_scale_x", 1.0f);
    next.mag_scale.y  = configFloat(values, "mag_scale_y", 1.0f);
    next.mag_scale.z  = configFloat(values, "mag_scale_z", 1.0f);

    calibration = next;
    return true;
}

bool CalibrationModel::save(const CompassCalibration& calibration)
{
    return saveTo(configPath(), calibration);
}

bool CalibrationModel::saveTo(const std::filesystem::path& path, const CompassCalibration& calibration)
{
    std::error_code error;
    const auto parent = path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, error);
        if (error) {
            spdlog::warn("CalibrationModel: failed to create config dir {}: {}", parent.string(), error.message());
            return false;
        }
    }

    const auto tmp_path = path.string() + ".tmp";
    {
        std::ofstream file(tmp_path, std::ios::trunc);
        if (!file.is_open()) {
            return false;
        }

        file << "# M5CardputerZero Compass calibration\n";
        file << "version=1\n";
        file << "mag_offset_x=" << calibration.mag_offset.x << "\n";
        file << "mag_offset_y=" << calibration.mag_offset.y << "\n";
        file << "mag_offset_z=" << calibration.mag_offset.z << "\n";
        file << "mag_scale_x=" << calibration.mag_scale.x << "\n";
        file << "mag_scale_y=" << calibration.mag_scale.y << "\n";
        file << "mag_scale_z=" << calibration.mag_scale.z << "\n";
    }

    std::filesystem::rename(tmp_path, path, error);
    if (error) {
        spdlog::warn("CalibrationModel: failed to save config {}: {}", path.string(), error.message());
        std::filesystem::remove(tmp_path, error);
        return false;
    }

    return true;
}

void CalibrationModel::resetCapture()
{
    _sample_count = 0;
    _start_ms     = 0;
    _has_sample   = false;
    _mag_min      = Axis3{};
    _mag_max      = Axis3{};
}

void CalibrationModel::captureMag(const Axis3& mag)
{
    if (!_has_sample) {
        _mag_min    = mag;
        _mag_max    = mag;
        _has_sample = true;
    } else {
        _mag_min.x = std::min(_mag_min.x, mag.x);
        _mag_min.y = std::min(_mag_min.y, mag.y);
        _mag_min.z = std::min(_mag_min.z, mag.z);
        _mag_max.x = std::max(_mag_max.x, mag.x);
        _mag_max.y = std::max(_mag_max.y, mag.y);
        _mag_max.z = std::max(_mag_max.z, mag.z);
    }

    ++_sample_count;
}

bool CalibrationModel::buildCalibration(CompassCalibration& calibration) const
{
    if (!_has_sample || _sample_count < kMinimumCalibrationSamples) {
        return false;
    }

    Axis3 radius;
    for (size_t i = 0; i < 3; ++i) {
        const float min_value = axisValue(_mag_min, i);
        const float max_value = axisValue(_mag_max, i);
        const float span      = max_value - min_value;
        if (span < kMinimumAxisSpan) {
            return false;
        }

        setAxisValue(calibration.mag_offset, i, (max_value + min_value) * 0.5f);
        setAxisValue(radius, i, span * 0.5f);
    }

    const float average_radius = (radius.x + radius.y + radius.z) / 3.0f;
    calibration.mag_scale.x    = average_radius / radius.x;
    calibration.mag_scale.y    = average_radius / radius.y;
    calibration.mag_scale.z    = average_radius / radius.z;
    calibration.valid          = true;
    return true;
}

void CalibrationModel::updateProgress()
{
    if (!_has_sample) {
        _progress.set(0.0f);
        return;
    }

    const float span_x          = _mag_max.x - _mag_min.x;
    const float span_y          = _mag_max.y - _mag_min.y;
    const float span_z          = _mag_max.z - _mag_min.z;
    const float coverage        = std::min({span_x, span_y, span_z}) / kCoverageTarget;
    const float sample_progress = static_cast<float>(_sample_count) / static_cast<float>(kMinimumCalibrationSamples);
    _progress.set(std::clamp(std::min(coverage, sample_progress), 0.0f, 1.0f));
}

}  // namespace compass
