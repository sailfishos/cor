#include <cor/notlisp.hpp>
#include <cor/sexp_impl.hpp>

namespace cor {
namespace sexp {

template
void parse(std::basic_istream<char> &, cor::notlisp::Interpreter &);

}}

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
    return expr_ptr(new BasicExpr<Expr::Nil>());
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
    return src ? src->do_eval(env, src) : mk_nil();
}

static void must_have_type(expr_ptr expr, Expr::Type t,
                           std::string const &failure_msg)
{
    if (!expr)
        throw Error(failure_msg + ". Null expression");
    if (expr->type() != t)
        throw Error
            ((failure_msg + ". expr %s: need type %d, got %d").c_str(),
             expr->value().c_str(), t, expr->type());
}

void to_string(expr_ptr expr, std::string &dst)
{
    must_have_type(expr, Expr::String, "to_string");
    dst = expr->value();
}

void to_long(expr_ptr expr, long &dst)
{
    must_have_type(expr, Expr::Integer, "to_long");
    dst = (long)*expr;
}

void to_double(expr_ptr expr, double &dst)
{
    must_have_type(expr, Expr::Real, "to_double");
    dst = (double)*expr;
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

Interpreter::Interpreter(env_ptr env, atom_converter_type atom_converter)
    : env(env),
      stack({expr_list_type()}),
      convert_atom(atom_converter)
{
}

void Interpreter::on_atom(std::string &&s)
{
    auto v = convert_atom(std::move(s));
    stack.top().push_back(eval(env, v));
}

void Interpreter::on_list_end()
{
    auto &t = stack.top();

    if (t.empty())
        throw Error("Evaluation of empty expression");

    auto &expr = *t.begin();
    auto p = eval(env, expr);
    if (!p)
        throw Error("Got null evaluating %s, expecting function",
                         expr->value().c_str());

    if (p->type() != Expr::Function)
        throw Error("Not a function, type %d", p->type());
    t.pop_front();
    expr_ptr res;
    try {
        res = static_cast<FunctionExpr&>(*p)(env, eval(env, t));
    } catch (cor::Error const &e) {
        std::cerr << "Error '" << e.what() << "' evaluating "
                  << *p << std::endl;
        throw e;
    }
    stack.pop();
    stack.top().push_back(res);
}

expr_list_type eval(env_ptr env, expr_list_type const &src)
{
    expr_list_type res;
    std::transform(src.begin(), src.end(),
                   std::back_inserter(res),
                   [env](expr_ptr p) { return eval(env, p); });
    return res;
}

bool ListAccessor::optional(expr_ptr &dst)
{
    if (!has_more())
        return false;

    dst = *cur++;
    return true;
}

ListAccessor & operator >> (ListAccessor &src, expr_ptr dst)
{
    dst = std::move(src.required());
    return src;
}

expr_ptr ListAccessor::required()
{
    if (cur == end)
        throw Error("Required param is absent");

    return *cur++;
}

template
std::basic_ostream<char> & operator <<
(std::basic_ostream<char> &dst, Expr const &src);

expr_ptr List::do_eval(env_ptr env, expr_ptr p)
{
    expr_list_type res;
    auto self = std::dynamic_pointer_cast<List>(p);
    if (!self)
        return mk_nil();
    auto &src = self->items;
    for (auto &v : src)
        res.push_back(eval(env, v));
    return std::make_shared<List>(std::move(res));
}

} // notlisp
} // cor
