#pragma once

#include <lvgl.h>

namespace recorder {

class View {
public:
    virtual ~View() = default;

    View(const View&)            = delete;
    View& operator=(const View&) = delete;

    virtual void onEnter(lv_obj_t* parent) = 0;
    virtual void onExit()                  = 0;

protected:
    View() = default;
};

}  // namespace recorder
