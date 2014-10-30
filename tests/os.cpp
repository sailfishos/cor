#include <cor/os.hpp>
#include <tut/tut.hpp>

#include "tests_common.hpp"

#include <string>
#include <stdexcept>
#include <list>
#include <array>
#include <sstream>
#include <thread>
#include <atomic>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

using std::string;
using std::runtime_error;

namespace tut
{

struct os_test
{
    virtual ~os_test()
    {
    }
};

typedef test_group<os_test> tf;
typedef tf::object object;
tf cor_os_test("os");

enum test_ids {
    tid_pipe = 1
    , tid_open_lock
};


template<> template<>
void object::test<tid_pipe>()
{
    using cor::posix::Pipe;
    using cor::use;
    using cor::poll;
    using cor::read;
    using cor::write;

    auto to = Pipe::create();
    auto from = Pipe::create();

    auto in = use<Pipe::Read>(from);
    auto out = use<Pipe::Write>(to);

    // non-reenterable
    static std::atomic<char> is_exit(ATOMIC_VAR_INIT(0));
    static std::atomic<char> poll_err_count(ATOMIC_VAR_INIT(0));

    // not passed to the thread directly because of gcc 4.6 bug with
    // movable objects
    auto other_out = use<Pipe::Write>(from);
    auto other_in = use<Pipe::Read>(to);
    auto other_end = [&other_out, &other_in]() {
        auto &out = other_out;
        auto &in = other_in;
        char res = 5;
        auto on_exit = cor::on_scope_exit([&res] () {
                std::atomic_exchange(&is_exit, res);
            });
        auto poll_in = [&in]() {
            auto ev = poll(in, POLLIN | POLLPRI, 1000 * 5);
            if ((ev & (POLLHUP | POLLERR)) || !ev) {
                ++poll_err_count;
                return false;
            }
            return true;
        };
        poll_in() && ++res
        && (read(in, res) == sizeof(res)) && ++res
        && (write(out, res) == sizeof(res)) && ++res
        && poll_in() && ++res
        && (read(in, res) == sizeof(res)) && ++res
        && (write(out, res) == sizeof(res)) && ++res;
    };
    auto events = poll(in, POLLIN | POLLPRI, 10);
    ensure_eq("No reply expected", events, 0);

    char data = 17;
    auto count = write(out, data);
    ensure_eq("Written", count, sizeof(data));

    auto t = std::thread(other_end);
    // do not join, thread can hang
    t.detach();

    auto poll_in = [&in](std::string const &msg, int data) {
        auto events = poll(in, POLLIN | POLLPRI, 1000 * 5);
        ensure_eq("Bad poll event:" + msg, events & (POLLHUP | POLLERR), 0);
        ensure_ne("No reply:" + msg, events, 0);
        char res = 0;
        auto count = read(in, res);
        ensure_eq("Read:" + msg, count, sizeof(res));
        ensure_eq("Read data is ok:" + msg, (int)res, data + 1);
        return res;
    };

    data = poll_in("Waiting 1st", data);
    data += 10;
    count = write(out, data);
    ensure_eq("Written", count, sizeof(data));
    data = poll_in("Waiting 1st", data);

    // wait for thread to stop - timeout 5s
    for (count = 0; count < 5000 && is_exit.load() < 5; ++count)
        ::usleep(1000);

    ensure("Timeout, thread is hanged?", count != 0);
    ensure_eq("Poll errors", poll_err_count, 0);
    ensure_eq("Different data", (int)data + 1, (int)is_exit);
}

template<> template<>
void object::test<tid_open_lock>()
{
    // auto ensure_peer_eq = [](Pipe &pipe, std::string const &msg, int expected) {
    //     int rc = 1;
    //     ensure_eq(msg + ":1b", read(pipe, rc), sizeof(rc));
    //     ensure_eq(msg, rc, expected);
    // };
    // auto send_peer_status = [](Pipe &pipe, std::string const &msg, int rc) {
    //     ensure_eq(msg + ": expected 1b", write(pipe, rc), sizeof(rc));
    // };

    // char name[] = "cor-open-lock.XXXXXX";
    // ::mkstemp(name);

    // auto open_lock_child = [&name](Pipe &in, Pipe &out)
    // {
    //     in.only_reader();
    //     out.only_writer();

    //     using namespace std::placeholders;
    //     auto send_status = std::bind(send_peer_status, std::ref(out), _1, _2);
    //     auto ensure_parent_ok = std::bind(ensure_peer_ok, std::ref(in), _1, _2);

    //     ensure_parent_ok();
    //     auto res = open_lock(name);
    //     send_status("lock status", static_cast<int>(std::get<0>(res)));
    //     send_status("lock status", static_cast<int>(std::get<1>(res).is_valid()));
    // };

    // Pipe to_child, from_child;
    // auto child = [&]() {
    //     to_child.only_reader();
    //     from_child.only_writer();

    //     using namespace std::placeholders;
    //     auto send_status = std::bind(send_peer_status, std::ref(from_child), _1, _2);
    //     auto ensure_parent_ok = std::bind(ensure_peer_ok, std::ref(to_child), _1);

    //     auto res = open_lock(name);
    //     auto status = std::get<0>(res);
    //     send_status("should lock", static_cast<int>(status));
    //     ensure_parent_ok("parent should't lock");
    // };
    // auto rc = ::fork();
    // ensure_ne("Can't fork", rc, -1);

    // auto parent = [&]() {

    //     to_child.only_writer();
    //     from_child.only_reader();

    //     using namespace std::placeholders;
    //     auto send_status = std::bind(send_peer_status, std::ref(to_child), _1, _2);
    //     auto ensure_child_ok = std::bind(ensure_peer_ok, std::ref(from_child), _1);

    //     ensure_child_ok("child can't lock");
    //     auto res = open_lock(name);
    //     auto status = std::get<0>(res);
    //     send_status("", (status != Status::Again ? 1 : 0));
    //     ensure_eq("parent shouldn't get Ok", (int)status, (int)Status::Ok);
    // };

    // auto wait_on_exit = [] () {
    //     int child_status = 255;
    //     ::wait(&child_status);
    //     return child_status;
    // };


    // if (!rc)
    //     child();
    // else {
    //     try {
    //         parent();
    //     } catch (...) {
    //         wait_on_exit();
    //         throw;
    //     }
    //     ensure_eq("Bad child", wait_on_exit(), 0);
    // }
}

}
