#pragma once

#include <string>

namespace recorder {

struct RecorderConfig {
    std::string recordings_dir;
};

RecorderConfig defaultRecorderConfig();
std::string defaultRecordingsDirectory();
std::string normalizeRecordingDirectory(const std::string& path);
bool ensureDirectoryExists(const std::string& path, const char* log_tag);

}  // namespace recorder
