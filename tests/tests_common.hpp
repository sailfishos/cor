#ifndef _COR_TESTS_TESTS_COMMON_HPP_
#define _COR_TESTS_TESTS_COMMON_HPP_

#include <list>
#include <cor/util.hpp>

namespace tut
{

template <class CharT, class T>
std::basic_ostream<CharT>& operator <<
(std::basic_ostream<CharT> &dst, std::list<T> const &src)
{
    dst << "(";
    for (auto v : src) {
        dst << " '" << v << "'";
    }
    dst << ")";
    return dst;
}

template <class T>
bool operator == (std::list<T> const &p1, std::list<T> const &p2)
{
    auto b1 = std::begin(p1);
    auto b2 = std::begin(p2);
    auto e1 = std::end(p1);
    auto e2 = std::end(p2);
    for (; b1 != e1; ++b1, ++b2) {
        if (b2 == e2)
            return false;

        if (*b1 != *b2)
            return false;
    }
    return (b2 == e2);
}

}

#endif // _COR_TESTS_TESTS_COMMON_HPP_
