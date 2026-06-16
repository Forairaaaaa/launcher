#pragma once

#include <cstdint>

namespace compass {

namespace compass_key {

constexpr uint32_t Up   = 0x10001;
constexpr uint32_t Down = 0x10002;

}  // namespace compass_key

enum class PageId {
    Compass = 0,
    Calibration,
};

const char* pageIdName(PageId page);

}  // namespace compass
