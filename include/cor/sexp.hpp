#ifndef _COR_SEXP_HPP_
#define _COR_SEXP_HPP_
/*
 * Very basic recursive descent s-expressions parser 
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

#include <cor/trace.hpp>
#include <cor/error.hpp>
#include <cor/util.hpp>

#include <iostream>
#include <string>
#include <functional>
#include <ctype.h>
#include <unordered_map>
#include <stack>
#include <deque>
#include <sstream>

namespace cor {
namespace sexp {

template <typename ... Args>
std::string mk_sexp_err_msg(std::basic_istream<char> &src,
                            char const *info, Args ... args)
{
    return concat("error parsing S-exp, pos ", src.tellg(), ": ",
                  mk_error_message(info, args...));
}

class Error : public cor::Error
{
public:
    template <typename ... Args>
    Error(std::basic_istream<char> &src, char const *info, Args ... args)
        : cor::Error(mk_sexp_err_msg(src, info, args...))
        , pos(src.tellg())
    {}

    size_t pos;
};

template <typename CharT>
int char2hex(CharT c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    else if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    else if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;

    return -1;
};

template <typename CharT, typename HandlerT>
extern void parse(std::basic_istream<CharT> &src, HandlerT &handler);

/// interface can be inherited by a handler in the case parser is used
/// for multiply handlers to avoid multiply instantiations
class AbstractHandler {
public:
    virtual void on_list_begin() =0;
    virtual void on_list_end() =0;
    virtual void on_comment(std::string &&s) =0;
    virtual void on_string(std::string &&s) =0;
    virtual void on_atom(std::string &&s) =0;
    virtual void on_eof() =0;
};

} // namespace
} // namespace


#endif // _COR_SEXP_HPP_
