Name:       e17-extra-modules
Summary:    The E17 Extra Modules The E17 extra modules consists of modules made by SAMSUNG
Version:    0.2
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
BuildRequires:  edje-tools
Requires: libx11

%description
The E17 Extra Modules  The E17 extra modules consists of modules made by SAMSUNG.

%prep
%setup -q


%build

%define DEF_SUBDIRS comp-slp illume2-slp keyrouter wmready

export CFLAGS+=" -Wall -g -fPIC -rdynamic -D_F_ENABLE_MOUSE_POPUP"
export LDFLAGS+=" -Wl,--hash-style=both -Wl,--as-needed -Wl,--rpath=/usr/lib"

%ifarch %{arm}
export CFLAGS+=" -D_ENV_ARM"
%endif


for FILE in %{DEF_SUBDIRS}
do 
        (cd $FILE && ./autogen.sh && ./configure --prefix=/usr && make )
done


%install
rm -rf %{buildroot}

for FILE in %{DEF_SUBDIRS}
do 
        (cd $FILE && make install DESTDIR=%{buildroot} )
done

find  %{buildroot}/usr/lib/enlightenment/modules -name *.la | xargs rm 

%files
%defattr(-,root,root,-)
%{_libdir}/enlightenment/modules/comp-slp/*
%{_libdir}/enlightenment/modules/illume2-slp/*
%{_libdir}/enlightenment/modules/e17-extra-module-keyrouter/*
%{_libdir}/enlightenment/modules/e17-extra-module-wmready/*
%{_datadir}/enlightenment/data/*
