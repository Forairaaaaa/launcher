#include "core/recorder_router.hpp"

namespace recorder {

void RecorderRouter::replace(PageId page)
{
    if (_current_page.get() == page) {
        return;
    }
    _current_page.set(page);
}

void RecorderRouter::push(PageId page)
{
    if (_current_page.get() == page) {
        return;
    }
    _history.push_back(_current_page.get());
    _current_page.set(page);
}

void RecorderRouter::back()
{
    if (_history.empty()) {
        replace(PageId::Recording);
        return;
    }

    PageId previous = _history.back();
    _history.pop_back();
    _current_page.set(previous);
}

} // namespace recorder
