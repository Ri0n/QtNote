
%if 0%{?fedora}
%define _pkgconfig pkgconfig
%define _qt5linguist qt5-linguist
%define _qt5basedevel qt5-qtbase-devel
%else
%define qmake_qt5 qmake-qt5
%define _pkgconfig pkg-config
%define _qt5linguist  libqt5-linguist
%define _qt5basedevel libqt5-qtbase-devel
%endif

Name:           qtnote
Version:        3.0.5
Release:        1%{?dist}
Summary:        Simple note-taking application in Qt
# FIXME: Select a correct license from https://github.com/openSUSE/spec-cleaner#spdx-licenses
License:        GPLv3+
# FIXME: use correct group, see "https://en.opensuse.org/openSUSE:Package_group_guidelines"
Group:          Productivity/Office/Other
Url:            http://ri0n.github.io/QtNote
Source:         https://github.com/Ri0n/QtNote/releases/download/%{version}/%{name}-%{version}.tar.gz
Requires:       libhunspell >= 1.6
Requires:       libQt5Widgets
BuildRequires:  gcc-c++ 
BuildRequires:  %{_pkgconfig}
BuildRequires:  %{_qt5linguist}
BuildRequires:  %{_qt5basedevel}
#BuildRequires:  qtsingleapplication-devel # unfortunately no in suse repos
BuildRequires:  hunspell-devel
BuildRequires:  kwindowsystem-devel
BuildRequires:  knotifications-devel
BuildRequires:  kglobalaccel-devel

%description
Note-taking application written with Qt in mind.
It's mostly Tomboy clone but with less features. So it's app
which lives in your system tray and allows you to make notes quickly

%package        devel
Summary:        QtNote development files
Group:          Development/Libraries
Requires:       %{name} = %{version}-%{release}

%description devel
This package contains headers and libraries required to build applications
using libqtnote and also to build QtNote plugins.

%prep
%autosetup -p1 -n %{name}-%{version}

%build
%if 0%{?fedora}
%{qmake_qt5} PREFIX=%{_prefix} LIBDIR=%{_libdir} qtnote.pro CONFIG+="DISABLE_FFMPEG" QMAKE_CXXFLAGS="%{optflags}" QMAKE_CFLAGS="%{optflags}"
%else
%{qmake_qt5} PREFIX=%{_prefix} LIBDIR=%{_libdir} qtnote.pro QMAKE_CXXFLAGS="%{optflags}" QMAKE_CFLAGS="%{optflags}"
%endif
%make_build

%install
make install INSTALL_ROOT=%{buildroot}

%post
%postun

%files
%{_bindir}/%{name}
%{_libdir}/%{name}
%{_libdir}/libqtnote.so.*
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor
%{_datadir}/%{name}/*.qm
%{_mandir}/man1/%{name}.1.gz
%license license.txt
%doc Changelog README.md

%files devel
%{_prefix}/include
%{_libdir}/libqtnote.so
%{_datadir}/%{name}/common.pri
%{_datadir}/%{name}/plugins/plugin.pri

%changelog
* Sun Nov 18 2018 Sergey Ilinykh <rion4ik@gmail.com> -  3.0.5-1
- First rpm build
