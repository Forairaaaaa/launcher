#pragma once

#include "models/feedback_tone_model.hpp"
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
    RecordingViewModel(RecorderRouter& router, RecordingModel& model, FeedbackToneModel& feedback_tone);

    PageId pageId() const override
    {
        return PageId::Recording;
    }

    void onEnter() override;
    void onKey(uint32_t key) override;
    void tick(uint32_t nowMs) override;

    smooth_ui_toolkit::SingleObservable<RecordingState>& state()
    {
        return _model.state();
    }

    smooth_ui_toolkit::SingleObservable<AudioFrame>& frame()
    {
        return _model.frame();
    }

    smooth_ui_toolkit::SingleObservable<uint32_t>& elapsedSec()
    {
        return _model.elapsedSec();
    }

    smooth_ui_toolkit::SingleObservable<PendingRecordingFile>& pendingRecording()
    {
        return _model.pendingRecording();
    }

    smooth_ui_toolkit::SingleObservable<std::string>& pendingRecordingName()
    {
        return _pending_recording_name;
    }

    smooth_ui_toolkit::SingleObservable<RecordingWaveformType>& waveformType()
    {
        return _waveform_type;
    }

    smooth_ui_toolkit::SingleObservable<uint32_t>& magic()
    {
        return _magic;
    }

    PendingRecordingCloseAction pendingRecordingCloseAction() const
    {
        return _pending_recording_close_action;
    }

    void setPendingRecordingName(std::string name);
    bool confirmPendingRecording();
    bool discardPendingRecording();

private:
    RecordingModel& _model;
    FeedbackToneModel& _feedback_tone;
    smooth_ui_toolkit::SingleObservable<RecordingWaveformType> _waveform_type{RecordingWaveformType::Basic};
    smooth_ui_toolkit::SingleObservable<std::string> _pending_recording_name{""};
    smooth_ui_toolkit::SingleObservable<uint32_t> _magic{0};
    PendingRecordingCloseAction _pending_recording_close_action = PendingRecordingCloseAction::None;
    bool _pending_recording_active                              = false;
    uint32_t _magic_count                                       = 0;

    void syncPendingRecordingName();
    bool canGenerateMagic() const;
    void generateMagic();
};

}  // namespace recorder
