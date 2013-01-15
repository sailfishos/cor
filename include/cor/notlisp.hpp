#ifndef _COR_NOTLISP_HPP_
#define _COR_NOTLISP_HPP_
/*
 * Very basic interpreter for basic language (s-expressions-based)
 *
 * Mostly for usage as configuration and communication (e.g. rpc)
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

#include <list>
#include <memory>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <stack>

#include <cor/error.hpp>

namespace cor
{
namespace notlisp
{

class Expr;
typedef std::shared_ptr<Expr> expr_ptr;
typedef std::list<expr_ptr> expr_list_type;

class Env;
typedef std::shared_ptr<Env> env_ptr;
typedef std::function<expr_ptr (env_ptr, expr_list_type&)> lambda_type;

class Env
{
public:
    typedef std::unordered_map<std::string, expr_ptr> dict_type;
    typedef typename dict_type::value_type item_type;

    Env() {}
    Env(std::initializer_list<item_type> syms) 
        : dict(syms)
    {}

    dict_type dict;
};

class Expr
{
public:
    enum Type {
        Symbol,
        Keyword,
        String,
        Function,
        Nil,
        Object,
        Integer,
        Real
    };

    Expr() : type_(Nil), s_(""), i_(0) {}
    Expr(std::string const &v, Type t) : type_(t), s_(v) {}
    Expr(int v) : type_(Integer), i_(v) {}
    Expr(long v) : type_(Integer), i_(v) {}
    Expr(double v) : type_(Real), r_(v) {}

    virtual ~Expr() {}

    std::string const& value() const
    {
        return s_;
    }

    operator long() const
    {
        return i_;
    }

    operator double() const
    {
        return r_;
    }

    Type type() const
    {
        return type_;
    }

protected:

    Type type_;

    std::string s_;
    union {
        long i_;
        double r_;
    };

    friend expr_ptr eval(env_ptr env, expr_ptr src);

    template <typename CharT> friend 
    std::basic_ostream<CharT> & operator <<
    (std::basic_ostream<CharT> &, Expr const &);

    virtual expr_ptr do_eval(env_ptr, expr_ptr) = 0;

private:
    Expr(Expr &);
    Expr& operator =(Expr &);
};

template <typename CharT>
std::basic_ostream<CharT> & operator <<
(std::basic_ostream<CharT> &dst, Expr const &src)
{
    switch (src.type()) {
    case Expr::String: dst << "\"" << src.value() << "\""; break;
    case Expr::Symbol: dst << src.value(); break;
    case Expr::Keyword: dst << ":" << src.value(); break;
    case Expr::Object: dst << src.value(); break;
    case Expr::Function: dst << src.value(); break;
    case Expr::Nil: dst << "nil"; break;
    case Expr::Integer: dst << (long)src; break;
    case Expr::Real: dst << (double)src; break;
    }
    return dst;
}


expr_ptr eval(env_ptr env, expr_ptr src);

/// evaluates list using environment env. Returns result list,
/// std::move is considered to be used
expr_list_type eval(env_ptr env, expr_list_type const &src);


template <Expr::Type T>
class BasicExpr : public Expr
{
public:
    BasicExpr(std::string const &s) : Expr(s, T) {}
protected:
    virtual expr_ptr do_eval(env_ptr, expr_ptr);
};

template <>
class BasicExpr<Expr::Nil> : public Expr
{
public:
    BasicExpr() : Expr() {}
protected:
    virtual expr_ptr do_eval(env_ptr, expr_ptr self) { return self; }
};

template <Expr::Type T>
expr_ptr mk_basic_expr(std::string const &s)
{
    return expr_ptr(new BasicExpr<T>(s));
}

expr_ptr mk_string(std::string const &s);
expr_ptr mk_keyword(std::string const &s);
expr_ptr mk_nil();

template <Expr::Type T>
expr_ptr BasicExpr<T>::do_eval(env_ptr, expr_ptr self)
{
    return self;
}

class PodExpr : public Expr
{
public:
    template <typename T>
    PodExpr(T v) : Expr(v) {}
protected:
    virtual expr_ptr do_eval(env_ptr, expr_ptr);
};

template <typename T>
expr_ptr mk_value(T v)
{
    return expr_ptr(new PodExpr(v));
}

class SymbolExpr : public Expr
{
public:
    SymbolExpr(std::string const &s) : Expr(s, Expr::Symbol) {}
protected:
    virtual expr_ptr do_eval(env_ptr, expr_ptr);
};

expr_ptr mk_symbol(std::string const &s);

class FunctionExpr : public Expr
{
public:
    FunctionExpr(std::string const &name) : Expr(name, Expr::Function) {}
    virtual expr_ptr operator ()(env_ptr, expr_list_type &&) =0;
};

class LambdaExpr : public FunctionExpr
{
public:
    LambdaExpr(std::string const &name,
               lambda_type fn)
        : FunctionExpr(name),
          fn(fn)
    {}

    virtual expr_ptr operator ()(env_ptr env, expr_list_type &&params)
    {
        return fn(env, params);
    }
protected:
    virtual expr_ptr do_eval(env_ptr, expr_ptr);
private:
    lambda_type fn;
};

expr_ptr mk_lambda(std::string const &name, lambda_type const &fn);

void to_string(expr_ptr expr, std::string &dst);
void to_long(expr_ptr expr, long &dst);
void to_double(expr_ptr expr, double &dst);

static inline Env::item_type mk_record
(std::string const &name, lambda_type const &fn)
{
    return std::make_pair(name, mk_lambda(name, fn));
}

static inline Env::item_type mk_const
(std::string const &name, std::string const &val)
{
    return std::make_pair(name, mk_string(val));
}

template <typename T>
static inline Env::item_type mk_const
(std::string const &name, T val)
{
    return std::make_pair(name, mk_value(val));
}

expr_ptr default_atom_convert(std::string &&s);

class Interpreter
{
public:
    typedef std::function<expr_ptr (std::string &&)> atom_converter_type;
    Interpreter(env_ptr env,
                atom_converter_type atom_converter
                = &cor::notlisp::default_atom_convert)
        : env(env),
          stack({expr_list_type()}),
          convert_atom(atom_converter)
    {}

    void on_list_begin()
    {
        stack.push(expr_list_type());
    }

    void on_list_end();

    void on_comment(std::string &&s) { }

    void on_string(std::string &&s) {
        stack.top().push_back(mk_string(s));
    }

    void on_atom(std::string &&s) {
        stack.top().push_back(convert_atom(std::move(s)));
    }

    expr_list_type const& results() const
    {
        return stack.top();
    }
private:
    env_ptr env;
    std::stack<expr_list_type> stack;
    atom_converter_type convert_atom;
};

class ObjectExpr : public Expr
{
public:
    ObjectExpr(std::string const &s) : Expr(s, Expr::Object) {}
protected:
    virtual expr_ptr do_eval(env_ptr, expr_ptr);
};

class ListAccessor
{
public:

    ListAccessor(expr_list_type const& params)
        : cur(params.begin()),
          end(params.end())
    {}

    /// access required parameters list member, write result using
    /// convert function to dst. \return this object to allow chained
    /// calls like accessor.required(convertx, p1).required(converty, p2)
    template <typename T>
    ListAccessor& required(void (*convert)(expr_ptr, T &dst), T &dst)
    {
        if (cur == end)
            throw cor::Error("Required param is absent");

        convert(*cur++, dst);
        return *this;
    }

    template <typename ConsumerT>
    bool optional(ConsumerT fn)
    {
        return (cur != end) ? fn(*cur++) : false;
    }

    bool optional(expr_ptr &dst)
    {
        if (cur == end)
            return false;

        dst = *cur++;
        return true;
    }

private:

    expr_list_type::const_iterator cur;
    expr_list_type::const_iterator end;
};

template <typename ConsumerT>
static void rest(ListAccessor &src, ConsumerT fn)
{
    while (src.optional(fn)) {}
}

template <typename T, typename FnT>
void rest_casted(ListAccessor &src, FnT fn)
{
    rest(src,
         [&fn](expr_ptr p) {
             auto res = std::dynamic_pointer_cast<T>(p);
             if (!res)
                 throw cor::Error("Can't be casted");
             fn(res);
             return !!res;
         });
}

template <typename ArgFnT, typename KeyFnT>
void rest(ListAccessor &src, ArgFnT arg, KeyFnT kwd_ard)
{
    expr_ptr k, v;
    while (src.optional(v)) {
        if (!k) {
            if (v && v->type() == Expr::Keyword)
                k = v;
            else
                arg(v);
        } else {
            kwd_ard(k, v);
            k = nullptr;
        }
    }
    if (k)
        throw cor::Error("Orphaned keyword");
}

template <typename ContainerT, typename ConvertT>
void push_rest(ListAccessor &src, ContainerT &dst, ConvertT convert)
{
    rest(src, [&dst, &convert](expr_ptr from) {
            dst.push_back(convert(from));
            return true;
        });
}

template <typename T>
void push_rest_casted(ListAccessor &src, T &dst) {
    typedef typename T::value_type ptr_type;
    typedef typename ptr_type::element_type cast_type;
    auto fn = [](expr_ptr from) {
        auto res = std::dynamic_pointer_cast<cast_type>(from);
        if (!res)
            throw cor::Error("Can't be casted");
        return res;
    };
    push_rest(src, dst, fn);
};

}} // cor::notlisp

#endif // _COR_NOTLISP_HPP_
