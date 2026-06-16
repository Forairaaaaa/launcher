#include "core/compass_app.hpp"
#include "hal_lvgl_bsp.h"
#include <lvgl.h>
#include <spdlog/spdlog.h>

namespace compass {

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

CompassApp::CompassApp()
    : _compass_vm(_router, _compass_model),
      _calibration_vm(_router, _calibration_model),
      _compass_view(_compass_vm),
      _calibration_view(_calibration_vm),
      _view_models{&_compass_vm, &_calibration_vm},
      _views{&_compass_view, &_calibration_view}
{
}

CompassApp::~CompassApp()
{
    if (_route_observer_id != 0) {
        _router.currentPage().removeObserver(_route_observer_id);
    }
    if (_input_group) {
        lv_group_del(_input_group);
        _input_group = nullptr;
    }
}

void CompassApp::start()
{
    spdlog::info("CompassApp start");
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, LV_PART_MAIN);
    setupInputGroup();
    _route_observer_id = _router.currentPage().observe(this, onRouteChanged);
#if LV_USE_SDL
    lv_obj_add_event_cb(lv_screen_active(), onKeyboardEvent, keyboardEventCode(), this);
#endif
    setCurrentPage(_router.page());
}

void CompassApp::onKey(uint32_t key)
{
    if (key == '\x1b' && _router.page() == PageId::Compass) {
        spdlog::info("CompassApp: quit requested");
        _quit_requested = true;
        return;
    }

    if (_current_vm) {
        _current_vm->onKey(key);
    }
}

void CompassApp::onLvglKey(uint32_t lv_key, const char* utf8)
{
    if (lv_key == LV_KEY_ESC) {
        onKey('\x1b');
        return;
    }

    if (lv_key == LV_KEY_ENTER) {
        onKey('\r');
        return;
    }

    switch (lv_key) {
        case LV_KEY_UP:
            onKey(compass_key::Up);
            return;
        case LV_KEY_DOWN:
            onKey(compass_key::Down);
            return;
        default:
            break;
    }

    if (utf8 && utf8[0] >= '0' && utf8[0] <= '9') {
        onKey(static_cast<uint32_t>(utf8[0]));
    }
}

void CompassApp::tick(uint32_t nowMs)
{
    if (_current_vm) {
        _current_vm->tick(nowMs);
    }
    if (_current_view) {
        _current_view->tick(nowMs);
    }
}

ViewModel* CompassApp::viewModelFor(PageId page)
{
    for (auto* vm : _view_models) {
        if (vm && vm->pageId() == page) {
            return vm;
        }
    }
    return nullptr;
}

View* CompassApp::viewFor(PageId page)
{
    const auto index = static_cast<size_t>(page);
    if (index >= _views.size()) {
        return nullptr;
    }
    return _views[index];
}

void CompassApp::setupInputGroup()
{
    if (_input_group) {
        return;
    }

    _input_group = lv_group_create();

    lv_indev_t* indev = lv_indev_get_next(nullptr);
    while (indev) {
        if (lv_indev_get_type(indev) == LV_INDEV_TYPE_KEYPAD) {
            lv_indev_set_group(indev, _input_group);
        }
        indev = lv_indev_get_next(indev);
    }
}

void CompassApp::setCurrentPage(PageId page)
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
    spdlog::info("Compass route -> {}", pageIdName(page));
    _current_vm->onEnter();
    if (_current_view) {
        _current_view->onEnter(lv_screen_active());
    }
}

void CompassApp::onRouteChanged(void* context, const PageId& page)
{
    auto* self = static_cast<CompassApp*>(context);
    if (self) {
        self->setCurrentPage(page);
    }
}

void CompassApp::onKeyboardEvent(lv_event_t* event)
{
    auto* self = static_cast<CompassApp*>(lv_event_get_user_data(event));
    auto* key  = static_cast<cp0_key_event_t*>(lv_event_get_param(event));
    if (!self || !key) {
        return;
    }

    if (key->key_state != LV_INDEV_STATE_PRESSED) {
        return;
    }

    self->onLvglKey(key->lv_key, key->utf8);
}

}  // namespace compass
