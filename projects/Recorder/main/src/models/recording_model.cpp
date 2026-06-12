#include "models/recording_model.hpp"

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <cerrno>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>
#include <spdlog/spdlog.h>
#include <sys/stat.h>
#include <unistd.h>

namespace recorder {

namespace {

constexpr ma_uint32 kCaptureChannels   = 1;
constexpr ma_uint32 kCaptureSampleRate = 48000;
constexpr ma_format kCaptureFormat     = ma_format_f32;
constexpr size_t kPreviewSampleCount   = 24;

std::string makeRecordingPath()
{
    char cwd[512] = {};
    if (getcwd(cwd, sizeof(cwd)) == nullptr) {
        spdlog::warn("RecordingModel: getcwd failed, fallback to current directory");
        std::snprintf(cwd, sizeof(cwd), ".");
    }

    std::string dir = std::string(cwd) + "/recordings";
    if (mkdir(dir.c_str(), 0755) != 0 && errno != EEXIST) {
        spdlog::warn("RecordingModel: failed to create {}, errno={}", dir, errno);
        dir = cwd;
    }

    auto now             = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &now_time);
#else
    localtime_r(&now_time, &tm);
#endif

    std::ostringstream name;
    name << dir << "/rec_" << std::put_time(&tm, "%Y%m%d_%H%M%S") << ".wav";
    return name.str();
}

const char* maResultName(ma_result result)
{
    switch (result) {
        case MA_SUCCESS:
            return "MA_SUCCESS";
        case MA_NO_BACKEND:
            return "MA_NO_BACKEND";
        case MA_NO_DEVICE:
            return "MA_NO_DEVICE";
        case MA_DEVICE_NOT_INITIALIZED:
            return "MA_DEVICE_NOT_INITIALIZED";
        case MA_FAILED_TO_INIT_BACKEND:
            return "MA_FAILED_TO_INIT_BACKEND";
        case MA_FAILED_TO_OPEN_BACKEND_DEVICE:
            return "MA_FAILED_TO_OPEN_BACKEND_DEVICE";
        default:
            return "MA_ERROR";
    }
}

}  // namespace

struct RecordingModel::Impl {
    ma_context context{};
    ma_device device{};
    ma_encoder encoder{};
    ma_device_id capture_device_id{};

    bool context_inited        = false;
    bool device_inited         = false;
    bool encoder_inited        = false;
    bool has_capture_device_id = false;

    std::string current_path;
    std::mutex frame_mutex;
    AudioFrame latest_frame;
    bool has_new_frame = false;
    std::atomic<uint64_t> captured_frames{0};

    ~Impl()
    {
        cleanup();
    }

    bool start()
    {
        cleanup();

        current_path = makeRecordingPath();
        captured_frames.store(0);
        {
            std::lock_guard<std::mutex> lock(frame_mutex);
            latest_frame = AudioFrame{};
            latest_frame.samples.assign(kPreviewSampleCount, 0.0f);
            has_new_frame = true;
        }

        if (!initContext()) {
            cleanup();
            return false;
        }

        logCaptureDevices();
        selectCaptureDevice();

        ma_encoder_config encoder_config =
            ma_encoder_config_init(ma_encoding_format_wav, kCaptureFormat, kCaptureChannels, kCaptureSampleRate);
        ma_result result = ma_encoder_init_file(current_path.c_str(), &encoder_config, &encoder);
        if (result != MA_SUCCESS) {
            spdlog::error("RecordingModel: ma_encoder_init_file failed, result={} {}, path={}",
                          static_cast<int>(result), maResultName(result), current_path);
            cleanup();
            return false;
        }
        encoder_inited = true;

        ma_device_config device_config  = ma_device_config_init(ma_device_type_capture);
        device_config.capture.format    = kCaptureFormat;
        device_config.capture.channels  = kCaptureChannels;
        device_config.capture.pDeviceID = has_capture_device_id ? &capture_device_id : nullptr;
        device_config.sampleRate        = kCaptureSampleRate;
        device_config.dataCallback      = dataCallback;
        device_config.pUserData         = this;

        result = ma_device_init(&context, &device_config, &device);
        if (result != MA_SUCCESS) {
            spdlog::error("RecordingModel: ma_device_init capture failed, result={} {}", static_cast<int>(result),
                          maResultName(result));
            cleanup();
            return false;
        }
        device_inited = true;

        result = ma_device_start(&device);
        if (result != MA_SUCCESS) {
            spdlog::error("RecordingModel: ma_device_start capture failed, result={} {}", static_cast<int>(result),
                          maResultName(result));
            cleanup();
            return false;
        }

        spdlog::info("RecordingModel: started recording path={}, channels={}, sampleRate={}", current_path,
                     kCaptureChannels, kCaptureSampleRate);
        return true;
    }

    bool pause()
    {
        if (!device_inited) {
            return false;
        }

        ma_result result = ma_device_stop(&device);
        if (result != MA_SUCCESS) {
            spdlog::error("RecordingModel: ma_device_stop for pause failed, result={} {}", static_cast<int>(result),
                          maResultName(result));
            return false;
        }

        spdlog::info("RecordingModel: paused recording path={}, capturedFrames={}", current_path,
                     captured_frames.load());
        return true;
    }

    bool resume()
    {
        if (!device_inited) {
            return false;
        }

        ma_result result = ma_device_start(&device);
        if (result != MA_SUCCESS) {
            spdlog::error("RecordingModel: ma_device_start for resume failed, result={} {}", static_cast<int>(result),
                          maResultName(result));
            return false;
        }

        spdlog::info("RecordingModel: resumed recording path={}", current_path);
        return true;
    }

    void stop()
    {
        uint64_t frames  = captured_frames.load();
        std::string path = current_path;

        cleanup();

        float duration_sec = kCaptureSampleRate == 0 ? 0.0f : static_cast<float>(frames) / kCaptureSampleRate;
        if (!path.empty()) {
            spdlog::info("RecordingModel: stopped recording path={}, frames={}, duration={:.2f}s", path, frames,
                         duration_sec);
        }
    }

    bool consumeFrame(AudioFrame& out)
    {
        std::lock_guard<std::mutex> lock(frame_mutex);
        if (!has_new_frame) {
            return false;
        }

        out           = latest_frame;
        has_new_frame = false;
        return true;
    }

    void cleanup()
    {
        if (device_inited) {
            ma_device_uninit(&device);
            device_inited = false;
        }

        if (encoder_inited) {
            ma_encoder_uninit(&encoder);
            encoder_inited = false;
        }

        if (context_inited) {
            ma_context_uninit(&context);
            context_inited = false;
        }

        has_capture_device_id = false;
        current_path.clear();
    }

    bool initContext()
    {
#if RECORDER_USE_PULSEAUDIO
        ma_backend backends[] = {ma_backend_pulseaudio};
        ma_result result      = ma_context_init(backends, 1, nullptr, &context);
        if (result != MA_SUCCESS) {
            spdlog::error("RecordingModel: ma_context_init PulseAudio failed, result={} {}", static_cast<int>(result),
                          maResultName(result));
            return false;
        }
        spdlog::info("RecordingModel: miniaudio context initialized with PulseAudio backend");
#else
        ma_result result = ma_context_init(nullptr, 0, nullptr, &context);
        if (result != MA_SUCCESS) {
            spdlog::error("RecordingModel: ma_context_init default backend failed, result={} {}",
                          static_cast<int>(result), maResultName(result));
            return false;
        }
        spdlog::info("RecordingModel: miniaudio context initialized with default backend");
#endif
        context_inited = true;
        return true;
    }

    void logCaptureDevices()
    {
        ma_device_info* playback_infos = nullptr;
        ma_uint32 playback_count       = 0;
        ma_device_info* capture_infos  = nullptr;
        ma_uint32 capture_count        = 0;

        ma_result result =
            ma_context_get_devices(&context, &playback_infos, &playback_count, &capture_infos, &capture_count);
        if (result != MA_SUCCESS) {
            spdlog::warn("RecordingModel: ma_context_get_devices failed, result={} {}", static_cast<int>(result),
                         maResultName(result));
            return;
        }

        spdlog::info("RecordingModel: capture devices count={}", capture_count);
        for (ma_uint32 i = 0; i < capture_count; ++i) {
            spdlog::info("RecordingModel: capture[{}] {}{}", i, capture_infos[i].name,
                         capture_infos[i].isDefault ? " [default]" : "");
        }
    }

    void selectCaptureDevice()
    {
        ma_device_info* playback_infos = nullptr;
        ma_uint32 playback_count       = 0;
        ma_device_info* capture_infos  = nullptr;
        ma_uint32 capture_count        = 0;

        ma_result result =
            ma_context_get_devices(&context, &playback_infos, &playback_count, &capture_infos, &capture_count);
        if (result != MA_SUCCESS) {
            return;
        }

        for (ma_uint32 i = 0; i < capture_count; ++i) {
            std::string name = capture_infos[i].name;
            if (name.find("ES8388") != std::string::npos || name.find("ES8389") != std::string::npos) {
                capture_device_id     = capture_infos[i].id;
                has_capture_device_id = true;
                spdlog::info("RecordingModel: selected ES8388/ES8389 capture device: {}", name);
                return;
            }
        }

        spdlog::info("RecordingModel: using default capture device");
    }

    static void dataCallback(ma_device* device, void* output, const void* input, ma_uint32 frame_count)
    {
        (void)output;

        auto* self = static_cast<Impl*>(device->pUserData);
        if (!self || !input || frame_count == 0) {
            return;
        }

        const auto* samples      = static_cast<const float*>(input);
        ma_uint64 frames_written = 0;
        ma_encoder_write_pcm_frames(&self->encoder, input, frame_count, &frames_written);
        self->captured_frames.fetch_add(frames_written);

        AudioFrame frame;
        frame.samples.assign(kPreviewSampleCount, 0.0f);

        float sum_abs = 0.0f;
        size_t step   = std::max<size_t>(1, frame_count / kPreviewSampleCount);
        for (ma_uint32 i = 0; i < frame_count; ++i) {
            sum_abs += std::abs(samples[i]);
        }

        for (size_t i = 0; i < frame.samples.size(); ++i) {
            size_t sample_index = std::min<size_t>(i * step, frame_count - 1);
            frame.samples[i]    = std::clamp((samples[sample_index] + 1.0f) * 0.5f, 0.0f, 1.0f);
        }

        frame.amp = std::clamp(sum_abs / static_cast<float>(frame_count), 0.0f, 1.0f);

        {
            std::lock_guard<std::mutex> lock(self->frame_mutex);
            self->latest_frame  = std::move(frame);
            self->has_new_frame = true;
        }
    }
};

RecordingModel::RecordingModel() : _impl(std::make_unique<Impl>())
{
}

RecordingModel::~RecordingModel()
{
    stop();
}

void RecordingModel::start()
{
    if (_state.get() == RecordingState::Recording) {
        spdlog::info("RecordingModel: start ignored, already recording");
        return;
    }

    if (_state.get() == RecordingState::Paused) {
        stop();
    }

    spdlog::info("RecordingModel: start requested");
    if (_impl->start()) {
        _state.set(RecordingState::Recording);
    } else {
        _state.set(RecordingState::Idle);
        _mic_amp.set(0.0f);
        _frame.set(AudioFrame{});
    }
}

void RecordingModel::stop()
{
    if (!_impl) {
        return;
    }

    if (_state.get() == RecordingState::Idle) {
        return;
    }

    spdlog::info("RecordingModel: stop requested");
    _impl->stop();
    _state.set(RecordingState::Idle);
    _mic_amp.set(0.0f);
    _frame.set(AudioFrame{});
}

void RecordingModel::pause()
{
    if (_state.get() == RecordingState::Recording) {
        spdlog::info("RecordingModel: pause requested");
        if (_impl->pause()) {
            _state.set(RecordingState::Paused);
        }
    }
}

void RecordingModel::resume()
{
    if (_state.get() == RecordingState::Paused) {
        spdlog::info("RecordingModel: resume requested");
        if (_impl->resume()) {
            _state.set(RecordingState::Recording);
        }
    }
}

void RecordingModel::tick(uint32_t nowMs)
{
    (void)nowMs;

    if (_state.get() != RecordingState::Recording) {
        return;
    }

    AudioFrame next;
    if (!_impl->consumeFrame(next)) {
        return;
    }
    _frame.set(next);
    _mic_amp.set(_frame.get().amp);
}

}  // namespace recorder
