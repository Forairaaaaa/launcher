#include "core/compass_app.hpp"
#include <lvgl.h>
#include <spdlog/spdlog.h>

namespace compass {

CompassApp::CompassApp()
    : _compass_vm(_router, _compass_model),
      _calibration_vm(_router, _calibration_model, _compass_model),
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

    if (utf8 && utf8[0] == ' ') {
        onKey(' ');
        return;
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
#if LV_USE_SDL
            lv_indev_add_event_cb(indev, onKeyboardEvent, LV_EVENT_KEY, this);
#endif
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
    auto* indev = static_cast<lv_indev_t*>(lv_event_get_target(event));
    if (!self || !indev || lv_indev_get_state(indev) != LV_INDEV_STATE_PRESSED) {
        return;
    }

    const uint32_t key = lv_indev_get_key(indev);
    char utf8[2]       = {0, 0};
    if (key >= 0x20 && key < 0x7f) {
        utf8[0] = static_cast<char>(key);
    }
    self->onLvglKey(key, utf8);
}

}  // namespace compass
