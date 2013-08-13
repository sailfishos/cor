Summary: Just another C++/C library
Name: cor
Version: 0.1.3
Release: 1
License: LGPL21
Group: Development/Liraries
URL: https://github.com/deztructor/cor
Source0: %{name}-%{version}.tar.bz2
BuildRequires: cmake >= 2.8
BuildRequires: pkgconfig(tut) >= 0.0.1
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%description
Just another C++/C library. Contains useful utilities which are not a
part of any other library :)

%package devel
Summary: cor headers etc.
Group: System Environment/Libraries
Requires: %{name} = %{version}-%{release}
%description devel
cor library header files etc.

%prep
%setup -q

%build
%cmake -DCOR_VERSION=%{version}
make %{?jobs:-j%jobs}
make %{?jobs:-j%jobs} check

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=%{buildroot}

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%{_libdir}/libcor.so

%files devel
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/cor.pc
%{_includedir}/cor/*.h
%{_includedir}/cor/*.hpp

%doc README.org

