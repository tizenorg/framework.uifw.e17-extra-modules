MAINTAINERCLEANFILES = Makefile.in
MODULE = processmgr

LDFLAGS +=

pkgdir                 = $(libdir)/enlightenment/modules/$(MODULE)/$(MODULE_ARCH)
pkg_LTLIBRARIES        = module.la
module_la_SOURCES      = e_mod_main.c \
                         e_mod_main.h \
                         e_mod_processmgr.c \
                         e_mod_processmgr.h \
                         e_mod_processmgr_cfdata.c \
                         e_mod_processmgr_cfdata.h \
                         e_mod_processmgr_debug.c \
                         e_mod_processmgr_debug.h \
                         e_mod_processmgr_dfps.c \
                         e_mod_processmgr_dfps.h \
                         e_mod_processmgr_shared_types.h \
                         e_mod_processmgr_util.c \
                         e_mod_processmgr_util.h
module_la_LIBADD       =
module_la_CFLAGS       = @ENLIGHTENMENT_CFLAGS@
module_la_LDFLAGS      = -module -avoid-version @ENLIGHTENMENT_LIBS@
module_la_DEPENDENCIES = $(top_builddir)/config.h
