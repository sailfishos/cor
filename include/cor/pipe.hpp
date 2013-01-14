#ifndef _COR_PIPE_HPP_
#define _COR_PIPE_HPP_

#include <cor/util.hpp>

#include <array>
#include <unistd.h>

namespace cor
{

class Pipe
{
public:

    Pipe() : fds([] () {
            int fds[2];
            int rc = ::pipe(fds);
            if (rc < 0)
                throw cor::Error("Can't open pipe, rc=%d", rc);
            return std::array<FdHandle, 2>({{fds[0], fds[1]}});
        }())
    { }

    Pipe(Pipe &&from) : fds(std::move(from.fds)) { }

    int first() const
    {
        return fds[0].value();
    }

    int second() const
    {
        return fds[1].value();
    }

    void close(size_t idx)
    {
        auto &fd = fds.at(idx);
        fd.close();
    }

private:
    std::array<FdHandle, 2> fds;
};

}

#endif // _COR_PIPE_HPP_
