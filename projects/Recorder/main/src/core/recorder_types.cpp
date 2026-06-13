#include "core/recorder_types.hpp"

namespace recorder {

const char* pageIdName(PageId page)
{
    switch (page) {
        case PageId::Recording:
            return "Recording";
        case PageId::Files:
            return "Files";
        case PageId::Playback:
            return "Playback";
    }
    return "Unknown";
}

const char* recordingStateName(RecordingState state)
{
    switch (state) {
        case RecordingState::Idle:
            return "Idle";
        case RecordingState::Recording:
            return "Recording";
        case RecordingState::Paused:
            return "Paused";
    }
    return "Unknown";
}

const char* playbackStateName(PlaybackState state)
{
    switch (state) {
        case PlaybackState::Stopped:
            return "Stopped";
        case PlaybackState::Playing:
            return "Playing";
        case PlaybackState::Paused:
            return "Paused";
    }
    return "Unknown";
}

}  // namespace recorder
