Name:       e17-extra-modules
Summary:    The E17 Extra Modules for Tizen
Version:    0.12.19
Release:    1
Group:      System/GUI/Other
License:    BSD 2-clause and Flora-1.1
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
BuildRequires:  pkgconfig(eina)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(ecore-x)
BuildRequires:  pkgconfig(xrandr)
BuildRequires:  pkgconfig(libxml-2.0)
BuildRequires:  pkgconfig(xrandr)
BuildRequires:  pkgconfig(evas)
BuildRequires:  pkgconfig(xi)
BuildRequires:  pkgconfig(xtst)
BuildRequires:  pkgconfig(cairo)
BuildRequires:  pkgconfig(pixman-1)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(edbus)
BuildRequires:  cmake
BuildRequires:  gettext
BuildRequires:  edje-tools
#libhwc, efl-assist was library for private
BuildRequires:  pkgconfig(libhwc)
BuildRequires:  pkgconfig(security-server)
BuildRequires:  pkgconfig(sensor)

Requires: libX11
Requires: vconf
Requires: libsensord

%description
The E17 Extra Modules  The E17 extra modules consists of modules made by SAMSUNG.

%prep
%setup -q


%build
cd po
rm -rf CMakeFiles
rm -rf CMakeCache.txt
cmake .
make %{?jobs:-j%jobs}
make install
cd ..

%if "%{?tizen_profile_name}" == "mobile"
%define DEF_SUBDIRS comp-tizen illume2-tizen keyrouter wmready accessibility move-tizen devicemgr extndialog screen-reader elogwatcher processmgr smack-checker
%elseif "%{?tizen_profile_name}" == "wearable"
%define DEF_SUBDIRS comp-tizen-wearable illume2-tizen keyrouter wmready accessibility-wearable move-tizen devicemgr-wearable extndialog screen-reader elogwatcher processmgr-wearable smack-checker
%endif

export GC_SECTIONS_FLAGS="-fdata-sections -ffunction-sections -Wl,--gc-sections"
export CFLAGS+=" -Wall -g -fPIC -rdynamic ${GC_SECTIONS_FLAGS} -D_F_REMAP_MOUSE_BUTTON_TO_HWKEY_ "
export LDFLAGS+=" -Wl,--hash-style=both -Wl,--as-needed -Wl,--rpath=/usr/lib"

export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"

%if "%{?tizen_profile_name}" == "mobile"
export CFLAGS+=" -D_ENV_MOBILE_"
%elseif "%{?tizen_profile_name}" == "wearable"
export CFLAGS+=" -D_ENV_WEARABLE_"
%endif

%ifarch %{arm}
export CFLAGS+=" -D_ENV_ARM"
%endif


for FILE in %{DEF_SUBDIRS}
do
    if test "x${FILE}" = "xcomp-tizen" ; then
        cd $FILE
        export CFLAGS+=" -D_F_USE_GRAB_KEY_SET_"
        %autogen
        %configure --enable-hwc \
                   --prefix=/usr
        make %{?jobs:-j%jobs}
        cd -
    elif test "x${FILE}" = "xcomp-tizen-wearable" ; then
        cd $FILE
        export CFLAGS+=" -D_F_USE_GRAB_KEY_SET_"
        %autogen
        %configure --prefix=/usr
        make %{?jobs:-j%jobs}
        cd -
    elif test "x${FILE}" = "xscreen-reader" ; then
        cd $FILE
        export CFLAGS+=" -DENABLE_RAPID_KEY_INPUT"
        %autogen
        %configure --prefix=/usr
        make %{?jobs:-j%jobs}
        cd -
    else
        cd $FILE
        %autogen
        %configure --prefix=/usr
        make %{?jobs:-j%jobs}
        cd -
    fi
done

%install
rm -rf %{buildroot}

# for smack rule
mkdir -p %{buildroot}/etc/smack/accesses.d
cp %{_builddir}/%{buildsubdir}/e17-extra-modules.efl %{buildroot}/etc/smack/accesses.d

# for license notification
mkdir -p %{buildroot}/usr/share/license
cp -a %{_builddir}/%{buildsubdir}/COPYING %{buildroot}/usr/share/license/%{name}

# for locale
cp %{_builddir}/%{buildsubdir}/po/locale %{buildroot}/usr/share -a

# for keyrouter
mkdir -p %{buildroot}/usr/bin
cp -af keyrouter/scripts/* %{buildroot}/usr/bin/

for FILE in %{DEF_SUBDIRS}
do
        (cd $FILE && make install DESTDIR=%{buildroot} )
done

find  %{buildroot}/usr/lib/enlightenment/modules -name *.la | xargs rm
find  %{buildroot}/usr/lib/enlightenment/modules -name *.a | xargs rm

mkdir -p %{buildroot}/usr/etc/dfps
find . -name DynamicFPS.xml -exec cp '{}' %{buildroot}/usr/etc/dfps/ \;

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
%{_libdir}/enlightenment/modules/processmgr
%{_libdir}/enlightenment/modules/smack-checker
%{_datadir}/enlightenment/data/*
%{_bindir}/extndialog
%{_bindir}/elogwatcher
%{_bindir}/*
/usr/share/license/%{name}
/usr/share/locale/*
/etc/smack/accesses.d/e17-extra-modules.efl
/usr/etc/dfps/DynamicFPS.xml

%if "%{?tizen_profile_name}" == "mobile"
%exclude %{_libdir}/enlightenment/modules/comp-tizen/effect/micro.so
%elseif "%{?tizen_profile_name}" == "wearable"
%exclude %{_libdir}/enlightenment/modules/comp-tizen/effect/common.so
%endif
%exclude %{_libdir}/enlightenment/modules/comp-tizen/effect/mobile.so
