#pragma once

#include "view_models/calibration_view_model.hpp"
#include "views/view.hpp"
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
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Label> _title_label;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Label> _hint_label;

    void renderState(CalibrationState state);
    static void onStateChanged(void* context, const CalibrationState& state);
};

}  // namespace compass
