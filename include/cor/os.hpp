#include <cor/util.hpp>
#include <poll.h>
#include <string.h>

namespace cor {

namespace posix {

struct Pipe {
    enum End { Read = 0, Write = 1 };
    typedef std::pair<FdHandle, FdHandle> type;
    static type create() {
        int fds[2];
        int rc = ::pipe(fds);
        if (rc < 0)
            throw cor::Error("Can't open pipe, rc=%d", rc);
        return type(fds[0], fds[1]);
    }
};

}

template <posix::Pipe::End end>
FdHandle & get(posix::Pipe::type &p)
{
    return std::get<end>(p);
}

template <posix::Pipe::End end>
FdHandle const & get(posix::Pipe::type const &p)
{
    return std::get<end>(p);
}

template <posix::Pipe::End end>
FdHandle use(posix::Pipe::type &p)
{
    return std::move(get<end>(p));
}

namespace {

ssize_t write(FdHandle const &h, void const *buf, size_t count)
{
    return ::write(h.value(), buf, count);
}

ssize_t read(FdHandle const &h, void *buf, size_t count)
{
    return ::read(h.value(), buf, count);
}

template <typename T>
ssize_t write(FdHandle const &h, T const &v)
{
    return write(h, (void const*)&v, sizeof(v));
}

template <typename T>
ssize_t read(FdHandle const &h, T &v)
{
    return read(h, (void*)&v, sizeof(v));
}

short poll(FdHandle const &h, short events, int timeout)
{
    struct pollfd fds = {h.value(), events, 0};
    auto rc = ::poll(&fds, 1, timeout);
    if (rc < 0)
        throw cor::Error("Poll error %s", ::strerror(errno));
    return rc == 1 ? fds.revents : 0;
}

} // anonymous

}
