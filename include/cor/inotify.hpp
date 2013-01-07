#ifndef _COR_INOTIFY_HPP_
#define _COR_INOTIFY_HPP_

#include <cor/util.hpp>
#include <sys/inotify.h>

namespace cor
{
namespace inotify
{

class Handle
{
public:
    Handle() : inotify_(inotify_init())
    {
        if (!inotify_.is_valid())
            throw cor::Error("Inotify was not initialized\n");
    }

    int fd() const
    {
        return inotify_.fd;
    }

    int read(void *dst, size_t max_len) const
    {
        return ::read(fd(), dst, max_len);
    }

private:
    Handle(Handle&);
    Handle & operator = (Handle&);

    cor::Fd inotify_;
};

class Watch
{
public:
    Watch(Handle &inotify, std::string const &path, uint32_t mask)
        : inotify_(inotify),
          fd_(inotify_add_watch(inotify_.fd(), path.c_str(), mask))
    {
    }

    Watch(Watch &&src)
        : inotify_(src.inotify_)
    {
        rm();
        std::swap(fd_, src.fd_);
    }

    ~Watch()
    {
        rm();
    }

private:

    void rm()
    {
        inotify_rm_watch(inotify_.fd(), fd_);
    }

    Watch(Watch &);
    Watch & operator =(Watch &);

    Handle &inotify_;
    int fd_;
};

}
}

#endif // _COR_INOTIFY_HPP_

