#ifndef _COR_MT_HPP_
#define _COR_MT_HPP_

#include <mutex>

namespace cor {

struct WLock
{
    template <typename T>
    typename T::WLock operator ()(T const &m) { return typename T::WLock(m); }
};

struct RLock
{
    template <typename T>
    typename T::RLock operator ()(T const &m) { return typename T::RLock(m); }
};

template <typename T>
typename T::WLock wlock(T const &m) { return WLock()(m); }

template <typename T>
typename T::RLock rlock(T const &m) { return RLock()(m); }

class Mutex
{
public:

    Mutex() {}
    virtual ~Mutex() {}

    class WLock
    {
    public:
        WLock(Mutex const &m) : m_(m), is_locked_(true)
        {
            m_.l_.lock();
        }

        WLock(WLock &&from)
            : m_(from.m_), is_locked_(from.is_locked_)
        {
            from.is_locked_ = false;
        }

        ~WLock()
        {
            unlock();
        }

        void unlock()
        {
            if (is_locked_) {
                m_.l_.unlock();
                is_locked_ = false;
            }
        }
    private:
        Mutex const &m_;
        bool is_locked_;
    };

    friend class Mutex::WLock;
    typedef Mutex::WLock RLock;

private:
    mutable std::mutex l_;
};


struct NoLock
{
    struct RLock
    {
        RLock(NoLock const&) { }
        void unlock() {}
        RLock(RLock&&) { }
    };

    typedef RLock WLock;
};

} // cor

#endif // _COR_MT_HPP_
