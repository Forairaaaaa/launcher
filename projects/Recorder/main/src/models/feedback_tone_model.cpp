#include "models/feedback_tone_model.hpp"
#include <miniaudio.h>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <atomic>
#include <cmath>

namespace recorder {

namespace {

constexpr ma_uint32 kToneChannels       = 2;
constexpr ma_uint32 kToneSampleRate     = 48000;
constexpr ma_format kToneFormat         = ma_format_f32;
constexpr float kToneFrequency          = 1760.0f;
constexpr float kToneGain               = 0.3f;
constexpr float kToneDurationSec        = 0.045f;
constexpr float kToneAttackSec          = 0.004f;
constexpr float kToneReleaseSec         = 0.018f;
constexpr ma_uint32 kToneDurationFrames = static_cast<ma_uint32>(kToneDurationSec * kToneSampleRate);
constexpr ma_uint32 kToneAttackFrames   = static_cast<ma_uint32>(kToneAttackSec * kToneSampleRate);
constexpr ma_uint32 kToneReleaseFrames  = static_cast<ma_uint32>(kToneReleaseSec * kToneSampleRate);
constexpr float kTwoPi                  = 6.28318530718f;

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

struct FeedbackToneModel::Impl {
    ma_context context{};
    ma_device device{};

    bool context_inited = false;
    bool device_inited  = false;
    bool disabled       = false;

    std::atomic<ma_uint32> cursor_frames{kToneDurationFrames};
    float phase = 0.0f;

    ~Impl()
    {
        cleanup();
    }

    void playHint()
    {
        if (disabled || !ensureDevice()) {
            return;
        }

        cursor_frames.store(0, std::memory_order_release);
    }

private:
    bool ensureDevice()
    {
        if (device_inited) {
            return true;
        }

        if (!initContext()) {
            return false;
        }

        ma_device_config device_config  = ma_device_config_init(ma_device_type_playback);
        device_config.playback.format   = kToneFormat;
        device_config.playback.channels = kToneChannels;
        device_config.sampleRate        = kToneSampleRate;
        device_config.dataCallback      = dataCallback;
        device_config.pUserData         = this;

        ma_result result = ma_device_init(&context, &device_config, &device);
        if (result != MA_SUCCESS) {
            spdlog::warn("FeedbackToneModel: ma_device_init failed, result={} {}", static_cast<int>(result),
                         maResultName(result));
            disabled = true;
            return false;
        }

        result = ma_device_start(&device);
        if (result != MA_SUCCESS) {
            spdlog::warn("FeedbackToneModel: ma_device_start failed, result={} {}", static_cast<int>(result),
                         maResultName(result));
            ma_device_uninit(&device);
            disabled = true;
            return false;
        }

        device_inited = true;
        spdlog::info("FeedbackToneModel: tone device initialized");
        return true;
    }

    bool initContext()
    {
        if (context_inited) {
            return true;
        }

#if RECORDER_USE_PULSEAUDIO
        ma_backend backends[] = {ma_backend_pulseaudio};
        ma_result result      = ma_context_init(backends, 1, nullptr, &context);
        if (result != MA_SUCCESS) {
            spdlog::warn("FeedbackToneModel: ma_context_init PulseAudio failed, result={} {}", static_cast<int>(result),
                         maResultName(result));
            disabled = true;
            return false;
        }
#else
        ma_result result = ma_context_init(nullptr, 0, nullptr, &context);
        if (result != MA_SUCCESS) {
            spdlog::warn("FeedbackToneModel: ma_context_init default backend failed, result={} {}",
                         static_cast<int>(result), maResultName(result));
            disabled = true;
            return false;
        }
#endif

        context_inited = true;
        return true;
    }

    void cleanup()
    {
        if (device_inited) {
            ma_device_uninit(&device);
            device_inited = false;
        }
        if (context_inited) {
            ma_context_uninit(&context);
            context_inited = false;
        }
    }

    float nextSample()
    {
        const ma_uint32 cursor = cursor_frames.load(std::memory_order_acquire);
        if (cursor >= kToneDurationFrames) {
            return 0.0f;
        }

        float envelope = 1.0f;
        if (cursor < kToneAttackFrames) {
            envelope = static_cast<float>(cursor) / static_cast<float>(std::max<ma_uint32>(kToneAttackFrames, 1));
        } else if (cursor + kToneReleaseFrames >= kToneDurationFrames) {
            const ma_uint32 release_cursor = kToneDurationFrames - cursor;
            envelope =
                static_cast<float>(release_cursor) / static_cast<float>(std::max<ma_uint32>(kToneReleaseFrames, 1));
        }

        const float sample = std::sin(phase) * kToneGain * std::clamp(envelope, 0.0f, 1.0f);
        phase += kTwoPi * kToneFrequency / static_cast<float>(kToneSampleRate);
        if (phase >= kTwoPi) {
            phase -= kTwoPi;
        }

        cursor_frames.store(cursor + 1, std::memory_order_release);
        return sample;
    }

    static void dataCallback(ma_device* device, void* output, const void* input, ma_uint32 frame_count)
    {
        (void)input;

        auto* self = static_cast<Impl*>(device->pUserData);
        auto* out  = static_cast<float*>(output);
        if (!self || !out) {
            return;
        }

        for (ma_uint32 frame = 0; frame < frame_count; ++frame) {
            const float sample = self->nextSample();
            for (ma_uint32 channel = 0; channel < kToneChannels; ++channel) {
                *out++ = sample;
            }
        }
    }
};

FeedbackToneModel::FeedbackToneModel() : _impl(std::make_unique<Impl>())
{
}

FeedbackToneModel::~FeedbackToneModel() = default;

void FeedbackToneModel::playHint()
{
    _impl->playHint();
}

}  // namespace recorder
