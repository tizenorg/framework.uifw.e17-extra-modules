MAINTAINERCLEANFILES = Makefile.in
MODULE = devmode-tizen

# data files for the module
filesdir = $(libdir)/enlightenment/modules/$(MODULE)

EXTRA_DIST = $(files_DATA)

# the module .so file
INCLUDES               = -I. \
                         -I$(top_srcdir) \
                         -I$(top_srcdir)/include \
                         -I$(top_srcdir)/src \
			 @ENLIGHTENMENT_CFLAGS@

pkgdir                 = $(libdir)/enlightenment/modules/$(MODULE)/$(MODULE_ARCH)
pkg_LTLIBRARIES        = module.la
module_la_SOURCES      = e_mod_main.c \
                         e_mod_main.h \
                         e_mod_config.c \
                         e_mod_config.h \
                         e_mod_devmode_main.c \
                         e_mod_devmode_main.h \
                         e_mod_devmode_color.c \
                         e_mod_devmode_color.h \
                         e_mod_devmode_event.c \
                         e_mod_devmode_event.h \
                         e_mod_devmode_trace_widget.c \
                         e_mod_devmode_trace_widget.h \
                         e_mod_devmode_cross_widget.c \
                         e_mod_devmode_cross_widget.h

module_la_LIBADD       = @ENLIGHTENMENT_LIBS@ @dlopen_libs@
module_la_LDFLAGS      = -module -avoid-version
module_la_DEPENDENCIES = $(top_builddir)/config.h

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enlightenment/modules/$(MODULE)
