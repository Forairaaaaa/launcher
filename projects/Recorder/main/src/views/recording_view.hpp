#pragma once

#include "view_models/recording_view_model.hpp"
#include "views/bottom_key_bar.hpp"
#include "views/view.hpp"
#include <lvgl/lvgl_cpp/obj.hpp>
#include <cstddef>
#include <memory>

namespace recorder {

class RecordingView : public View {
public:
    explicit RecordingView(RecordingViewModel& view_model);
    ~RecordingView() override;

    RecordingView(const RecordingView&)            = delete;
    RecordingView& operator=(const RecordingView&) = delete;

    void onEnter(lv_obj_t* parent) override;
    void onExit() override;

private:
    RecordingViewModel& _view_model;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Container> _root;
    std::unique_ptr<BottomKeyBar> _key_bar;
    size_t _state_observer_id = 0;

    void renderState(RecordingState state);
    static void onStateChanged(void* context, const RecordingState& state);
};

}  // namespace recorder
