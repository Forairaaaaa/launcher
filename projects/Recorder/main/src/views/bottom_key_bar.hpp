#pragma once

#include <lvgl/lvgl_cpp/image.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>

namespace recorder {

class BottomKeyBar {
public:
    struct Item {
        uint32_t key      = 0;
        const void* image = nullptr;
    };

    explicit BottomKeyBar(lv_obj_t* parent);

    void setItems(std::initializer_list<Item> items);

private:
    struct KeySlot {
        uint32_t key = 0;
        int32_t x    = 0;
        std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Image> icon;
    };

    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Image> _indicator;
    std::array<KeySlot, 5> _slots;

    KeySlot* slotFor(uint32_t key);
    void hideAllIcons();
};

}  // namespace recorder
