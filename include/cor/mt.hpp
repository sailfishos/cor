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
        WLock(Mutex const &m);
        WLock(WLock &&from);
        ~WLock();
        void unlock();
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
