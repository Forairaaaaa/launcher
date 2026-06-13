#include "view_models/files_view_model.hpp"
#include <spdlog/spdlog.h>

namespace recorder {

FilesViewModel::FilesViewModel(RecorderRouter& router, RecordingFilesModel& filesModel, PlaybackModel& playbackModel)
    : ViewModel(router), _files_model(filesModel), _playback_model(playbackModel)
{
}

void FilesViewModel::onEnter()
{
    const bool preserve_selection  = _preserve_selection_next_enter;
    _preserve_selection_next_enter = false;
    spdlog::info("FilesViewModel enter, preserveSelection={}", preserve_selection);
    _files_model.refresh(preserve_selection);
}

void FilesViewModel::onKey(uint32_t key)
{
    if (_pending_delete_recording.get().active) {
        switch (key) {
            case '\x1b':
                cancelDeleteRecording();
                break;
            case '\r':
            case '\n':
                confirmDeleteRecording();
                break;
            default:
                break;
        }
        return;
    }

    switch (key) {
        case '4':
            _router.back();
            break;
        case '5':
            _files_model.selectPrevious();
            break;
        case '6':
            _files_model.selectNext();
            break;
        case '7': {
            const RecordingFile* selected = _files_model.selectedFile();
            if (selected) {
                _playback_model.load(*selected);
                _preserve_selection_next_enter = true;
                _router.push(PageId::Playback);
            }
            break;
        }
        case '8':
            requestDeleteSelected();
            break;
        default:
            break;
    }
}

void FilesViewModel::requestDeleteSelected()
{
    const RecordingFile* selected = _files_model.selectedFile();
    if (!selected) {
        return;
    }

    spdlog::info("FilesViewModel: request delete {}", selected->path);
    _delete_recording_close_action = DeleteRecordingCloseAction::None;
    _pending_delete_recording.set(PendingDeleteRecordingFile{true, *selected});
}

void FilesViewModel::cancelDeleteRecording()
{
    spdlog::info("FilesViewModel: cancel delete");
    _delete_recording_close_action = DeleteRecordingCloseAction::Cancel;
    _pending_delete_recording.set(PendingDeleteRecordingFile{});
}

void FilesViewModel::confirmDeleteRecording()
{
    spdlog::info("FilesViewModel: confirm delete");
    _delete_recording_close_action = DeleteRecordingCloseAction::Confirm;
    _pending_delete_recording.set(PendingDeleteRecordingFile{});
    _files_model.deleteSelected();
}

}  // namespace recorder
