Summary: Just another C++/C library
Name: cor
Version: 0.1.0
Release: 1
License: LGPL21
Group: Development/Liraries
URL: https://github.com/deztructor/cor
Source0: %{name}-%{version}.tar.bz2
BuildRequires: cmake >= 2.8

%description
Just another C++/C library. Contains useful utilities which are not a
part of any other library :)

%prep
%setup -q

%build
%cmake
make %{?jobs:-j%jobs}

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=%{buildroot}

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%{_libdir}/libcor.so
%{_libdir}/pkgconfig/cor.pc
%{_includedir}/cor/*.h
%{_includedir}/cor/*.hpp

%doc README.org

