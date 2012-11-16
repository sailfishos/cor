#ifndef _COR_SO_HPP_
#define _COR_SO_HPP_
/*
 * dlopen... wrapper
 *
 * Mostly for usage as configuration and communication (e.g. rpc)
 *
 * Copyright (C) 2012 Jolla Ltd.
 * Contact: Denis Zalevskiy <denis.zalevskiy@jollamobile.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 */

#include <dlfcn.h>

namespace cor
{

class SharedLib
{
public:
    SharedLib(std::string const &name, int flag)
        : h(dlopen(name.c_str(), flag))
    {}

    SharedLib(SharedLib &&from)
        : h(from.h)
    {
        from.h = 0;
    }

    virtual ~SharedLib()
    {
        close();
    }

    bool is_loaded() const
    {
        return (h != 0);
    }

    void close()
    {
        if (is_loaded()) {
            dlclose(h);
            h = 0;
        }
    }

    template <typename T>
    void sym(T *p, std::string const &name)
    {
        p = h ? reinterpret_cast<T*>(dlsym(h, name.c_str())) : 0;
    }

private:

    SharedLib(SharedLib &);

    void *h;
};

} // cor

#endif // _COR_SO_HPP_
