#include "views/recording_view.hpp"
#include "assets/assets.h"

namespace recorder {

RecordingView::RecordingView(RecordingViewModel& view_model) : _view_model(view_model)
{
}

RecordingView::~RecordingView()
{
    onExit();
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

    _key_bar           = std::make_unique<BottomKeyBar>(_root->raw_ptr());
    _state_observer_id = _view_model.state().observe(this, onStateChanged);
}

void RecordingView::onExit()
{
    if (_state_observer_id != 0) {
        _view_model.state().removeObserver(_state_observer_id);
        _state_observer_id = 0;
    }

    _key_bar.reset();
    _root.reset();
}

void RecordingView::renderState(RecordingState state)
{
    if (!_key_bar) {
        return;
    }

    switch (state) {
        case RecordingState::Idle:
            _key_bar->setItems({
                {'6', &image_icon_record},
                {'8', &image_icon_list},
            });
            break;
        case RecordingState::Recording:
            _key_bar->setItems({
                {'5', &image_icon_pause},
                {'6', &image_icon_stop},
                {'8', &image_icon_list},
            });
            break;
        case RecordingState::Paused:
            _key_bar->setItems({
                {'5', &image_icon_record},
                {'6', &image_icon_stop},
                {'8', &image_icon_list},
            });
            break;
    }
}

void RecordingView::onStateChanged(void* context, const RecordingState& state)
{
    auto* self = static_cast<RecordingView*>(context);
    if (self) {
        self->renderState(state);
    }
}

}  // namespace recorder
