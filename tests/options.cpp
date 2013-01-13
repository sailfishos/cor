#include <cor/util.hpp>
#include <cor/options.hpp>
#include <tut/tut.hpp>
#include <string>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <array>

using std::string;
using std::runtime_error;

namespace tut
{

/**
 * Testing ensure_equals() method.
 */
struct options_test
{
    virtual ~options_test()
    {
    }
};

typedef test_group<options_test> tf;
typedef tf::object object;
tf ensure_op_test("options");

enum test_ids {
    tid_only_exec = 1,
    tid_exec_param,
    tid_short_unknown,
    tid_short,
    tid_short_wo_param,
    tid_short_w_param,
    tid_long_unknown_no_short,
    tid_long,
    tid_long_w_param,
    tid_long_w_param_eq,
    tid_long_w_long_param_eq

};

typedef cor::OptParse<std::string> option_parser_type;
using cor::concat;

void check_params
(std::vector<char const*> params,
 std::vector<char const *> const &argv,
 size_t params_count,
 std::initializer_list<std::pair<size_t, size_t> > argv_param)
{
    ensure_eq("params count", params_count, params.size());
    for (auto &i : argv_param)
        ensure_eq(concat("param #", i.first, "==", "argv #", i.second),
                  std::string(params.at(i.first)), argv.at(i.second));
}

void check_options
(option_parser_type::map_type const &options,
 std::initializer_list<option_parser_type::item_type> expected)
{
    ensure_eq("options count", expected.size(), options.size());
    auto p = options.begin();
    for (auto &i : expected) {
        ensure_eq("option name", std::string(i.first), p->first);
        ensure_eq("option value", std::string(i.second), p->second);
        ++p;
    }
}

template<>
template<>
void object::test<tid_only_exec>()
{
    std::vector<char const*> argv({{"test"}});
    option_parser_type options({}, {}, {}, {});
    option_parser_type::map_type opts;
    std::vector<char const*> params;
    options.parse(argv.size(), &argv[0], opts, params);
    check_params(params, argv, 1, {{0, 0}});
    check_options(opts, {});
}

template<>
template<>
void object::test<tid_exec_param>()
{
    std::vector<char const*> argv({{"test", "x"}});
    option_parser_type options({}, {}, {}, {});
    option_parser_type::map_type opts;
    std::vector<char const*> params;
    options.parse(argv.size(), &argv[0], opts, params);
    check_params(params, argv, 2, {{0, 0}, {1, 1}});
    check_options(opts, {});
}

/// if option is not processed it goes into params
template<>
template<>
void object::test<tid_short_unknown>()
{
    std::vector<char const*> argv({{"test", "-x"}});
    option_parser_type options({}, {}, {}, {});
    option_parser_type::map_type opts;
    std::vector<char const*> params;
    options.parse(argv.size(), &argv[0], opts, params);
    check_params(params, argv, 2, {{0, 0}, {1, 1}});
    check_options(opts, {});
}

/// short option without param
template<>
template<>
void object::test<tid_short>()
{
    std::vector<char const*> argv({{"test", "-x"}});
    option_parser_type options({{'x', "o-x"}}, {}, {}, {});
    option_parser_type::map_type opts;
    std::vector<char const*> params;
    options.parse(argv.size(), &argv[0], opts, params);
    check_params(params, argv, 1, {{0, 0}});
    check_options(opts, {{"o-x", ""}});
}

/// short option without param, followed by param
template<>
template<>
void object::test<tid_short_wo_param>()
{
    std::vector<char const*> argv({{"test", "-x", "X"}});
    option_parser_type options({{'x', "o-x"}}, {}, {}, {});
    option_parser_type::map_type opts;
    std::vector<char const*> params;
    options.parse(argv.size(), &argv[0], opts, params);
    check_params(params, argv, 2, {{0, 0}, {1, 2}});
    check_options(opts, {{"o-x", ""}});
}

/// short option with param
template<>
template<>
void object::test<tid_short_w_param>()
{
    std::vector<char const*> argv({{"test", "-x", "X"}});
    option_parser_type options({{'x', "o-x"}}, {}, {"o-x"}, {});
    option_parser_type::map_type opts;
    std::vector<char const*> params;
    options.parse(argv.size(), &argv[0], opts, params);
    check_params(params, argv, 1, {{0, 0}});
    check_options(opts, {{"o-x", "X"}});
}

/// if long option is not processed it goes into params, also it
/// should not be relaced by short option
template<>
template<>
void object::test<tid_long_unknown_no_short>()
{
    std::vector<char const*> argv({{"test", "--x", "X"}});
    option_parser_type options({{'x', "o-x"}}, {}, {"o-x"}, {});
    option_parser_type::map_type opts;
    std::vector<char const*> params;
    options.parse(argv.size(), &argv[0], opts, params);
    check_params(params, argv, 3, {{0, 0}, {1, 1}, {2, 2}});
    check_options(opts, {});
}

/// long option w/o param
template<>
template<>
void object::test<tid_long>()
{
    std::vector<char const*> argv({{"test", "--x", "X"}});
    option_parser_type options({{'x', "o-short-x"}},
                               {{"x", "o-long-x"}},
                               {},
                               {});
    option_parser_type::map_type opts;
    std::vector<char const*> params;
    options.parse(argv.size(), &argv[0], opts, params);
    check_params(params, argv, 2, {{0, 0}, {1, 2}});
    check_options(opts, {{"o-long-x", ""}});
}

/// long option with param
template<>
template<>
void object::test<tid_long_w_param>()
{
    std::vector<char const*> argv({{"test", "--x", "X"}});
    option_parser_type options({{'x', "o-short-x"}},
                               {{"x", "o-long-x"}},
                               {"o-long-x"},
                               {});
    option_parser_type::map_type opts;
    std::vector<char const*> params;
    options.parse(argv.size(), &argv[0], opts, params);
    check_params(params, argv, 1, {{0, 0}});
    check_options(opts, {{"o-long-x", "X"}});
}

template<>
template<>
void object::test<tid_long_w_param_eq>()
{
    std::vector<char const*> argv({{"test", "--x=X"}});
    option_parser_type options({{'x', "o-short-x"}},
                               {{"x", "o-long-x"}},
                               {"o-long-x"},
                               {});
    option_parser_type::map_type opts;
    std::vector<char const*> params;
    options.parse(argv.size(), &argv[0], opts, params);
    check_params(params, argv, 1, {{0, 0}});
    check_options(opts, {{"o-long-x", "X"}});
}

template<>
template<>
void object::test<tid_long_w_long_param_eq>()
{
    std::vector<char const*> argv({{"test", "--x=X-X"}});
    option_parser_type options({{'x', "o-short-x"}},
                               {{"x", "o-long-x"}},
                               {"o-long-x"},
                               {});
    option_parser_type::map_type opts;
    std::vector<char const*> params;
    options.parse(argv.size(), &argv[0], opts, params);
    check_params(params, argv, 1, {{0, 0}});
    check_options(opts, {{"o-long-x", "X-X"}});
}

}
