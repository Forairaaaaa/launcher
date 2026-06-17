#pragma once

#include "core/compass_router.hpp"
#include "core/compass_types.hpp"

namespace compass {

class ViewModel {
public:
    explicit ViewModel(CompassRouter& router) : _router(router)
    {
    }
    virtual ~ViewModel() = default;

    ViewModel(const ViewModel&)            = delete;
    ViewModel& operator=(const ViewModel&) = delete;

    virtual PageId pageId() const = 0;
    virtual void onEnter()
    {
    }
    virtual void onExit()
    {
    }
    virtual void onKey(uint32_t key)
    {
        (void)key;
    }
    virtual void tick(uint32_t nowMs)
    {
        (void)nowMs;
    }

protected:
    CompassRouter& _router;
};

}  // namespace compass
