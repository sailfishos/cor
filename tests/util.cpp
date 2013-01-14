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
    tid_move_handle
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
    TestTraits::c_ = 0;
    while (0) {
        test_type h(2);
        ensure_eq("counting", TestTraits::c_, 1);
        ensure_eq("handle", h.value(), 2);
    }
    ensure_eq("zero counter", TestTraits::c_, 0);
}

template<> template<>
void object::test<tid_move_handle>()
{
    TestTraits::c_ = 0;
    while (0) {
        test_type h(2);
        test_type h2(std::move(h));
        ensure_eq("handle", h.value(), -1);
        ensure_eq("moved handle", h2.value(), 2);
        test_type h3;
        h3 = std::move(2);
        ensure_eq("handle", h2.value(), -1);
        ensure_eq("moved handle", h3.value(), 2);
    }
    ensure_eq("zero counter", TestTraits::c_, 0);
}

}
