%{!?_with_udev: %{!?_without_udev: %define _with_udev --with-udev}}
%{!?cmake_install: %define cmake_install make install DESTDIR=%{buildroot}}

Summary: Just another C++/C library
Name:    cor
Version: 0.1.3
Release: 1
License: LGPLv2+
Group:   Development/Libraries
URL:     https://git.sailfishos.org/mer-core/cor
Source0: %{name}-%{version}.tar.bz2
BuildRequires: cmake >= 2.8
BuildRequires: pkgconfig(tut) >= 0.0.1
%if 0%{?_with_udev:1}
BuildRequires: pkgconfig(libudev) >= 187
BuildRequires: pkgconfig(udev) >= 187
%endif
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%description
Just another C++/C library. Contains useful utilities which are not a
part of any other library :)

%package devel
Summary: cor headers etc.
Requires: %{name} = %{version}-%{release}
Requires: libudev-devel >= 187
%description devel
cor library header files etc.

%package tests
Summary:    Tests for cor
Requires:   %{name} = %{version}-%{release}
%description tests
%summary

%prep
%setup -q

%build
%cmake -DVERSION=%{version} %{?_with_multiarch:-DENABLE_MULTIARCH=ON} %{?_without_udev:-DENABLE_UDEV=OFF}
make %{?jobs:-j%jobs}

%install
rm -rf $RPM_BUILD_ROOT
%cmake_install

%files
%defattr(-,root,root,-)
%{_libdir}/libcor.so
%{_libdir}/libcor.so.0
%{_libdir}/libcor.so.%{version}
%if 0%{?_with_udev:1}
%{_libdir}/libcor-udev.so
%{_libdir}/libcor-udev.so.0
%{_libdir}/libcor-udev.so.%{version}
%endif
%license COPYING

%files devel
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/*.pc
%dir %{_includedir}/cor
%{_includedir}/cor/*.h
%{_includedir}/cor/*.hpp
%if 0%{?_with_udev:1}
%{_includedir}/cor/udev/*.hpp
%endif
%dir %{_libdir}/cmake/cor
%{_libdir}/cmake/cor/*

%doc README.org

%files tests
%defattr(-,root,root,-)
%dir /opt/tests/cor
/opt/tests/cor/*

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig
