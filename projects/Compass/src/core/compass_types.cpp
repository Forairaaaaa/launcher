#include "core/compass_types.hpp"

namespace compass {

const char* pageIdName(PageId page)
{
    switch (page) {
        case PageId::Compass:
            return "Compass";
        case PageId::Calibration:
            return "Calibration";
    }
    return "Unknown";
}

}  // namespace compass
