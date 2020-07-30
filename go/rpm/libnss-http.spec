Name:       libnss-http
Version:    %{PKG_VER}
Release:    %{PKG_REL}
Summary:    NSS http plugin
License:    MIT

Source0:    libnss_http.tar.gz

Requires:   glibc

Obsoletes: %{name} < %{version}-%{release}
Provides: %{name} = %{version}-%{release}
BuildArch: x86_64

%global debug_package %{nil}

%filter_provides_in libnss_http.so
%filter_provides_in libnss_http.so.2
%filter_requires_in libnss_http.so
%filter_requires_in libnss_http.so.2
%filter_setup

%description
NSS plugin that uses an HTTP server as a backend

%prep
%setup -q -c

%install
mkdir -p %{buildroot}/{etc,usr/lib64}
cp -af nss_http.conf %{buildroot}/etc
cp -fa libnss_http.so.%{version} %{buildroot}/usr/lib64/
ln -sf libnss_http.so.%{version} %{buildroot}/usr/lib64/libnss_http.so.2
ln -sf libnss_http.so.2 %{buildroot}/usr/lib64/libnss_http.so

%files
%config(noreplace) /etc/nss_http.conf
/usr/lib64/libnss_http.so
/usr/lib64/libnss_http.so.2
/usr/lib64/libnss_http.so.%{version}
