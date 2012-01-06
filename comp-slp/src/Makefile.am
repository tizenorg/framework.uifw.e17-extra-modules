MAINTAINERCLEANFILES = Makefile.in
MODULE = comp-slp

#EDJE_CC = @edje_cc@
#EDJE_FLAGS = -v \
#           @EDJE_DEF@
           
# data files for the module
filesdir = $(libdir)/enlightenment/modules/$(MODULE)

EXTRA_DIST = $(files_DATA)

# the module .so file
INCLUDES               = -I. \
                         -I$(top_srcdir) \
                         -I$(top_srcdir)/src \
			 @ENLIGHTENMENT_CFLAGS@
			 
pkgdir                 = $(libdir)/enlightenment/modules/$(MODULE)/$(MODULE_ARCH)
pkg_LTLIBRARIES        = module.la
module_la_SOURCES      = e_mod_main.c \
                         e_mod_main.h \
                         e_mod_config.c \
                         e_mod_config.h \
                         e_mod_comp_data.h \
                         e_mod_comp.c \
                         e_mod_comp.h \
                         e_mod_comp_update.c \
                         e_mod_comp_update.h \
                         e_mod_comp_rotation.c \
                         e_mod_comp_rotation.h \
                         e_mod_comp_policy.c \
                         e_mod_comp_effect.c \
                         e_mod_comp_animation.c \
                         e_mod_comp_animation.h

module_la_LIBADD       = @ENLIGHTENMENT_LIBS@ @dlopen_libs@
module_la_LDFLAGS      = -module -avoid-version
module_la_DEPENDENCIES = $(top_builddir)/config.h

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enlightenment/modules/$(MODULE)
