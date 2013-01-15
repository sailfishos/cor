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

struct sexp_test
{
    virtual ~sexp_test()
    {
    }
};

typedef test_group<sexp_test> tf;
typedef tf::object object;
tf cor_sexp_test("sexp");

enum test_ids {
    tid_empty = 1,
    tid_enclosured,
    tid_comment,
    tid_string,
    tid_atom
};

namespace sexp = cor::sexp;

class BasicTestHandler : public sexp::AbstractHandler {
public:

    BasicTestHandler()
        : enter_count(0), exit_count(0), depth(0), is_eof(false)
    {}

    void on_list_begin() {
        ++enter_count; ++depth;
    }
    void on_list_end() {
        ++exit_count; --depth;
    }
    void on_comment(std::string &&s) {
        throw cor::Error("no comments expected, got %s", s.c_str());
    }
    void on_string(std::string &&s) {
        throw cor::Error("no string expected, got %s", s.c_str());
    }
    void on_atom(std::string &&s) {
        throw cor::Error("no atom expected, got %s", s.c_str());
    }

    void on_eof() { is_eof = true; }

    int enter_count;
    int exit_count;
    int depth;
    bool is_eof;
};

using cor::concat;

static void basic_test_with
(std::string const &exp, int enters, int exits, int depth)
{
    BasicTestHandler handler;
    std::string suffix(". parsing'" + exp + "'");
    std::istringstream in(exp);
    cor::sexp::parse(in, static_cast<sexp::AbstractHandler&>(handler));
    ensure_eq("depth" + suffix, handler.depth, depth);
    ensure_eq("enters" + suffix, handler.enter_count, enters);
    ensure_eq("exits" + suffix, handler.exit_count, exits);
    ensure_eq("eof" + suffix, handler.is_eof, true);
};

template<> template<>
void object::test<tid_empty>()
{
    for (auto &v : { "()", "( )", "(\n)", " ()", "() ", "(\t)" })
        basic_test_with(v, 1, 1, 0);
}

template<> template<>
void object::test<tid_enclosured>()
{
    for (auto &v : { "(())", "( ())", "(() )", "(\n())", "(()\n)" })
        basic_test_with(v, 2, 2, 0);
}

typedef std::pair<std::string, std::string> compare_type;

template <typename HandlerT>
static void test_with
(HandlerT &handler, std::string const &name, compare_type const &p,
 int enters = 0, int exits = 0, int depth = 0)
{
    std::string const &exp = p.first;
    std::string const &expected = p.second;
    std::string suffix(". parsing'" + exp + "'");
    std::istringstream in(exp);
    cor::sexp::parse(in, static_cast<sexp::AbstractHandler&>(handler));
    ensure_eq("depth" + suffix, handler.depth, depth);
    ensure_eq("enters" + suffix, handler.enter_count, enters);
    ensure_eq("exits" + suffix, handler.exit_count, exits);
    ensure_eq(name + suffix, handler.data, expected);
    ensure_eq("eof" + suffix, handler.is_eof, true);
};

template<> template<>
void object::test<tid_comment>()
{
    struct TestHandler : public BasicTestHandler {
        void on_comment(std::string &&s) {
            data = std::move(s);
        }
        std::string data;
    };

    TestHandler handler;
    std::vector<compare_type> data
        = {{";", ""}, {";X", "X"}, {";X\n", "X"}};
    for (auto &v : data)
        test_with(handler, "comment", v);
}

template<> template<>
void object::test<tid_string>()
{
    struct TestHandler : public BasicTestHandler {
        void on_string(std::string &&s) {
            data = std::move(s);
        }
        std::string data;
    };
    TestHandler handler;
    std::vector<compare_type> data
        = {{"\"\"", ""}, {"\"S\"", "S"}, {"\"\n\t \"", "\n\t "},
           {"\"(\"", "("}, {"\")\"", ")"}, {"\"\\\"\"", "\""},
           {"\"\\\n\"", "\n"}, {"\"\\\"r\"", "\"r"}};
    for (auto &v : data)
        test_with(handler, "string", v);
}

template<> template<>
void object::test<tid_atom>()
{
    struct TestHandler : public BasicTestHandler {
        void on_atom(std::string &&s) {
            data = std::move(s);
        }
        std::string data;
    };
    TestHandler handler;
    std::vector<compare_type> data
        = {{"a", "a"}, {" b", "b"}, {"c ", "c"},
           {"1", "1"}, {"1.1", "1.1"}, {"-12.34", "-12.34"},
           {"=d@", "=d@"}, {"e\n", "e"} };
    for (auto &v : data)
        test_with(handler, "atom", v);

    std::vector<compare_type> data2
        = {{"a(", "a"}, {"a (", "a"}, {"(a", "a"}, {"(a", "a"}};
    for (auto &v : data2) {
        TestHandler handler2;
        test_with(handler2, "atom", v, 1, 0, 1);
    }
}

}
