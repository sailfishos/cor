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

class BasicTestBuilder {
public:
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

    int enter_count = 0;
    int exit_count = 0;
    int depth = 0;
};

using cor::concat;

template<> template<>
void object::test<tid_empty>()
{
    class TestBuilder : public BasicTestBuilder {};
    auto test_with = [](std::string const &exp) {
        TestBuilder builder;
        std::string suffix(". parsing'" + exp + "'");
        std::istringstream in(exp);
        cor::sexp::parse(in, builder);
        ensure_eq("depth" + suffix, builder.depth, 0);
        ensure_eq("enters" + suffix, builder.enter_count, 1);
        ensure_eq("exits" + suffix, builder.exit_count, 1);
    };
    for (auto &v : { "()", "( )", "(\n)", " ()", "() ", "(\t)" })
        test_with(v);
}

template<> template<>
void object::test<tid_enclosured>()
{
    class TestBuilder : public BasicTestBuilder {};
    auto test_with = [](std::string const &exp) {
        TestBuilder builder;
        std::string suffix(". parsing'" + exp + "'");
        std::istringstream in(exp);
        cor::sexp::parse(in, builder);
        ensure_eq("depth" + suffix, builder.depth, 0);
        ensure_eq("enters" + suffix, builder.enter_count, 2);
        ensure_eq("exits" + suffix, builder.exit_count, 2);
    };
    for (auto &v : { "(())", "( ())", "(() )", "(\n())", "(()\n)" })
        test_with(v);
}

typedef std::pair<std::string, std::string> compare_type;

template <typename BuilderT>
static void test_with
(BuilderT &builder, std::string const &name, compare_type const &p,
 int enters = 0, int exits = 0, int depth = 0)
{
    std::string const &exp = p.first;
    std::string const &expected = p.second;
    std::string suffix(". parsing'" + exp + "'");
    std::istringstream in(exp);
    cor::sexp::parse(in, builder);
    ensure_eq("depth" + suffix, builder.depth, depth);
    ensure_eq("enters" + suffix, builder.enter_count, enters);
    ensure_eq("exits" + suffix, builder.exit_count, exits);
    ensure_eq(name + suffix, builder.data, expected);
};

template<> template<>
void object::test<tid_comment>()
{
    struct TestBuilder : public BasicTestBuilder {
        void on_comment(std::string &&s) {
            data = std::move(s);
        }
        std::string data;
    };

    TestBuilder builder;
    std::vector<compare_type> data
        = {{";", ""}, {";X", "X"}, {";X\n", "X"}};
    for (auto &v : data)
        test_with(builder, "comment", v);
}

template<> template<>
void object::test<tid_string>()
{
    struct TestBuilder : public BasicTestBuilder {
        void on_string(std::string &&s) {
            data = std::move(s);
        }
        std::string data;
    };
    TestBuilder builder;
    std::vector<compare_type> data
        = {{"\"\"", ""}, {"\"S\"", "S"}, {"\"\n\t \"", "\n\t "},
           {"\"(\"", "("}, {"\")\"", ")"}, {"\"\\\"\"", "\""},
           {"\"\\\n\"", "\n"}, {"\"\\\"r\"", "\"r"}};
    for (auto &v : data)
        test_with(builder, "string", v);
}

template<> template<>
void object::test<tid_atom>()
{
    struct TestBuilder : public BasicTestBuilder {
        void on_atom(std::string &&s) {
            data = std::move(s);
        }
        std::string data;
    };
    TestBuilder builder;
    std::vector<compare_type> data
        = {{"a", "a"}, {" b", "b"}, {"c ", "c"},
           {"1", "1"}, {"1.1", "1.1"}, {"-12.34", "-12.34"},
           {"=d@", "=d@"}, {"e\n", "e"} };
    for (auto &v : data)
        test_with(builder, "atom", v);

    std::vector<compare_type> data2
        = {{"a(", "a"}, {"a (", "a"}, {"(a", "a"}, {"(a", "a"}};
    for (auto &v : data2) {
        TestBuilder builder2;
        test_with(builder2, "atom", v, 1, 0, 1);
    }
}

}
