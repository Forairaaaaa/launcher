#pragma once

#include "models/playback_model.hpp"
#include "view_models/view_model.hpp"

namespace recorder {

class PlaybackViewModel : public ViewModel {
public:
    PlaybackViewModel(RecorderRouter& router, PlaybackModel& model);

    PageId pageId() const override
    {
        return PageId::Playback;
    }

    void onEnter() override;
    void onExit() override;
    void onKey(uint32_t key) override;
    void tick(uint32_t nowMs) override;

    smooth_ui_toolkit::Observable<PlaybackState>& state()
    {
        return _model.state();
    }

    smooth_ui_toolkit::Observable<RecordingFile>& file()
    {
        return _model.file();
    }

    smooth_ui_toolkit::Observable<float>& progressSec()
    {
        return _model.progressSec();
    }

    smooth_ui_toolkit::Observable<float>& speed()
    {
        return _model.speed();
    }

private:
    PlaybackModel& _model;
};

}  // namespace recorder
