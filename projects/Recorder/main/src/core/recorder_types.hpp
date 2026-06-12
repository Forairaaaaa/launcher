#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace recorder {

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
};

struct RecordingFile {
    std::string path;
    uint32_t durationSec = 0;
};

const char* pageIdName(PageId page);
const char* recordingStateName(RecordingState state);
const char* playbackStateName(PlaybackState state);

} // namespace recorder
