#include <cor/udev.hpp>
#include <cor/error.hpp>

namespace cor { namespace udevpp {

Root::Root()
    : p(udev_new(), [](struct udev *p) { if (p) ::udev_unref(p); })
{}

// Root::~Root()
// {
// }

udev_device_ptr Root::mk_device(udev_device *d) const
{
    udev_device_ptr res
        (d , [](udev_device* p) {
            if (p) udev_device_unref(p);
        });
    return res;
}

udev_device_ptr Root::mk_device(char const *path) const
{
    return mk_device(udev_device_new_from_syspath(p.get(), path));
}

udev_enumerate_ptr Root::mk_enumerate() const
{
    udev_enumerate_ptr res
        (udev_enumerate_new(p.get())
         , [](udev_enumerate *p) {
            if (p) udev_enumerate_unref(p);
        });
    return res;
}

udev_monitor_ptr Root::mk_monitor(char const *name) const
{
    udev_monitor_ptr res
        (udev_monitor_new_from_netlink(p.get(), name)
         , [](udev_monitor *p) {
            if (p) udev_monitor_unref(p);
        });
    return res;
}

Property::Property(ListEntry<Property> const &entry)
    : entry_(entry)
{}

// Property::~Property()
// {}

DeviceInfo::DeviceInfo(ListEntry<DeviceInfo> const &entry)
    : entry_(entry)
{}

// DeviceInfo::~DeviceInfo()
// {}


// Enumerate::Enumerate(Root const &root)
//     : p(root.mk_enumerate())
// {}

// Enumerate::~Enumerate()
// {
// }

ListEntry<DeviceInfo> Enumerate::devices() const
{
    udev_enumerate_scan_devices(p.get());
    return ListEntry<DeviceInfo>(udev_enumerate_get_list_entry(p.get()));
}

Monitor::Monitor(Root const &udev, char const *subsystem, char const *devtype)
    : fd_(-1)
    , h_([&udev, subsystem, devtype, this]() {
            auto h = udev.mk_monitor("udev");
            if (!h)
                throw cor::Error("Can't create udev monitor");
            udev_monitor_filter_add_match_subsystem_devtype
                (h.get(), subsystem, devtype);
            udev_monitor_enable_receiving(h_.get());
            fd_ = udev_monitor_get_fd(h_.get());
            return h;
        }())
{
}


// Monitor::~Monitor()
// {}

// Device::Device(Root const &root, char const *path)
//     : p(root.mk_device(path))
// {}

// Device::~Device()
// {
// }

Device Monitor::device(Root const &root) const
{
    return Device(root.mk_device(udev_monitor_receive_device(h_.get())));
}

// Device::Device(Root const &root, Monitor const &m)
//     : p(m.device(root))
// {
// }

}}
