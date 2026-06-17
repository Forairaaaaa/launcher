#include "models/compass_model.hpp"
#include "models/calibration_model.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <utility>

namespace compass {

namespace {

constexpr uint32_t kSampleIntervalMs = 33;
constexpr float kPi                  = 3.14159265359f;
constexpr float kRadToDeg            = 180.0f / kPi;
constexpr float kDegToRad            = kPi / 180.0f;
constexpr float kBubbleTiltRangeDeg  = 18.0f;

constexpr const char* kBmi270I2cSysfsRoot     = "/sys/bus/i2c/devices";
constexpr const char* kBmi270IioSysfsRoot     = "/sys/bus/iio/devices";
constexpr const char* kBmi270Compatible       = "bosch,bmi270";
constexpr const char* kBmi270DeviceName       = "bmi270";
constexpr const char* kBmi270I2cAddressSuffix = "-0068";
constexpr const char* kBmi270IioDevicePath    = "/sys/bus/iio/devices/iio:device0";
constexpr const char* kBmm150IioDevicePath    = "/sys/bus/iio/devices/iio:device2";
constexpr const char* kBmm150Compatible       = "bosch,bmm150";
constexpr const char* kBmm150DeviceName       = "bmm150";
constexpr const char* kIioAccelXRaw           = "in_accel_x_raw";
constexpr const char* kIioAccelYRaw           = "in_accel_y_raw";
constexpr const char* kIioAccelZRaw           = "in_accel_z_raw";
constexpr const char* kIioAccelScale          = "in_accel_scale";
constexpr const char* kIioGyroXRaw            = "in_anglvel_x_raw";
constexpr const char* kIioGyroYRaw            = "in_anglvel_y_raw";
constexpr const char* kIioGyroZRaw            = "in_anglvel_z_raw";
constexpr const char* kIioGyroScale           = "in_anglvel_scale";
constexpr const char* kIioMagnXRaw            = "in_magn_x_raw";
constexpr const char* kIioMagnYRaw            = "in_magn_y_raw";
constexpr const char* kIioMagnZRaw            = "in_magn_z_raw";
constexpr const char* kIioMagnScale           = "in_magn_scale";

struct ImuDevice {
    std::string i2c_path;
    std::string iio_path;
    std::string mag_iio_path;
    std::string display_name;
    std::string mag_display_name;
};

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

std::string lowerCopy(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

bool readTextFile(const std::filesystem::path& path, std::string& value)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    value = trim(buffer.str());
    return true;
}

bool readDoubleFile(const std::filesystem::path& path, double& value)
{
    std::string text;
    if (!readTextFile(path, text)) {
        return false;
    }

    try {
        value = std::stod(text);
    } catch (...) {
        return false;
    }
    return true;
}

bool containsBmi270(const std::string& text)
{
    const auto lower = lowerCopy(text);
    return lower.find(kBmi270DeviceName) != std::string::npos || lower.find(kBmi270Compatible) != std::string::npos;
}

bool containsBmm150(const std::string& text)
{
    const auto lower = lowerCopy(text);
    return lower.find(kBmm150DeviceName) != std::string::npos || lower.find(kBmm150Compatible) != std::string::npos;
}

bool isI2cBmi270Node(const std::filesystem::path& path)
{
    const auto filename     = path.filename().string();
    const size_t suffix_len = std::char_traits<char>::length(kBmi270I2cAddressSuffix);
    if (filename.size() >= suffix_len &&
        filename.compare(filename.size() - suffix_len, suffix_len, kBmi270I2cAddressSuffix) == 0) {
        return true;
    }

    std::string text;
    if (readTextFile(path / "name", text) && containsBmi270(text)) {
        return true;
    }
    if (readTextFile(path / "modalias", text) && containsBmi270(text)) {
        return true;
    }
    if (readTextFile(path / "of_node" / "compatible", text) && containsBmi270(text)) {
        return true;
    }
    return false;
}

bool findI2cNode(std::string& i2c_path)
{
    const std::filesystem::path root(kBmi270I2cSysfsRoot);
    if (!std::filesystem::exists(root)) {
        return false;
    }

    for (const auto& entry : std::filesystem::directory_iterator(root)) {
        if (!entry.is_directory() && !entry.is_symlink()) {
            continue;
        }
        if (isI2cBmi270Node(entry.path())) {
            i2c_path = entry.path().string();
            return true;
        }
    }
    return false;
}

bool isIioBmi270Node(const std::filesystem::path& path)
{
    std::string name;
    if (readTextFile(path / "name", name) && containsBmi270(name)) {
        return true;
    }

    return std::filesystem::exists(path / kIioAccelXRaw) && std::filesystem::exists(path / kIioGyroXRaw);
}

bool isIioBmm150Node(const std::filesystem::path& path)
{
    std::string name;
    if (readTextFile(path / "name", name) && containsBmm150(name)) {
        return true;
    }

    return std::filesystem::exists(path / kIioMagnXRaw) && std::filesystem::exists(path / kIioMagnYRaw) &&
           std::filesystem::exists(path / kIioMagnZRaw);
}

bool readIioDisplayName(const std::filesystem::path& path, const char* fallback, std::string& display_name)
{
    if (!readTextFile(path / "name", display_name) || display_name.empty()) {
        display_name = fallback && fallback[0] != '\0' ? fallback : path.filename().string();
    }
    return true;
}

bool findIioNode(const char* preferred_path, bool (*matches)(const std::filesystem::path&), const char* fallback_name,
                 std::string& iio_path, std::string& display_name)
{
    if (preferred_path && preferred_path[0] != '\0') {
        const std::filesystem::path preferred(preferred_path);
        if (std::filesystem::exists(preferred) && matches(preferred)) {
            iio_path = preferred.string();
            readIioDisplayName(preferred, fallback_name, display_name);
            return true;
        }
    }

    const std::filesystem::path root(kBmi270IioSysfsRoot);
    if (!std::filesystem::exists(root)) {
        return false;
    }

    for (const auto& entry : std::filesystem::directory_iterator(root)) {
        if (!entry.is_directory() && !entry.is_symlink()) {
            continue;
        }
        if (!matches(entry.path())) {
            continue;
        }

        iio_path = entry.path().string();
        readIioDisplayName(entry.path(), fallback_name, display_name);
        return true;
    }
    return false;
}

bool readScaledAxis(const std::filesystem::path& root, const char* raw_file, double scale, float& value)
{
    double raw = 0.0;
    if (!readDoubleFile(root / raw_file, raw)) {
        return false;
    }
    value = static_cast<float>(raw * scale);
    return true;
}

float normalizeDegrees(float deg)
{
    while (deg < 0.0f) {
        deg += 360.0f;
    }
    while (deg >= 360.0f) {
        deg -= 360.0f;
    }
    return deg;
}

float clampUnit(float value)
{
    return std::clamp(value, -1.0f, 1.0f);
}

void fillPose(CompassSample& sample)
{
    const float ax = sample.accel.x;
    const float ay = sample.accel.y;
    const float az = sample.accel.z;
    const float mx = sample.mag.x;
    const float my = sample.mag.y;
    const float mz = sample.mag.z;

    const float roll  = std::atan2(ay, az);
    const float pitch = std::atan2(-ax, std::sqrt(ay * ay + az * az));

    const float mx2 = mx * std::cos(pitch) + mz * std::sin(pitch);
    const float my2 =
        mx * std::sin(roll) * std::sin(pitch) + my * std::cos(roll) - mz * std::sin(roll) * std::cos(pitch);

    sample.headingDeg = normalizeDegrees(std::atan2(-my2, mx2) * kRadToDeg);
    sample.pitchDeg   = pitch * kRadToDeg;
    sample.rollDeg    = roll * kRadToDeg;
    sample.bubbleX    = clampUnit(sample.rollDeg / kBubbleTiltRangeDeg);
    sample.bubbleY    = clampUnit(sample.pitchDeg / kBubbleTiltRangeDeg);
}

class ImuBackend {
public:
    virtual ~ImuBackend()                                                        = default;
    virtual bool init(std::string& error)                                        = 0;
    virtual bool read(uint32_t nowMs, CompassSample& sample, std::string& error) = 0;
};

class MockImuBackend : public ImuBackend {
public:
    bool init(std::string& error) override
    {
        (void)error;
        return true;
    }

    bool read(uint32_t nowMs, CompassSample& sample, std::string& error) override
    {
        (void)error;

        const float t       = static_cast<float>(nowMs) / 1000.0f;
        const float heading = normalizeDegrees(t * 18.0f);
        const float pitch   = std::sin(t * 0.8f) * 9.0f;
        const float roll    = std::cos(t * 0.65f) * 12.0f;

        sample.source     = CompassDataSource::Mock;
        sample.available  = true;
        sample.status     = "Mock IMU";
        sample.headingDeg = heading;
        sample.pitchDeg   = pitch;
        sample.rollDeg    = roll;
        sample.bubbleX    = clampUnit(roll / kBubbleTiltRangeDeg);
        sample.bubbleY    = clampUnit(pitch / kBubbleTiltRangeDeg);

        const float pitch_rad = pitch * kDegToRad;
        const float roll_rad  = roll * kDegToRad;
        sample.accel.x        = -std::sin(pitch_rad) * 9.81f;
        sample.accel.y        = std::sin(roll_rad) * std::cos(pitch_rad) * 9.81f;
        sample.accel.z        = std::cos(roll_rad) * std::cos(pitch_rad) * 9.81f;
        sample.gyro.x         = std::cos(t * 0.8f) * 0.12f;
        sample.gyro.y         = std::sin(t * 0.65f) * 0.10f;
        sample.gyro.z         = 0.31f;
        sample.mag.x          = std::cos(heading * kDegToRad) * 42.0f;
        sample.mag.y          = -std::sin(heading * kDegToRad) * 42.0f;
        sample.mag.z          = 5.0f + std::sin(t * 0.42f) * 2.0f;
        sample.rawMag         = sample.mag;
        return true;
    }
};

#if COMPASS_USE_IIO_IMU
class IioImuBackend : public ImuBackend {
public:
    bool init(std::string& error) override
    {
        findI2cNode(_device.i2c_path);

        if (!findIioNode(kBmi270IioDevicePath, isIioBmi270Node, kBmi270DeviceName, _device.iio_path,
                         _device.display_name)) {
            error = "BMI270 IIO device not found";
            return false;
        }

        if (!findIioNode(kBmm150IioDevicePath, isIioBmm150Node, kBmm150DeviceName, _device.mag_iio_path,
                         _device.mag_display_name)) {
            error = "BMM150 IIO device not found";
            return false;
        }

        if (_device.display_name.empty()) {
            _device.display_name = kBmi270DeviceName;
        }
        if (_device.mag_display_name.empty()) {
            _device.mag_display_name = kBmm150DeviceName;
        }

        spdlog::info("CompassModel: IIO IMU initialized imu={} mag={} i2c={}", _device.iio_path, _device.mag_iio_path,
                     _device.i2c_path.empty() ? "(none)" : _device.i2c_path);
        return true;
    }

    bool read(uint32_t nowMs, CompassSample& sample, std::string& error) override
    {
        (void)nowMs;

        const std::filesystem::path imu_root(_device.iio_path);
        const std::filesystem::path mag_root(_device.mag_iio_path);
        double accel_scale = 0.0;
        double gyro_scale  = 0.0;
        double magn_scale  = 0.0;
        if (!readDoubleFile(imu_root / kIioAccelScale, accel_scale)) {
            error = "Failed to read BMI270 accel scale";
            return false;
        }
        if (!readDoubleFile(imu_root / kIioGyroScale, gyro_scale)) {
            error = "Failed to read BMI270 gyro scale";
            return false;
        }
        if (!readDoubleFile(mag_root / kIioMagnScale, magn_scale)) {
            error = "Failed to read BMM150 magnetometer scale";
            return false;
        }

        sample.source    = CompassDataSource::Iio;
        sample.available = true;
        sample.status    = _device.display_name + " + " + _device.mag_display_name;

        if (!readScaledAxis(imu_root, kIioAccelXRaw, accel_scale, sample.accel.x) ||
            !readScaledAxis(imu_root, kIioAccelYRaw, accel_scale, sample.accel.y) ||
            !readScaledAxis(imu_root, kIioAccelZRaw, accel_scale, sample.accel.z) ||
            !readScaledAxis(imu_root, kIioGyroXRaw, gyro_scale, sample.gyro.x) ||
            !readScaledAxis(imu_root, kIioGyroYRaw, gyro_scale, sample.gyro.y) ||
            !readScaledAxis(imu_root, kIioGyroZRaw, gyro_scale, sample.gyro.z) ||
            !readScaledAxis(mag_root, kIioMagnXRaw, magn_scale, sample.mag.x) ||
            !readScaledAxis(mag_root, kIioMagnYRaw, magn_scale, sample.mag.y) ||
            !readScaledAxis(mag_root, kIioMagnZRaw, magn_scale, sample.mag.z)) {
            error = "Failed to read BMI270/BMM150 nine-axis data";
            return false;
        }

        sample.rawMag = sample.mag;
        fillPose(sample);
        return true;
    }

private:
    ImuDevice _device;
};
#endif

std::unique_ptr<ImuBackend> makePrimaryBackend()
{
#if COMPASS_USE_MOCK_IMU
    return std::make_unique<MockImuBackend>();
#elif COMPASS_USE_IIO_IMU
    return std::make_unique<IioImuBackend>();
#else
    return std::make_unique<MockImuBackend>();
#endif
}

}  // namespace

struct CompassModel::Impl {
    std::unique_ptr<ImuBackend> backend = makePrimaryBackend();
    uint32_t last_sample_ms             = 0;
    bool using_mock                     = false;
    CompassCalibration calibration;

    Impl()
    {
#if COMPASS_USE_MOCK_IMU
        using_mock = true;
#endif
    }

    bool init()
    {
        loadCalibration();

        std::string error;
        if (backend && backend->init(error)) {
            spdlog::info("CompassModel: using {} backend", using_mock ? "mock" : "IIO");
            return true;
        }

        spdlog::warn("CompassModel: IMU init failed: {}; falling back to mock", error);
        useMock("Mock IMU");
        return backend->init(error);
    }

    void useMock(const std::string& reason)
    {
        using_mock = true;
        backend    = std::make_unique<MockImuBackend>();
        spdlog::info("CompassModel: fallback backend active: {}", reason);
    }

    bool loadCalibration()
    {
        CompassCalibration next;
        if (CalibrationModel::load(next)) {
            calibration = next;
            spdlog::info("CompassModel: calibration loaded from {}", CalibrationModel::configPath().string());
            return true;
        }

        calibration = CompassCalibration{};
        spdlog::info("CompassModel: no calibration file at {}", CalibrationModel::configPath().string());
        return false;
    }
};

CompassModel::CompassModel() : _impl(std::make_unique<Impl>())
{
    _impl->init();
}

CompassModel::~CompassModel() = default;

bool CompassModel::reloadCalibration()
{
    return _impl->loadCalibration();
}

void CompassModel::tick(uint32_t nowMs)
{
    if (_impl->last_sample_ms != 0 && nowMs - _impl->last_sample_ms < kSampleIntervalMs) {
        return;
    }
    _impl->last_sample_ms = nowMs;

    CompassSample next;
    std::string error;
    if (!_impl->backend->read(nowMs, next, error)) {
        spdlog::warn("CompassModel: IMU read failed: {}; falling back to mock", error);
        _impl->useMock(error);
        error.clear();
        if (!_impl->backend->init(error) || !_impl->backend->read(nowMs, next, error)) {
            next.source    = CompassDataSource::Mock;
            next.available = false;
            next.status    = error.empty() ? "No IMU data" : error;
        }
    }

    if (next.available && next.rawMag.x == 0.0f && next.rawMag.y == 0.0f && next.rawMag.z == 0.0f) {
        next.rawMag = next.mag;
    }

    if (next.available && next.source == CompassDataSource::Iio) {
        next.mag = applyMagCalibration(next.mag, _impl->calibration);
        fillPose(next);
    }

    _sample.set(std::move(next));
}

}  // namespace compass
