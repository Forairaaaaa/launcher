#include "view_models/files_view_model.hpp"
#include <spdlog/spdlog.h>

namespace recorder {

FilesViewModel::FilesViewModel(RecorderRouter& router, RecordingFilesModel& filesModel, PlaybackModel& playbackModel)
    : ViewModel(router), _files_model(filesModel), _playback_model(playbackModel)
{
}

void FilesViewModel::onEnter()
{
    spdlog::info("FilesViewModel enter");
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
