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

} // cor
