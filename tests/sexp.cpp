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
    tid_string
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

template<> template<>
void object::test<tid_comment>()
{
    struct TestBuilder : public BasicTestBuilder {
        void on_comment(std::string &&s) {
            comment = std::move(s);
        }
        std::string comment;
    };
    typedef std::pair<std::string, std::string> data_type;

    auto test_with = [](data_type const &p) {
        std::string const &exp = p.first;
        std::string const &expected = p.second;
        TestBuilder builder;
        std::string suffix(". parsing'" + exp + "'");
        std::istringstream in(exp);
        cor::sexp::parse(in, builder);
        ensure_eq("depth" + suffix, builder.depth, 0);
        ensure_eq("enters" + suffix, builder.enter_count, 0);
        ensure_eq("exits" + suffix, builder.exit_count, 0);
        ensure_eq("comment" + suffix, builder.comment, expected);
    };
    std::vector<data_type> data
        = {{";", ""}, {";X", "X"}, {";X\n", "X"}};
    for (auto &v : data)
        test_with(v);
}

template<> template<>
void object::test<tid_string>()
{
    struct TestBuilder : public BasicTestBuilder {
        void on_string(std::string &&s) {
            str = std::move(s);
        }
        std::string str;
    };
    typedef std::pair<std::string, std::string> data_type;

    auto test_with = [](data_type const &p) {
        std::string const &exp = p.first;
        std::string const &expected = p.second;
        TestBuilder builder;
        std::string suffix(". parsing'" + exp + "'");
        std::istringstream in(exp);
        cor::sexp::parse(in, builder);
        ensure_eq("depth" + suffix, builder.depth, 0);
        ensure_eq("enters" + suffix, builder.enter_count, 0);
        ensure_eq("exits" + suffix, builder.exit_count, 0);
        ensure_eq("string" + suffix, builder.str, expected);
    };
    std::vector<data_type> data
        = {{"\"\"", ""}, {"\"X\"", "X"}, {"\"\n\t \"", "\n\t "},
           {"\"(\"", "("}, {"\")\"", ")"}, {"\"\\\"\"", "\""}};
    for (auto &v : data)
        test_with(v);
}

}
