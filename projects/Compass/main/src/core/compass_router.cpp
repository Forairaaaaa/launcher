#include "core/compass_router.hpp"

namespace compass {

void CompassRouter::replace(PageId page)
{
    if (_current_page.get() == page) {
        return;
    }
    _current_page.set(page);
}

void CompassRouter::push(PageId page)
{
    if (_current_page.get() == page) {
        return;
    }
    _history.push_back(_current_page.get());
    _current_page.set(page);
}

void CompassRouter::back()
{
    if (_history.empty()) {
        replace(PageId::Compass);
        return;
    }

    PageId previous = _history.back();
    _history.pop_back();
    _current_page.set(previous);
}

}  // namespace compass
