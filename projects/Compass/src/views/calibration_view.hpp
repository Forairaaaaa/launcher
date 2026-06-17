#pragma once

#include "view_models/calibration_view_model.hpp"
#include "views/view.hpp"
#include <lvgl/lvgl_cpp/image.hpp>
#include <lvgl/lvgl_cpp/label.hpp>
#include <lvgl/lvgl_cpp/obj.hpp>
#include <memory>

namespace compass {

class CalibrationView : public View {
public:
    explicit CalibrationView(CalibrationViewModel& view_model);
    ~CalibrationView() override;

    void onEnter(lv_obj_t* parent) override;
    void onExit() override;
    void tick(uint32_t nowMs) override;

private:
    CalibrationViewModel& _view_model;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Container> _root;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Image> _gesture_image;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Label> _hint_label;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Container> _progress_track;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Container> _progress_fill;

    void renderState(CalibrationState state);
    void renderStatus(const std::string& status);
    void renderProgress(float progress);
    static void onStateChanged(void* context, const CalibrationState& state);
    static void onStatusChanged(void* context, const std::string& status);
    static void onProgressChanged(void* context, const float& progress);
};

}  // namespace compass
