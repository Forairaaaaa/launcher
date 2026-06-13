#include "views/recording_view.hpp"
#include "assets/assets.h"
#include <core/animation/animate_value/animate_value.hpp>
#include <core/animation/generators/generators.hpp>
#include <core/color/color.hpp>
#include <core/easing/ease.hpp>
#include <lvgl/lvgl_cpp/label.hpp>
#include <lvgl/number_flow/number_flow.hpp>
#include <tools/ring_buffer/ring_buffer.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace recorder {

namespace {

constexpr int32_t kWaveformPanelWidth            = 256;
constexpr int32_t kWaveformPanelHeight           = 90;
constexpr int32_t kWaveformPanelX                = 0;
constexpr int32_t kWaveformPanelY                = -21;
constexpr uint32_t kWaveformIdleColor            = 0xFED40D;
constexpr uint32_t kWaveformRecordingColor       = 0xF0544D;
constexpr float kWaveformColorDuration           = 0.4f;
constexpr int32_t kWaveformBarWidth              = 1;
constexpr int32_t kWaveformBarPitch              = 3;
constexpr int32_t kWaveformMinBarHeight          = 2;
constexpr int32_t kWaveformMaxBarHeight          = 86;
constexpr size_t kWaveformBarCount               = 86;
constexpr size_t kWaveformEdgeFadeBarCount       = 3;
constexpr size_t kLineWaveformPointCount         = 128;
constexpr float kLineWaveformGain                = 8.0f;
constexpr uint32_t kWaveformHistoryMs            = 3000;
constexpr uint32_t kWaveformSampleIntervalMs     = kWaveformHistoryMs / kWaveformBarCount;
constexpr uint32_t kLineWaveformSampleIntervalMs = kWaveformHistoryMs / kLineWaveformPointCount;
constexpr float kWaveformGain                    = 8.0f;
constexpr int32_t kDurationPanelWidth            = 70;
constexpr int32_t kDurationPanelHeight           = 18;
constexpr int32_t kDurationPanelX                = 0;
constexpr int32_t kDurationPanelY                = 38;
constexpr int32_t kDurationPanelHiddenWidth      = 18;
constexpr int32_t kDurationPanelHiddenY          = 64;
constexpr int32_t kPausedLabelX                  = 0;
constexpr int32_t kPausedLabelY                  = -75;
constexpr float kPausedLabelFadeDuration         = 0.3f;

}  // namespace

class RecordingWaveformViewBase {
public:
    explicit RecordingWaveformViewBase(lv_obj_t* parent)
        : _panel(std::make_unique<smooth_ui_toolkit::lvgl_cpp::Container>(parent)), _color(kWaveformIdleColor)
    {
        _color.duration       = kWaveformColorDuration;
        _color.easingFunction = smooth_ui_toolkit::ease::ease_out_quad;
        _color.begin();

        _panel->setSize(kWaveformPanelWidth, kWaveformPanelHeight);
        _panel->align(LV_ALIGN_CENTER, kWaveformPanelX, kWaveformPanelY);
        _panel->setBgOpa(LV_OPA_TRANSP);
        _panel->setBorderWidth(0);
        _panel->setShadowWidth(0);
        _panel->setPaddingAll(0);
        _panel->setScrollbarMode(LV_SCROLLBAR_MODE_OFF);
        _panel->removeFlag(LV_OBJ_FLAG_SCROLLABLE);
    }

    virtual ~RecordingWaveformViewBase() = default;

    virtual void setFrame(const AudioFrame& frame) = 0;

    virtual void setRecording(bool recording)
    {
        _color.move(recording ? kWaveformRecordingColor : kWaveformIdleColor);
    }

    virtual void tick(uint32_t nowMs)
    {
        (void)nowMs;
        _color.update();
        lv_obj_invalidate(_panel->raw_ptr());
    }

protected:
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Container> _panel;

    lv_color_t currentColor() const
    {
        return lv_color_hex(_color.toHex());
    }

    static lv_opa_t edgeOpacity(size_t index, size_t count)
    {
        const size_t edge_distance = std::min(index, count - 1 - index);
        if (edge_distance >= kWaveformEdgeFadeBarCount) {
            return LV_OPA_COVER;
        }

        const size_t fade_step = edge_distance + 1;
        return static_cast<lv_opa_t>((LV_OPA_COVER * fade_step) / (kWaveformEdgeFadeBarCount + 1));
    }

private:
    smooth_ui_toolkit::color::AnimateRgb_t _color;
};

class BasicWaveformView : public RecordingWaveformViewBase {
public:
    explicit BasicWaveformView(lv_obj_t* parent) : RecordingWaveformViewBase(parent)
    {
        _panel->addEventCb(onDraw, LV_EVENT_DRAW_MAIN, this);

        for (size_t i = 0; i < kWaveformBarCount; ++i) {
            _bars.push(0.0f);
        }
    }

    void setFrame(const AudioFrame& frame) override
    {
        float amp = frame.amp;
        if (!frame.samples.empty()) {
            float sample_sum = 0.0f;
            for (float sample : frame.samples) {
                sample_sum += std::abs(sample * 2.0f - 1.0f);
            }
            amp = std::max(amp, sample_sum / static_cast<float>(frame.samples.size()));
        }
        _target_amp = std::max(_target_amp, std::clamp(amp, 0.0f, 1.0f));
    }

    void tick(uint32_t nowMs) override
    {
        updateAmp(nowMs);

        if (_last_sample_ms == 0) {
            _last_sample_ms = nowMs;
        }

        while (nowMs - _last_sample_ms >= kWaveformSampleIntervalMs) {
            _bars.push(toVisualAmp(_display_amp));
            _last_sample_ms += kWaveformSampleIntervalMs;
        }

        RecordingWaveformViewBase::tick(nowMs);
    }

private:
    smooth_ui_toolkit::RingBuffer<float, kWaveformBarCount> _bars;
    float _target_amp        = 0.0f;
    float _display_amp       = 0.0f;
    uint32_t _last_tick_ms   = 0;
    uint32_t _last_sample_ms = 0;

    void updateAmp(uint32_t nowMs)
    {
        float dt = 0.0f;
        if (_last_tick_ms != 0 && nowMs >= _last_tick_ms) {
            dt = static_cast<float>(nowMs - _last_tick_ms) / 1000.0f;
        }
        _last_tick_ms = nowMs;

        const float response = _target_amp >= _display_amp ? 0.42f : 0.32f;
        _display_amp += (_target_amp - _display_amp) * response;
        _target_amp *= std::pow(0.03f, dt * 6.0f);
        if (_target_amp < 0.0005f) {
            _target_amp = 0.0f;
        }
        if (_display_amp < 0.0005f) {
            _display_amp = 0.0f;
        }
    }

    static float toVisualAmp(float amp)
    {
        const float gained = std::clamp(amp * kWaveformGain, 0.0f, 1.0f);
        return std::pow(gained, 0.68f);
    }

    void draw(lv_event_t* event)
    {
        lv_layer_t* layer = lv_event_get_layer(event);
        if (!layer) {
            return;
        }

        lv_area_t coords;
        lv_obj_get_coords(_panel->raw_ptr(), &coords);

        lv_draw_line_dsc_t line_dsc;
        lv_draw_line_dsc_init(&line_dsc);
        line_dsc.color = currentColor();
        line_dsc.width = kWaveformBarWidth;
        line_dsc.opa   = LV_OPA_COVER;

        const int32_t mid_y = coords.y1 + kWaveformPanelHeight / 2;
        const int32_t start_x =
            coords.x1 + (kWaveformPanelWidth - ((kWaveformBarCount - 1) * kWaveformBarPitch + 1)) / 2;
        size_t index = 0;
        _bars.peekAll([&](const float& value, bool& stopPeeking) {
            (void)stopPeeking;

            int32_t bar_h = kWaveformMinBarHeight +
                            static_cast<int32_t>(std::round(value * (kWaveformMaxBarHeight - kWaveformMinBarHeight)));
            bar_h         = std::clamp(bar_h, kWaveformMinBarHeight, kWaveformMaxBarHeight);

            const int32_t x    = start_x + static_cast<int32_t>(index) * kWaveformBarPitch;
            const int32_t half = bar_h / 2;
            line_dsc.p1.x      = x;
            line_dsc.p1.y      = mid_y - half;
            line_dsc.p2.x      = x;
            line_dsc.p2.y      = mid_y + half;
            line_dsc.opa       = edgeOpacity(index, kWaveformBarCount);
            lv_draw_line(layer, &line_dsc);
            ++index;
        });
    }

    static void onDraw(lv_event_t* event)
    {
        auto* self = static_cast<BasicWaveformView*>(lv_event_get_user_data(event));
        if (self) {
            self->draw(event);
        }
    }
};

class LineWaveformView : public RecordingWaveformViewBase {
public:
    explicit LineWaveformView(lv_obj_t* parent) : RecordingWaveformViewBase(parent)
    {
        _panel->addEventCb(onDraw, LV_EVENT_DRAW_MAIN, this);

        for (size_t i = 0; i < kLineWaveformPointCount; ++i) {
            _points.push(0.0f);
        }
    }

    void setFrame(const AudioFrame& frame) override
    {
        _samples = frame.samples;
        if (_sample_cursor >= _samples.size()) {
            _sample_cursor = 0;
        }
    }

    void tick(uint32_t nowMs) override
    {
        if (_last_sample_ms == 0) {
            _last_sample_ms = nowMs;
        }

        while (nowMs - _last_sample_ms >= kLineWaveformSampleIntervalMs) {
            _points.push(nextPoint());
            _last_sample_ms += kLineWaveformSampleIntervalMs;
        }

        RecordingWaveformViewBase::tick(nowMs);
    }

private:
    smooth_ui_toolkit::RingBuffer<float, kLineWaveformPointCount> _points;
    std::vector<float> _samples;
    size_t _sample_cursor    = 0;
    uint32_t _last_sample_ms = 0;

    float nextPoint()
    {
        if (_samples.empty()) {
            return 0.0f;
        }

        const float sample = _samples[_sample_cursor];
        _sample_cursor     = (_sample_cursor + 1) % _samples.size();
        return std::clamp((sample * 2.0f - 1.0f) * kLineWaveformGain, -1.0f, 1.0f);
    }

    void draw(lv_event_t* event)
    {
        lv_layer_t* layer = lv_event_get_layer(event);
        if (!layer) {
            return;
        }

        lv_area_t coords;
        lv_obj_get_coords(_panel->raw_ptr(), &coords);

        lv_draw_line_dsc_t line_dsc;
        lv_draw_line_dsc_init(&line_dsc);
        line_dsc.color = currentColor();
        line_dsc.width = kWaveformBarWidth;
        line_dsc.opa   = LV_OPA_COVER;

        const int32_t mid_y = coords.y1 + kWaveformPanelHeight / 2;
        const float x_step =
            static_cast<float>(kWaveformPanelWidth - 1) / static_cast<float>(kLineWaveformPointCount - 1);
        const float y_scale = static_cast<float>(kWaveformMaxBarHeight) * 0.5f;

        bool has_previous = false;
        lv_point_precise_t previous{};
        size_t index = 0;
        _points.peekAll([&](const float& value, bool& stopPeeking) {
            (void)stopPeeking;

            lv_point_precise_t current{};
            current.x = coords.x1 + x_step * static_cast<float>(index);
            current.y = mid_y - std::clamp(value, -1.0f, 1.0f) * y_scale;

            if (has_previous) {
                line_dsc.p1  = previous;
                line_dsc.p2  = current;
                line_dsc.opa = edgeOpacity(index, kLineWaveformPointCount);
                lv_draw_line(layer, &line_dsc);
            }

            previous     = current;
            has_previous = true;
            ++index;
        });
    }

    static void onDraw(lv_event_t* event)
    {
        auto* self = static_cast<LineWaveformView*>(lv_event_get_user_data(event));
        if (self) {
            self->draw(event);
        }
    }
};

class RecordingView::DurationPanel {
public:
    explicit DurationPanel(lv_obj_t* parent)
        : _panel(std::make_unique<smooth_ui_toolkit::lvgl_cpp::Container>(parent)),
          _width(kDurationPanelHiddenWidth),
          _y(kDurationPanelHiddenY)
    {
        _width.springOptions().visualDuration = 0.35f;
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
        flow->animationOptions.springVisualDuration = 0.3f;
        flow->minDigits                             = 2;
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

class RecordingView::PausedLabel {
public:
    explicit PausedLabel(lv_obj_t* parent)
        : _label(std::make_unique<smooth_ui_toolkit::lvgl_cpp::Label>(parent)), _opacity(0)
    {
        _opacity.easingOptions().duration       = kPausedLabelFadeDuration;
        _opacity.easingOptions().easingFunction = smooth_ui_toolkit::ease::ease_out_quad;

        _label->setText("-PAUSED-");
        _label->setTextFont(&font_chivo_mono_medium_12);
        _label->setTextColor(lv_color_hex(0xFED40D));
        _label->setTextAlign(LV_TEXT_ALIGN_CENTER);
        _label->align(LV_ALIGN_CENTER, kPausedLabelX, kPausedLabelY);
        _label->setOpa(0);
        _label->addFlag(LV_OBJ_FLAG_HIDDEN);
    }

    void setVisible(bool visible)
    {
        if (_visible == visible) {
            return;
        }

        _visible = visible;
        if (_visible) {
            _label->removeFlag(LV_OBJ_FLAG_HIDDEN);
            _opacity.move(255);
        } else {
            _opacity.move(0);
        }
    }

    void tick()
    {
        _opacity.update();
        _label->setOpa(toOpa(_opacity.directValue()));

        if (!_visible && _opacity.done() && _opacity.directValue() <= 0.0f) {
            _label->addFlag(LV_OBJ_FLAG_HIDDEN);
        }
    }

private:
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Label> _label;
    smooth_ui_toolkit::AnimateValue _opacity;
    bool _visible = false;

    static lv_opa_t toOpa(float value)
    {
        return static_cast<lv_opa_t>(std::clamp(static_cast<int>(std::round(value)), 0, 255));
    }
};

RecordingView::RecordingView(RecordingViewModel& view_model) : _view_model(view_model)
{
}

RecordingView::~RecordingView()
{
    onExit();
}

void RecordingView::createWaveform(RecordingWaveformType type)
{
    if (!_root) {
        return;
    }

    switch (type) {
        case RecordingWaveformType::Basic:
            _waveform = std::make_unique<BasicWaveformView>(_root->raw_ptr());
            break;
        case RecordingWaveformType::Line:
            _waveform = std::make_unique<LineWaveformView>(_root->raw_ptr());
            break;
    }

    _waveform->setRecording(_view_model.state().get() == RecordingState::Recording);
    _waveform->setFrame(_view_model.frame().get());
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

    createWaveform(_view_model.waveformType().get());
    _duration_panel = std::make_unique<DurationPanel>(_root->raw_ptr());
    _paused_label   = std::make_unique<PausedLabel>(_root->raw_ptr());

    _key_bar              = std::make_unique<BottomKeyBar>(_root->raw_ptr());
    _state_observer_id    = _view_model.state().observe(this, onStateChanged);
    _elapsed_observer_id  = _view_model.elapsedSec().observe(this, onElapsedChanged);
    _frame_observer_id    = _view_model.frame().observe(this, onFrameChanged);
    _waveform_observer_id = _view_model.waveformType().observe(this, onWaveformTypeChanged);
}

void RecordingView::onExit()
{
    if (_waveform_observer_id != 0) {
        _view_model.waveformType().removeObserver(_waveform_observer_id);
        _waveform_observer_id = 0;
    }
    if (_frame_observer_id != 0) {
        _view_model.frame().removeObserver(_frame_observer_id);
        _frame_observer_id = 0;
    }
    if (_elapsed_observer_id != 0) {
        _view_model.elapsedSec().removeObserver(_elapsed_observer_id);
        _elapsed_observer_id = 0;
    }
    if (_state_observer_id != 0) {
        _view_model.state().removeObserver(_state_observer_id);
        _state_observer_id = 0;
    }

    _key_bar.reset();
    _paused_label.reset();
    _duration_panel.reset();
    _waveform.reset();
    _root.reset();
}

void RecordingView::tick(uint32_t nowMs)
{
    (void)nowMs;

    if (_duration_panel) {
        _duration_panel->tick();
    }
    if (_paused_label) {
        _paused_label->tick();
    }
    if (_waveform) {
        _waveform->tick(nowMs);
    }
    if (_key_bar) {
        _key_bar->tick();
    }
}

void RecordingView::renderState(RecordingState state)
{
    if (!_key_bar) {
        return;
    }

    if (_waveform) {
        _waveform->setRecording(state == RecordingState::Recording);
    }

    switch (state) {
        case RecordingState::Idle:
            if (_duration_panel) {
                _duration_panel->setVisible(false);
            }
            if (_paused_label) {
                _paused_label->setVisible(false);
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
            if (_paused_label) {
                _paused_label->setVisible(false);
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
            if (_paused_label) {
                _paused_label->setVisible(true);
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

void RecordingView::renderFrame(const AudioFrame& frame)
{
    if (_waveform) {
        _waveform->setFrame(frame);
    }
}

void RecordingView::renderWaveformType(RecordingWaveformType type)
{
    createWaveform(type);
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

void RecordingView::onFrameChanged(void* context, const AudioFrame& frame)
{
    auto* self = static_cast<RecordingView*>(context);
    if (self) {
        self->renderFrame(frame);
    }
}

void RecordingView::onWaveformTypeChanged(void* context, const RecordingWaveformType& type)
{
    auto* self = static_cast<RecordingView*>(context);
    if (self) {
        self->renderWaveformType(type);
    }
}

}  // namespace recorder
