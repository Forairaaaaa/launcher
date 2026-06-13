#pragma once

#include "models/playback_model.hpp"
#include "models/recording_files_model.hpp"
#include "view_models/view_model.hpp"

namespace recorder {

class FilesViewModel : public ViewModel {
public:
    FilesViewModel(RecorderRouter& router, RecordingFilesModel& filesModel, PlaybackModel& playbackModel);

    PageId pageId() const override
    {
        return PageId::Files;
    }

    void onEnter() override;
    void onKey(uint32_t key) override;

    smooth_ui_toolkit::Observable<std::vector<RecordingFile>>& files()
    {
        return _files_model.files();
    }

    smooth_ui_toolkit::Observable<int>& selectedIndex()
    {
        return _files_model.selectedIndex();
    }

private:
    RecordingFilesModel& _files_model;
    PlaybackModel& _playback_model;
    bool _preserve_selection_next_enter = false;
};

}  // namespace recorder
