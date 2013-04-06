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
    tid_generic_handle,
    tid_tagged_storage
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

struct TestTagged
{
    static int obj_count;
    TestTagged(int v, std::string const &v2)
        : i(v), s(v2) { ++obj_count; }

    TestTagged(TestTagged &src)
        : i(src.i), s(src.s) { ++obj_count; }

    TestTagged(TestTagged &&src)
        : i(src.i), s(src.s) {
        src.i = -1;
        src.s = "";
        ++obj_count;
    }

    ~TestTagged() { --obj_count; }

    int i;
    std::string s;
};

struct TestTagged2 : public TestTagged
{
    TestTagged2() : TestTagged(7, "2nd") {}
};

int TestTagged::obj_count = 0;

template<> template<>
void object::test<tid_tagged_storage>()
{
    using namespace cor;
    static const unsigned v1 = 33;
    static const std::string v2 = "some string";

    intptr_t h = new_tagged_handle<TestTagged>(v1, v2);
    ensure("Not null on creation", h);
    ensure_eq("Constructed", TestTagged::obj_count, 1);

    TestTagged *p1 = tagged_handle_pointer<TestTagged>(h);
    ensure("Not null", !!p1);

    TestTagged2 *p2 = tagged_handle_pointer<TestTagged2>(h);
    ensure("Null", !p2);

    ensure_eq("V1 is initialized", p1->i, v1);
    ensure_eq("V2 is initialized", p1->s, v2);

    // copying/moving
    intptr_t h2 = new_tagged_handle<TestTagged>(*p1);
    ensure_eq("2nd is constructed", TestTagged::obj_count, 2);
    ensure("Copy is not null on creation", h2);
    TestTagged *p1_2 = tagged_handle_pointer<TestTagged>(h2);
    ensure("Copy is not null", !!p1_2);

    ensure_eq("Src V1 is initialized", p1->i, v1);
    ensure_eq("Src V2 is initialized", p1->s, v2);

    ensure_eq("V1 copy is ok", p1_2->i, p1->i);
    ensure_eq("V2 copy is ok", p1_2->s, p1_2->s);

    delete_tagged_handle<TestTagged>(h);
    ensure_eq("Destructor was called", TestTagged::obj_count, 1);

    delete_tagged_handle<TestTagged>(h2);
    ensure_eq("2nd destructor was called", TestTagged::obj_count, 0);

    // checking for derived and base class clashing
    h = new_tagged_handle<TestTagged2>();
    ensure("Not null on creation", h);
    ensure_eq("2 is constructed", TestTagged::obj_count, 1);

    p2 = tagged_handle_pointer<TestTagged2>(h);
    ensure("2nd not null", !!p2);

    p1 = tagged_handle_pointer<TestTagged>(h);
    ensure("1st null", !p1);

    ensure_eq("2nd: V1 is initialized", p2->i, 7);
    ensure_eq("2nd: V2 is initialized", p2->s, "2nd");
    ensure_eq("Destructor is not called yet", TestTagged::obj_count, 1);
    // check it is not mistakenly deleted

    delete_tagged_handle<TestTagged>(h);
    ensure_eq("Check deleting by mistake", TestTagged::obj_count, 1);

    delete_tagged_handle<TestTagged2>(h);
    ensure_eq("Destructor was called", TestTagged::obj_count, 0);

}

}
