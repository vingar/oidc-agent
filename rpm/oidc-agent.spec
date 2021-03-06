Name: oidc-agent
Version: 1.2.0
Release: 1
Summary: Commandline tool for obtaining OpenID Connect Access tokens on the commandline
Group: Misc
License: MIT-License
URL: https://github.com/indigo-dc/oidc-agent
Source0: oidc-agent.tar

BuildRequires: libcurl-devel >= 7.29
BuildRequires: libsodium-devel >= 1.0.11
BuildRequires: libmicrohttpd-devel >= 0.9.37
BuildRequires: help2man >= 1.46.4

Requires: libsodium18 >= 1.0.11
Requires: libcurl >= 7.29
Requires: libmicrohttpd10 >= 0.9.37

BuildRoot:	%{_tmppath}/%{name}

%description
Commandline tool for obtaining OpenID Connect Access tokens on the commandline5???

%prep
%setup -q

%build
make 

%install
echo "Buildroot: ${RPM_BUILD_ROOT}"
echo "ENV: "
env | grep -i rpm
echo "PWD"
pwd
make install INSTALL_PATH=${RPM_BUILD_ROOT}/usr MAN_PATH=${RPM_BUILD_ROOT}/usr/share/man CONFIG_PATH=${RPM_BUILD_ROOT}/etc

%files
%defattr(-,root,root,-)
%{_bindir}/*
#%doc
#%{_mandir}/*

%changelog
