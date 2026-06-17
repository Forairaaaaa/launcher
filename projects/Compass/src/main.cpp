#include "core/compass_app.hpp"
#include "hal/compass_lvgl_hal.hpp"
#include "input/compass_keypad.hpp"
#include <core/hal/hal.hpp>
#include <lvgl.h>
#include <spdlog/spdlog.h>
#include <stdio.h>
#include <unistd.h>

int main()
{
    constexpr int32_t kScreenWidth  = 320;
    constexpr int32_t kScreenHeight = 170;

    lv_init();
    if (!compass::initLvglHal(kScreenWidth, kScreenHeight)) {
        return 1;
    }

    lv_display_t* disp = lv_display_get_default();
    if (disp == nullptr) {
        fprintf(stderr, "Compass: failed to create LVGL display\n");
        return 1;
    }

    spdlog::info("Compass: display {}x{}", static_cast<int>(lv_display_get_horizontal_resolution(disp)),
                 static_cast<int>(lv_display_get_vertical_resolution(disp)));

    smooth_ui_toolkit::ui_hal::on_get_tick([]() { return lv_tick_get(); });
    smooth_ui_toolkit::ui_hal::on_delay([](uint32_t ms) { usleep(ms * 1000); });

    compass::CompassApp app;

#if !LV_USE_SDL
    compass::CompassKeypad keypad;
    keypad.setKeyCallback([&app](uint32_t key, const char* utf8) { app.onLvglKey(key, utf8); });
    keypad.openDefault();
#endif

    app.start();
    lv_obj_invalidate(lv_screen_active());

    while (!app.quitRequested()) {
#if !LV_USE_SDL
        keypad.poll();
#endif
        lv_timer_handler();
        app.tick(lv_tick_get());
        usleep(10000);
    }

    spdlog::info("Compass: exit requested");
    compass::shutdownLvglHal();
    return 0;
}
