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
    tid_basic_future = 1
};

template<> template<>
void object::test<tid_basic_future>()
{
    cor::Future f;
    int nr = 0;
    std::mutex m;
    std::unique_lock<std::mutex> l(m);
    std::thread t = std::thread(f.wrap([&nr, &m]() {
                std::lock_guard<std::mutex> l(m);
                nr = 2;
            }));
    auto join = cor::on_scope_exit([&t](){t.join();});
    nr = 3;
    ::usleep(50);
    ensure_eq("Thread should not be started yet", nr, 3);
    l.unlock();
    auto res = f.wait(std::chrono::milliseconds(10000));
    ensure_ne("No timeout", (int)res, (int)std::cv_status::timeout);
    ensure_eq("Should wait for future to be completed", nr, 2);
}

}
