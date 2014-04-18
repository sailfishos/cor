#include <cor/notlisp.hpp>
#include <cor/sexp.hpp>
#include <cor/util.hpp>
#include <tut/tut.hpp>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <tuple>
#include <string>
#include <sstream>
#include <stdexcept>

namespace tut
{

struct notlisp_test
{
    virtual ~notlisp_test()
    {
    }
};

typedef test_group<notlisp_test> tf;
typedef tf::object object;
tf cor_notlisp_test("notlisp");

enum test_ids {
    tid_noinput = 1,
    tid_optional,
    tid_required,
    tid_types,
    tid_const,
    tid_wrong_expr,
    tid_simple_fn,
    tid_list
};

template<> template<>
void object::test<tid_noinput>()
{
    using namespace cor::notlisp;

    env_ptr env(new Env({}));
    Interpreter interpreter(env);
    std::istringstream in("");
    cor::sexp::parse(in, interpreter);
    ListAccessor res(interpreter.results());
    ensure_throws<Error>
        ("Nothing is expected", [&res]() {long d; res.required(to_long, d); });
}

template<> template<>
void object::test<tid_optional>()
{
    using namespace cor::notlisp;

    env_ptr env(new Env({}));
    Interpreter interpreter(env);
    std::istringstream in("4");
    cor::sexp::parse(in, interpreter);
    ListAccessor res(interpreter.results());
    expr_ptr e;
    ensure_eq("there should be expr", res.optional(e), true);
    ensure_eq("non-null expr", !!e, true);
    ensure_eq("expr type", e->type(), Expr::Integer);
    ensure_eq("expr type", (long)*e, 4);
    ensure_eq("no expressions left", res.optional(e), false);
    ensure_eq("not has_more()", res.has_more(), false);
}

template<> template<>
void object::test<tid_required>()
{
    using namespace cor::notlisp;

    env_ptr env(new Env({}));
    Interpreter interpreter(env);
    std::istringstream in("4");
    cor::sexp::parse(in, interpreter);
    ListAccessor res(interpreter.results());
    long i = 0;
    res.required(to_long, i);
    ensure_throws<Error>("No params left", [&res]() { res.required(); });
}

template<> template<>
void object::test<tid_types>()
{
    using namespace cor::notlisp;

    env_ptr env(new Env({}));
    Interpreter interpreter(env);
    std::istringstream in("1.2 3 \"X\"");
    cor::sexp::parse(in, interpreter);
    ListAccessor res(interpreter.results());
    double a = 0;
    long b = 0;
    std::string c;
    res.required(to_double, a).required(to_long, b).required(to_string, c);
    ensure_eq("double", a, 1.2);
    ensure_eq("int", b, 3);
    ensure_eq("string", c, "X");
    ensure_throws<Error>("No more params", [&res]() { res.required(); });
}

template<> template<>
void object::test<tid_const>()
{
    using namespace cor::notlisp;

    env_ptr env(new Env({mk_const("x", 3), mk_const("y", 1.2)}));
    Interpreter interpreter(env);
    std::istringstream in("x y");
    cor::sexp::parse(in, interpreter);
    ListAccessor res(interpreter.results());
    long i = 0;
    double r = 0;
    res.required(to_long, i).required(to_double, r);
    ensure_eq("int", i, 3);
    ensure_eq("double", r, 1.2);
}

template<> template<>
void object::test<tid_wrong_expr>()
{
    using namespace cor::notlisp;

    env_ptr env(new Env({}));
    auto exec = [&env] (std::string const &data, size_t pos) {
        std::istringstream in(data);
        Interpreter interpreter(env);
        auto parse = [&in, &interpreter]() {
            cor::sexp::parse(in, interpreter);
        };

        ensure_throws<Error>("Should fail on evaluation", parse);
        ensure_eq("correct position", in.tellg(), pos);
    };
    std::vector<std::pair<std::string, size_t>> data =
        {{"()", 2}, {"(e)", 3}};
    for (auto &v : data)
        exec(v.first, v.second);
}

template<> template<>
void object::test<tid_simple_fn>()
{
    using namespace cor::notlisp;
    using cor::sexp::parse;

    typedef std::function<void (ListAccessor &)> check_type;
    typedef std::tuple<std::string, env_ptr, check_type> data_type;

    auto exec = [] (data_type &data) {
        std::istringstream in(std::get<0>(data));
        Interpreter interpreter(std::get<1>(data));
        parse(in, interpreter);
        ListAccessor res(interpreter.results());
        std::get<2>(data)(res);
    };

    lambda_type fn = [](env_ptr, expr_list_type &params) {
        ListAccessor src(params);
        ensure_eq("no fn params left", src.has_more(), false);
        return mk_string("X");
    };
    check_type check_fn = [](ListAccessor &src) {
        std::string s;
        src.required(to_string, s);
        ensure_eq("fn result", s, "X");
        ensure_eq("no fn results left", src.has_more(), false);
    };

    lambda_type fn1 = [](env_ptr, expr_list_type &params) {
        std::string s;
        ListAccessor src(params);
        src.required(to_string, s);
        ensure_eq("no fn1 params left", src.has_more(), false);
        return mk_string(s + "B");
    };
    check_type check_fn1 = [](ListAccessor &src) {
        std::string s;
        src.required(to_string, s);
        ensure_eq("fn result", s, "AB");
        ensure_eq("no fn1 results left", src.has_more(), false);
    };

    std::vector<data_type> data =
        {std::make_tuple(
                "(fn)",
                mk_env({mk_record("fn", fn)}),
                check_fn),
         std::make_tuple(
             "(fn  \"A\")",
             mk_env({mk_record("fn", fn1)}),
             check_fn1)
        };
    for (auto &v : data) exec(v);
}

template<> template<>
void object::test<tid_list>()
{
    using namespace cor::notlisp;
    using cor::sexp::parse;

    env_ptr env(new Env({
                mk_record("list", [](env_ptr, expr_list_type &params) {
                        return mk_list(params); }),
                    }));

    auto exec = [&env] (std::string const &src,
                        std::function<void (ListAccessor &)> check) {
        std::istringstream in(src);
        Interpreter interpreter(env);
        parse(in, interpreter);
        ListAccessor res(interpreter.results());
        check(res);
    };

    exec("(list)", [](ListAccessor &res) {
            auto alist = res.required<List>();
            ensure("Expecting List", alist.get());
            ensure_eq("Empty list", alist->items.size(), 0);
            ListAccessor params(alist->items);
            ensure_throws<Error>("Nothing is expected",
                                 [&params]() { params.required(); });
        });
    exec("(list 1 2)", [](ListAccessor &res) {
            auto alist = res.required<List>();
            ensure("Expecting List", alist.get());
            ensure_eq("List with 2 items", alist->items.size(), 2);
            int vbefore = 0;
            rest(alist->items, [&vbefore](expr_ptr e) {
                    ensure_eq("List of integers", e->type(), Expr::Integer);
                    long vnow = -1;
                    to_long(e, vnow);
                    ensure_eq("List of consequent integers", vnow, vbefore + 1);
                    vbefore = vnow;
                    return true;
                });
        });

    exec("(list 1 \"2\" list)", [](ListAccessor &res) {
            auto alist = res.required<List>();
            ensure("Expecting List", alist.get());
            ensure_eq("List with 3 items", alist->items.size(), 3);
            ListAccessor params(alist->items);
            long l = -1;
            std::string s;
            params.required(to_long, l);
            ensure_eq("1st int", l, 1);
            params.required(to_string, s);
            ensure_eq("2nd str", s, "2");
            auto e = params.required();
            ensure_eq("Last is symbol", e->type(), Expr::Function);
            ensure_throws<Error>("Nothing is expected",
                                 [&params]() { params.required(); });
        });
}

}
