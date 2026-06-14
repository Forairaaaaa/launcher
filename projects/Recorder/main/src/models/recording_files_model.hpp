#pragma once

#include "core/recorder_types.hpp"
#include <tools/observable/single_observable.hpp>

namespace recorder {

class RecordingFilesModel {
public:
    smooth_ui_toolkit::SingleObservable<std::vector<RecordingFile>>& files()
    {
        return _files;
    }

    smooth_ui_toolkit::SingleObservable<int>& selectedIndex()
    {
        return _selected_index;
    }

    const RecordingFile* selectedFile() const;
    void refresh(bool preserveSelected = false);
    void selectPrevious();
    void selectNext();
    void deleteSelected();

private:
    smooth_ui_toolkit::SingleObservable<std::vector<RecordingFile>> _files{std::vector<RecordingFile>{}};
    smooth_ui_toolkit::SingleObservable<int> _selected_index{-1};
};

}  // namespace recorder
