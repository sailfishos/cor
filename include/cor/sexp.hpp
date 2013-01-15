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

template <typename CharT>
class Parser
{
public:
    Parser(std::basic_istream<CharT> &src)
        : src(src)
    {}

    template <typename HandlerT>
    void operator ()(HandlerT &handler);

    std::basic_istream<CharT> &src;
};

template <typename CharT>
Parser<CharT> mk_parser(std::basic_istream<CharT> &src)
{
    return Parser<CharT>(src);
}


template <typename CharT, typename HandlerT>
void parse(std::basic_istream<CharT> &src, HandlerT &handler)
{
    Parser<CharT> parser(src);
    parser(handler);
}

template <typename CharT>
template <typename HandlerT>
void Parser<CharT>::operator ()(HandlerT &handler)
{
    enum Action {
        Stay,
        Skip
    };
    const int eos = -1;
    typedef std::function<Action (int)> parser_t;
    parser_t rule, top, in_string, in_atom, in_comment,
        append_escaped, append_escaped_hex;
    stack<parser_t> ctx;
    unsigned level = 0;

    std::string data;
    int hex_byte;

    auto rule_use = [&](parser_t const &p) {
        data = "";
        data.reserve(256);
        rule = p;
    };

    auto rule_pop = [&]() {
        auto p = ctx.top();
        ctx.pop();
        rule = p;
    };

    auto rule_push = [&](parser_t const &after,
                         parser_t const &current) {
        ctx.push(after);
        rule = current;
    };

    top = [&](int c) -> Action {
        if (c == ')') {
            if (!level)
                throw Error(src, "Unexpected ')'");
            --level;
            handler.on_list_end();
        } else if (c == '(') {
            ++level;
            handler.on_list_begin();
        } else if (c == ';') {
            rule_use(in_comment);
        } else if (::isspace(c)) {
            // do nothing
        } else if (c == '"') {
            rule_use(in_string);
        } else if (c != eos) {
            rule_use(in_atom);
            return Stay;
        }
        return Skip;
    };

    in_comment = [&](int c) -> Action {
        if (c != '\n' && c != eos) {
            data += c;
        } else {
            handler.on_comment(std::move(data));
            rule_use(top);
        }
        return Skip;
    };

    auto in_hex = [&](int c) -> Action {
        if (c != eos) {
            int n = char2hex(c);
            if (n >= 0) {
                if (hex_byte < 0) {
                    hex_byte = (n << 4);
                    return Skip;
                }
                hex_byte |= n;
            }
        }
        rule_pop();
        return Stay;
    };

    append_escaped_hex = [&](int) -> Action {
        if (hex_byte < 0)
            throw Error(src, "Escaped hex is empty");

        data += static_cast<char>(hex_byte);
        rule_pop();
        return Stay;
    };

    auto process_hex = [&](parser_t const &after) -> Action {
        hex_byte = -1;
        rule_push(after, in_hex);
        return Skip;
    };

    append_escaped = [&](int c) -> Action {
        static const std::unordered_map<char, char>
        assoc{{'n', '\n'}, {'t', '\t'}, {'r', '\r'}, {'a', '\a'},
              {'b', '\b'}, {'v', '\v'}};

        if (c == eos)
            throw Error(src, "Expected escaped symbol, got EOS");

        if (c == 'x')
            return process_hex(append_escaped_hex);

        auto p = assoc.find(c);
        if (p != assoc.end()) {
            data += p->second;
        } else {
            data += c;
        }
        rule_pop();
        return Skip;
    };

    auto process_escaped = [&]() -> Action {
        rule_push(rule, append_escaped);
        return Skip;
    };

    in_atom = [&](int c) -> Action {
        static const std::string bound("()");
        if (bound.find(c) != std::string::npos || isspace(c) || c == eos) {
            handler.on_atom(std::move(data));
            rule_use(top);
            return Stay;
        } else if (c == '\\') {
            return process_escaped();
        } else {
            data += c;
        }
        return Skip;
    };

    in_string = [&](int c) -> Action {
        if (c == '"') {
            handler.on_string(std::move(data));
            rule_use(top);
        } else if (c == '\\') {
            return process_escaped();
        } else if (c == eos) {
            throw Error(src, "string is not limited, got EOS");
        } else {
            data += c;
        }
        return Skip;
    };

    rule_use(top);
    try {
        while (true) {
            CharT c = src.get();
            if (src.gcount() == 0) {
                rule(eos);
                break;
            }
            
            while (rule(c) == Stay) {}
        }
    } catch (Error const &e) {
        throw;
    } catch (std::exception const &e) {
        throw;
    }
    handler.on_eof();
}

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
