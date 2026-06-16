#include "views/compass_view.hpp"
#include "assets/assets.h"
#include <core/animation/animate_value/animate_value.hpp>
#include <core/easing/ease.hpp>
#include <lvgl/lvgl_cpp/image.hpp>
#include <lvgl/lvgl_cpp/label.hpp>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>

namespace compass {

namespace {

constexpr int32_t kCompassPanelSize          = 150;
constexpr int32_t kCompassExpandedX          = -68;
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
constexpr int32_t kInfoPanelX                = 184;
constexpr int32_t kInfoPanelHiddenOffsetX    = 150;
constexpr int32_t kInfoPanelWidth            = 124;
constexpr int32_t kInfoPanelHeight           = 38;
constexpr int32_t kInfoSidebarWidth          = 3;
constexpr int32_t kInfoSidebarRadius         = 1;
constexpr int32_t kInfoLabelX                = 10;
constexpr int32_t kInfoLabelWidth            = 54;
constexpr int32_t kInfoLabelHeight           = 12;
constexpr int32_t kInfoBarX                  = 67;
constexpr int32_t kInfoBarMaxWidth           = 56;
constexpr int32_t kInfoBarHeight             = 6;
constexpr int32_t kInfoBarRadius             = 1;
constexpr uint32_t kInfoSidebarColor         = 0x4C4C4C;
constexpr uint32_t kInfoTextColor            = 0xF2F2F2;
constexpr uint32_t kInfoPositiveBarColor     = 0x53D671;
constexpr uint32_t kInfoNegativeBarColor     = 0xFED40D;
constexpr float kInfoMoveDuration            = 0.5f;
constexpr float kInfoMoveBounce              = 0.3f;
constexpr float kInfoFadeDuration            = 0.8f;
constexpr float kInfoFadeDelayStep           = 0.08f;
constexpr std::array<int32_t, 3> kInfoPanelY = {11, 52, 94};
constexpr std::array<int32_t, 3> kInfoRowY   = {0, 14, 28};
constexpr float kAccelRange                  = 10.0f;
constexpr float kGyroRange                   = 1.0f;
constexpr float kMagRange                    = 50.0f;

float clampNormalized(float value)
{
    return std::clamp(value, -1.0f, 1.0f);
}

float clampZeroToOne(float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}

lv_opa_t toOpa(float value)
{
    return static_cast<lv_opa_t>(std::clamp(static_cast<int>(std::round(value)), 0, 255));
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

class CompassInfoPanel {
public:
    CompassInfoPanel(lv_obj_t* parent, int32_t y, std::array<const char*, 3> labels, float range)
        : _y(y), _labels(labels), _range(range)
    {
        _opacity.easingOptions().duration       = kInfoFadeDuration;
        _opacity.easingOptions().easingFunction = smooth_ui_toolkit::ease::ease_out_quad;
        _opacity.teleport(0.0f);

        _panel = std::make_unique<smooth_ui_toolkit::lvgl_cpp::Container>(parent);
        _panel->setSize(kInfoPanelWidth, kInfoPanelHeight);
        _panel->setBgOpa(LV_OPA_TRANSP);
        _panel->setBorderWidth(0);
        _panel->setOutlineWidth(0);
        _panel->setShadowWidth(0);
        _panel->setPaddingAll(0);
        _panel->removeFlag(LV_OBJ_FLAG_SCROLLABLE);
        _panel->addFlag(LV_OBJ_FLAG_HIDDEN);

        _sidebar = std::make_unique<smooth_ui_toolkit::lvgl_cpp::Container>(_panel->raw_ptr());
        _sidebar->setSize(kInfoSidebarWidth, kInfoPanelHeight);
        _sidebar->setPos(0, 0);
        _sidebar->setBgColor(lv_color_hex(kInfoSidebarColor));
        _sidebar->setBgOpa(LV_OPA_COVER);
        _sidebar->setRadius(kInfoSidebarRadius);
        _sidebar->setBorderWidth(0);
        _sidebar->setOutlineWidth(0);
        _sidebar->setShadowWidth(0);
        _sidebar->setPaddingAll(0);
        _sidebar->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

        for (size_t i = 0; i < _value_labels.size(); ++i) {
            _value_labels[i] = std::make_unique<smooth_ui_toolkit::lvgl_cpp::Label>(_panel->raw_ptr());
            _value_labels[i]->setTextFont(&font_chivo_mono_medium_12);
            _value_labels[i]->setTextColor(lv_color_hex(kInfoTextColor));
            _value_labels[i]->setTextAlign(LV_TEXT_ALIGN_LEFT);
            _value_labels[i]->setSize(kInfoLabelWidth, kInfoLabelHeight);
            _value_labels[i]->setPos(kInfoLabelX, kInfoRowY[i] - 1);
            _value_labels[i]->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

            _bars[i] = std::make_unique<smooth_ui_toolkit::lvgl_cpp::Container>(_panel->raw_ptr());
            _bars[i]->setSize(0, kInfoBarHeight);
            _bars[i]->setPos(kInfoBarX, kInfoRowY[i] + 2);
            _bars[i]->setBgOpa(LV_OPA_COVER);
            _bars[i]->setRadius(kInfoBarRadius);
            _bars[i]->setBorderWidth(0);
            _bars[i]->setOutlineWidth(0);
            _bars[i]->setShadowWidth(0);
            _bars[i]->setPaddingAll(0);
            _bars[i]->removeFlag(LV_OBJ_FLAG_SCROLLABLE);
        }

        setValues(Axis3{});
        setX(kInfoPanelX + kInfoPanelHiddenOffsetX);
        applyOpacity();
    }

    void setX(int32_t x)
    {
        if (_panel) {
            _panel->setPos(x, _y);
        }
    }

    void setExpanded(bool expanded, float delay)
    {
        if (expanded == _expanded) {
            return;
        }

        _opacity.update();
        _expanded      = expanded;
        _opacity.delay = expanded ? delay : 0.0f;
        if (_expanded) {
            _panel->removeFlag(LV_OBJ_FLAG_HIDDEN);
            _opacity.move(255.0f);
        } else {
            _opacity.move(0.0f);
        }
        applyOpacity();
    }

    void setValues(const Axis3& values)
    {
        _values = values;
        applyValues();
    }

    void tick()
    {
        _opacity.update();
        applyOpacity();

        if (!_expanded && _opacity.done() && _opacity.directValue() <= 0.0f) {
            _panel->addFlag(LV_OBJ_FLAG_HIDDEN);
        }
    }

private:
    int32_t _y = 0;
    std::array<const char*, 3> _labels{};
    float _range = 1.0f;
    Axis3 _values;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Container> _panel;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Container> _sidebar;
    std::array<std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Label>, 3> _value_labels;
    std::array<std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Container>, 3> _bars;
    smooth_ui_toolkit::AnimateValue _opacity{0};
    bool _expanded = false;

    float valueAt(size_t index) const
    {
        switch (index) {
            case 0:
                return _values.x;
            case 1:
                return _values.y;
            default:
                return _values.z;
        }
    }

    void applyValues()
    {
        for (size_t i = 0; i < _value_labels.size(); ++i) {
            const float value = valueAt(i);

            char buffer[16] = {};
            std::snprintf(buffer, sizeof(buffer), "%s:%+.1f", _labels[i], value);
            _value_labels[i]->setText(buffer);

            const float ratio = _range <= 0.0f ? 0.0f : std::min(std::abs(value) / _range, 1.0f);
            _bars[i]->setWidth(static_cast<int32_t>(std::round(ratio * kInfoBarMaxWidth)));
            _bars[i]->setBgColor(lv_color_hex(value >= 0.0f ? kInfoPositiveBarColor : kInfoNegativeBarColor));
        }
    }

    void applyOpacity()
    {
        if (_panel) {
            _panel->setOpa(toOpa(_opacity.directValue()));
        }
    }
};

class CompassInfoView {
public:
    explicit CompassInfoView(lv_obj_t* parent)
    {
        _x.springOptions().visualDuration = kInfoMoveDuration;
        _x.springOptions().bounce         = kInfoMoveBounce;
        _x.teleport(kInfoPanelHiddenOffsetX);

        _accel_panel = std::make_unique<CompassInfoPanel>(parent, kInfoPanelY[0],
                                                          std::array<const char*, 3>{"AX", "AY", "AZ"}, kAccelRange);
        _gyro_panel  = std::make_unique<CompassInfoPanel>(parent, kInfoPanelY[1],
                                                          std::array<const char*, 3>{"GX", "GY", "GZ"}, kGyroRange);
        _mag_panel   = std::make_unique<CompassInfoPanel>(parent, kInfoPanelY[2],
                                                          std::array<const char*, 3>{"MX", "MY", "MZ"}, kMagRange);
        applyX();
    }

    void setExpanded(bool expanded)
    {
        if (expanded == _expanded) {
            return;
        }

        _expanded = expanded;
        _x.move(_expanded ? 0.0f : static_cast<float>(kInfoPanelHiddenOffsetX));
        _accel_panel->setExpanded(_expanded, 0.0f);
        _gyro_panel->setExpanded(_expanded, kInfoFadeDelayStep);
        _mag_panel->setExpanded(_expanded, kInfoFadeDelayStep * 2.0f);
    }

    void setSample(const CompassSample& sample)
    {
        _accel_panel->setValues(sample.accel);
        _gyro_panel->setValues(sample.gyro);
        _mag_panel->setValues(sample.mag);
    }

    void tick()
    {
        _x.update();
        applyX();
        _accel_panel->tick();
        _gyro_panel->tick();
        _mag_panel->tick();
    }

private:
    std::unique_ptr<CompassInfoPanel> _accel_panel;
    std::unique_ptr<CompassInfoPanel> _gyro_panel;
    std::unique_ptr<CompassInfoPanel> _mag_panel;
    smooth_ui_toolkit::AnimateValue _x{kInfoPanelHiddenOffsetX};
    bool _expanded = false;

    void applyX()
    {
        const int32_t x = kInfoPanelX + static_cast<int32_t>(std::round(_x.directValue()));
        _accel_panel->setX(x);
        _gyro_panel->setX(x);
        _mag_panel->setX(x);
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
    _info_view    = std::make_unique<CompassInfoView>(_root->raw_ptr());
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
    _info_view.reset();
    _compass_dial.reset();
    _root.reset();
}

void CompassView::tick(uint32_t nowMs)
{
    (void)nowMs;
    if (_compass_dial) {
        _compass_dial->tick();
    }
    if (_info_view) {
        _info_view->tick();
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
    if (_info_view) {
        _info_view->setSample(sample);
    }
}

void CompassView::renderInfoExpanded(bool expanded)
{
    if (_compass_dial) {
        _compass_dial->setExpanded(expanded);
    }
    if (_info_view) {
        _info_view->setExpanded(expanded);
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
