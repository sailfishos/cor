#include <cor/util.hpp>
#include <tut/tut.hpp>

#include "tests_common.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string>
#include <stdexcept>
#include <list>
#include <array>
#include <sstream>

using std::string;
using std::runtime_error;

enum class Rec1 { First, Second, Last_ = Second };
template <> struct RecordTraits<Rec1> {
    typedef std::tuple<std::string, int> type;
    RECORD_NAMES(Rec1, "First", "Second");
};

enum class Rec2 { First, Second, Last_ = Second };
template <> struct RecordTraits<Rec2> {
    typedef std::tuple<std::string, Record<Rec1> > type;
    RECORD_NAMES(Rec2, "First", "Second");
};

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
    tid_handle_args,
    tid_close,
    tid_move_handle,
    tid_generic_handle,
    tid_tagged_storage,
    tid_string_join,
    tid_string_split,
    tid_tuple,
    tid_enum,
    tid_enum_struct,
    tid_ptr_traits
};

class TestTraits
{
public:
    typedef int handle_type;
    TestTraits() { c_ += 1; }
    TestTraits(int arg) { arg_ = arg; }
    void close_(int v) { c_ -= 1; }
    bool is_valid_(int v) const { return v >= 0; }
    int invalid_() const { return -1; }

    static int c_;
    int arg_;
};

int TestTraits::c_ = 0;

typedef cor::Handle<TestTraits> test_type;

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

    ensure_throws<cor::Error>
        ("Throws on invalid handle"
         , []() {
            test_type h(-1, cor::only_valid_handle);
        });
}

struct DerivedHandle : public test_type
{
    template <typename ... Args>
    DerivedHandle(handle_type v, Args&&... args) : test_type(v, args...) {}
    int get_arg() const { return this->arg_; }
};

template<> template<>
void object::test<tid_handle_args>()
{
    DerivedHandle h(13, 31);
    ensure_eq("handle", h.value(), 13);
    ensure_eq("w", h.get_arg(), 31);
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
    typedef Handle<GenericHandleTraits<int, -1> > generic_test_type;
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

template<> template<>
void object::test<tid_string_join>()
{
    using cor::join;
    std::list<std::string> src{"a", "bc", "d"};
    auto res = join(src.begin(), src.end(), "");
    ensure_eq("simple join using empty symbol", res, "abcd");
    res = join(src.begin(), src.end(), ".");
    ensure_eq("join using one symbol", res, "a.bc.d");
    res = join(src.begin(), src.end(), "<>");
    ensure_eq("join using 2 symbols", res, "a<>bc<>d");

    res = join(src, ".");
    ensure_eq("join container", res, "a.bc.d");

    std::list<std::string> empty_src;
    res = join(empty_src, ".");
    ensure_eq("join empty container", res, "");

    std::list<std::string> src2{"", ""};
    res = join(src2, ".");
    ensure_eq("join empty strings", res, ".");

    std::array<std::string, 2> arr{{"a", "c"}};
    res = join(arr, "b");
    ensure_eq("join array", res, "abc");
}

template<> template<>
void object::test<tid_string_split>()
{
    using cor::split;
    typedef std::list<std::string> dst_type;

    dst_type dst;
    split("a.b", ".", std::back_inserter(dst));
    dst_type expected{"a", "b"};
    ensure_eq("basic split", dst, expected);

    dst.clear();
    split(".", ".", std::back_inserter(dst));
    dst_type expected0{"", ""};
    ensure_eq("split 2 empty parts", dst, expected0);

    dst.clear();
    split("a.b/c", "/.", std::back_inserter(dst));
    dst_type expected2{"a", "b", "c"};
    ensure_eq("split using 2 symbols", dst, expected2);

    dst.clear();
    split("a.b/c-", "/.-", std::back_inserter(dst));
    ensure_eq("split: 3 symbols and empty tail", dst
              , dst_type{"a", "b", "c", ""});

    dst.clear();
    split("", ".", std::back_inserter(dst));
    dst_type expected4{""};
    ensure_eq("empty string", dst, dst_type{""});
    
}

template <size_t N, class CharT, class ... Args>
void out_tuple(std::basic_ostream<CharT> &dst
               , std::tuple<Args...> const &src
               , cor::TupleSelector<N, N> const &selector)
{
    dst << selector.get(src);
}

template <size_t N, size_t P, class CharT, class ... Args>
void out_tuple(std::basic_ostream<CharT> &dst
               , std::tuple<Args...> const &src
               , cor::TupleSelector<N, P> const &selector)
{
    dst << selector.get(src) << ", ";
    out_tuple(dst, src, selector.next());
}

template <class CharT, class ... Args>
std::basic_ostream<CharT>& operator <<
(std::basic_ostream<CharT> &dst, std::tuple<Args...> const &src)
{
    dst << "(";
    out_tuple(dst, src, cor::selector(src));
    dst << ")";
    return dst;
}

template<> template<>
void object::test<tid_tuple>()
{
    typedef std::tuple<int> t1_type;
    using cor::apply_if_changed;
    t1_type v1{1}, v2{1};
    auto actions1 = std::make_tuple([](int) {fail("1");});
    apply_if_changed(v1, v2, actions1);

    t1_type v3{3};
    int res1 = 0;
    auto set_res1 = [&res1](int v) { res1 = v; };
    auto actions2 = std::make_tuple(set_res1);
    apply_if_changed(v1, v3, actions2);
    ensure_eq("function was called", res1, std::get<0>(v3));
    ensure_ne("unchaged v1 and v3", v1, v3);

    typedef std::tuple<std::string, int> t2_type;
    t2_type si1{"same", 6}, si2{"same", 6};
    auto a1_si = std::make_tuple
        ([](std::string const&) {fail("a1_si 1");},
         [](int) {fail("a1_si 2");});
    apply_if_changed(si1, si2, a1_si);

    t2_type si3{"other", 6};
    std::string s("x");
    auto set_s = [&s](std::string const &v) { s = v;};
    auto a2_si = std::make_tuple
        (set_s, [](int) {fail("a2_si i");});
    apply_if_changed(si1, si3, a2_si);
    ensure_eq("1 function was called", s, std::get<0>(si3));
    ensure_ne("unchaged si1&3", si1, si3);

    using cor::copy_apply_if_changed;

    s = "X";
    copy_apply_if_changed(si1, si3, a2_si);
    ensure_eq("1 function was called", s, std::get<0>(si3));
    ensure_eq("synchronized si1&3", si1, si3);

    std::ostringstream ss;
    typedef std::tuple<int, double, std::string, std::string> test_copy_apply_type;
    test_copy_apply_type ids1{98, .12, "eof", "dummy0"};
    test_copy_apply_type ids2{76, .54, "dod", "dummy1"};
    auto a_ids = std::make_tuple
        ([&ss](int const &v) { ss << v; },
         [&ss](double const &v) { ss << v; },
         [&ss](std::string const &v) { ss << v; },
         cor::dummy<std::string>);
    auto res = copy_apply_if_changed(ids1, ids2, a_ids);
    ensure_eq("props to be copied", res, 4);
    ensure_eq("synchronized ids", ids1, ids2);
    ensure_eq("output ids", ss.str(), "760.54dod");

    copy_apply_if_changed(ids1, ids2, a_ids);
    ensure_eq("still synchronized ids", ids1, ids2);
    ensure_eq("no output 2nd time ids", ss.str(), "760.54dod");
    
    
}

template<> template<>
void object::test<tid_enum>()
{
    enum class E1 { A = 0, B = 1, Last_ = B };
    ensure_eq("Enum size", cor::enum_size<E1>(), 2);
    ensure_eq("Enum index", cor::enum_index(E1::B), 1);
    std::tuple<int, std::string> t{1, "2"};
    ensure_eq("tuple enum", std::get<0>(t), std::get<cor::enum_index(E1::A) >(t));
}

template<> template<>
void object::test<tid_enum_struct>()
{
    Record<Rec1> test{"value1", 12};
    ensure_eq("1st field", test.get<Rec1::First>(), "value1");
    ensure_eq("2nd field", test.get<Rec1::Second>(), 12);
    test.get<Rec1::First>() = "new_value";
    ensure_eq("1st field after assignment", test.get<Rec1::First>(), "new_value");
    test.get<Rec1::Second>() = 123;
    ensure_eq("2nd field after assignment", test.get<Rec1::Second>(), 123);
    std::stringstream ss;
    ss << test;
    ensure_eq("Output", ss.str(), "(First=new_value, Second=123)");
    Record<Rec2> test2{"r2", Record<Rec1>{"v1", 3}};
    ss.str("");
    ss << test2;
    ensure_eq("Output", ss.str(), "(First=r2, Second=(First=v1, Second=3))");
}

template<> template<>
void object::test<tid_ptr_traits>()
{
    static bool a = false, b = false;
    struct Test { ~Test() { a = true; }};
    typedef typename cor::ptr_traits<Test>::unique_ptr ptr;
    do {
        ptr p(new Test(), [](Test *p) { b = true; delete p; });
    } while (0);
    ensure("Not deleted", a);
    ensure("Custom deleter is not called", b);
}

}
