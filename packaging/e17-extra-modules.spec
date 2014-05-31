Name:       e17-extra-modules
Summary:    The E17 Extra Modules The E17 extra modules consists of modules made by SAMSUNG
Version:    0.10.116
Release:    1
VCS:        framework/uifw/e17-extra-modules#e17-extra-modules-0.9.118-2-g442d1a2f868f2e50a53d425f61879176d0681468
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
%if "%{_repository}" == "wearable"
BuildRequires:  pkgconfig(edbus)
BuildRequires:  gettext
BuildRequires:  cmake
%endif

Requires: libX11
Requires: vconf
%if "%{_repository}" == "wearable"
Requires: libsensord
%elseif "%{_repository}" == "mobile"
Requires: sensor
%endif

%description
The E17 Extra Modules  The E17 extra modules consists of modules made by SAMSUNG.

%prep
%setup -q


%build

%if "%{_repository}" == "wearable"
cd wearable/po
rm -rf CMakeFiles
rm -rf CMakeCache.txt
cmake .
make %{?jobs:-j%jobs}
make install
cd ../../
%endif


%if "%{_repository}" == "wearable"
%define DEF_SUBDIRS comp-tizen illume2-tizen keyrouter wmready accessibility move-tizen devicemgr extndialog screen-reader devmode-tizen elogwatcher
%elseif "%{_repository}" == "mobile"
%define DEF_SUBDIRS comp-tizen illume2-tizen keyrouter wmready accessibility move-tizen devicemgr extndialog screen-reader devmode-tizen
%endif

%if "%{_repository}" == "wearable"
export GC_SECTIONS_FLAGS="-fdata-sections -ffunction-sections -Wl,--gc-sections"
export CFLAGS+=" -Wall -g -fPIC -rdynamic ${GC_SECTIONS_FLAGS} -D_F_REMAP_MOUSE_BUTTON_TO_HWKEY_ "
%if "%{sec_build_project_name}" == "tizenw_master" || "%{sec_build_project_name}" == "tizenw2_master"
export CFLAGS="$CFLAGS -DW_WIN_CHECK"
%endif
%if 0%{?sec_build_binary_debug_enable}
export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
%endif
%elseif "%{_repository}" == "mobile"
export CFLAGS+=" -Wall -g -fPIC -rdynamic"
# use dlog
export CFLAGS+=" -DUSE_DLOG"
%endif

export LDFLAGS+=" -Wl,--hash-style=both -Wl,--as-needed -Wl,--rpath=/usr/lib"

%ifarch %{arm}
export CFLAGS+=" -D_ENV_ARM"
%endif

%if "%{_repository}" == "wearable"
export CFLAGS+=" -D_F_WEARABLE_PROFILE_ "
%endif


%if "%{_repository}" == "wearable"
cd wearable
%elseif "%{_repository}" == "mobile"
cd mobile
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

%if "%{_repository}" == "wearable"
# for smack rule
mkdir -p %{buildroot}/etc/smack/accesses2.d
cp %{_builddir}/%{buildsubdir}/wearable/e17-extra-modules.rule %{buildroot}/etc/smack/accesses2.d
%endif

# for license notification
mkdir -p %{buildroot}/usr/share/license
%if "%{_repository}" == "wearable"
cp -a %{_builddir}/%{buildsubdir}/wearable/COPYING %{buildroot}/usr/share/license/%{name}
%elseif "%{_repository}" == "mobile"
cp -a %{_builddir}/%{buildsubdir}/mobile/COPYING %{buildroot}/usr/share/license/%{name}
cat %{_builddir}/%{buildsubdir}/mobile/COPYING.Flora >> %{buildroot}/usr/share/license/%{name}
%endif

# for locale
%if "%{_repository}" == "wearable"
cp %{_builddir}/%{buildsubdir}/wearable/po/locale %{buildroot}/usr/share -a
%endif

# for keyrouter
mkdir -p %{buildroot}/usr/bin
%if "%{_repository}" == "wearable"
cp -af wearable/keyrouter/scripts/* %{buildroot}/usr/bin/
%elseif "%{_repository}" == "mobile"
cp -af mobile/keyrouter/scripts/* %{buildroot}/usr/bin/
%endif

%if "%{_repository}" == "wearable"
cd wearable
%elseif "%{_repository}" == "mobile"
cd mobile
%endif

for FILE in %{DEF_SUBDIRS}
do
        (cd $FILE && make install DESTDIR=%{buildroot} )
done

find  %{buildroot}/usr/lib/enlightenment/modules -name *.la | xargs rm
find  %{buildroot}/usr/lib/enlightenment/modules -name *.a | xargs rm

%files
%if "%{_repository}" == "wearable"
%manifest wearable/e17-extra-modules.manifest
%elseif "%{_repository}" == "mobile"
%manifest mobile/e17-extra-modules.manifest
%endif
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

%if "%{_repository}" == "wearable"
%{_bindir}/elogwatcher
%endif

%{_bindir}/*
/usr/share/license/%{name}

%if "%{_repository}" == "wearable"
/usr/share/locale/*
/etc/smack/accesses2.d/e17-extra-modules.rule
%endif
