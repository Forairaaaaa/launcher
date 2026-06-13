#pragma once

#include "models/recording_model.hpp"
#include "view_models/view_model.hpp"

namespace recorder {

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

private:
    RecordingModel& _model;
};

}  // namespace recorder
