#include "views/recording_view.hpp"
#include "assets/assets.h"
#include <core/animation/animate_value/animate_value.hpp>
#include <core/animation/generators/generators.hpp>
#include <lvgl/lvgl_cpp/label.hpp>
#include <lvgl/number_flow/number_flow.hpp>
#include <cmath>

namespace recorder {

namespace {

constexpr int32_t kDurationPanelWidth       = 70;
constexpr int32_t kDurationPanelHeight      = 18;
constexpr int32_t kDurationPanelX           = 0;
constexpr int32_t kDurationPanelY           = 38;
constexpr int32_t kDurationPanelHiddenWidth = 18;
constexpr int32_t kDurationPanelHiddenY     = 64;

}  // namespace

class RecordingView::DurationPanel {
public:
    explicit DurationPanel(lv_obj_t* parent)
        : _panel(std::make_unique<smooth_ui_toolkit::lvgl_cpp::Container>(parent)),
          _width(kDurationPanelHiddenWidth),
          _y(kDurationPanelHiddenY)
    {
        _width.springOptions().visualDuration = 0.4f;
        _width.springOptions().bounce         = 0.5f;
        _y.springOptions().visualDuration     = 0.3f;
        _y.springOptions().bounce             = 0.5f;

        _panel->setSize(kDurationPanelWidth, kDurationPanelHeight);
        _panel->setBgColor(lv_color_hex(0xF0544D));
        _panel->setBgOpa(LV_OPA_COVER);
        _panel->setRadius(6);
        _panel->setBorderWidth(0);
        _panel->setShadowWidth(0);
        _panel->setPaddingAll(0);
        _panel->setPadColumn(0);
        _panel->setScrollbarMode(LV_SCROLLBAR_MODE_OFF);
        _panel->removeFlag(LV_OBJ_FLAG_SCROLLABLE);
        _panel->setFlexFlow(LV_FLEX_FLOW_ROW);
        _panel->setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        _panel->addFlag(LV_OBJ_FLAG_HIDDEN);

        createNumber(_hours);
        createSeparator(_colon_hour);
        createNumber(_minutes);
        createSeparator(_colon_minute);
        createNumber(_seconds);
        setElapsed(0);
        applyAnimatedValue();
    }

    void setElapsed(uint32_t elapsed_sec)
    {
        const uint32_t hours   = elapsed_sec / 3600;
        const uint32_t minutes = (elapsed_sec / 60) % 60;
        const uint32_t seconds = elapsed_sec % 60;

        _hours->setValue(static_cast<int>(hours % 100));
        _minutes->setValue(static_cast<int>(minutes));
        _seconds->setValue(static_cast<int>(seconds));
    }

    void tick()
    {
        _width.update();
        _y.update();
        applyAnimatedValue();

        _hours->update();
        _minutes->update();
        _seconds->update();

        if (!_visible && _width.done() && _y.done()) {
            _panel->addFlag(LV_OBJ_FLAG_HIDDEN);
        }
    }

    void setVisible(bool visible)
    {
        if (_visible == visible) {
            return;
        }

        _visible = visible;
        if (_visible) {
            _panel->removeFlag(LV_OBJ_FLAG_HIDDEN);
            _width.move(kDurationPanelWidth);
            _y.move(kDurationPanelY);
        } else {
            _width.move(kDurationPanelHiddenWidth);
            _y.move(kDurationPanelHiddenY);
        }
    }

private:
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Container> _panel;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::NumberFlow> _hours;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::NumberFlow> _minutes;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::NumberFlow> _seconds;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Label> _colon_hour;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Label> _colon_minute;
    smooth_ui_toolkit::AnimateValue _width;
    smooth_ui_toolkit::AnimateValue _y;
    bool _visible = false;

    void createNumber(std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::NumberFlow>& flow)
    {
        flow                = std::make_unique<smooth_ui_toolkit::lvgl_cpp::NumberFlow>(_panel->raw_ptr());
        flow->animationType = smooth_ui_toolkit::AnimationType::Spring;
        flow->minDigits     = 2;
        flow->setTextFont(&font_chivo_mono_medium_12);
        flow->setDigitColor(lv_color_hex(0xFFFFFF));
        flow->init();
    }

    void createSeparator(std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Label>& label)
    {
        label = std::make_unique<smooth_ui_toolkit::lvgl_cpp::Label>(_panel->raw_ptr());
        label->setText(":");
        label->setTextFont(&font_chivo_mono_medium_12);
        label->setTextColor(lv_color_hex(0xFFFFFF));
        label->setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        label->setPaddingAll(0);
    }

    void applyAnimatedValue()
    {
        _panel->setWidth(static_cast<int32_t>(std::round(_width.directValue())));
        _panel->setHeight(kDurationPanelHeight);
        _panel->align(LV_ALIGN_CENTER, kDurationPanelX, static_cast<int32_t>(std::round(_y.directValue())));
    }
};

RecordingView::RecordingView(RecordingViewModel& view_model) : _view_model(view_model)
{
}

RecordingView::~RecordingView()
{
    onExit();
}

void RecordingView::onEnter(lv_obj_t* parent)
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

    _duration_panel      = std::make_unique<DurationPanel>(_root->raw_ptr());
    _key_bar             = std::make_unique<BottomKeyBar>(_root->raw_ptr());
    _state_observer_id   = _view_model.state().observe(this, onStateChanged);
    _elapsed_observer_id = _view_model.elapsedSec().observe(this, onElapsedChanged);
}

void RecordingView::onExit()
{
    if (_elapsed_observer_id != 0) {
        _view_model.elapsedSec().removeObserver(_elapsed_observer_id);
        _elapsed_observer_id = 0;
    }
    if (_state_observer_id != 0) {
        _view_model.state().removeObserver(_state_observer_id);
        _state_observer_id = 0;
    }

    _key_bar.reset();
    _duration_panel.reset();
    _root.reset();
}

void RecordingView::tick(uint32_t nowMs)
{
    (void)nowMs;

    if (_duration_panel) {
        _duration_panel->tick();
    }
}

void RecordingView::renderState(RecordingState state)
{
    if (!_key_bar) {
        return;
    }

    switch (state) {
        case RecordingState::Idle:
            if (_duration_panel) {
                _duration_panel->setVisible(false);
            }
            _key_bar->setItems({
                {'6', &image_icon_record},
                {'8', &image_icon_list},
            });
            break;
        case RecordingState::Recording:
            if (_duration_panel) {
                _duration_panel->setVisible(true);
            }
            _key_bar->setItems({
                {'5', &image_icon_pause},
                {'6', &image_icon_stop},
                {'8', &image_icon_list},
            });
            break;
        case RecordingState::Paused:
            if (_duration_panel) {
                _duration_panel->setVisible(true);
            }
            _key_bar->setItems({
                {'5', &image_icon_record},
                {'6', &image_icon_stop},
                {'8', &image_icon_list},
            });
            break;
    }
}

void RecordingView::renderElapsed(uint32_t elapsed_sec)
{
    if (_duration_panel) {
        _duration_panel->setElapsed(elapsed_sec);
    }
}

void RecordingView::onStateChanged(void* context, const RecordingState& state)
{
    auto* self = static_cast<RecordingView*>(context);
    if (self) {
        self->renderState(state);
    }
}

void RecordingView::onElapsedChanged(void* context, const uint32_t& elapsed_sec)
{
    auto* self = static_cast<RecordingView*>(context);
    if (self) {
        self->renderElapsed(elapsed_sec);
    }
}

}  // namespace recorder
