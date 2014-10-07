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
    std::list<std::string> traces1, traces2;
    try {
        fn1();
    } catch (cor::Error const &e) {
        depth1 = e.trace.end() - e.trace.begin();
        std::for_each(e.trace.begin(), e.trace.end()
                      , [&traces1](char const *p) {
                          traces1.push_back(p);
                      });
    }
    try {
        fn2();
    } catch (cor::Error const &e) {
        depth2 = e.trace.end() - e.trace.begin();
        std::for_each(e.trace.begin(), e.trace.end()
                      , [&traces2](char const *p) {
                          traces2.push_back(p);
                      });
    }
    ensure_eq("Similar depths", depth1, depth2);
    auto it1 = traces1.begin();
    auto it2 = traces2.begin();
    size_t depth = 0;
    for (;it1 != traces1.end() && it2 != traces2.end();
         ++it2, ++it1) {
        if (*it1 != *it2)
            break;
        ++depth;
    };
    // 1) Backtrace 2) Error 3) fn3 4) fn2
    ensure_eq("Backtrace is different after 4 levels", depth, 4);
}

}
