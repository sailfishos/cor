#include <cor/trace.hpp>
#include <tut/tut.hpp>

namespace tut
{

struct trace_test
{
    virtual ~trace_test()
    {
    }
};

typedef test_group<trace_test> tf;
typedef tf::object object;
tf cor_trace_test("tracing");

enum test_ids {
    tid_trace_print =  1
    , tid_trace_print_ge
    , tid_trace_log
    , tid_trace_level
};

template<> template<>
void object::test<tid_trace_print>()
{
    using namespace cor::debug;
    std::stringstream dst;
    std::string v("1");
    print_to(dst, v);
    ensure_eq("Should be the same", dst.str(), v);
    dst.str("");
    print_line_to(dst, v);
    ensure_eq("Should be the with eol", dst.str(), v + "\n");
}

template<> template<>
void object::test<tid_trace_print_ge>()
{
    using namespace cor::debug;
    std::stringstream dst;
    std::string v("1");
    print_line_ge(dst, Level::Critical, v);
    ensure_eq("Should be the same", dst.str(), v + "\n");
}

template<> template<>
void object::test<tid_trace_log>()
{
    using namespace cor::debug;
    std::stringstream dst;
    std::string v("1");

    Log log{"Test", dst};
    log.critical(v);
    ensure_eq("Expecting critical log", dst.str(), log.prefix() + v + "\n");

    dst.str("");
    std::string v2("2");
    log.critical(v, v2);
    ensure_eq("Expecting combined critical log", dst.str(), log.prefix() + v + v2 + "\n");

    dst.str("");
    log.critical(Trace::NoEol);
    ensure_eq("Expecting combined critical log", dst.str(), log.prefix());
}

template<> template<>
void object::test<tid_trace_level>()
{
    using namespace cor::debug;
    std::stringstream dst;
    std::string v("1"), v2("2");
    
    Log log{"TestLevel", dst};
    log.critical(v);
    ensure_eq("Critical is logged", dst.str(), log.prefix() + v + "\n");

    log.debug(v2);
    ensure_eq("Only critical is logged", dst.str(), log.prefix() + v + "\n");
    
    dst.str("");
    level(Level::Debug);
    log.critical(v);
    auto expected = log.prefix() + v + "\n";
    ensure_eq("Critical is still logged", dst.str(), expected);
    log.debug(v2);
    expected += (log.prefix() + v2 + "\n");
    ensure_eq("Debug should be added", dst.str(), expected);
}

}
