#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace recorder {

namespace recorder_key {

constexpr uint32_t Up   = 0x10000001;
constexpr uint32_t Down = 0x10000002;

}  // namespace recorder_key

enum class PageId {
    Recording,
    Files,
    Playback,
};

enum class RecordingState {
    Idle,
    Recording,
    Paused,
};

enum class PlaybackState {
    Stopped,
    Playing,
    Paused,
};

struct AudioFrame {
    float amp = 0.0f;
    std::vector<float> samples;
    std::vector<float> spectrum;
};

struct RecordingFile {
    std::string path;
    uint32_t durationSec = 0;
};

struct PendingRecordingFile {
    bool active = false;
    std::string path;
    std::string name;
    uint32_t durationSec = 0;
};

const char* pageIdName(PageId page);
const char* recordingStateName(RecordingState state);
const char* playbackStateName(PlaybackState state);

}  // namespace recorder
