#ifndef _COR_TRACE_HPP_
#define _COR_TRACE_HPP_
/*
 * Dumb debug tracing
 *
 * Copyright (C) 2012-2015 Jolla Ltd.
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

namespace cor { namespace debug {

std::basic_ostream<char> &default_stream();

enum class Level { Debug = 1, Info, Warning, Error, Critical };

/// EOL - output end-of-line, NoEol - no end-of-line
enum class Trace { Eol, NoEol };

void init();

template <typename StreamT>
class TraceContext {
public:
    typedef StreamT stream_type;

    TraceContext(StreamT &stream, bool eol = false) : stream_(stream), eol_(eol) {}
    ~TraceContext() { if (eol_) this->stream_ << std::endl; }

    StreamT &stream_;
    bool eol_;
};

template <typename StreamT, typename T>
TraceContext<StreamT> & operator << (TraceContext<StreamT> &dst, T v)
{
    dst.stream_ << v;
    return dst;
}

template <typename StreamT>
TraceContext<StreamT> & operator << (TraceContext<StreamT> &dst, Trace f)
{
    switch (f) {
    case Trace::Eol:
        dst.eol_ = true;
        break;
    case Trace::NoEol:
        dst.eol_ = false;
        break;
    }
    return dst;
}

template <typename CharT>
TraceContext<std::basic_ostream<char> > tracer(std::basic_ostream<CharT> &dst)
{
    return TraceContext<std::basic_ostream<CharT> >(dst, false);
}

template <typename CharT>
TraceContext<std::basic_ostream<char> > line_tracer(std::basic_ostream<CharT> &dst)
{
    return TraceContext<std::basic_ostream<CharT> >(dst, true);
}

template <typename StreamT, typename T, typename ... Args>
TraceContext<StreamT> & operator <<
(TraceContext<StreamT> &d, std::function<T(Args...)> const &)
{
    d << "std::function<...>";
    return d;
}

// final function stub in recursion
template <typename StreamT>
static inline void print(TraceContext<StreamT> &&) { }

template <typename StreamT, typename T, typename ... A>
void print(TraceContext<StreamT> &&d, T &&v1, A&& ...args)
{
    d << v1;
    return print(std::move(d), std::forward<A>(args)...);
}

template <typename CharT, typename ... A>
void print_to(std::basic_ostream<CharT> &dst, A&& ...args)
{
    return print(tracer(dst), std::forward<A>(args)...);
}

template <typename CharT, typename ... A>
void print_line_to(std::basic_ostream<CharT> &dst, A&& ...args)
{
    return print(line_tracer(dst), std::forward<A>(args)...);
}

template <typename ... A>
void print(A&& ...args)
{
    return print_to(default_stream(), std::forward<A>(args)...);
}

template <typename StreamT, typename ... A>
void print_line(A&& ...args)
{
    return print_line_to(default_stream(), std::forward<A>(args)...);
}

void level(Level);
bool is_tracing_level(Level);

template <typename CharT, typename ... A>
void print_line_ge(std::basic_ostream<CharT> &dst, Level print_level, A&& ...args)
{
    if (is_tracing_level(print_level))
        print_line_to(dst, std::forward<A>(args)...);
}

template <typename ... A>
void print_line_ge(Level print_level, A&& ...args)
{
    print_line_ge(default_stream(), print_level, std::forward<A>(args)...);
}

/// by default print with end-of-line, pass Trace::NoEol to print w/o
/// it
class Log
{
public:
    Log(std::string const &prefix, std::basic_ostream<char> &stream)
        : prefix_(prefix + ": "), stream_(stream)
    {}
    
    template <typename ... A>
    void debug(A&& ...args)
    {
        print_line_ge(stream_, Level::Debug, prefix_, std::forward<A>(args)...);
    }

    template <typename ... A>
    void info(A&& ...args)
    {
        print_line_ge(stream_, Level::Info, prefix_, std::forward<A>(args)...);
    }

    template <typename ... A>
    void warning(A&& ...args)
    {
        print_line_ge(stream_, Level::Warning, prefix_, std::forward<A>(args)...);
    }

    template <typename ... A>
    void error(A&& ...args)
    {
        print_line_ge(stream_, Level::Error, prefix_, std::forward<A>(args)...);
    }

    template <typename ... A>
    void critical(A&& ...args)
    {
        print_line_ge(stream_, Level::Critical, prefix_, std::forward<A>(args)...);
    }
    
    std::string prefix() const
    {
        return prefix_;
    }
    
private:
    std::string prefix_;
    std::basic_ostream<char> &stream_;
};

}}

#endif // _COR_TRACE_HPP_
