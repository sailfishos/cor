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
    typedef int handle_type;
    void close_(int v) { ::close(v); }
    bool is_valid_(int v) const { return v >= 0; }
    int invalid_() const { return -1; }
};

template <typename T, T Invalid>
class GenericHandleTraits
{
public:
    typedef T handle_type;
    GenericHandleTraits(std::function<void (T)> close)
        : close_impl_(close) {}
    void close_(T v) { close_impl_(v); }
    bool is_valid_(T v) const { return v != Invalid; }
    T invalid_() const { return Invalid; }
private:
    std::function<void (T)> close_impl_;
};

enum handle_option {
    only_valid_handle,
    allow_invalid_handle
};

template <typename TraitsT>
class Handle : protected TraitsT
{
public:
    typedef TraitsT traits_type;
    typedef typename TraitsT::handle_type handle_type;

    Handle() : v_(traits_type::invalid_()) {}

    Handle(handle_type v, handle_option option)
        : v_((option == allow_invalid_handle) ? v : validate(v))
    {}

    Handle(handle_type v) : v_(v) {}

    template <typename ... Args>
    Handle(handle_type v, Args&&... args)
        : traits_type(args...), v_(v)
    {}

    template <typename ... Args>
    Handle(handle_type v, handle_option option, Args&&... args)
        : traits_type(args...)
        , v_((option == allow_invalid_handle) ? v : validate(v))
    {}

    ~Handle() { close(); }

    Handle(Handle &&from)
        : v_(from.v_)
    {
        from.v_ = traits_type::invalid_();
    }

    Handle & operator =(Handle &&from)
    {
        v_ = from.v_;
        from.v_ = traits_type::invalid_();
        return *this;
    }

    bool is_valid() const {
        return traits_type::is_valid_(v_);
    }

    void close()
    {
        if (is_valid()) {
            traits_type::close_(v_);
            v_ = traits_type::invalid_();
        }
    }

    handle_type value() const { return v_; }
    handle_type& ref() { return v_; }
    handle_type const& cref() const { return v_; }

    void reset(handle_type v)
    {
        close();
        v_ = v;
    }

private:

    handle_type validate(handle_type h)
    {
        if (!traits_type::is_valid_(h))
            throw cor::Error("Handle is not valid");
        return h;
    }

    Handle(Handle &);
    Handle & operator = (Handle &);

    handle_type v_;
};

typedef Handle<FdTraits> FdHandle;

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


template <typename T>
class TaggedObject : public T
{
    typedef bool (TaggedObject<T>::*tag_t)() const;

public:
    template <typename ... Args>
    TaggedObject(Args&&... args)
        : T(args...)
        , tag_(&TaggedObject<T>::is_tagged_storage_valid)
    {}

    bool is_tagged_storage_valid() const
    {
        return (tag_ == &TaggedObject<T>::is_tagged_storage_valid);
    }

private:
    TaggedObject(TaggedObject const&);
    TaggedObject operator =(TaggedObject const&);

    tag_t tag_;
};

template <typename T, typename ... Args>
intptr_t new_tagged_handle(Args... args)
{
    auto s = new TaggedObject<T>(args...);

    if (!s->is_tagged_storage_valid())
        throw Error("New handle is corrupted?");
    return reinterpret_cast<intptr_t>(s);
}

template <typename T>
T * tagged_handle_pointer(intptr_t h)
{
    auto s = reinterpret_cast<TaggedObject<T>*>(h);
    return s->is_tagged_storage_valid() ? static_cast<T*>(s) : nullptr;
}

template <typename T>
void delete_tagged_handle(intptr_t h)
{
    auto s = reinterpret_cast<TaggedObject<T>*>(h);
    if (s->is_tagged_storage_valid())
        delete s;
}

/**
 * get class member offset (type M) in the object of class T
 *
 * @param m is a class member pointer, smth. like &T::m
 *
 * @return member offset in the object
 */
template <typename T, typename M>
size_t member_offset(M const T::* m)
{
    return reinterpret_cast<size_t>(&(((T const*)nullptr)->*m));
}

/**
 * get class member offset (type M) in the object of class T
 *
 * @param m is a class member pointer, smth. like &T::m
 *
 * @return member offset in the object
 */
template <typename T, typename M>
size_t member_offset(M T::* m)
{
    return reinterpret_cast<size_t>(&(((T*)nullptr)->*m));
}

/**
 * return pointer to the object from pointer to the member
 *
 * @param p pointer to the member of the object of class T
 * @param m class member pointer, smth. like &T::m
 *
 * @return pointer to containing object
 */
template <typename T, typename M>
T *member_container(M *p, M T::*m)
{
    return reinterpret_cast<T*>(
        (reinterpret_cast<char*>(p) - member_offset(m)));
}

/**
 * return pointer to the object from pointer to the member
 *
 * @param p pointer to the member of the object of class T
 * @param m class member pointer, smth. like &T::m
 *
 * @return pointer to containing object
 */
template <typename T, typename M>
T const* member_container(M const* p, M const T::* m)
{
    return reinterpret_cast<T const*>(
        (reinterpret_cast<char const*>(p) - member_offset(m)));
}


} // namespace cor

#endif // _COR_UTIL_HPP_
