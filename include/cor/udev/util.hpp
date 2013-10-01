#ifndef _COR_UDEV_UTIL_HPP_
#define _COR_UDEV_UTIL_HPP_

#include <cor/udev.hpp>

namespace cor { namespace udevpp {

bool is_keyboard_available();
bool is_keyboard(Device const &);

}}

#endif // _COR_UDEV_UTIL_HPP_
