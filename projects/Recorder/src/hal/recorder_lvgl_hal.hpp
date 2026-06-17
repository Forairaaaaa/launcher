#pragma once

#include <cstdint>

namespace recorder {

bool initLvglHal(int32_t width, int32_t height);
void shutdownLvglHal();

}  // namespace recorder
