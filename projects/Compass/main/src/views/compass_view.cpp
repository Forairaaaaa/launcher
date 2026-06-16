#include "views/compass_view.hpp"
#include "assets/assets.h"
#include <core/animation/animate_value/animate_value.hpp>
#include <core/easing/ease.hpp>
#include <lvgl/lvgl_cpp/image.hpp>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>

namespace compass {

namespace {

constexpr int32_t kCompassPanelSize          = 150;
constexpr int32_t kCompassExpandedX          = -73;
constexpr int32_t kLargeCrossSize            = 72;
constexpr int32_t kSmallCrossSize            = 21;
constexpr int32_t kCrossThickness            = 1;
constexpr int32_t kBubbleMinSize             = 43;
constexpr int32_t kBubbleMaxSize             = 53;
constexpr int32_t kBubbleTravel              = 26;
constexpr uint32_t kCrossColor               = 0xAAAAAA;
constexpr uint32_t kBubbleColor              = 0x303030;
constexpr float kGravityMetersPerSecond      = 9.81f;
constexpr float kCompassUnavailableHeading   = 0.0f;
constexpr float kCompassUnavailableBubblePos = 0.0f;
constexpr float kCompassUnavailableBubbleZ   = 1.0f;

float clampNormalized(float value)
{
    return std::clamp(value, -1.0f, 1.0f);
}

float clampZeroToOne(float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}

float unwrapAngle(float previous_unwrapped_angle, float next_angle)
{
    float previous_angle = std::fmod(previous_unwrapped_angle, 360.0f);
    if (previous_angle < 0.0f) {
        previous_angle += 360.0f;
    }

    float delta = std::fmod(next_angle - previous_angle, 360.0f);
    if (delta < -180.0f) {
        delta += 360.0f;
    } else if (delta > 180.0f) {
        delta -= 360.0f;
    }

    return previous_unwrapped_angle + delta;
}

}  // namespace

class CompassDialView {
public:
    explicit CompassDialView(lv_obj_t* parent)
    {
        _x.springOptions().visualDuration = 0.45f;
        _x.springOptions().bounce         = 0.4f;

        _heading.easingOptions().duration       = 0.2f;
        _heading.easingOptions().easingFunction = smooth_ui_toolkit::ease::linear;

        _bubble_x.easingOptions().duration       = 0.1f;
        _bubble_x.easingOptions().easingFunction = smooth_ui_toolkit::ease::linear;

        _bubble_y.easingOptions().duration       = 0.1f;
        _bubble_y.easingOptions().easingFunction = smooth_ui_toolkit::ease::linear;

        _bubble_size.easingOptions().duration       = 0.3f;
        _bubble_size.easingOptions().easingFunction = smooth_ui_toolkit::ease::linear;

        _panel = std::make_unique<smooth_ui_toolkit::lvgl_cpp::Container>(parent);
        _panel->setSize(kCompassPanelSize, kCompassPanelSize);
        _panel->align(LV_ALIGN_CENTER, 0, 0);
        _panel->setBgOpa(LV_OPA_TRANSP);
        _panel->setBorderWidth(0);
        _panel->setOutlineWidth(0);
        _panel->setShadowWidth(0);
        _panel->setPaddingAll(0);
        _panel->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

        _dial = std::make_unique<smooth_ui_toolkit::lvgl_cpp::Image>(_panel->raw_ptr());
        _dial->setSrc(&image_compass_dial);
        _dial->setPivot(kCompassPanelSize / 2, kCompassPanelSize / 2);
        _dial->align(LV_ALIGN_CENTER, 0, 0);

        _ball = std::make_unique<smooth_ui_toolkit::lvgl_cpp::Container>(_panel->raw_ptr());
        _ball->setBgColor(lv_color_hex(kBubbleColor));
        _ball->setBgOpa(LV_OPA_COVER);
        _ball->setBorderWidth(0);
        _ball->setOutlineWidth(0);
        _ball->setShadowWidth(0);
        _ball->setPaddingAll(0);
        _ball->setRadius(LV_RADIUS_CIRCLE);
        _ball->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

        _small_cross_horizontal = std::make_unique<smooth_ui_toolkit::lvgl_cpp::Container>(_panel->raw_ptr());
        setupLine(*_small_cross_horizontal, kSmallCrossSize, kCrossThickness);

        _small_cross_vertical = std::make_unique<smooth_ui_toolkit::lvgl_cpp::Container>(_panel->raw_ptr());
        setupLine(*_small_cross_vertical, kCrossThickness, kSmallCrossSize);

        _large_cross_horizontal = std::make_unique<smooth_ui_toolkit::lvgl_cpp::Container>(_panel->raw_ptr());
        setupLine(*_large_cross_horizontal, kLargeCrossSize, kCrossThickness);
        _large_cross_horizontal->align(LV_ALIGN_CENTER, 0, 0);

        _large_cross_vertical = std::make_unique<smooth_ui_toolkit::lvgl_cpp::Container>(_panel->raw_ptr());
        setupLine(*_large_cross_vertical, kCrossThickness, kLargeCrossSize);
        _large_cross_vertical->align(LV_ALIGN_CENTER, 0, 0);

        setSample(CompassSample{});
        applyState();
    }

    void setExpanded(bool expanded)
    {
        _x.move(expanded ? kCompassExpandedX : 0.0f);
    }

    void setSample(const CompassSample& sample)
    {
        const float heading  = sample.available ? sample.headingDeg : kCompassUnavailableHeading;
        const float bubble_x = sample.available ? sample.bubbleX : kCompassUnavailableBubblePos;
        const float bubble_y = sample.available ? sample.bubbleY : kCompassUnavailableBubblePos;
        const float bubble_z =
            sample.available ? std::abs(sample.accel.z) / kGravityMetersPerSecond : kCompassUnavailableBubbleZ;

        if (!_heading_initialized) {
            _heading_unwrapped   = heading;
            _heading_initialized = true;
            _heading.teleport(_heading_unwrapped);
        } else {
            _heading_unwrapped = unwrapAngle(_heading_unwrapped, heading);
            _heading.move(_heading_unwrapped);
        }

        _bubble_x.move(clampNormalized(bubble_x));
        _bubble_y.move(clampNormalized(bubble_y));
        _bubble_size.move(clampZeroToOne(bubble_z));
    }

    void tick()
    {
        _x.update();
        _heading.update();
        _bubble_x.update();
        _bubble_y.update();
        _bubble_size.update();
        applyState();
    }

private:
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Container> _panel;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Image> _dial;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Container> _large_cross_horizontal;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Container> _large_cross_vertical;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Container> _ball;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Container> _small_cross_horizontal;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Container> _small_cross_vertical;
    smooth_ui_toolkit::AnimateValue _x{0};
    smooth_ui_toolkit::AnimateValue _heading{0};
    smooth_ui_toolkit::AnimateValue _bubble_x{0};
    smooth_ui_toolkit::AnimateValue _bubble_y{0};
    smooth_ui_toolkit::AnimateValue _bubble_size{1};
    float _heading_unwrapped  = 0.0f;
    bool _heading_initialized = false;

    void applyState()
    {
        if (!_panel || !_dial || !_ball) {
            return;
        }

        _panel->align(LV_ALIGN_CENTER, static_cast<int32_t>(std::round(_x.directValue())), 0);

        const float heading = _heading.directValue();
        _dial->setRotation(static_cast<int32_t>(std::round(-heading * 10.0f)));

        const float bubble_size_value = clampZeroToOne(_bubble_size.directValue());
        const int32_t ball_size =
            kBubbleMinSize + static_cast<int32_t>(std::round((kBubbleMaxSize - kBubbleMinSize) * bubble_size_value));
        const int32_t offset_x =
            static_cast<int32_t>(std::round(kBubbleTravel * clampNormalized(_bubble_x.directValue())));
        const int32_t offset_y =
            static_cast<int32_t>(std::round(kBubbleTravel * clampNormalized(_bubble_y.directValue())));

        _ball->setSize(ball_size, ball_size);
        _ball->align(LV_ALIGN_CENTER, offset_x, offset_y);
        _small_cross_horizontal->align(LV_ALIGN_CENTER, offset_x, offset_y);
        _small_cross_vertical->align(LV_ALIGN_CENTER, offset_x, offset_y);
    }

    void setupLine(smooth_ui_toolkit::lvgl_cpp::Container& line, int32_t width, int32_t height)
    {
        line.setSize(width, height);
        line.setBgColor(lv_color_hex(kCrossColor));
        line.setBgOpa(LV_OPA_COVER);
        line.setBorderWidth(0);
        line.setOutlineWidth(0);
        line.setShadowWidth(0);
        line.setPaddingAll(0);
        line.setRadius(LV_RADIUS_CIRCLE);
        line.removeFlag(LV_OBJ_FLAG_SCROLLABLE);
    }
};

CompassView::CompassView(CompassViewModel& view_model) : _view_model(view_model)
{
}

CompassView::~CompassView()
{
    onExit();
}

void CompassView::onEnter(lv_obj_t* parent)
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

    _compass_dial = std::make_unique<CompassDialView>(_root->raw_ptr());
    _key_bar      = std::make_unique<BottomKeyBar>(_root->raw_ptr());

    _view_model.sample().observe(this, onSampleChanged);
    _view_model.infoExpanded().observe(this, onInfoExpandedChanged);
    renderSample(_view_model.sample().get());
    renderInfoExpanded(_view_model.infoExpanded().get());

    spdlog::info("CompassView enter");
}

void CompassView::onExit()
{
    _view_model.infoExpanded().removeObserver();
    _view_model.sample().removeObserver();
    _key_bar.reset();
    _compass_dial.reset();
    _root.reset();
}

void CompassView::tick(uint32_t nowMs)
{
    (void)nowMs;
    if (_compass_dial) {
        _compass_dial->tick();
    }
    if (_key_bar) {
        _key_bar->tick();
    }
}

void CompassView::renderSample(const CompassSample& sample)
{
    if (_compass_dial) {
        _compass_dial->setSample(sample);
    }
}

void CompassView::renderInfoExpanded(bool expanded)
{
    if (_compass_dial) {
        _compass_dial->setExpanded(expanded);
    }

    if (!_key_bar) {
        return;
    }

    if (expanded) {
        _key_bar->setItems({
            {'7', &image_icon_calibration},
            {'8', &image_icon_back},
        });
        return;
    }

    _key_bar->setItems({
        {'8', &image_icon_more},
    });
}

void CompassView::onSampleChanged(void* context, const CompassSample& sample)
{
    auto* self = static_cast<CompassView*>(context);
    if (self) {
        self->renderSample(sample);
    }
}

void CompassView::onInfoExpandedChanged(void* context, const bool& expanded)
{
    auto* self = static_cast<CompassView*>(context);
    if (self) {
        self->renderInfoExpanded(expanded);
    }
}

}  // namespace compass
