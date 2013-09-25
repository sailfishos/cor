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

    void invalidate() {
        tag_ = nullptr;
    }
private:
    TaggedObject(TaggedObject const&);
    TaggedObject operator =(TaggedObject const&);

    tag_t tag_;
};

template <typename T, typename... Args>
intptr_t new_tagged_handle(Args&&... args)
{
    auto s = new TaggedObject<T>(std::forward<Args>(args)...);

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
    if (s->is_tagged_storage_valid()) {
        s->invalidate();
        delete s;
    }
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

template <typename T>
class ScopeExit
{
public:
    ScopeExit(T&& fn) : fn_(std::move(fn)) {}
    ScopeExit(ScopeExit &&src) : fn_(src.fn_) {}
    ~ScopeExit() { fn_(); }
private:
    ScopeExit(ScopeExit const&);
    ScopeExit& operator =(ScopeExit const&);
    T fn_;
};

/**
 * set function to be executed at the end of the scope to use RAII
 * for unique use cases w/o creating specialized classes
 *
 * @param fn function to be executed at the end of the scope
 */
template <typename T>
ScopeExit<T> on_scope_exit(T && fn) { return ScopeExit<T>(std::move(fn)); }

/**
 * split string to parts by borders defined by any of provided symbols
 *
 * @param src source string
 * @param symbols symbols used as a separators
 * @param dst destination iterator
 */
template <class OutputIterator>
void split(std::string const& src
           , std::string const &symbols
           , OutputIterator dst)
{
    size_t begin = 0, end = 0, len = src.size();
    auto next = [&src, symbols, &begin]() {
        return src.find_first_of(symbols, begin);
    };
    for (end = next(); end != std::string::npos; end = next()) {
        *dst = src.substr(begin, end - begin);
        begin = end + 1;
        if (begin >= len)
            break;
    }
    if (begin != end)
        *dst = src.substr(begin, end - begin);
}

/**
 * join strings from input range [begin, end) using provided separator
 *
 * @param begin begin of the range
 * @param end end of the range
 * @param sep separator put between joined parts
 *
 * @return joined string
 */
template <class InIter>
std::string join(InIter begin, InIter end, std::string const &sep)
{
    if (begin == end)
        return "";

    typedef decltype(*begin) ValueT;
    size_t len = 0;
    size_t count = 0;
    auto gather_info = [&len, &count](ValueT &v) {
        ++count;
        len += v.size();
    };
    std::for_each(begin, end, gather_info);
    std::string res;
    res.reserve(len + (count * sep.size()));

    res.append(*begin);
     auto insert_following = [&res, &sep](ValueT &v) {
        res.append(sep);
        res.append(v);
    };
    std::for_each(++begin, end, insert_following);
    return res;
}

/**
 * join all strings from container using provided separator
 *
 * @param src source container
 * @param sep separator put between joined parts
 *
 * @return resulting string
 */
template <class ContainerT>
std::string join(ContainerT const&src, std::string const &sep)
{
    return join(std::begin(src), std::end(src), sep);
}

template <size_t N, size_t P>
struct TupleSelector
{

    static const size_t size = N;
    static const size_t pos = P - 1;

    typedef TupleSelector<N, P + 1> next_type;
    constexpr next_type next() const { return next_type(); }

    template <typename ... Args>
    typename std::tuple_element<pos, std::tuple<Args...> >::type const &
    get(std::tuple<Args...> const &v) const
    {
        return std::get<pos>(v);
    }

    template <typename ... Args>
    typename std::tuple_element<pos, std::tuple<Args...> >::type &
    get(std::tuple<Args...> &v) const
    {
        return std::get<pos>(v);
    }
};


template <size_t N> struct TupleSelector<N, N>
{

    static const size_t size = N;
    static const size_t pos = N - 1;

    template <typename ... Args>
    typename std::tuple_element<pos, std::tuple<Args...> >::type const &
    get(std::tuple<Args...> const &v) const
    {
        return std::get<pos>(v);
    }

    template <typename ... Args>
    typename std::tuple_element<pos, std::tuple<Args...> >::type &
    get(std::tuple<Args...> &v) const
    {
        return std::get<pos>(v);
    }
};

template <typename ... Args>
TupleSelector<std::tuple_size<std::tuple<Args...> >::value, 1>
selector(std::tuple<Args...> const &)
{
    static const size_t N = std::tuple_size<std::tuple<Args...> >::value;
    TupleSelector<N, 1> res;
    return res;
}

template <typename ... Args>
struct Operations
{
    typedef std::tuple<Args...> tuple_type;
    typedef std::tuple<Args...> & tuple_ref;
    typedef std::tuple<Args...> const& tuple_cref;

    template <size_t N, typename ActionsT>
    static void apply_if_changed
    (tuple_cref before, tuple_cref current, ActionsT const &actions
     , TupleSelector<N, N> const &selector)
    {
        apply_if_changed_(before, current, actions, selector);
    }

    template <size_t N, size_t P, typename ActionsT>
    static void apply_if_changed
    (tuple_cref before, tuple_cref current, ActionsT const &actions
     , TupleSelector<N, P> const &selector)
    {
        apply_if_changed_(before, current, actions, selector);
        apply_if_changed(before, current, actions, selector.next());
    }

    template <size_t N, typename ActionsT>
    static void copy_apply_if_changed
    (tuple_ref values, tuple_cref current, ActionsT const &actions
     , TupleSelector<N, N> const &selector)
    {
        copy_apply_if_changed_(values, current, actions, selector);
    }

    template <size_t N, size_t P, typename ActionsT>
    static void copy_apply_if_changed
    (tuple_ref values, tuple_cref current, ActionsT const &actions
     , TupleSelector<N, P> const &selector)
    {
        copy_apply_if_changed_(values, current, actions, selector);
        copy_apply_if_changed(values, current, actions, selector.next());
    }

private:

    template <size_t N, size_t P, typename ActionsT>
    static void apply_if_changed_
    (tuple_cref before, tuple_cref current, ActionsT const &actions
     , TupleSelector<N, P> const &selector)
    {
        auto const &v1 = selector.get(before);
        auto const &v2 = selector.get(current);
        if (v2 != v1)
            std::get<selector.pos>(actions)(v2);
    }

    template <size_t N, size_t P, typename ActionsT>
    static void copy_apply_if_changed_
    (tuple_ref values, tuple_cref current, ActionsT const &actions
     , TupleSelector<N, P> const &selector)
    {
        auto &v1 = selector.get(values);
        auto const &v2 = selector.get(current);
        if (v2 != v1) {
            std::get<selector.pos>(actions)(v2);
            v1 = v2;
        }
    }

};

template <typename ActionsT, typename ...Args>
void apply_if_changed(std::tuple<Args...> const &before
                      , std::tuple<Args...> const &current
                      , ActionsT const &actions)
{
    Operations<Args...>::template
        apply_if_changed<>(before, current, actions, selector(before));
}

template <typename ActionsT, typename ...Args>
void copy_apply_if_changed(std::tuple<Args...> &values
                           , std::tuple<Args...> const &current
                           , ActionsT const &actions)
{
    Operations<Args...>::template
        copy_apply_if_changed<>(values, current, actions, selector(values));
}

} // namespace cor

#endif // _COR_UTIL_HPP_
