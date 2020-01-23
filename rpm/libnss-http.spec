Name:       libnss-http
Version:    <PKG_VER>
Release:    <PKG_REL>%{?dist}
Summary:    NSS http plugin
License:    MIT

%global debug_package %{nil}

BuildRequires: json-c-devel
BuildRequires: libcurl-devel
BuildRequires: gcc

Provides: %{name}-%{version}-%{release}
BuildArch: x86_64

%description
NSS plugin with HTTP REST API as a backend

%prep
cp -rf %{_sourcedir}/nss_http/* .

%build
%make_build

%install
%make_install

%files
/usr/lib/libnss_http.so.%{version}
/usr/lib/libnss_http.so.2
/usr/lib/libnss_http.so
