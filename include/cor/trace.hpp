#ifndef _COR_TRACE_HPP_
#define _COR_TRACE_HPP_
/*
 * Dumb debug tracing
 *
 * Copyright (C) 2012 Jolla Ltd.
 * Contact: Denis Zalevskiy <denis.zalevskiy@jollamobile.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 */

#include <cor/error.hpp>
#include <iostream>

#ifdef DEBUG
static inline std::basic_ostream<char> & trace()
{
    return std::cerr;
}

static inline std::string caller_name(size_t level = 3)
{
    if (level > 4)
        return "? level>4";
    cor::Backtrace<5> bt;
    return bt.name(level);
}

#else // DEBUG

struct null_stream : public std::basic_ostream<char> { };

static null_stream null_stream_;

template <typename T>
null_stream & operator << (null_stream & dst, T const &)
{
    return dst;
}

static inline null_stream & trace()
{
    return null_stream_;
}

static inline std::string caller_name()
{
    return "";
}

#endif // DEBUG

#endif // _COR_TRACE_HPP_
