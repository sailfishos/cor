#include <cor/notlisp.hpp>

namespace cor
{
namespace notlisp
{

expr_ptr mk_string(std::string const &s)
{
    return mk_basic_expr<Expr::String>(s);
}

expr_ptr mk_keyword(std::string const &s)
{
    return mk_basic_expr<Expr::Keyword>(s);
}

expr_ptr mk_nil()
{
    return mk_basic_expr<Expr::Nil>("");
}

expr_ptr mk_symbol(std::string const &s)
{
    return expr_ptr(new SymbolExpr(s));
}

expr_ptr mk_lambda(std::string const &name, lambda_type const &fn)
{
    return expr_ptr(new LambdaExpr(name, fn));
}

expr_ptr eval(env_ptr env, expr_ptr src)
{
    return src->do_eval(env, src);
}

void to_string(expr_ptr expr, std::string &dst)
{
    if (!expr)
        throw cor::Error("to_string: Null");
    if (expr->type() != Expr::String)
        throw cor::Error("%s is not string", expr->value().c_str());
    dst = expr->value();
}

expr_ptr LambdaExpr::do_eval(env_ptr, expr_ptr self)
{
    return self;
}

expr_ptr PodExpr::do_eval(env_ptr, expr_ptr self)
{
    return self;
}

expr_ptr SymbolExpr::do_eval(env_ptr env, expr_ptr)
{
    return env->dict[value()];
}

expr_ptr ObjectExpr::do_eval(env_ptr, expr_ptr self)
{
    return self;
}

expr_ptr default_atom_convert(std::string &&s)
{
    expr_ptr res(mk_nil());
    if (s.size() && s[0] == ':')
        res = mk_keyword(s.substr(1));
    else {
        double f;
        long i;
        auto convert = [&](char const *cstr, size_t len) {
            char * endptr = nullptr;
            char const* end = cstr + len;

            i = std::strtol(cstr, &endptr, 10);
            if (endptr == end)
                return mk_value(i);

            f = std::strtod(cstr, &endptr);
            if (endptr == end)
                return mk_value(f);

            return mk_symbol(s);
        };
        res = convert(s.c_str(), s.size());
    }
    return res;
}

} // notlisp
} // cor
