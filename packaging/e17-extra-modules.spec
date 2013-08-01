Name:       e17-extra-modules
Summary:    The E17 Extra Modules The E17 extra modules consists of modules made by SAMSUNG
Version:    0.14.7
Release:    1
Group:      System/GUI/Other
License:    BSD
Source0:    %{name}-%{version}.tar.gz
BuildRequires:  pkgconfig(enlightenment)
BuildRequires:  pkgconfig(utilX)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(xextproto)
BuildRequires:  pkgconfig(xfixes)
BuildRequires:  pkgconfig(xext)
BuildRequires:  pkgconfig(edje)
BuildRequires:  pkgconfig(xrandr)
BuildRequires:  pkgconfig(evas)
BuildRequires:  pkgconfig(xi)
BuildRequires:  pkgconfig(xtst)
BuildRequires:  pkgconfig(pixman-1)
BuildRequires:  pkgconfig(sensor)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  edje-tools
Requires: libX11
Requires: sensor
Requires: vconf

%description
The E17 Extra Modules  The E17 extra modules consists of modules made by SAMSUNG.

%prep
%setup -q


%build

%define DEF_SUBDIRS comp-tizen illume2-tizen keyrouter wmready accessibility move-tizen devicemgr extndialog screen-reader devmode-tizen

export CFLAGS+=" -Wall -g -fPIC -rdynamic"
export LDFLAGS+=" -Wl,--hash-style=both -Wl,--as-needed -Wl,--rpath=/usr/lib"
# use dlog
export CFLAGS+=" -DUSE_DLOG"

%ifarch %{arm}
export CFLAGS+=" -D_ENV_ARM"
%endif


for FILE in %{DEF_SUBDIRS}
do 
        cd $FILE 
        %autogen
        %configure --prefix=/usr
        make
        cd -
done

%install
rm -rf %{buildroot}

# for license notification
mkdir -p %{buildroot}/usr/share/license
cp -a %{_builddir}/%{buildsubdir}/COPYING %{buildroot}/usr/share/license/%{name}

# for keyrouter
mkdir -p %{buildroot}/usr/bin
cp -af keyrouter/scripts/* %{buildroot}/usr/bin/

for FILE in %{DEF_SUBDIRS}
do 
        (cd $FILE && make install DESTDIR=%{buildroot} )
done

find  %{buildroot}/usr/lib/enlightenment/modules -name *.la | xargs rm 
find  %{buildroot}/usr/lib/enlightenment/modules -name *.a | xargs rm 

%files
%manifest e17-extra-modules.manifest
%defattr(-,root,root,-)
%{_libdir}/enlightenment/modules/comp-tizen
%{_libdir}/enlightenment/modules/e17-extra-module-accessibility
%{_libdir}/enlightenment/modules/illume2-tizen
%{_libdir}/enlightenment/modules/e17-extra-module-keyrouter
%{_libdir}/enlightenment/modules/e17-extra-module-wmready
%{_libdir}/enlightenment/modules/move-tizen
%{_libdir}/enlightenment/modules/e17-extra-module-devicemgr
%{_libdir}/enlightenment/modules/screen-reader
%{_libdir}/enlightenment/modules/devmode-tizen
%{_datadir}/enlightenment/data/*
%{_bindir}/extndialog
%{_bindir}/*
/usr/share/license/%{name}
