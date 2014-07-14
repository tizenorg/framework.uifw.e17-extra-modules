MAINTAINERCLEANFILES = Makefile.in
MODULE = e17-extra-module-eom

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
                         e_mod_eom_config.c \
                         e_mod_eom_config.h \
                         e_mod_eom_privates.h \
                         e_mod_eom_scrnconf.c \
                         e_mod_eom_scrnconf.h \
						 e_mod_eom_server.c \
						 e_mod_eom_server.h \
						 e_mod_eom_clone.c \
						 e_mod_eom_clone.h \
						 e_mod_eom_ownership.c \
						 e_mod_eom_onwership.h \
                         e_mod_eom_dbus.c \
                         e_mod_eom_dbus.h

module_la_LIBADD       =
module_la_CFLAGS       = @ENLIGHTENMENT_CFLAGS@ -Wall -Werror
module_la_LDFLAGS      = -module -avoid-version @ENLIGHTENMENT_LIBS@
module_la_DEPENDENCIES = $(top_builddir)/config.h

#uninstall:
#	rm -rf $(DESTDIR)$(libdir)/enlightenment/modules/$(MODULE)
