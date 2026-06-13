#pragma once

#include "core/recorder_router.hpp"
#include "core/recorder_types.hpp"

namespace recorder {

class ViewModel {
public:
    explicit ViewModel(RecorderRouter& router) : _router(router)
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
    }
    virtual void tick(uint32_t nowMs)
    {
    }

protected:
    RecorderRouter& _router;
};

}  // namespace recorder
