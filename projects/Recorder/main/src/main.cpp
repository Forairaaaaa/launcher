#include "core/recorder_app.hpp"
#include "core/recorder_config.hpp"
#include "hal_lvgl_bsp.h"
#include <core/hal/hal.hpp>
#include <stdio.h>
#include <string>
#include <unistd.h>
#include <lvgl.h>
#include <spdlog/spdlog.h>

namespace {

void printUsage(const char* program)
{
    printf("Usage: %s [--recordings-dir DIR]\n", program);
    printf("       %s [-r DIR]\n", program);
}

bool parseArgs(int argc, char* argv[], recorder::RecorderConfig& config)
{
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i] ? argv[i] : "";
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0] ? argv[0] : "Recorder");
            return false;
        }

        if (arg == "--recordings-dir" || arg == "-r") {
            if (i + 1 >= argc) {
                spdlog::warn("Recorder: missing value for {}", arg);
                continue;
            }
            config.recordings_dir = recorder::normalizeRecordingDirectory(argv[++i]);
            continue;
        }

        constexpr const char* kRecordingsDirPrefix = "--recordings-dir=";
        if (arg.rfind(kRecordingsDirPrefix, 0) == 0) {
            config.recordings_dir =
                recorder::normalizeRecordingDirectory(arg.substr(std::string(kRecordingsDirPrefix).size()));
            continue;
        }

        spdlog::warn("Recorder: ignored unknown argument {}", arg);
    }
    return true;
}

}  // namespace

int main(int argc, char* argv[])
{
    recorder::RecorderConfig config = recorder::defaultRecorderConfig();
    if (!parseArgs(argc, argv, config)) {
        return 0;
    }

    lv_init();
    cp0_lvgl_init();

    lv_display_t* disp = lv_display_get_default();
    if (disp == nullptr) {
        fprintf(stderr, "Recorder: failed to create LVGL display\n");
        return 1;
    }

    spdlog::info("Recorder: display {}x{}", (int)lv_display_get_horizontal_resolution(disp),
                 (int)lv_display_get_vertical_resolution(disp));

    smooth_ui_toolkit::ui_hal::on_get_tick([]() { return lv_tick_get(); });
    smooth_ui_toolkit::ui_hal::on_delay([](uint32_t ms) { usleep(ms * 1000); });

    recorder::RecorderApp app(config);
    app.start();

    lv_obj_invalidate(lv_screen_active());

    while (1) {
        lv_timer_handler();
        app.tick(lv_tick_get());
        usleep(10000);
    }
}
