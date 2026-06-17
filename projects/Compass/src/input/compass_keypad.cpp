#include "input/compass_keypad.hpp"

#include <spdlog/spdlog.h>
#include <cstdlib>

#if !LV_USE_SDL && defined(__linux__)
#include <array>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace compass {

#if !LV_USE_SDL && defined(__linux__)
namespace {

template <size_t N>
bool testBit(const std::array<unsigned long, N>& bits, unsigned int bit)
{
    constexpr unsigned int bits_per_word = sizeof(unsigned long) * 8;
    const unsigned int index             = bit / bits_per_word;
    const unsigned int offset            = bit % bits_per_word;
    return index < bits.size() && ((bits[index] >> offset) & 1UL) != 0;
}

bool hasCompassKeys(int fd)
{
    constexpr size_t key_bits_size = (KEY_MAX + sizeof(unsigned long) * 8) / (sizeof(unsigned long) * 8);
    std::array<unsigned long, key_bits_size> key_bits{};
    if (::ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits.data()) < 0) {
        return false;
    }

    return testBit(key_bits, KEY_ESC) || testBit(key_bits, KEY_ENTER) || testBit(key_bits, KEY_KPENTER) ||
           testBit(key_bits, KEY_UP) || testBit(key_bits, KEY_DOWN) || testBit(key_bits, KEY_LEFT) ||
           testBit(key_bits, KEY_RIGHT) || testBit(key_bits, KEY_4) || testBit(key_bits, KEY_5) ||
           testBit(key_bits, KEY_6) || testBit(key_bits, KEY_7) || testBit(key_bits, KEY_8);
}

bool envEnabled(const char* name, bool fallback)
{
    const char* value = std::getenv(name);
    if (!value || value[0] == '\0') {
        return fallback;
    }

    return std::strcmp(value, "0") != 0 && std::strcmp(value, "false") != 0 && std::strcmp(value, "False") != 0 &&
           std::strcmp(value, "off") != 0 && std::strcmp(value, "OFF") != 0;
}

}  // namespace
#endif

CompassKeypad::~CompassKeypad()
{
    close();
}

bool CompassKeypad::openDefault()
{
#if !LV_USE_SDL && defined(__linux__)
    if (const char* device_path = std::getenv("COMPASS_KEYBOARD_DEVICE")) {
        return openDevice(device_path, false);
    }
    if (const char* device_path = std::getenv("APPLAUNCH_LINUX_KEYBOARD_DEVICE")) {
        return openDevice(device_path, false);
    }
    if (const char* device_path = std::getenv("LV_LINUX_KEYBOARD_DEVICE")) {
        return openDevice(device_path, false);
    }

    if (openDevice("/dev/input/by-path/platform-3f804000.i2c-event", false)) {
        return true;
    }

    bool opened = false;
    for (int index = 0; index < 32; ++index) {
        opened = openDevice("/dev/input/event" + std::to_string(index), true) || opened;
    }

    if (!opened) {
        spdlog::warn("CompassKeypad: no keyboard input device opened");
    }
    return opened;
#else
    return false;
#endif
}

bool CompassKeypad::openDevice(const std::string& path, bool require_compass_keys)
{
#if !LV_USE_SDL && defined(__linux__)
    if (!ensureIndev()) {
        return false;
    }

    const int fd = ::open(path.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        return false;
    }

    if (require_compass_keys && !hasCompassKeys(fd)) {
        ::close(fd);
        return false;
    }

    const bool grab_input = envEnabled("COMPASS_KEYBOARD_GRAB", false);
    if (grab_input && ::ioctl(fd, EVIOCGRAB, 1) < 0) {
        spdlog::warn("CompassKeypad: failed to grab {}: {}", path, std::strerror(errno));
        ::close(fd);
        return false;
    }

    _event_fds.push_back(fd);
    spdlog::info("CompassKeypad: opened {}{}", path, grab_input ? " with grab" : "");
    return true;
#else
    (void)path;
    (void)require_compass_keys;
    return false;
#endif
}

void CompassKeypad::close()
{
#if !LV_USE_SDL && defined(__linux__)
    for (int fd : _event_fds) {
        if (fd >= 0) {
            (void)::ioctl(fd, EVIOCGRAB, 0);
            ::close(fd);
        }
    }
#endif
    _event_fds.clear();
    _pending_keys.clear();

    if (_indev) {
        lv_indev_delete(_indev);
        _indev = nullptr;
    }
}

void CompassKeypad::poll()
{
#if !LV_USE_SDL && defined(__linux__)
    for (int fd : _event_fds) {
        while (true) {
            input_event event{};
            const ssize_t bytes_read = ::read(fd, &event, sizeof(event));
            if (bytes_read == sizeof(event)) {
                if (event.type == EV_KEY) {
                    pushKeyEvent(event.code, event.value);
                }
                continue;
            }

            if (bytes_read < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
                break;
            }

            if (bytes_read < 0) {
                spdlog::warn("CompassKeypad: failed to read input event: {}", std::strerror(errno));
            }
            break;
        }
    }
#endif
}

lv_indev_t* CompassKeypad::indev() const
{
    return _indev;
}

void CompassKeypad::setKeyCallback(KeyCallback callback)
{
    _key_callback = std::move(callback);
}

void CompassKeypad::readCb(lv_indev_t* indev, lv_indev_data_t* data)
{
    auto* keypad = static_cast<CompassKeypad*>(lv_indev_get_user_data(indev));
    if (!keypad || !data) {
        return;
    }

    if (!keypad->_pending_keys.empty()) {
        const KeyEvent event = keypad->_pending_keys.front();
        keypad->_pending_keys.pop_front();
        keypad->_last_key      = event.key;
        data->key              = event.key;
        data->state            = event.pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
        data->continue_reading = !keypad->_pending_keys.empty();
        return;
    }

    data->key   = keypad->_last_key;
    data->state = LV_INDEV_STATE_RELEASED;
}

bool CompassKeypad::ensureIndev()
{
    if (_indev) {
        return true;
    }

    _indev = lv_indev_create();
    if (!_indev) {
        spdlog::error("CompassKeypad: failed to create LVGL keypad indev");
        return false;
    }

    lv_indev_set_type(_indev, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(_indev, readCb);
    lv_indev_set_user_data(_indev, this);
    return true;
}

void CompassKeypad::pushKeyEvent(uint16_t code, int32_t value)
{
    if (value != 0 && value != 1) {
        return;
    }

    const uint32_t key = translateKey(code);
    if (key == 0) {
        return;
    }

    const bool pressed = value == 1;
    _pending_keys.push_back({key, pressed});

    if (pressed && _key_callback) {
        _key_callback(key, keyUtf8(key));
    }
}

uint32_t CompassKeypad::translateKey(uint16_t code) const
{
#if !LV_USE_SDL && defined(__linux__)
    switch (code) {
        case KEY_ESC:
            return LV_KEY_ESC;
        case KEY_ENTER:
        case KEY_KPENTER:
            return LV_KEY_ENTER;
        case KEY_UP:
            return LV_KEY_UP;
        case KEY_DOWN:
            return LV_KEY_DOWN;
        case KEY_LEFT:
            return LV_KEY_LEFT;
        case KEY_RIGHT:
            return LV_KEY_RIGHT;
        case KEY_SPACE:
            return ' ';
        case KEY_4:
        case KEY_KP4:
            return '4';
        case KEY_5:
        case KEY_KP5:
            return '5';
        case KEY_6:
        case KEY_KP6:
            return '6';
        case KEY_7:
        case KEY_KP7:
            return '7';
        case KEY_8:
        case KEY_KP8:
            return '8';
        default:
            return 0;
    }
#else
    (void)code;
    return 0;
#endif
}

const char* CompassKeypad::keyUtf8(uint32_t key) const
{
    static char text[2] = {0, 0};
    if (key >= 0x20 && key < 0x7f) {
        text[0] = static_cast<char>(key);
        text[1] = '\0';
        return text;
    }
    return "";
}

}  // namespace compass
