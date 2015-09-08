MAINTAINERCLEANFILES = Makefile.in
MODULE = e17-extra-module-devicemgr

LDFLAGS +=

#CPPFLAGS = \
#	   -I. \
#	   -I$(includedir) \
#	   -I$(includedir)/enlightenment

# the module .so file
#INCLUDES               = -I. \
#			 -I$(includedir)
#                         -I$(includedir)/enlightenment \
#                         @e_cflags@
pkgdir                 = $(libdir)/enlightenment/modules/$(MODULE)/$(MODULE_ARCH)
pkg_LTLIBRARIES        = module.la
module_la_SOURCES      = e_mod_main.c \
                         e_mod_main.h \
                         e_mod_config.c \
                         e_mod_config.h \
                         e_devicemgr_privates.h \
                         e_mod_scrnconf.c \
                         e_mod_scrnconf.h \
						 scrnconf_devicemgr.c \
						 scrnconf_devicemgr.h \
						 e_mod_drv.c \
						 e_mod_drv.h \
						 virt_monitor_devicemgr.h \
						 hib_devicemgr.h \
						 rotation_devicemgr.c \
						 rotation_devicemgr.h

module_la_LIBADD       =
module_la_CFLAGS       = @ENLIGHTENMENT_CFLAGS@
module_la_LDFLAGS      = -module -avoid-version @ENLIGHTENMENT_LIBS@
module_la_DEPENDENCIES = $(top_builddir)/config.h

#uninstall:
#	rm -rf $(DESTDIR)$(libdir)/enlightenment/modules/$(MODULE)
