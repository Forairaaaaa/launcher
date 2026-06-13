#pragma once

#include "models/playback_model.hpp"
#include "models/recording_files_model.hpp"
#include "view_models/view_model.hpp"
#include <tools/observable/single_observable.hpp>

namespace recorder {

enum class DeleteRecordingCloseAction {
    None,
    Confirm,
    Cancel,
};

struct PendingDeleteRecordingFile {
    bool active = false;
    RecordingFile file;
};

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

    smooth_ui_toolkit::SingleObservable<PendingDeleteRecordingFile>& pendingDeleteRecording()
    {
        return _pending_delete_recording;
    }

    DeleteRecordingCloseAction deleteRecordingCloseAction() const
    {
        return _delete_recording_close_action;
    }

    void requestDeleteSelected();
    void cancelDeleteRecording();
    void confirmDeleteRecording();

private:
    RecordingFilesModel& _files_model;
    PlaybackModel& _playback_model;
    smooth_ui_toolkit::SingleObservable<PendingDeleteRecordingFile> _pending_delete_recording{
        PendingDeleteRecordingFile{},
    };
    DeleteRecordingCloseAction _delete_recording_close_action = DeleteRecordingCloseAction::None;
    bool _preserve_selection_next_enter                       = false;
};

}  // namespace recorder
