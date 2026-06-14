#pragma once

#include "core/recorder_config.hpp"
#include "models/playback_model.hpp"
#include "models/recording_files_model.hpp"
#include "models/recording_model.hpp"
#include "view_models/files_view_model.hpp"
#include "view_models/playback_view_model.hpp"
#include "view_models/recording_view_model.hpp"
#include "views/recording_files_view.hpp"
#include "views/playback_view.hpp"
#include "views/recording_view.hpp"
#include "views/view.hpp"
#include <lvgl.h>
#include <array>
#include <utility>

namespace recorder {

class RecorderApp {
public:
    explicit RecorderApp(RecorderConfig config = defaultRecorderConfig());
    ~RecorderApp();

    RecorderApp(const RecorderApp&)            = delete;
    RecorderApp& operator=(const RecorderApp&) = delete;

    void start();
    void onKey(uint32_t key);
    void tick(uint32_t nowMs);

    RecorderRouter& router()
    {
        return _router;
    }

private:
    RecorderConfig _config;
    RecorderRouter _router;
    RecordingModel _recording_model;
    RecordingFilesModel _files_model;
    PlaybackModel _playback_model;
    RecordingViewModel _recording_vm;
    FilesViewModel _files_vm;
    PlaybackViewModel _playback_vm;
    RecordingView _recording_view;
    RecordingFilesView _recording_files_view;
    PlaybackView _playback_view;
    ViewModel* _current_vm    = nullptr;
    View* _current_view       = nullptr;
    lv_group_t* _input_group  = nullptr;
    size_t _route_observer_id = 0;

    std::array<ViewModel*, 3> _view_models;
    std::array<View*, 3> _views;

    ViewModel* viewModelFor(PageId page);
    View* viewFor(PageId page);
    void setupInputGroup();
    void setCurrentPage(PageId page);
    static void onRouteChanged(void* context, const PageId& page);
    static void onKeyboardEvent(lv_event_t* event);
};

}  // namespace recorder
