#pragma once

#include "view_models/compass_view_model.hpp"
#include "views/bottom_key_bar.hpp"
#include "views/view.hpp"
#include <lvgl/lvgl_cpp/obj.hpp>
#include <memory>

namespace compass {

class CompassDialView;
class CompassInfoView;
class MagicView;

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
    std::unique_ptr<CompassDialView> _compass_dial;
    std::unique_ptr<CompassInfoView> _info_view;
    std::unique_ptr<BottomKeyBar> _key_bar;
    std::unique_ptr<MagicView> _magic_view;
    uint32_t _magic_serial_seen = 0;

    void renderSample(const CompassSample& sample);
    void renderInfoExpanded(bool expanded);
    void renderMagic(uint32_t magic_serial);
    static void onSampleChanged(void* context, const CompassSample& sample);
    static void onInfoExpandedChanged(void* context, const bool& expanded);
    static void onMagicChanged(void* context, const uint32_t& magic_serial);
};

}  // namespace compass
