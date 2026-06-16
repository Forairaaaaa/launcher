#include "views/compass_view.hpp"
#include "assets/assets.h"
#include <spdlog/spdlog.h>

namespace compass {

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

    _key_bar = std::make_unique<BottomKeyBar>(_root->raw_ptr());

    _view_model.infoExpanded().observe(this, onInfoExpandedChanged);
    renderInfoExpanded(_view_model.infoExpanded().get());

    spdlog::info("CompassView enter");
}

void CompassView::onExit()
{
    _view_model.infoExpanded().removeObserver();
    _key_bar.reset();
    _root.reset();
}

void CompassView::tick(uint32_t nowMs)
{
    (void)nowMs;
    if (_key_bar) {
        _key_bar->tick();
    }
}

void CompassView::renderInfoExpanded(bool expanded)
{
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

void CompassView::onInfoExpandedChanged(void* context, const bool& expanded)
{
    auto* self = static_cast<CompassView*>(context);
    if (self) {
        self->renderInfoExpanded(expanded);
    }
}

}  // namespace compass
