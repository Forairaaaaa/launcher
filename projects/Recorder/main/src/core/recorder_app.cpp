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

} // namespace

RecorderApp::RecorderApp()
    : _recording_vm(_router, _recording_model),
      _files_vm(_router, _files_model, _playback_model),
      _playback_vm(_router, _playback_model),
      _view_models {&_recording_vm, &_files_vm, &_playback_vm}
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

void RecorderApp::setCurrentPage(PageId page)
{
    ViewModel* next = viewModelFor(page);
    if (!next || next == _current_vm) {
        return;
    }

    if (_current_vm) {
        _current_vm->onExit();
    }
    _current_vm = next;
    spdlog::info("Recorder route -> {}", pageIdName(page));
    _current_vm->onEnter();
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
    auto* key = static_cast<cp0_key_event_t*>(lv_event_get_param(event));
    if (!self || !key) {
        return;
    }

    if (key->utf8[0] >= '0' && key->utf8[0] <= '9') {
        self->onKey(static_cast<uint32_t>(key->utf8[0]));
    }
}

} // namespace recorder
