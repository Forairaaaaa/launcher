#pragma once

#include <lvgl.h>
#include <cstdint>

namespace compass {

bool initLvglHal(int32_t width, int32_t height);
void shutdownLvglHal();

}  // namespace compass
