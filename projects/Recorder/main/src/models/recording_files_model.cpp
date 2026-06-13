#include "models/recording_files_model.hpp"
#include <miniaudio.h>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <dirent.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace recorder {

namespace {

std::string recordingsDirectory()
{
    char cwd[512] = {};
    if (getcwd(cwd, sizeof(cwd)) == nullptr) {
        spdlog::warn("RecordingFilesModel: getcwd failed, fallback to current directory");
        std::snprintf(cwd, sizeof(cwd), ".");
    }

    std::string dir = std::string(cwd) + "/recordings";
    if (mkdir(dir.c_str(), 0755) != 0 && errno != EEXIST) {
        spdlog::warn("RecordingFilesModel: failed to create {}, errno={}", dir, errno);
    }
    return dir;
}

bool hasWavExtension(const std::string& name)
{
    if (name.size() < 4) {
        return false;
    }

    std::string ext = name.substr(name.size() - 4);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext == ".wav";
}

uint32_t wavDurationSec(const std::string& path)
{
    ma_decoder decoder{};
    ma_result result = ma_decoder_init_file(path.c_str(), nullptr, &decoder);
    if (result != MA_SUCCESS) {
        spdlog::warn("RecordingFilesModel: failed to read wav duration, path={}, result={}", path,
                     static_cast<int>(result));
        return 0;
    }

    ma_uint64 frame_count       = 0;
    result                      = ma_decoder_get_length_in_pcm_frames(&decoder, &frame_count);
    const ma_uint32 sample_rate = decoder.outputSampleRate;
    ma_decoder_uninit(&decoder);

    if (result != MA_SUCCESS || sample_rate == 0) {
        return 0;
    }

    return static_cast<uint32_t>(std::round(static_cast<double>(frame_count) / static_cast<double>(sample_rate)));
}

}  // namespace

const RecordingFile* RecordingFilesModel::selectedFile() const
{
    const auto& list = _files.get();
    int index        = _selected_index.get();
    if (index < 0 || index >= static_cast<int>(list.size())) {
        return nullptr;
    }
    return &list[index];
}

void RecordingFilesModel::refresh(bool preserveSelected)
{
    const RecordingFile* selected   = preserveSelected ? selectedFile() : nullptr;
    const std::string selected_path = selected ? selected->path : "";
    const std::string dir           = recordingsDirectory();

    struct Entry {
        RecordingFile file;
        time_t modified = 0;
    };

    std::vector<Entry> entries;
    DIR* handle = opendir(dir.c_str());
    if (!handle) {
        spdlog::warn("RecordingFilesModel: failed to open recordings directory {}, errno={}", dir, errno);
        _files.set(std::vector<RecordingFile>{});
        _selected_index.set(-1);
        return;
    }

    while (dirent* entry = readdir(handle)) {
        const std::string name = entry->d_name;
        if (name == "." || name == ".." || !hasWavExtension(name)) {
            continue;
        }

        const std::string path = dir + "/" + name;
        struct stat info{};
        if (stat(path.c_str(), &info) != 0 || !S_ISREG(info.st_mode)) {
            continue;
        }

        entries.push_back({RecordingFile{path, wavDurationSec(path)}, info.st_mtime});
    }
    closedir(handle);

    std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) {
        if (a.modified != b.modified) {
            return a.modified > b.modified;
        }
        return a.file.path < b.file.path;
    });

    std::vector<RecordingFile> list;
    list.reserve(entries.size());
    int selected_index = entries.empty() ? -1 : 0;
    for (size_t i = 0; i < entries.size(); ++i) {
        if (!selected_path.empty() && entries[i].file.path == selected_path) {
            selected_index = static_cast<int>(i);
        }
        list.push_back(std::move(entries[i].file));
    }

    _files.set(std::move(list));
    _selected_index.set(selected_index);
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

    const std::string path = list[index].path;
    if (std::remove(path.c_str()) != 0) {
        spdlog::warn("RecordingFilesModel: failed to delete recording path={}, errno={}", path, errno);
    } else {
        spdlog::info("RecordingFilesModel: deleted recording path={}", path);
    }

    list.erase(list.begin() + index);
    int next_index = index;
    if (list.empty()) {
        next_index = -1;
    } else if (index >= static_cast<int>(list.size())) {
        next_index = static_cast<int>(list.size()) - 1;
    }

    _files.set(std::move(list));
    _selected_index.set(next_index);
}

}  // namespace recorder
