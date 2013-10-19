#include <cor/util.hpp>
#include <cor/mt.hpp>

namespace cor
{

Mutex::WLock::WLock(Mutex const &m) : lock_(m.l_)
{
}

Mutex::WLock::WLock(WLock &&from)
    : lock_(std::move(from.lock_))
{
}

Mutex::WLock::~WLock()
{
}

void Mutex::WLock::unlock()
{
    lock_.unlock();
}

void Completion::up()
{
    std::lock_guard<std::mutex> lock(mutex_);
    ++counter_;
}

void Completion::down()
{
    std::unique_lock<std::mutex> lock(mutex_);
    --counter_;
    if (!counter_) {
        lock.unlock();
        done_.notify_one();
    }
}

void Completion::wait()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (!counter_)
        return;

    done_.wait(lock);
}
} // cor
