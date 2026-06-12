#pragma once

#include "core/recorder_types.hpp"
#include <tools/observable/observable.hpp>

namespace recorder {

class RecordingFilesModel {
public:
    smooth_ui_toolkit::Observable<std::vector<RecordingFile>>& files()
    {
        return _files;
    }

    smooth_ui_toolkit::Observable<int>& selectedIndex()
    {
        return _selected_index;
    }

    const RecordingFile* selectedFile() const;
    void selectPrevious();
    void selectNext();
    void deleteSelected();

private:
    smooth_ui_toolkit::Observable<std::vector<RecordingFile>> _files {
        std::vector<RecordingFile> {
            {"rec_001.wav", 37},
            {"rec_002.wav", 82},
            {"rec_003.wav", 14},
        },
    };
    smooth_ui_toolkit::Observable<int> _selected_index {0};
};

} // namespace recorder
