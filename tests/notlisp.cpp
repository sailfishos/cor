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
    tid_values,
    tid_const,
    tid_wrong_expr,
    tid_simple_fn
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
void object::test<tid_values>()
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
    ensure_throws<Error>
        ("No more params", [&res]() {long d; res.required(to_long, d); });
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
    using cor::sexp::mk_parser;

    env_ptr env(new Env({}));
    auto exec = [&env] (std::string const &data, size_t pos) {
        std::istringstream in(data);
        auto parser(mk_parser(in));
        Interpreter interpreter(env);
        auto check_position = [&in, pos](Error const &e) {
            ensure_eq("correct position", in.tellg(), pos);
        };
        auto parse = [&parser, &interpreter]() {
            parser(interpreter);
        };

        ensure_throws_verify<Error>(
            "Non-executable should fail on evaluation",
            parse, check_position);
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
    using cor::sexp::mk_parser;

    typedef std::function<void (ListAccessor &)> check_type;
    typedef std::tuple<std::string, env_ptr, check_type> data_type;

    auto exec = [] (data_type &data) {
        std::istringstream in(std::get<0>(data));
        auto parser(mk_parser(in));
        Interpreter interpreter(std::get<1>(data));
        parser(interpreter);
        ListAccessor res(interpreter.results());
        std::get<2>(data)(res);
    };

    lambda_type fn = [](env_ptr, expr_list_type &params) {
        return mk_string("X");
    };
    check_type check_fn = [](ListAccessor &src) {
        std::string s;
        src.required(to_string, s);
        ensure_eq("fn result", s, "X");
    };
    std::vector<data_type> data =
        {std::make_tuple(
                "(fn)",
                mk_env({mk_record("fn", fn)}),
                check_fn),
        };
    for (auto &v : data) exec(v);
}

}
