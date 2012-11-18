#include <cor/mt.hpp>

namespace cor
{

Mutex::WLock::WLock(Mutex const &m) : m_(m), is_locked_(true)
{
        m_.l_.lock();
}

Mutex::WLock::WLock(WLock &&from)
        : m_(from.m_), is_locked_(from.is_locked_)
{
        from.is_locked_ = false;
}

Mutex::WLock::~WLock()
{
        unlock();
}

void Mutex::WLock::unlock()
{
        if (is_locked_) {
                m_.l_.unlock();
                is_locked_ = false;
        }
}

} // cor
