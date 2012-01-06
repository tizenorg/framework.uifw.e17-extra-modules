MAINTAINERCLEANFILES = Makefile.in 
MODULE = illume2-slp
SUBDIRS = images policies

EDJE_CC = @EDJE_CC@
EDJE_FLAGS = -v -id $(top_srcdir)/src/images

# data files for the module
filesdir = $(libdir)/enlightenment/modules/$(MODULE)
files_DATA = \
           e-module-$(MODULE).edj \
           quickpanel.edj 


EXTRA_DIST = \
           e-module-$(MODULE).edc \
           quickpanel.edc \
           $(top_srcdir)/src/images/*.png 


# the module .so file
INCLUDES               = -I. \
                         -I$(top_srcdir) \
                         -I$(top_srcdir)/src/ \
                         -I$(includedir) \
                         @ENLIGHTENMENT_CFLAGS@

pkgdir                 = $(libdir)/enlightenment/modules/$(MODULE)/$(MODULE_ARCH)
pkg_LTLIBRARIES        = module.la

module_la_SOURCES = \
                  e_illume.h \
                  e_illume.c \
                  e_illume_private.h \
                  e_mod_config_policy.h \
                  e_mod_config_policy.c \
                  e_mod_select_window.h \
                  e_mod_select_window.c \
                  e_mod_config_windows.h \
                  e_mod_config_windows.c \
                  e_mod_config_animation.h \
                  e_mod_config_animation.c \
                  e_mod_quickpanel.h \
                  e_mod_quickpanel.c \
                  e_mod_policy.h \
                  e_mod_policy.c \
                  e_mod_config.h \
                  e_mod_config.c \
                  e_mod_main.h \
                  e_mod_main.c

module_la_LIBADD       = @dlopen_libs@
module_la_CFLAGS       = @ENLIGHTENMENT_CFLAGS@
module_la_LDFLAGS      = -module -avoid-version @ENLIGHTENMENT_LIBS@
module_la_DEPENDENCIES = $(top_builddir)/config.h
module_la_LIBTOOLFLAGS = --tag=disable-static


e-module-$(MODULE).edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/src/e-module-$(MODULE).edc \
	$(top_builddir)/src/e-module-$(MODULE).edj

quickpanel.edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/src/quickpanel.edc \
	$(top_builddir)/src/quickpanel.edj

.PHONY: doc

# Documentation

doc:
	@echo "entering doc/"
	make -C doc doc

clean-local:
	rm -f *.edj

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enlightenment/modules/$(MODULE)
	rm -rf $(DESTDIR)$(libdir)/enlightenment/modules/$(MODULE)/policies
