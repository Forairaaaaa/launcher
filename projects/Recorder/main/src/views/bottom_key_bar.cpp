#include "views/bottom_key_bar.hpp"
#include "assets/assets.h"
#include <core/animation/animate_value/animate_value.hpp>
#include <core/easing/ease.hpp>
#include <lvgl/lvgl_cpp/image.hpp>
#include <algorithm>
#include <array>
#include <cmath>

namespace recorder {

namespace {

constexpr int32_t kIndicatorX = 0;
constexpr int32_t kIndicatorY = 82;
constexpr int32_t kIconY      = 64;
constexpr float kFadeDuration = 0.16f;

lv_opa_t toOpa(float value)
{
    return static_cast<lv_opa_t>(std::clamp(static_cast<int>(std::round(value)), 0, 255));
}

void setupFadeAnimation(smooth_ui_toolkit::AnimateValue& value)
{
    value.easingOptions().duration       = kFadeDuration;
    value.easingOptions().easingFunction = smooth_ui_toolkit::ease::ease_out_quad;
}

}  // namespace

class BottomKeyBar::Indicator {
public:
    explicit Indicator(lv_obj_t* parent) : _image(std::make_unique<smooth_ui_toolkit::lvgl_cpp::Image>(parent))
    {
        _image->setSrc(&image_key_indicators);
        _image->align(LV_ALIGN_CENTER, kIndicatorX, kIndicatorY);
    }

private:
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Image> _image;
};

class BottomKeyBar::KeyIcon {
public:
    KeyIcon(lv_obj_t* parent, int32_t x)
        : _current(std::make_unique<smooth_ui_toolkit::lvgl_cpp::Image>(parent)),
          _previous(std::make_unique<smooth_ui_toolkit::lvgl_cpp::Image>(parent)),
          _current_opa(0),
          _previous_opa(0)
    {
        setupFadeAnimation(_current_opa);
        setupFadeAnimation(_previous_opa);

        _current->align(LV_ALIGN_CENTER, x, kIconY);
        _previous->align(LV_ALIGN_CENTER, x, kIconY);
        _current->addFlag(LV_OBJ_FLAG_HIDDEN);
        _previous->addFlag(LV_OBJ_FLAG_HIDDEN);
    }

    void setImage(const void* image)
    {
        if (image == _image) {
            return;
        }

        _current_opa.update();
        _previous_opa.update();
        applyOpacity();

        const void* old_image = _image;
        _image                = image;

        if (old_image) {
            _previous->setSrc(old_image);
            _previous->removeFlag(LV_OBJ_FLAG_HIDDEN);
            _previous_opa.teleport(_current_opa.directValue());
            _previous_opa.move(0);
        }

        if (_image) {
            _current->setSrc(_image);
            _current->removeFlag(LV_OBJ_FLAG_HIDDEN);
            _current_opa.teleport(0);
            _current_opa.move(255);
        } else {
            _current->addFlag(LV_OBJ_FLAG_HIDDEN);
            _current_opa.teleport(0);
        }
    }

    void tick()
    {
        _current_opa.update();
        _previous_opa.update();
        applyOpacity();

        if (!_image) {
            _current->addFlag(LV_OBJ_FLAG_HIDDEN);
        }
        if (_previous_opa.done() && _previous_opa.directValue() <= 0.0f) {
            _previous->addFlag(LV_OBJ_FLAG_HIDDEN);
        }
    }

private:
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Image> _current;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Image> _previous;
    smooth_ui_toolkit::AnimateValue _current_opa;
    smooth_ui_toolkit::AnimateValue _previous_opa;
    const void* _image = nullptr;

    void applyOpacity()
    {
        _current->setOpa(toOpa(_current_opa.directValue()));
        _previous->setOpa(toOpa(_previous_opa.directValue()));
    }
};

BottomKeyBar::BottomKeyBar(lv_obj_t* parent)
    : _indicator(std::make_unique<Indicator>(parent)),
      _slots{KeySlot{'4', -112, nullptr}, KeySlot{'5', -56, nullptr}, KeySlot{'6', 0, nullptr},
             KeySlot{'7', 56, nullptr}, KeySlot{'8', 112, nullptr}}
{
    for (auto& slot : _slots) {
        slot.icon = std::make_unique<KeyIcon>(parent, slot.x);
    }
}

BottomKeyBar::~BottomKeyBar() = default;

void BottomKeyBar::setItems(std::initializer_list<Item> items)
{
    std::array<const void*, 5> images{};

    for (const auto& item : items) {
        auto* slot = slotFor(item.key);
        if (!slot) {
            continue;
        }

        const auto index = static_cast<size_t>(slot - _slots.data());
        images[index]    = item.image;
    }

    for (size_t i = 0; i < _slots.size(); ++i) {
        _slots[i].icon->setImage(images[i]);
    }
}

void BottomKeyBar::tick()
{
    for (auto& slot : _slots) {
        slot.icon->tick();
    }
}

BottomKeyBar::KeySlot* BottomKeyBar::slotFor(uint32_t key)
{
    for (auto& slot : _slots) {
        if (slot.key == key) {
            return &slot;
        }
    }
    return nullptr;
}

}  // namespace recorder
