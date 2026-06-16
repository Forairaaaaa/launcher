#include "views/calibration_view.hpp"
#include <spdlog/spdlog.h>

namespace compass {

CalibrationView::CalibrationView(CalibrationViewModel& view_model) : _view_model(view_model)
{
}

CalibrationView::~CalibrationView()
{
    onExit();
}

void CalibrationView::onEnter(lv_obj_t* parent)
{
    onExit();

    _root = std::make_unique<smooth_ui_toolkit::lvgl_cpp::Container>(parent);
    _root->setSize(lv_pct(100), lv_pct(100));
    _root->setBgColor(lv_color_hex(0x000000));
    _root->setBgOpa(LV_OPA_COVER);
    _root->setBorderWidth(0);
    _root->setPaddingAll(0);
    _root->setScrollbarMode(LV_SCROLLBAR_MODE_OFF);
    _root->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

    _title_label = std::make_unique<smooth_ui_toolkit::lvgl_cpp::Label>(_root->raw_ptr());
    _title_label->setText("Calibration");
    _title_label->setTextColor(lv_color_hex(0xFFFFFF));
    _title_label->setTextAlign(LV_TEXT_ALIGN_CENTER);
    _title_label->align(LV_ALIGN_CENTER, 0, -12);

    _hint_label = std::make_unique<smooth_ui_toolkit::lvgl_cpp::Label>(_root->raw_ptr());
    _hint_label->setTextColor(lv_color_hex(0x777777));
    _hint_label->setTextAlign(LV_TEXT_ALIGN_CENTER);
    _hint_label->align(LV_ALIGN_CENTER, 0, 18);

    _view_model.state().observe(this, onStateChanged);
    renderState(_view_model.state().get());

    spdlog::info("CalibrationView enter");
}

void CalibrationView::onExit()
{
    _view_model.state().removeObserver();
    _hint_label.reset();
    _title_label.reset();
    _root.reset();
}

void CalibrationView::tick(uint32_t nowMs)
{
    (void)nowMs;
}

void CalibrationView::renderState(CalibrationState state)
{
    if (!_hint_label) {
        return;
    }

    switch (state) {
        case CalibrationState::Idle:
            _hint_label->setText("4 / Esc: Back    6 / Enter: Start");
            break;
        case CalibrationState::Running:
            _hint_label->setText("Calibrating...");
            break;
        case CalibrationState::Done:
            _hint_label->setText("Done");
            break;
    }
}

void CalibrationView::onStateChanged(void* context, const CalibrationState& state)
{
    auto* self = static_cast<CalibrationView*>(context);
    if (self) {
        self->renderState(state);
    }
}

}  // namespace compass
