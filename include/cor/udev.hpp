#ifndef _COR_UDEV_HPP_
#define _COR_UDEV_HPP_
/*
 * Tiny libudev wrapper
 *
 * Copyright (C) 2012 Jolla Ltd.
 * Contact: Denis Zalevskiy <denis.zalevskiy@jollamobile.com>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <libudev.h>
#include <stddef.h>

#include <memory>
#include <string>

namespace cor {

namespace udevpp {

typedef std::unique_ptr<udev_device, void (*)(udev_device*)> udev_device_ptr;
typedef std::unique_ptr<udev_enumerate, void (*)(udev_enumerate*)>
udev_enumerate_ptr;
typedef std::unique_ptr<struct udev, void (*)(struct udev*)> udev_ptr;
typedef std::unique_ptr<struct udev_monitor, void (*)(struct udev_monitor*)>
udev_monitor_ptr;

class Root
{
public:
    Root();
    ~Root() {}

    operator bool() const { return !!p; }
    udev_device_ptr mk_device(char const *) const;
    udev_device_ptr mk_device(udev_device *d) const;
    udev_enumerate_ptr mk_enumerate() const;
    udev_monitor_ptr mk_monitor(char const *) const;
private:
    udev_ptr p;
};

template <class T>
class ListEntry
{
public:
    ListEntry(struct udev_list_entry *p) : p_(p) {}

    ListEntry(ListEntry &&src) : p_(std::move(src.p_)) {}

    ~ListEntry() {}

    template <typename FnT>
    void find(FnT const &fn)
    {
        struct udev_list_entry *entry;
        udev_list_entry_foreach(entry, p_) {
            if (!fn(T(ListEntry<T>(entry))))
                break;
        }
    }

    template <typename FnT>
    void for_each(FnT const &fn)
    {
        struct udev_list_entry *entry;
        udev_list_entry_foreach(entry, p_) {
            fn(T(ListEntry<T>(entry)));
        }
    }

    char const *name() const
    {
        return udev_list_entry_get_name(p_);
    }

    char const *value() const
    {
        return udev_list_entry_get_value(p_);
    }

private:

    struct udev_list_entry *p_;
};

class Property
{
public:
    Property(ListEntry<Property> const &entry);
    ~Property() {}

    char const * name() const
    {
        return entry_.name();
    }

    char const * value() const
    {
        return entry_.value();
    }
private:
    ListEntry<Property> const &entry_;
};

class DeviceInfo
{
public:
    DeviceInfo(ListEntry<DeviceInfo> const &);
    ~DeviceInfo() {}

    char const * path() const
    {
        return entry_.name();
    }

private:
    ListEntry<DeviceInfo> const &entry_;
};


class Enumerate
{
public:
    Enumerate(Root const &root)
        : p(root.mk_enumerate())
    {}
    ~Enumerate() {}

    operator bool() const { return !!p; }

    void subsystem_add(char const *name)
    {
        udev_enumerate_add_match_subsystem(p.get(), name);
    }

    ListEntry<DeviceInfo> devices() const;

private:
    udev_enumerate_ptr p;
};

class Device
{
public:
    Device(Root const &root, char const *path)
        : p(root.mk_device(path))
    {}
    Device(udev_device_ptr &&from)
        : p(std::move(from))
    {}
    // Device(Root const &, char const *);
    // Device(Root const &, Monitor const&);
    Device(Device &&src)
        : p(std::move(src.p))
    {}

    ~Device() {}

    operator bool() const { return !!p; }

    ListEntry<Property> properties() const
    {
        return udev_device_get_properties_list_entry(p.get());
    }

    char const *attr(char const *name) const
    {
        return udev_device_get_sysattr_value(p.get(), name);
    }
    
    char const *path() const
    {
        return udev_device_get_devpath(p.get());
    }

    char const *dev_type() const
    {
        return udev_device_get_devtype(p.get());
    }

    char const *action() const
    {
        return udev_device_get_action(p.get());
    }

private:
    udev_device_ptr p;
};

static inline bool operator == (Device const &d1, Device const &d2)
{
    auto p1 = d1.path(), p2 = d2.path();
    return std::string(p1 ? p1 : "") == std::string(p2 ? p2 : "");
}

static inline bool operator != (Device const &d1, Device const &d2)
{
    return !(d1 == d2);
}

class Monitor
{
public:
    Monitor(Root const &, char const *, char const *);
    Monitor(Monitor &&src)
        : fd_(src.fd_)
        , h_(std::move(src.h_))
    {}
    ~Monitor() {}

    int fd() const
    {
        return fd_;
    }

    Device device(Root const&) const;

private:
    int fd_;
    udev_monitor_ptr h_;
};



} // namespace udev

} // namespace cor

#endif // _COR_UDEV_HPP_
