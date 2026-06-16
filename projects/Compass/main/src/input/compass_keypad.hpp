#pragma once

#include <lvgl.h>
#include <cstdint>
#include <deque>
#include <functional>
#include <string>
#include <vector>

namespace compass {

class CompassKeypad {
public:
    using KeyCallback = std::function<void(uint32_t, const char*)>;

    CompassKeypad() = default;
    ~CompassKeypad();

    CompassKeypad(const CompassKeypad&)            = delete;
    CompassKeypad& operator=(const CompassKeypad&) = delete;

    bool openDefault();
    bool openDevice(const std::string& path, bool require_compass_keys);
    void close();
    void poll();
    lv_indev_t* indev() const;
    void setKeyCallback(KeyCallback callback);

private:
    struct KeyEvent {
        uint32_t key = 0;
        bool pressed = false;
    };

    static void readCb(lv_indev_t* indev, lv_indev_data_t* data);

    bool ensureIndev();
    void pushKeyEvent(uint16_t code, int32_t value);
    uint32_t translateKey(uint16_t code) const;
    const char* keyUtf8(uint32_t key) const;

    lv_indev_t* _indev = nullptr;
    std::vector<int> _event_fds;
    std::deque<KeyEvent> _pending_keys;
    KeyCallback _key_callback;
    uint32_t _last_key = 0;
};

}  // namespace compass
