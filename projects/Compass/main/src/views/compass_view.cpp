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
#include <random>

namespace compass {

namespace {

constexpr int32_t kCompassPanelSize                  = 170;
constexpr int32_t kCompassExpandedX                  = -68;
constexpr int32_t kDirectionLabelRadius              = 50;
constexpr int32_t kDirectionLabelWidth               = 14;
constexpr int32_t kDirectionLabelHeight              = 12;
constexpr int32_t kLargeCrossSize                    = 72;
constexpr int32_t kSmallCrossSize                    = 21;
constexpr int32_t kCrossThickness                    = 1;
constexpr int32_t kBubbleMinSize                     = 43;
constexpr int32_t kBubbleMaxSize                     = 53;
constexpr int32_t kBubbleTravel                      = 26;
constexpr uint32_t kCrossColor                       = 0x5D5D5D;
constexpr uint32_t kDirectionLabelColor              = 0xFFFFFF;
constexpr uint32_t kBubbleColor                      = 0x303030;
constexpr float kGravityMetersPerSecond              = 9.81f;
constexpr float kCompassUnavailableHeading           = 0.0f;
constexpr float kCompassUnavailableBubblePos         = 0.0f;
constexpr float kCompassUnavailableBubbleZ           = 1.0f;
constexpr int32_t kInfoPanelX                        = 184;
constexpr int32_t kInfoPanelHiddenOffsetX            = 150;
constexpr int32_t kInfoPanelWidth                    = 124;
constexpr int32_t kInfoPanelHeight                   = 38;
constexpr int32_t kInfoSidebarWidth                  = 3;
constexpr int32_t kInfoSidebarRadius                 = 1;
constexpr int32_t kInfoLabelX                        = 10;
constexpr int32_t kInfoLabelWidth                    = 54;
constexpr int32_t kInfoLabelHeight                   = 12;
constexpr int32_t kInfoBarX                          = 67;
constexpr int32_t kInfoBarMaxWidth                   = 56;
constexpr int32_t kInfoBarHeight                     = 6;
constexpr int32_t kInfoBarRadius                     = 1;
constexpr uint32_t kInfoSidebarColor                 = 0x4C4C4C;
constexpr uint32_t kInfoTextColor                    = 0xF2F2F2;
constexpr uint32_t kInfoPositiveBarColor             = 0x53D671;
constexpr uint32_t kInfoNegativeBarColor             = 0xFED40D;
constexpr std::array<const char*, 4> kDirectionTexts = {"N", "E", "S", "W"};
constexpr std::array<float, 4> kDirectionAngles      = {0.0f, 90.0f, 180.0f, 270.0f};
constexpr std::array<int32_t, 3> kInfoPanelY         = {11, 52, 94};
constexpr std::array<int32_t, 3> kInfoRowY           = {0, 14, 28};
constexpr float kAccelRange                          = 10.0f;
constexpr float kGyroRange                           = 1.0f;
constexpr float kMagRange                            = 50.0f;
constexpr int32_t kMagicPlaneSize                    = 18;
constexpr int32_t kMagicFireSize                     = 6;
constexpr size_t kMagicFireCount                     = 32;
constexpr size_t kMagicPathPointCount                = 7;
constexpr uint32_t kMagicMinDurationMs               = 2800;
constexpr uint32_t kMagicMaxDurationMs               = 4000;
constexpr uint32_t kMagicFireCleanupMs               = 3200;

struct MagicPoint {
    float x = 0.0f;
    float y = 0.0f;
};

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

        for (size_t i = 0; i < _direction_labels.size(); ++i) {
            _direction_labels[i] = std::make_unique<smooth_ui_toolkit::lvgl_cpp::Label>(_panel->raw_ptr());
            _direction_labels[i]->setText(kDirectionTexts[i]);
            _direction_labels[i]->setTextFont(&font_chivo_mono_medium_12);
            _direction_labels[i]->setTextColor(lv_color_hex(kDirectionLabelColor));
            _direction_labels[i]->setTextAlign(LV_TEXT_ALIGN_CENTER);
            _direction_labels[i]->setSize(kDirectionLabelWidth, kDirectionLabelHeight);
            _direction_labels[i]->removeFlag(LV_OBJ_FLAG_SCROLLABLE);
        }

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
    std::array<std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Label>, 4> _direction_labels;
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
        applyDirectionLabels(heading);

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

    void applyDirectionLabels(float heading)
    {
        constexpr float kDegreesToRadians = 3.14159265358979323846f / 180.0f;

        for (size_t i = 0; i < _direction_labels.size(); ++i) {
            if (!_direction_labels[i]) {
                continue;
            }

            const float angle = (kDirectionAngles[i] - heading) * kDegreesToRadians;
            const int32_t x   = static_cast<int32_t>(std::round(std::sin(angle) * kDirectionLabelRadius));
            const int32_t y   = static_cast<int32_t>(std::round(-std::cos(angle) * kDirectionLabelRadius));
            _direction_labels[i]->align(LV_ALIGN_CENTER, x, y);
        }
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
        _panel = std::make_unique<smooth_ui_toolkit::lvgl_cpp::Container>(parent);
        _panel->setSize(kInfoPanelWidth, kInfoPanelHeight);
        _panel->setBgOpa(LV_OPA_TRANSP);
        _panel->setBorderWidth(0);
        _panel->setOutlineWidth(0);
        _panel->setShadowWidth(0);
        _panel->setPaddingAll(0);
        _panel->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

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
    }

    void setX(int32_t x)
    {
        if (_panel) {
            _panel->setPos(x, _y);
        }
    }

    void setValues(const Axis3& values)
    {
        _values = values;
        applyValues();
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
            if (std::abs(value) < 10.0f) {
                std::snprintf(buffer, sizeof(buffer), "%s:%+.1f", _labels[i], value);
            } else {
                std::snprintf(buffer, sizeof(buffer), "%s:%+.0f", _labels[i], value);
            }
            _value_labels[i]->setText(buffer);

            const float ratio = _range <= 0.0f ? 0.0f : std::min(std::abs(value) / _range, 1.0f);
            _bars[i]->setWidth(static_cast<int32_t>(std::round(ratio * kInfoBarMaxWidth)));
            _bars[i]->setBgColor(lv_color_hex(value >= 0.0f ? kInfoPositiveBarColor : kInfoNegativeBarColor));
        }
    }
};

class CompassInfoView {
public:
    explicit CompassInfoView(lv_obj_t* parent)
    {
        _x.springOptions().visualDuration = 0.5;
        _x.springOptions().bounce         = 0.3;
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

class MagicView {
public:
    explicit MagicView(lv_obj_t* parent) : _parent(parent)
    {
        _plane = std::make_unique<smooth_ui_toolkit::lvgl_cpp::Image>(parent);
        _plane->setSrc(&image_magic);
        _plane->setPivot(kMagicPlaneSize / 2, kMagicPlaneSize / 2);
        _plane->setHidden(true);
        _plane->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

        for (auto& fire : _fires) {
            fire.image = std::make_unique<smooth_ui_toolkit::lvgl_cpp::Image>(parent);
            fire.image->setSrc(&image_magic_fire);
            fire.image->setPivot(kMagicFireSize / 2, kMagicFireSize / 2);
            fire.image->setHidden(true);
            fire.image->removeFlag(LV_OBJ_FLAG_SCROLLABLE);
        }
    }

    void generate(uint32_t magic_serial)
    {
        if (magic_serial == 0 || !_parent || !_plane) {
            return;
        }

        const int32_t width  = parentWidth();
        const int32_t height = parentHeight();
        std::mt19937 rng(0xC0FFEEu ^ (magic_serial * 0x9E3779B9u));
        const int32_t left_max_x = std::max(62, width / 2 - 10);
        std::uniform_int_distribution<int32_t> x_dist(24, left_max_x);
        std::uniform_int_distribution<int32_t> y_dist(0, std::max(0, height));
        std::uniform_int_distribution<int32_t> edge_dist(0, 2);
        std::uniform_int_distribution<uint32_t> duration_dist(kMagicMinDurationMs, kMagicMaxDurationMs);
        std::uniform_real_distribution<float> fire_drift_dist(-18.0f, 18.0f);
        std::uniform_real_distribution<float> fire_speed_dist(145.0f, 220.0f);

        _duration_ms                     = duration_dist(rng);
        _start_ms                        = lv_tick_get();
        _active                          = true;
        const uint32_t latest_fire_start = _duration_ms > 520 ? _duration_ms - 520 : 180;
        std::uniform_int_distribution<uint32_t> fire_time_dist(180, std::max<uint32_t>(180, latest_fire_start));

        const int32_t margin = 28;
        _path_points[0]      = offscreenPoint(edge_dist(rng), height, left_max_x, margin, rng);
        for (size_t i = 1; i + 1 < _path_points.size(); ++i) {
            _path_points[i] = MagicPoint{static_cast<float>(x_dist(rng)), static_cast<float>(y_dist(rng))};
        }
        _path_points[_path_points.size() - 1] = offscreenPoint(edge_dist(rng), height, left_max_x, margin, rng);

        _plane->setRotation(0);
        _plane->moveForeground();
        _plane->setHidden(false);

        for (size_t i = 0; i < _fires.size(); ++i) {
            auto& fire    = _fires[i];
            fire.start_ms = fire_time_dist(rng) + static_cast<uint32_t>((i % 3) * 28);
            fire.origin_t =
                std::clamp(static_cast<float>(fire.start_ms) / static_cast<float>(_duration_ms), 0.0f, 1.0f);
            const MagicPoint origin = pointAt(fire.origin_t);
            fire.origin_x           = origin.x + kMagicPlaneSize * 0.45f;
            fire.origin_y           = origin.y;
            fire.vel_x              = fire_speed_dist(rng);
            fire.vel_y              = fire_drift_dist(rng);
            fire.image->moveForeground();
            fire.image->setHidden(true);
        }

        applyState(0);
    }

    void tick(uint32_t nowMs)
    {
        if (!_active) {
            return;
        }

        const uint32_t elapsed = nowMs >= _start_ms ? nowMs - _start_ms : 0;
        if (elapsed >= _duration_ms + kMagicFireCleanupMs) {
            hide();
            return;
        }

        applyState(elapsed);
    }

private:
    struct Fire {
        std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Image> image;
        uint32_t start_ms = 0;
        float origin_t    = 0.0f;
        float origin_x    = 0.0f;
        float origin_y    = 0.0f;
        float vel_x       = 0.0f;
        float vel_y       = 0.0f;
    };

    lv_obj_t* _parent = nullptr;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Image> _plane;
    std::array<Fire, kMagicFireCount> _fires;
    std::array<MagicPoint, kMagicPathPointCount> _path_points;
    uint32_t _start_ms    = 0;
    uint32_t _duration_ms = kMagicMinDurationMs;
    bool _active          = false;

    int32_t parentWidth() const
    {
        lv_obj_update_layout(_parent);
        const int32_t width = lv_obj_get_width(_parent);
        if (width > 0) {
            return width;
        }
        auto* display = lv_display_get_default();
        return display ? lv_display_get_horizontal_resolution(display) : 320;
    }

    int32_t parentHeight() const
    {
        lv_obj_update_layout(_parent);
        const int32_t height = lv_obj_get_height(_parent);
        if (height > 0) {
            return height;
        }
        auto* display = lv_display_get_default();
        return display ? lv_display_get_vertical_resolution(display) : 240;
    }

    static float lerp(float from, float to, float t)
    {
        return from + (to - from) * t;
    }

    static MagicPoint offscreenPoint(int32_t edge, int32_t height, int32_t left_max_x, int32_t margin,
                                     std::mt19937& rng)
    {
        std::uniform_int_distribution<int32_t> x_dist(24, left_max_x);
        std::uniform_int_distribution<int32_t> y_dist(0, std::max(0, height));

        switch (edge) {
            case 1:
                return MagicPoint{static_cast<float>(x_dist(rng)), static_cast<float>(-margin)};
            case 2:
                return MagicPoint{static_cast<float>(x_dist(rng)), static_cast<float>(height + margin)};
            default:
                return MagicPoint{static_cast<float>(-margin), static_cast<float>(y_dist(rng))};
        }
    }

    MagicPoint pointAt(float t) const
    {
        t                    = std::clamp(t, 0.0f, 1.0f);
        const float scaled_t = t * static_cast<float>(_path_points.size() - 1);
        const size_t index   = std::min(static_cast<size_t>(scaled_t), _path_points.size() - 2);
        const float local_t  = scaled_t - static_cast<float>(index);
        const float smooth_t = local_t * local_t * (3.0f - 2.0f * local_t);
        return MagicPoint{
            lerp(_path_points[index].x, _path_points[index + 1].x, smooth_t),
            lerp(_path_points[index].y, _path_points[index + 1].y, smooth_t),
        };
    }

    void applyState(uint32_t elapsed)
    {
        const int32_t width  = parentWidth();
        const int32_t height = parentHeight();
        const int32_t margin = 16;
        const float t        = std::clamp(static_cast<float>(elapsed) / static_cast<float>(_duration_ms), 0.0f, 1.0f);
        const MagicPoint plane_pos = pointAt(t);

        if (elapsed <= _duration_ms) {
            _plane->setHidden(false);
            _plane->setPos(static_cast<int32_t>(std::round(plane_pos.x - kMagicPlaneSize / 2.0f)),
                           static_cast<int32_t>(std::round(plane_pos.y - kMagicPlaneSize / 2.0f)));
        } else {
            _plane->setHidden(true);
        }

        for (auto& fire : _fires) {
            if (!fire.image || elapsed < fire.start_ms) {
                if (fire.image) {
                    fire.image->setHidden(true);
                }
                continue;
            }

            const float fire_t = static_cast<float>(elapsed - fire.start_ms) / 1000.0f;
            const float fire_x = fire.origin_x + fire.vel_x * fire_t;
            const float fire_y = fire.origin_y + fire.vel_y * fire_t;
            if (fire_x > width + margin || fire_y < -margin || fire_y > height + margin) {
                fire.image->setHidden(true);
                continue;
            }

            fire.image->setHidden(false);
            fire.image->setPos(static_cast<int32_t>(std::round(fire_x - kMagicFireSize / 2.0f)),
                               static_cast<int32_t>(std::round(fire_y - kMagicFireSize / 2.0f)));
        }
    }

    void hide()
    {
        _active = false;
        if (_plane) {
            _plane->setHidden(true);
        }
        for (auto& fire : _fires) {
            if (fire.image) {
                fire.image->setHidden(true);
            }
        }
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
    _magic_view   = std::make_unique<MagicView>(_root->raw_ptr());

    _magic_serial_seen = _view_model.magic().get();
    _view_model.sample().observe(this, onSampleChanged);
    _view_model.infoExpanded().observe(this, onInfoExpandedChanged);
    _view_model.magic().observe(this, onMagicChanged);
    renderSample(_view_model.sample().get());
    renderInfoExpanded(_view_model.infoExpanded().get());

    spdlog::info("CompassView enter");
}

void CompassView::onExit()
{
    _view_model.magic().removeObserver();
    _view_model.infoExpanded().removeObserver();
    _view_model.sample().removeObserver();
    _magic_view.reset();
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
    if (_magic_view) {
        _magic_view->tick(nowMs);
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

void CompassView::renderMagic(uint32_t magic_serial)
{
    if (magic_serial == 0 || magic_serial == _magic_serial_seen) {
        return;
    }

    _magic_serial_seen = magic_serial;
    if (_magic_view) {
        _magic_view->generate(magic_serial);
    }
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

void CompassView::onMagicChanged(void* context, const uint32_t& magic_serial)
{
    auto* self = static_cast<CompassView*>(context);
    if (self) {
        self->renderMagic(magic_serial);
    }
}

}  // namespace compass
