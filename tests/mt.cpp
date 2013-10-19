#include <cor/mt.hpp>
#include <cor/util.hpp>
#include <tut/tut.hpp>

#include "tests_common.hpp"

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
    , tid_completion
    , tid_task_queue
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

template <class CharT>
std::basic_ostream<CharT>& operator <<
(std::basic_ostream<CharT> &dst, std::cv_status src)
{
    dst << static_cast<int>(src);
    return dst;
}

template<> template<>
void object::test<tid_completion>()
{
    cor::Completion c;
    c.up();
    std::thread wait_t = std::thread([&c]() {
            ensure_eq("Timeout waiting 1"
                      , c.wait_for(std::chrono::milliseconds(5000))
                      , std::cv_status::no_timeout);
        });
    std::thread release_t = std::thread([&c]() { c.down(); });
    release_t.join();
    wait_t.join();

    std::thread wait0_t = std::thread([&c]() {
            ensure_eq("Timeout waiting 0"
                      , c.wait_for(std::chrono::milliseconds(5000))
                      , std::cv_status::no_timeout);
        });
    wait0_t.join();

    std::thread lock1_t = std::thread([&c]() { c.up(); });
    std::thread lock2_t = std::thread([&c]() { c.up(); });
    lock2_t.join();
    lock1_t.join();
    std::thread wait2_t = std::thread([&c]() {
            ::usleep(1000);
            ensure_eq("Timeout waiting 2"
                      , c.wait_for(std::chrono::milliseconds(5000))
                      , std::cv_status::no_timeout);
        });
    std::thread down1_t = std::thread([&c]() { c.down(); });
    std::thread down2_t = std::thread([&c]() { c.down(); });
    down2_t.join();
    down1_t.join();
    wait2_t.join();

}

template<> template<>
void object::test<tid_task_queue>()
{
    cor::TaskQueue q;
    std::list<int> data;
    int count = 0;
    for (int i = 0; i < 3; ++i) {
        auto is_queued = q.enqueue([&count, &data, i]() {
                ++count;
                data.push_back(i);
            });
        ensure("Should be enqueued", is_queued);
    }
    int wait_c = 1000;
    while (--wait_c) {
        if (q.empty())
            break;
        ::usleep(1000);
    }
    ensure_eq("Executed 3 times", count, 3);
    std::list<int> expected{0, 1, 2};
    ensure_eq("Tasks saved proper numbers", data, expected);
    q.stop();
    auto is_queued = q.enqueue([]() {});
    ensure("Should not be enqueued when stopped", !is_queued);
}

}
