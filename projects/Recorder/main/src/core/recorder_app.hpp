#pragma once

#include "models/playback_model.hpp"
#include "models/recording_files_model.hpp"
#include "models/recording_model.hpp"
#include "view_models/files_view_model.hpp"
#include "view_models/playback_view_model.hpp"
#include "view_models/recording_view_model.hpp"
#include <lvgl.h>
#include <array>

namespace recorder {

class RecorderApp {
public:
    RecorderApp();
    ~RecorderApp();

    RecorderApp(const RecorderApp&) = delete;
    RecorderApp& operator=(const RecorderApp&) = delete;

    void start();
    void onKey(uint32_t key);
    void tick(uint32_t nowMs);

    RecorderRouter& router()
    {
        return _router;
    }

private:
    RecorderRouter _router;
    RecordingModel _recording_model;
    RecordingFilesModel _files_model;
    PlaybackModel _playback_model;
    RecordingViewModel _recording_vm;
    FilesViewModel _files_vm;
    PlaybackViewModel _playback_vm;
    ViewModel* _current_vm = nullptr;
    size_t _route_observer_id = 0;

    std::array<ViewModel*, 3> _view_models;

    ViewModel* viewModelFor(PageId page);
    void setCurrentPage(PageId page);
    static void onRouteChanged(void* context, const PageId& page);
    static void onKeyboardEvent(lv_event_t* event);
};

} // namespace recorder
