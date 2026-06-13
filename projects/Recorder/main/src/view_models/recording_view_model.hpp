#pragma once

#include "models/recording_model.hpp"
#include "view_models/view_model.hpp"
#include <string>

namespace recorder {

enum class RecordingWaveformType {
    Basic,
    Line,
    Prism,
    Spectrum,
};

enum class PendingRecordingCloseAction {
    None,
    Confirm,
    Discard,
};

class RecordingViewModel : public ViewModel {
public:
    RecordingViewModel(RecorderRouter& router, RecordingModel& model);

    PageId pageId() const override
    {
        return PageId::Recording;
    }

    void onEnter() override;
    void onKey(uint32_t key) override;
    void tick(uint32_t nowMs) override;

    smooth_ui_toolkit::Observable<RecordingState>& state()
    {
        return _model.state();
    }

    smooth_ui_toolkit::Observable<float>& micAmp()
    {
        return _model.micAmp();
    }

    smooth_ui_toolkit::Observable<AudioFrame>& frame()
    {
        return _model.frame();
    }

    smooth_ui_toolkit::Observable<uint32_t>& elapsedSec()
    {
        return _model.elapsedSec();
    }

    smooth_ui_toolkit::Observable<PendingRecordingFile>& pendingRecording()
    {
        return _model.pendingRecording();
    }

    smooth_ui_toolkit::Observable<std::string>& pendingRecordingName()
    {
        return _pending_recording_name;
    }

    smooth_ui_toolkit::Observable<RecordingWaveformType>& waveformType()
    {
        return _waveform_type;
    }

    smooth_ui_toolkit::Observable<PendingRecordingCloseAction>& pendingRecordingCloseAction()
    {
        return _pending_recording_close_action;
    }

    void setPendingRecordingName(std::string name);
    bool confirmPendingRecording();
    bool discardPendingRecording();

private:
    RecordingModel& _model;
    smooth_ui_toolkit::Observable<RecordingWaveformType> _waveform_type{RecordingWaveformType::Basic};
    smooth_ui_toolkit::Observable<std::string> _pending_recording_name{""};
    smooth_ui_toolkit::Observable<PendingRecordingCloseAction> _pending_recording_close_action{
        PendingRecordingCloseAction::None};
    bool _pending_recording_active = false;

    void syncPendingRecordingName();
};

}  // namespace recorder
