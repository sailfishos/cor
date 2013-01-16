#ifndef _COR_UTIL_HPP_
#define _COR_UTIL_HPP_

#include <cor/error.hpp>

#include <unistd.h>

#include <vector>
#include <cstdio>
#include <cstdarg>
#include <stack>
#include <sstream>

#include <ctime>

namespace cor
{

template <char Fmt>
std::string binary_hex(unsigned char const *src, size_t len)
{
    static_assert(Fmt == 'X' || Fmt == 'x', "Format should be [xX]");
    char const *fmt = (Fmt == 'X') ? "%02X" : "%02x";
    std::string str(len * 2 + 1, '\0');
    for (size_t i = 0; i < len; ++i)
        sprintf(&str[i * 2], fmt, src[i]);
    str.resize(str.size() - 1);
    return str;
}

template <char Fmt>
std::string binary_hex(char const *src, size_t len)
{
    return binary_hex<Fmt>(reinterpret_cast<unsigned char const*>(src), len);
}

template <char Fmt, typename T>
std::string binary_hex(T const &src)
{
    return binary_hex<Fmt>(&src[0], src.size());
}

static inline unsigned char hex2char(char const *src)
{
        unsigned c = 0;
        int rc = sscanf(src, "%02x", &c);
        if (!rc)
            rc = sscanf(src, "%02X", &c);
        if (rc != 1)
            throw CError(rc, "Can't match input hex string");
        return (unsigned char)c;
}

std::string string_sprintf(size_t max_len, char const *fmt, ...);
std::vector<unsigned char> hex2bin(char const *src, size_t len);

template <typename DstT>
std::vector<unsigned char> hex2bin(char const *src, size_t len);

static inline std::vector<unsigned char> hex2bin(std::string const &src)
{
    return hex2bin(src.c_str(), src.size());
}

class Stopwatch
{
public:
    Stopwatch() : begin(std::time(0)) {}
    std::time_t now() const { return std::time(0) - begin; }
private:
    std::time_t begin;
};

// adding .clear() to std::stack instantiated /w deque
template <typename T>
class stack : public std::stack<T, std::deque<T> >
{
public:
    void clear()
    {
        this->c.clear();
    }
};

struct FdTraits
{
    void close_(int v) { ::close(v); }
    bool is_valid_(int v) const { return v >= 0; }
    int invalid_() const { return -1; }
};

template <typename T, T Invalid>
struct GenericHandleTraits
{
    GenericHandleTraits(std::function<void (T)> close)
        : close_impl_(close) {}
    void close_(T v) { close_impl_(v); }
    bool is_valid_(T v) const { return v != Invalid; }
    T invalid_() const { return Invalid; }
private:
    std::function<void (T)> close_impl_;
};

template <typename T, typename TraitsT>
class Handle : private TraitsT
{
    typedef TraitsT base_type;
public:

    Handle() : v_(base_type::invalid_()) {}
    Handle(T v) : v_(v) {}
    template <typename ... Args>
    Handle(T v, Args ... args) : base_type(args...), v_(v) {}
    ~Handle() { close(); }

    Handle(Handle &&from)
        : v_(from.v_)
    {
        from.v_ = base_type::invalid_();
    }

    Handle & operator =(Handle &&from)
    {
        v_ = from.v_;
        from.v_ = base_type::invalid_();
        return *this;
    }

    bool is_valid() const {
        return base_type::is_valid_(v_);
    }

    void close()
    {
        if (is_valid()) {
            base_type::close_(v_);
            v_ = base_type::invalid_();
        }
    }

    T value() const { return v_; }
    T& ref() { return v_; }
    T const& cref() const { return v_; }

private:
    Handle(Handle &);
    Handle & operator = (Handle &);

    T v_;
};

typedef Handle<int, FdTraits> FdHandle;

static inline std::string concat(std::stringstream &s)
{
    return s.str();
}

template <typename T, typename ... Args>
std::string concat(std::stringstream &s, T head, Args ... tail)
{
    s << head;
    return concat(s, tail...);
}

template <typename T, typename ... Args>
std::string concat(T head, Args ... tail)
{
    std::stringstream ss;
    ss << head;
    return concat(ss, tail...);
}


} // namespace cor

#endif // _COR_UTIL_HPP_
