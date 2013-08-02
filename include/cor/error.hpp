#ifndef _COR_ERROR_HPP_
#define _COR_ERROR_HPP_

/*
 * Exceptions and error processing
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

#include <execinfo.h>
#include <cxxabi.h>
#include <dlfcn.h>

#include <algorithm>
#include <array>
#include <string>
#include <cstdio>
#include <stdexcept>
#include <iostream>
#include <memory>

namespace cor
{


template <typename T>
std::unique_ptr<T, void (*)(void*)> mk_cmem_handle(T *from)
{
    return std::unique_ptr<T, void (*)(void*)>(from, &std::free);
}

template <typename T>
std::unique_ptr<T, void (*)(void*)> mk_cmem_handle()
{
    return mk_cmem_handle((char*)0);
}

template <int Frames>
class Backtrace
{
public:
    Backtrace()
        : count_(::backtrace(&frames[0], frames.size())),
          symbols(nullptr)
    { }

    Backtrace(Backtrace const &src)
    {
        count_ = src.count_;
        std::copy(&src.frames[0], &src.frames[count_], frames.begin());
        if (src.symbols) {
            symbols = (const char**)calloc(src.count_, sizeof(symbols[0]));
            std::copy(&src.symbols[0], &src.symbols[count_], &symbols[0]);
        }
    }

    ~Backtrace() noexcept
    {
        if (symbols)
            ::free(symbols);
    }

    const char **begin() const
    {
        update_symbols();
        return &symbols[0];
    }

    const char **end() const
    {
        update_symbols();
        return &symbols[count_];
    }

    char const* at(size_t index) const
    {
        update_symbols();
        return (index < count_) ? symbols[index] : "???";
    }

    bool dladdr(size_t index, Dl_info &info) const
    {
        return (index < count_) && (::dladdr(frames[index], &info) != 0);
    }

    std::string name(size_t index) const
    {
        Dl_info info;
        int status = -1;
        auto p(mk_cmem_handle<char>());
        if (dladdr(index, info) && info.dli_sname)
            p.reset(abi::__cxa_demangle(info.dli_sname, NULL, 0, &status));

        return p ? std::string(p.get()) : std::string(at(index));
    }

    size_t size() const
    {
        return count_;
    }

private:

    void update_symbols() const
    {
        if (symbols)
            return;

        symbols = const_cast<char const**>
            (backtrace_symbols(&frames[0], count_));

        if (!symbols) {
            symbols = static_cast<char const**>
                (::calloc(1, sizeof(symbols[0])));
            symbols[0] = "?";
            count_ = 1;
        }
    }

    std::array<void*, Frames> frames;
    mutable size_t count_;

    mutable char const** symbols;
};

template <typename ... Args>
std::string mk_error_message(std::string const &info, Args ...args)
{
    auto len = info.size();
    if (!len)
        return info;

    std::string res(len * 2, 0);
    int rc;
    while (res.size() < 1024 * 64) {
        rc = snprintf(&res[0], res.size(), info.c_str(), args...);
        if (rc >= 0 && (unsigned)rc < res.size()) {
            res.resize(rc);
            break;
        }
        res.resize(res.size() * 2);
    }
    return res;
}

class Error : public std::runtime_error
{
public:
    template <typename ... Args>
    Error(std::string const &info, Args ... args)
        : std::runtime_error(mk_error_message(info, args...))
    {
    }

    virtual void print_trace() const
    {
        auto pr = [](char const *s) {
            std::cerr << s << std::endl;
        };
        std::for_each(trace.begin(), trace.end(), pr);
    }

private:
    Backtrace<30> trace;
};

class CError : public Error
{
public:
    CError(long rc, std::string const &info)
        : Error(info), rc(rc) {}
    long rc;

    virtual void print_trace() const
    {
        fprintf(stderr, "rc: %lx\n", rc);
        Error::print_trace();
    }
};

template <typename FnT, typename ...Args>
void error_tracer(FnT const &fn, Args ...args)
{
    try {
        fn(args...);
    } catch(Error const &e) {
        std::cerr << "CAUGHT cor::Error\n";
        e.print_trace();
        throw e;
    } catch(std::exception const &e) {
        std::cerr << "CAUGHT std::exception " << e.what() << std::endl;
        throw e;
    }
}

} // namespace cor

#endif // _COR_ERROR_HPP_
