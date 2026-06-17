#include "views/calibration_view.hpp"
#include "assets/assets.h"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace compass {

namespace {

constexpr int32_t kProgressTrackWidth  = 126;
constexpr int32_t kProgressTrackHeight = 3;
constexpr uint32_t kTitleColor         = 0xFFFFFF;
constexpr uint32_t kHintColor          = 0xF2F2F2;
constexpr uint32_t kProgressTrackColor = 0x333333;
constexpr uint32_t kProgressFillColor  = 0x53D671;

}  // namespace

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

    _gesture_image = std::make_unique<smooth_ui_toolkit::lvgl_cpp::Image>(_root->raw_ptr());
    _gesture_image->setSrc(&image_calibration_guesture);
    _gesture_image->align(LV_ALIGN_CENTER, 0, -17 - 12);

    _hint_label = std::make_unique<smooth_ui_toolkit::lvgl_cpp::Label>(_root->raw_ptr());
    _hint_label->setTextFont(&font_chivo_medium_14);
    _hint_label->setTextColor(lv_color_hex(kHintColor));
    _hint_label->setTextAlign(LV_TEXT_ALIGN_CENTER);
    _hint_label->setSize(280, 18);
    _hint_label->align(LV_ALIGN_CENTER, 0, 29 - 12);

    _progress_track = std::make_unique<smooth_ui_toolkit::lvgl_cpp::Container>(_root->raw_ptr());
    _progress_track->setSize(kProgressTrackWidth, kProgressTrackHeight);
    _progress_track->setBgColor(lv_color_hex(kProgressTrackColor));
    _progress_track->setBgOpa(LV_OPA_COVER);
    _progress_track->setBorderWidth(0);
    _progress_track->setOutlineWidth(0);
    _progress_track->setShadowWidth(0);
    _progress_track->setPaddingAll(0);
    _progress_track->setRadius(kProgressTrackHeight / 2);
    _progress_track->align(LV_ALIGN_CENTER, 0, 52 - 12);
    _progress_track->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

    _progress_fill = std::make_unique<smooth_ui_toolkit::lvgl_cpp::Container>(_progress_track->raw_ptr());
    _progress_fill->setSize(0, kProgressTrackHeight);
    _progress_fill->setBgColor(lv_color_hex(kProgressFillColor));
    _progress_fill->setBgOpa(LV_OPA_COVER);
    _progress_fill->setBorderWidth(0);
    _progress_fill->setOutlineWidth(0);
    _progress_fill->setShadowWidth(0);
    _progress_fill->setPaddingAll(0);
    _progress_fill->setRadius(kProgressTrackHeight / 2);
    _progress_fill->setPos(0, 0);
    _progress_fill->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

    _view_model.state().observe(this, onStateChanged);
    _view_model.status().observe(this, onStatusChanged);
    _view_model.progress().observe(this, onProgressChanged);
    renderState(_view_model.state().get());
    renderStatus(_view_model.status().get());
    renderProgress(_view_model.progress().get());

    spdlog::info("CalibrationView enter");
}

void CalibrationView::onExit()
{
    _view_model.progress().removeObserver();
    _view_model.status().removeObserver();
    _view_model.state().removeObserver();
    _progress_fill.reset();
    _progress_track.reset();
    _hint_label.reset();
    _gesture_image.reset();
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
            _hint_label->setText("Press Enter to start calibration");
            break;
        case CalibrationState::Running:
            renderStatus(_view_model.status().get());
            break;
        case CalibrationState::Done:
            _hint_label->setText("Calibration saved");
            break;
    }
}

void CalibrationView::renderStatus(const std::string& status)
{
    if (!_hint_label || _view_model.state().get() != CalibrationState::Running) {
        return;
    }

    _hint_label->setText(status.empty() ? "Move in a figure eight" : status);
}

void CalibrationView::renderProgress(float progress)
{
    if (!_progress_fill) {
        return;
    }

    progress = std::clamp(progress, 0.0f, 1.0f);
    _progress_fill->setWidth(static_cast<int32_t>(progress * kProgressTrackWidth));
}

void CalibrationView::onStateChanged(void* context, const CalibrationState& state)
{
    auto* self = static_cast<CalibrationView*>(context);
    if (self) {
        self->renderState(state);
    }
}

void CalibrationView::onStatusChanged(void* context, const std::string& status)
{
    auto* self = static_cast<CalibrationView*>(context);
    if (self) {
        self->renderStatus(status);
    }
}

void CalibrationView::onProgressChanged(void* context, const float& progress)
{
    auto* self = static_cast<CalibrationView*>(context);
    if (self) {
        self->renderProgress(progress);
    }
}

}  // namespace compass
