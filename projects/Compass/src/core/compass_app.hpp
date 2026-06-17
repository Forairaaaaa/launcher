#pragma once

#include "core/compass_router.hpp"
#include "models/calibration_model.hpp"
#include "models/compass_model.hpp"
#include "view_models/calibration_view_model.hpp"
#include "view_models/compass_view_model.hpp"
#include "views/calibration_view.hpp"
#include "views/compass_view.hpp"
#include "views/view.hpp"
#include <lvgl.h>
#include <array>

namespace compass {

class CompassApp {
public:
    CompassApp();
    ~CompassApp();

    CompassApp(const CompassApp&)            = delete;
    CompassApp& operator=(const CompassApp&) = delete;

    void start();
    void onKey(uint32_t key);
    void onLvglKey(uint32_t lv_key, const char* utf8);
    void tick(uint32_t nowMs);

    bool quitRequested() const
    {
        return _quit_requested;
    }

private:
    CompassRouter _router;
    CompassModel _compass_model;
    CalibrationModel _calibration_model;
    CompassViewModel _compass_vm;
    CalibrationViewModel _calibration_vm;
    CompassView _compass_view;
    CalibrationView _calibration_view;
    ViewModel* _current_vm    = nullptr;
    View* _current_view       = nullptr;
    lv_group_t* _input_group  = nullptr;
    size_t _route_observer_id = 0;
    bool _quit_requested      = false;

    std::array<ViewModel*, 2> _view_models;
    std::array<View*, 2> _views;

    ViewModel* viewModelFor(PageId page);
    View* viewFor(PageId page);
    void setupInputGroup();
    void setCurrentPage(PageId page);
    static void onRouteChanged(void* context, const PageId& page);
    static void onKeyboardEvent(lv_event_t* event);
};

}  // namespace compass
