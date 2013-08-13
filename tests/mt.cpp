#include <cor/mt.hpp>
#include <cor/util.hpp>
#include <tut/tut.hpp>
#include <iostream>
#include <unistd.h>

namespace tut
{

struct mt_test
{
    virtual ~mt_test()
    {
    }
};

typedef test_group<mt_test> tf;
typedef tf::object object;
tf cor_mt_test("mt");

enum test_ids {
    tid_basic_future_wake_after =  1,
    tid_basic_future_wake_before
};

template <typename Pred>
bool wait_while(Pred pred, unsigned timeout_msec)
{
    for (unsigned i = 0; pred(); ++i) {
        ::usleep(10);
        if (i == timeout_msec * 100)
            return false;
    }
    return true;
}

template<> template<>
void object::test<tid_basic_future_wake_after>()
{
    cor::Future f;
    enum {
        initial,
        before_wake,
        after_wake
    } stage = initial;

    std::mutex m;

    auto before = [&stage]() { stage = before_wake; };
    auto after = [&stage]() { stage = after_wake; };
    std::thread t = std::thread(f.wrap(before, after));
    auto join = cor::on_scope_exit([&t](){t.join();});

    ensure("Waiting too long for wake thread to exit"
           , wait_while([&stage](){ return stage != after_wake; }, 1000));

    auto res = f.wait(std::chrono::milliseconds(5000));
    ensure_ne("No timeout", (int)res, (int)std::cv_status::timeout);
}

template<> template<>
void object::test<tid_basic_future_wake_before>()
{
    // cor::Future f;
    // enum {
    //     initial,
    //     before_wake,
    //     after_wake
    // } stage = initial;
    // auto before = [&stage, &m]() { stage = before_wake; };
    // auto after = [&stage]() { stage = after_wake; };
    // std::thread t = std::thread(f.wrap(before, after));
    // auto join = cor::on_scope_exit([&t](){t.join();});
    // for (int i = 0; stage != after_wake; ++i) {
    //     ::usleep(10);
    //     if (i == 100000)
    //         fail("Waiting too long for wake thread to exit");
    // }

    // auto res = f.wait(std::chrono::milliseconds(5000));
    // ensure_ne("No timeout", (int)res, (int)std::cv_status::timeout);
}

}
