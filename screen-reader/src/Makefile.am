MAINTAINERCLEANFILES = Makefile.in
MODULE = screen-reader

LDFLAGS +=

# the module .so file
pkgdir                 = $(libdir)/enlightenment/modules/$(MODULE)/$(MODULE_ARCH)
pkg_LTLIBRARIES        = module.la
module_la_SOURCES      = e_mod_main.c \
                         e_mod_main.h \
			               e_mod_config.c
module_la_LIBADD       =
module_la_CFLAGS       = @ENLIGHTENMENT_CFLAGS@
module_la_LDFLAGS      = -module -avoid-version @ENLIGHTENMENT_LIBS@
module_la_DEPENDENCIES = $(top_builddir)/config.h
