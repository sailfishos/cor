#include <cor/error.hpp>
#include <tut/tut.hpp>
#include <iostream>
#include <unistd.h>
#include <string>
#include <list>

namespace tut
{

struct error_test
{
    virtual ~error_test()
    {
    }
};

typedef test_group<error_test> tf;
typedef tf::object object;
tf cor_error_test("error processing");

enum test_ids {
    tid_error_dump =  1
};

static void fn3() { throw cor::Error("Test error"); }
static void fn2() { fn3(); }
static void fn1() { fn2(); }

template<> template<>
void object::test<tid_error_dump>()
{
    using namespace cor;
    size_t depth1, depth2;
    std::list<std::string> traces1;
    try {
        fn1();
    } catch (cor::Error const &e) {
        depth1 = e.trace.end() - e.trace.begin();
        std::for_each(e.trace.begin(), e.trace.end()
                      , [&](char const *p) {
                          traces1.push_back(p);
                      });
    }
    try {
        fn2();
    } catch (cor::Error const &e) {
        depth2 = e.trace.end() - e.trace.begin();
    }
    ensure_eq("Depth difference", depth1 - depth2, 1);
}

}
