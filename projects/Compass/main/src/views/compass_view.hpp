#pragma once

#include "view_models/compass_view_model.hpp"
#include "views/bottom_key_bar.hpp"
#include "views/view.hpp"
#include <lvgl/lvgl_cpp/obj.hpp>
#include <memory>

namespace compass {

class CompassView : public View {
public:
    explicit CompassView(CompassViewModel& view_model);
    ~CompassView() override;

    void onEnter(lv_obj_t* parent) override;
    void onExit() override;
    void tick(uint32_t nowMs) override;

private:
    CompassViewModel& _view_model;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Container> _root;
    std::unique_ptr<BottomKeyBar> _key_bar;

    void renderInfoExpanded(bool expanded);
    static void onInfoExpandedChanged(void* context, const bool& expanded);
};

}  // namespace compass
