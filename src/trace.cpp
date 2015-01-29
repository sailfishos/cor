/**
 * @file trace.cpp
 * @brief Debug tracing support
 * @author Denis Zalevskiy <denis.zalevskiy@jolla.com>
 * @copyright (C) 2014 Jolla Ltd.
 * @par License: LGPL 2.1 http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 */

#include <cor/trace.hpp>
#include <mutex>
#include <string>
#include <stdlib.h>

namespace cor { namespace debug {

namespace {

int current_level = static_cast<int>(Level::Error);
std::once_flag set_level_once_;
std::basic_ostream<char> &default_stream_ = std::cerr;
}

const char *level_tags[] = {
    "1:", "2:", "3:", "4:", "5:"
};
static_assert(sizeof(level_tags) / sizeof(level_tags[0])
              == cor::enum_size<Level>()
              , "Check level tag names size");

void init()
{
    std::call_once(set_level_once_, []() {
            auto c = ::getenv("COR_DEBUG");
            if (c) {
                std::string name(c);
                current_level = name.size() ? std::stoi(name) : (int)Level::Error;
            }
        });
}

void level(Level level)
{
    init();
    current_level = static_cast<int>(level);
}

bool is_tracing_level(Level level)
{
    init();
    auto l = static_cast<int>(level);
    return current_level ? l >= current_level : false;
}

std::basic_ostream<char> &default_stream()
{
    return default_stream_;
}

}}
