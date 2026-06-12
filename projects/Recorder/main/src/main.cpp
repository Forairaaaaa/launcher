#include "hal_lvgl_bsp.h"
#include <stdio.h>
#include <unistd.h>
#include <lvgl.h>
#include <smooth_lvgl.hpp>
#include <spdlog/spdlog.h>

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    lv_init();
    cp0_lvgl_init();

    lv_display_t *disp = lv_display_get_default();
    if (disp == nullptr) {
        fprintf(stderr, "Recorder: failed to create LVGL display\n");
        return 1;
    }

    spdlog::info("Recorder: display {}x{}", (int)lv_display_get_horizontal_resolution(disp),
                 (int)lv_display_get_vertical_resolution(disp));

    lv_obj_invalidate(lv_screen_active());

    while (1) {
        lv_timer_handler();
        usleep(10000);
    }
}
