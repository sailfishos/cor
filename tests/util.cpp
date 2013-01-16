#include <cor/util.hpp>
#include <cor/pipe.hpp>
#include <tut/tut.hpp>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string>
#include <stdexcept>

using std::string;
using std::runtime_error;

namespace tut
{

struct util_test
{
    virtual ~util_test()
    {
    }
};

typedef test_group<util_test> tf;
typedef tf::object object;
tf cor_util_test("util");

enum test_ids {
    tid_basic_handle = 1,
    tid_close,
    tid_move_handle,
    tid_generic_handle
};

class TestTraits
{
public:
    TestTraits() { c_ += 1; }
    void close_(int v) { c_ -= 1; }
    bool is_valid_(int v) const { return v >= 0; }
    int invalid_() const { return -1; }

    static int c_;
};

int TestTraits::c_ = 0;

typedef cor::Handle<int, TestTraits> test_type;

template<> template<>
void object::test<tid_basic_handle>()
{
    int expected_c = 0;
    do {
        TestTraits::c_ = 0;
        test_type h(2);
        expected_c += 1;
        ensure_eq("counting", TestTraits::c_, expected_c);
        ensure_eq("handle", h.value(), 2);
    } while (0);
    expected_c -= 1;
    ensure_eq("zero counter", TestTraits::c_, expected_c);
}

template<> template<>
void object::test<tid_close>()
{
    int expected_c = 0;
    do {
        TestTraits::c_ = 0;
        test_type h(17);
        expected_c += 1;
        ensure_eq("counting", TestTraits::c_, expected_c);
        ensure_eq("handle", h.value(), 17);
        h.close();
        ensure_eq("handle after closing", h.value(), -1);
        ensure_eq("invalid after closing", h.is_valid(), false);
    } while (0);
    expected_c -= 1;
    ensure_eq("zero counter", TestTraits::c_, expected_c);
}

template<> template<>
void object::test<tid_move_handle>()
{
    int expected_c = 0;
    do {
        TestTraits::c_ = 0;
        test_type h(2);
        expected_c += 1;
        test_type h2(std::move(h));
        expected_c += 1;
        ensure_eq("h2 ctor", TestTraits::c_, expected_c);
        ensure_eq("invalidated handle #1", h.value(), -1);
        ensure_eq("moved handle", h2.value(), 2);
        test_type h3;
        expected_c += 1;
        ensure_eq("h3 ctor", TestTraits::c_, expected_c);
        h3 = std::move(h2);
        ensure_eq("invalidated handle #2", h2.value(), -1);
        ensure_eq("moved handle", h3.value(), 2);
    } while (0);
    expected_c -= 1;
    ensure_eq("zero counter", TestTraits::c_, expected_c);
}


template<> template<>
void object::test<tid_generic_handle>()
{
    using namespace cor;
    typedef Handle<int, GenericHandleTraits<int, -1> > generic_test_type;
    int counter = 1;
    do {
        generic_test_type h(13, [&counter](int v) { --counter; });
        ensure_eq("valid", h.is_valid(), true);
        ensure_eq("set correctly", h.value(), 13);
        ensure_eq("counter is ok", counter, 1);
    } while (0);
    ensure_eq("closed once", counter, 0);
}

}
