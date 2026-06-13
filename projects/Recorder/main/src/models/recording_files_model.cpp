#include "models/recording_files_model.hpp"

namespace recorder {

const RecordingFile* RecordingFilesModel::selectedFile() const
{
    const auto& list = _files.get();
    int index        = _selected_index.get();
    if (index < 0 || index >= static_cast<int>(list.size())) {
        return nullptr;
    }
    return &list[index];
}

void RecordingFilesModel::selectPrevious()
{
    const auto& list = _files.get();
    if (list.empty()) {
        _selected_index.set(-1);
        return;
    }

    int index = _selected_index.get() - 1;
    if (index < 0) {
        index = static_cast<int>(list.size()) - 1;
    }
    _selected_index.set(index);
}

void RecordingFilesModel::selectNext()
{
    const auto& list = _files.get();
    if (list.empty()) {
        _selected_index.set(-1);
        return;
    }

    int index = _selected_index.get() + 1;
    if (index >= static_cast<int>(list.size())) {
        index = 0;
    }
    _selected_index.set(index);
}

void RecordingFilesModel::deleteSelected()
{
    auto list = _files.get();
    int index = _selected_index.get();
    if (index < 0 || index >= static_cast<int>(list.size())) {
        return;
    }

    list.erase(list.begin() + index);
    if (list.empty()) {
        _selected_index.set(-1);
    } else if (index >= static_cast<int>(list.size())) {
        _selected_index.set(static_cast<int>(list.size()) - 1);
    }
    _files.set(std::move(list));
}

}  // namespace recorder
