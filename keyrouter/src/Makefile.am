MAINTAINERCLEANFILES = Makefile.in
MODULE = e17-extra-module-keyrouter

LDFLAGS += 
#LDFLAGS += -lvconf -lappfwk

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
                         e_mod_main.h
module_la_LIBADD       = 
module_la_CFLAGS       = @ENLIGHTENMENT_CFLAGS@
module_la_LDFLAGS      = -module -avoid-version @ENLIGHTENMENT_LIBS@
module_la_DEPENDENCIES = $(top_builddir)/config.h

#uninstall:
#	rm -rf $(DESTDIR)$(libdir)/enlightenment/modules/$(MODULE)
