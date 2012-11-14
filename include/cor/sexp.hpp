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

namespace cor {
namespace sexp {

template <typename ... Args>
std::string mk_sexp_err_msg(std::basic_istream<char> &src,
                            char const *info, Args ... args)
{
    std::stringstream ss;
    ss << "S-exp parsing error @ " << src.tellg() << ". "
       << mk_error_message(info, args...);
    return ss.str();
}

class Error : public cor::Error
{
public:
    template <typename ... Args>
    Error(std::basic_istream<char> &src,
          char const *info, Args ... args)
        : cor::Error(mk_sexp_err_msg(src, info, args...)),
          src(src) {}
private:
    std::basic_istream<char> &src;
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


template <typename CharT, typename BuilderT>
void parse(std::basic_istream<CharT> &src, BuilderT &builder)
{
    enum Action {
        Stay,
        Skip
    };
    typedef std::function<Action (CharT)> parser_t;
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
        rule_use(p);
    };

    auto rule_push = [&](parser_t const &after,
                              parser_t const &current) {
        ctx.push(after);
        rule_use(current);
    };

    top = [&](CharT c) -> Action {
        if (c == ')') {
            if (!level)
                throw Error(src, "Unexpected ')'");
            --level;
            builder.on_list_end();
        } else if (c == '(') {
            ++level;
            builder.on_list_begin();
        } else if (c == ';') {
            rule_use(in_comment);
        } else if (::isspace(c)) {
            // do nothing
        } else if (c == '"') {
            rule_use(in_string);
        } else {
            rule_use(in_atom);
            return Stay;
        }
        return Skip;
    };

    in_comment = [&](CharT c) -> Action {
        if (c != '\n') {
            data += c;
        } else {
            builder.on_comment(std::move(data));
            rule_use(top);
        }
        return Skip;
    };

    auto in_hex = [&](CharT c) -> Action {
        int n = char2hex(c);
        if (n < 0)
            goto append;

        if (hex_byte < 0) {
            hex_byte = (n << 4);
            return Skip;
        }
        hex_byte |= n;
    append:
        rule_pop();
        return Stay;
    };

    append_escaped_hex = [&](CharT) -> Action {
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

    append_escaped = [&](CharT c) -> Action {
        static const std::unordered_map<char, char>
        assoc{{'n', '\n'}, {'t', '\t'}, {'r', '\r'}, {'a', '\a'},
              {'b', '\b'}, {'v', '\v'}};

        if (c == 'x')
            return process_hex(append_escaped_hex);

        auto p = assoc.find(c);
        if (p != assoc.end())
            data += p->second;
        else
            data += c;

        rule_pop();
        return Skip;
    };

    auto process_escaped = [&]() -> Action {
        rule_push(rule, append_escaped);
        return Skip;
    };

    in_atom = [&](CharT c) -> Action {
        static const std::string bound("()");
        if (bound.find(c) != std::string::npos || isspace(c)) {
            builder.on_atom(std::move(data));
            rule_use(top);
            return Stay;
        } else if (c == '\\') {
            return process_escaped();
        } else {
            data += c;
        }
        return Skip;
    };

    in_string = [&](CharT c) -> Action {
        if (c == '"') {
            builder.on_string(std::move(data));
            rule_use(top);
        } else if (c == '\\') {
            return process_escaped();
        } else {
            data += c;
        }
        return Skip;
    };

    rule_use(top);
    try {
        while (true) {
            CharT c = src.get();
            if (src.gcount() == 0)
                break;
            
            while (rule(c) == Stay) {}
        }
    } catch (std::exception const &e) {
        std::cerr << mk_sexp_err_msg(src, e.what()) << std::endl;
        throw e;
    }

}

} // namespace
} // namespace


#endif // _COR_SEXP_HPP_
