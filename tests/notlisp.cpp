#include <cor/notlisp.hpp>
#include <cor/sexp.hpp>
#include <cor/util.hpp>
#include <tut/tut.hpp>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

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
    tid_wrong_expr
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

    env_ptr env(new Env({}));
    Interpreter interpreter(env);
    std::istringstream in("()");
    ensure_throws<cor::sexp::ErrorWrapper>(
        "Non-executable should fail on evaluation",
        [&in, &interpreter]() { cor::sexp::parse(in, interpreter); });
}

}
