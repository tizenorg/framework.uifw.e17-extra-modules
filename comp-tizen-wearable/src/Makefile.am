MAINTAINERCLEANFILES = Makefile.in
MODULE = comp-tizen
SUBDIRS = effect

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
			 @HWC_CFLAGS@ \
			 @ENLIGHTENMENT_CFLAGS@

pkgdir                 = $(libdir)/enlightenment/modules/$(MODULE)/$(MODULE_ARCH)
pkg_LTLIBRARIES        = module.la
module_la_SOURCES      = e_mod_comp_atoms.c \
                         e_mod_comp_atoms.h \
                         e_mod_comp_aux_hint.c \
                         e_mod_comp_aux_hint.h \
                         e_mod_comp_bg_win.c \
                         e_mod_comp_bg_win.h \
                         e_mod_comp_cfdata.c \
                         e_mod_comp_cfdata.h \
                         e_mod_comp.c \
                         e_mod_comp.h \
                         e_mod_comp_canvas.c \
                         e_mod_comp_canvas.h \
                         e_mod_comp_debug.c \
                         e_mod_comp_debug.h \
                         e_mod_comp_effect.h \
                         e_mod_comp_effect_image_launch.h \
                         e_mod_comp_effect_win_rotation.h \
                         e_mod_comp_hw_ov_win.c \
                         e_mod_comp_hw_ov_win.h \
                         e_mod_comp_object.c \
                         e_mod_comp_object.h \
                         e_mod_comp_policy.c \
                         e_mod_comp_policy.h \
                         e_mod_comp_screen.c \
                         e_mod_comp_screen.h \
                         e_mod_comp_shared_types.h \
                         e_mod_comp_update.c \
                         e_mod_comp_update.h \
                         e_mod_comp_util.c \
                         e_mod_comp_util.h \
                         e_mod_comp_win_type.c \
                         e_mod_comp_win_type.h \
                         e_mod_comp_win_shape_input.c \
                         e_mod_comp_win_shape_input.h \
                         e_mod_comp_hwcomp.c \
                         e_mod_comp_hwcomp.h \
                         e_mod_config.c \
                         e_mod_config.h \
                         e_mod_main.c \
                         e_mod_main.h

module_la_LIBADD       = @ENLIGHTENMENT_LIBS@ @HWC_LIBS@ @dlopen_libs@
module_la_LDFLAGS      = -module -avoid-version
module_la_DEPENDENCIES = $(top_builddir)/config.h

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enlightenment/modules/$(MODULE)
