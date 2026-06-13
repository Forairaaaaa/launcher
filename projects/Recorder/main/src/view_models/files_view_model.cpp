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
            _files_model.deleteSelected();
            break;
        default:
            break;
    }
}

}  // namespace recorder
