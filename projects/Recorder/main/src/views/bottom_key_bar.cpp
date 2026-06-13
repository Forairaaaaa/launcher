#include "views/bottom_key_bar.hpp"
#include "assets/assets.h"

namespace recorder {

namespace {

constexpr int32_t kIndicatorX = 0;
constexpr int32_t kIndicatorY = 82;
constexpr int32_t kIconY      = 64;

}  // namespace

BottomKeyBar::BottomKeyBar(lv_obj_t* parent)
    : _indicator(std::make_unique<smooth_ui_toolkit::lvgl_cpp::Image>(parent)),
      _slots{KeySlot{'4', -112, nullptr}, KeySlot{'5', -56, nullptr}, KeySlot{'6', 0, nullptr},
             KeySlot{'7', 56, nullptr}, KeySlot{'8', 112, nullptr}}
{
    _indicator->setSrc(&image_key_indicators);
    _indicator->align(LV_ALIGN_CENTER, kIndicatorX, kIndicatorY);

    for (auto& slot : _slots) {
        slot.icon = std::make_unique<smooth_ui_toolkit::lvgl_cpp::Image>(parent);
        slot.icon->align(LV_ALIGN_CENTER, slot.x, kIconY);
        slot.icon->addFlag(LV_OBJ_FLAG_HIDDEN);
    }
}

void BottomKeyBar::setItems(std::initializer_list<Item> items)
{
    hideAllIcons();

    for (const auto& item : items) {
        auto* slot = slotFor(item.key);
        if (!slot || !item.image) {
            continue;
        }

        slot->icon->setSrc(item.image);
        slot->icon->removeFlag(LV_OBJ_FLAG_HIDDEN);
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

void BottomKeyBar::hideAllIcons()
{
    for (auto& slot : _slots) {
        slot.icon->addFlag(LV_OBJ_FLAG_HIDDEN);
    }
}

}  // namespace recorder
