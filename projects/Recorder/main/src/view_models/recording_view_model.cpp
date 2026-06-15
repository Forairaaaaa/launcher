#include "view_models/recording_view_model.hpp"
#include <spdlog/spdlog.h>
#include <utility>

namespace recorder {

namespace {

RecordingWaveformType nextWaveformType(RecordingWaveformType type)
{
    switch (type) {
        case RecordingWaveformType::Basic:
            return RecordingWaveformType::Spectrum;
        case RecordingWaveformType::Spectrum:
            return RecordingWaveformType::Line;
        case RecordingWaveformType::Line:
            return RecordingWaveformType::Prism;
        case RecordingWaveformType::Prism:
            return RecordingWaveformType::Basic;
    }
    return RecordingWaveformType::Basic;
}

const char* waveformTypeName(RecordingWaveformType type)
{
    switch (type) {
        case RecordingWaveformType::Basic:
            return "Basic";
        case RecordingWaveformType::Line:
            return "Line";
        case RecordingWaveformType::Prism:
            return "Prism";
        case RecordingWaveformType::Spectrum:
            return "Spectrum";
    }
    return "Unknown";
}

}  // namespace

RecordingViewModel::RecordingViewModel(RecorderRouter& router, RecordingModel& model) : ViewModel(router), _model(model)
{
}

void RecordingViewModel::onEnter()
{
    spdlog::info("RecordingViewModel enter");
    _magic_count = 0;
}

void RecordingViewModel::onKey(uint32_t key)
{
    if (_model.pendingRecording().get().active) {
        switch (key) {
            case '\x1b':
                discardPendingRecording();
                break;
            case '\r':
            case '\n':
                confirmPendingRecording();
                break;
            default:
                break;
        }
        return;
    }

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
        case '4': {
            RecordingWaveformType next_type = nextWaveformType(_waveform_type.get());
            spdlog::info("RecordingViewModel: switch waveform type={}", waveformTypeName(next_type));
            _waveform_type.set(next_type);
            break;
        }
        case '5':
            if (_model.state().get() == RecordingState::Recording) {
                _model.pause();
            } else if (_model.state().get() == RecordingState::Paused) {
                _model.resume();
            }
            break;
        case '6':
            if (_model.state().get() == RecordingState::Idle) {
                _model.start();
            } else {
                _model.stop();
                syncPendingRecordingName();
            }
            break;
        case '8':
            _router.push(PageId::Files);
            break;
        default:
            break;
    }
}

void RecordingViewModel::tick(uint32_t nowMs)
{
    _model.tick(nowMs);
    syncPendingRecordingName();
}

void RecordingViewModel::setPendingRecordingName(std::string name)
{
    _pending_recording_name.set(std::move(name));
}

bool RecordingViewModel::confirmPendingRecording()
{
    _pending_recording_close_action = PendingRecordingCloseAction::Confirm;
    const bool ok                   = _model.confirmPendingRecording(_pending_recording_name.get());
    if (ok) {
        syncPendingRecordingName();
    }
    return ok;
}

bool RecordingViewModel::discardPendingRecording()
{
    _pending_recording_close_action = PendingRecordingCloseAction::Discard;
    const bool ok                   = _model.discardPendingRecording();
    if (ok) {
        syncPendingRecordingName();
    }
    return ok;
}

void RecordingViewModel::syncPendingRecordingName()
{
    const auto& pending = _model.pendingRecording().get();
    if (pending.active == _pending_recording_active) {
        return;
    }

    _pending_recording_active = pending.active;
    if (pending.active) {
        _pending_recording_close_action = PendingRecordingCloseAction::None;
    }
    _pending_recording_name.set(pending.active ? pending.name : "");
}

bool RecordingViewModel::canGenerateMagic() const
{
    const auto state = _model.state().get();
    const auto type  = _waveform_type.get();
    return state == RecordingState::Idle &&
           (type == RecordingWaveformType::Basic || type == RecordingWaveformType::Line);
}

void RecordingViewModel::generateMagic()
{
    _magic.set(_magic.get() + 1);
}

}  // namespace recorder
