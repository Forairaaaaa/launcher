#include "core/recorder_app.hpp"
#include "hal_lvgl_bsp.h"
#include <lvgl.h>
#include <spdlog/spdlog.h>

namespace recorder {

namespace {

#if LV_USE_SDL
extern "C" uint32_t LV_C_EVENT_KEYBOARD;
#endif

lv_event_code_t keyboardEventCode()
{
#if LV_USE_SDL
    return static_cast<lv_event_code_t>(LV_C_EVENT_KEYBOARD);
#else
    return static_cast<lv_event_code_t>(lv_c_event[CP0_C_EVENT_KEYBOARD]);
#endif
}

}  // namespace

RecorderApp::RecorderApp()
    : _recording_vm(_router, _recording_model),
      _files_vm(_router, _files_model, _playback_model),
      _playback_vm(_router, _playback_model),
      _recording_view(_recording_vm),
      _view_models{&_recording_vm, &_files_vm, &_playback_vm},
      _views{&_recording_view, nullptr, nullptr}
{
}

RecorderApp::~RecorderApp()
{
    if (_route_observer_id != 0) {
        _router.currentPage().removeObserver(_route_observer_id);
    }
}

void RecorderApp::start()
{
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, LV_PART_MAIN);
    _route_observer_id = _router.currentPage().observe(this, onRouteChanged);
    lv_obj_add_event_cb(lv_screen_active(), onKeyboardEvent, keyboardEventCode(), this);
    setCurrentPage(_router.page());
}

void RecorderApp::onKey(uint32_t key)
{
    if (_current_vm) {
        _current_vm->onKey(key);
    }
}

void RecorderApp::tick(uint32_t nowMs)
{
    if (_current_vm) {
        _current_vm->tick(nowMs);
    }
}

ViewModel* RecorderApp::viewModelFor(PageId page)
{
    for (auto* vm : _view_models) {
        if (vm && vm->pageId() == page) {
            return vm;
        }
    }
    return nullptr;
}

View* RecorderApp::viewFor(PageId page)
{
    const auto index = static_cast<size_t>(page);
    if (index >= _views.size()) {
        return nullptr;
    }
    return _views[index];
}

void RecorderApp::setCurrentPage(PageId page)
{
    ViewModel* next = viewModelFor(page);
    View* next_view = viewFor(page);
    if (!next || (next == _current_vm && next_view == _current_view)) {
        return;
    }

    if (_current_view) {
        _current_view->onExit();
    }
    if (_current_vm) {
        _current_vm->onExit();
    }
    _current_vm   = next;
    _current_view = next_view;
    spdlog::info("Recorder route -> {}", pageIdName(page));
    _current_vm->onEnter();
    if (_current_view) {
        _current_view->onEnter(lv_screen_active());
    }
}

void RecorderApp::onRouteChanged(void* context, const PageId& page)
{
    auto* self = static_cast<RecorderApp*>(context);
    if (self) {
        self->setCurrentPage(page);
    }
}

void RecorderApp::onKeyboardEvent(lv_event_t* event)
{
    auto* self = static_cast<RecorderApp*>(lv_event_get_user_data(event));
    auto* key  = static_cast<cp0_key_event_t*>(lv_event_get_param(event));
    if (!self || !key) {
        return;
    }

    if (key->key_state != LV_INDEV_STATE_PRESSED) {
        return;
    }

    if (key->utf8[0] >= '0' && key->utf8[0] <= '9') {
        self->onKey(static_cast<uint32_t>(key->utf8[0]));
    }
}

}  // namespace recorder
