%define osrm_name osrm-routed

Name:           osrm
Version:        0.3.7
Release:        11%{?dist}
Summary:        Open Source Routing Machine computes shortest paths in a graph. It was designed to run well with map data from the Openstreetmap Project.
License:        Simplified 2-clause BSD
URL:            https://github.com/DennisOSRM/Project-OSRM
Source0:        Project-OSRM-%{version}.tar.gz
BuildRequires:  cmake, boost-devel >= 1.48, luabind-devel, libluajit-devel, libluajit-static, protobuf-devel, stxxl-devel, osmpbf
BuildRequires:  gcc, gcc-c++
Source1:	%{osrm_name}.init
Source2:	%{osrm_name}.sysconfig
Source3:	%{osrm_name}.logrotate
Source4:	%{osrm_name}.serverini

%description
ProjectOSRM. Open Source Routing Machine computes shortest paths in a graph. It was designed to run well with map data from the Openstreetmap Project.
 
%prep
%setup -q -n Project-OSRM-%{version}
 
%build
mkdir build
cd build
cmake -D Boost_INCLUDE_DIR=/usr/include -D Boost_LIBRARY_DIRS=/usr/lib64 -D LUAJIT_LIBRARY=/usr/lib64/libluajit-5.1.so -D LUA_INCLUDE_DIR="/usr/include/luajit-2.0 ..

#rpmbuild -ba osrm.spec --define 'jobs 8' 
%{__make} %{?jobs:-j%jobs}
 
%install
mkdir -p %{buildroot}%{_libdir}
mkdir -p %{buildroot}%{_bindir}
mkdir -p %{buildroot}%{_datadir}/%{name}
mkdir -p %{buildroot}%{_sysconfdir}/%{name}
mkdir -p %{buildroot}%{_localstatedir}/log/%{name}
mkdir -p %{buildroot}%{_localstatedir}/run/%{name}
%{__install} -p -D -m 0644 build/libOSRM.so %{buildroot}%{_libdir}
%{__install} -p -D -m 0644 build/libUUID.a %{buildroot}%{_libdir}
%{__install} -p -D -m 0755 build/osrm-* %{buildroot}%{_bindir}
cp -a profiles %{buildroot}%{_datadir}/osrm
#%{__install} -a -p -D -m 0755 profiles %{buildroot}%{_datadir}/osrm
%{__install} -p -D -m 0755 %{SOURCE1} %{buildroot}%{_initrddir}/%{osrm_name}
%{__install} -p -D -m 0644 %{SOURCE2} %{buildroot}%{_sysconfdir}/sysconfig/%{osrm_name}
%{__install} -p -D -m 0644 %{SOURCE3} %{buildroot}%{_sysconfdir}/logrotate.d/%{osrm_name}
%{__install} -D -m 644 %{SOURCE4} %{buildroot}%{_sysconfdir}/%{name}/server.ini

%post
/sbin/chkconfig --add %{osrm_name}
/sbin/chkconfig %{osrm_name} on

%preun
/sbin/service %{osrm_name} stop >/dev/null 2>&1
/sbin/chkconfig --del %{osrm_name}

%files
%defattr(-, root, root, -)
%dir %{_sysconfdir}/%{name}
%{_initrddir}/%{osrm_name}
%config(noreplace) %{_sysconfdir}/sysconfig/%{osrm_name}
%config(noreplace) %{_sysconfdir}/logrotate.d/%{osrm_name}
%config(noreplace) %{_sysconfdir}/%{name}/server.ini
%{_libdir}/libOSRM.so
%{_libdir}/libUUID.a
%{_bindir}/osrm-*
%{_datadir}/osrm
%attr(-,apache,apache) %dir %{_localstatedir}/log/%{name}
%attr(-,apache,apache) %dir %{_localstatedir}/run/%{name}

%changelog
* Wed Mar 19 2014 <kay.diam@gmail.com> - 0.3.7-11
- Updated luajit dependency

* Thu Feb 18 2014 <kay.diam@gmail.com> - 0.3.7-9
- Typo fixed in init script

* Mon Feb 17 2014 <kay.diam@gmail.com> - 0.3.7-8
- Added stop delay to init script

* Fri Feb 14 2014 <kay.diam@gmail.com> - 0.3.7-7
- Added "ulimit -l unlimited" to init script

* Thu Feb 13 2014 <kay.diam@gmail.com> - 0.3.7-5
- Added lua profiles to package
- Added git version
- Added external boost libraries dependency
- Added LUAJIT library as dependency
- Added /etc files
- Initialize init scripts

* Mon Nov 25 2013 <kay.diam@gmail.com> - 0.3.7-1
- Bump up new release

* Tue Sep 24 2013 <kay.diam@gmail.com> - 0.3.5-1
- Initial release
