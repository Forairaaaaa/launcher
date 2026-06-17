#include "core/recorder_config.hpp"
#include <spdlog/spdlog.h>
#include <cerrno>
#include <cstdio>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace recorder {

namespace {

std::string currentDirectory()
{
    char cwd[512] = {};
    if (getcwd(cwd, sizeof(cwd)) == nullptr) {
        spdlog::warn("RecorderConfig: getcwd failed, fallback to current directory");
        std::snprintf(cwd, sizeof(cwd), ".");
    }
    return cwd;
}

bool isDirectory(const std::string& path)
{
    struct stat info{};
    return stat(path.c_str(), &info) == 0 && S_ISDIR(info.st_mode);
}

bool ensureSingleDirectory(const std::string& path, const char* log_tag)
{
    if (isDirectory(path)) {
        return true;
    }

    if (mkdir(path.c_str(), 0755) == 0) {
        return true;
    }

    if (errno == EEXIST && isDirectory(path)) {
        return true;
    }

    spdlog::warn("{}: failed to create directory {}, errno={}", log_tag, path, errno);
    return false;
}

}  // namespace

std::string defaultRecordingsDirectory()
{
    return normalizeRecordingDirectory("recordings");
}

RecorderConfig defaultRecorderConfig()
{
    return RecorderConfig{defaultRecordingsDirectory()};
}

std::string normalizeRecordingDirectory(const std::string& path)
{
    std::string normalized = path.empty() ? "recordings" : path;
    if (!normalized.empty() && normalized.front() != '/') {
        normalized = currentDirectory() + "/" + normalized;
    }

    while (normalized.size() > 1 && normalized.back() == '/') {
        normalized.pop_back();
    }
    return normalized;
}

bool ensureDirectoryExists(const std::string& path, const char* log_tag)
{
    if (path.empty()) {
        spdlog::warn("{}: empty directory path", log_tag);
        return false;
    }

    if (path == "/") {
        return true;
    }

    std::string current;
    size_t index = 0;
    if (path.front() == '/') {
        current = "/";
        index   = 1;
    }

    while (index < path.size()) {
        const size_t slash     = path.find('/', index);
        const std::string part = path.substr(index, slash == std::string::npos ? std::string::npos : slash - index);
        index                  = slash == std::string::npos ? path.size() : slash + 1;
        if (part.empty()) {
            continue;
        }

        if (current.empty()) {
            current = part;
        } else if (current == "/") {
            current += part;
        } else {
            current += "/" + part;
        }

        if (!ensureSingleDirectory(current, log_tag)) {
            return false;
        }
    }
    return true;
}

}  // namespace recorder
