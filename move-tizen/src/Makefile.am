MAINTAINERCLEANFILES = Makefile.in
MODULE = move-tizen

LDFLAGS +=

pkgdir                 = $(libdir)/enlightenment/modules/$(MODULE)/$(MODULE_ARCH)
pkg_LTLIBRARIES        = module.la
module_la_SOURCES      = e_mod_main.c \
                         e_mod_main.h \
                         e_mod_move.c \
                         e_mod_move.h \
                         e_mod_move_atoms.c \
                         e_mod_move_atoms.h \
                         e_mod_move_border_shape_input.c \
                         e_mod_move_border_shape_input.h \
                         e_mod_move_border_contents.c \
                         e_mod_move_border_contents.h \
                         e_mod_move_border_type.c \
                         e_mod_move_border_type.h \
                         e_mod_move_canvas.c \
                         e_mod_move_canvas.h \
                         e_mod_move_cfdata.c \
                         e_mod_move_cfdata.h \
                         e_mod_move_debug.c \
                         e_mod_move_debug.h \
                         e_mod_move_event.c \
                         e_mod_move_event.h \
                         e_mod_move_object.c \
                         e_mod_move_object.h \
                         e_mod_move_control_object.c \
                         e_mod_move_control_object.h \
                         e_mod_move_dim_object.c \
                         e_mod_move_dim_object.h \
                         e_mod_move_evas_object.c \
                         e_mod_move_evas_object.h \
                         e_mod_move_shared_types.h \
                         e_mod_move_util.c \
                         e_mod_move_util.h \
                         e_mod_move_indicator.c \
                         e_mod_move_indicator.h \
                         e_mod_move_apptray.c \
                         e_mod_move_apptray.h \
                         e_mod_move_quickpanel.c \
                         e_mod_move_quickpanel.h \
                         e_mod_move_mini_apptray.c \
                         e_mod_move_mini_apptray.h \
                         e_mod_move_lockscreen.c \
                         e_mod_move_lockscreen.h \
                         e_mod_move_taskmanager.c \
                         e_mod_move_taskmanager.h \
                         e_mod_move_pwlock.c \
                         e_mod_move_pwlock.h \
                         e_mod_move_flick.c \
                         e_mod_move_flick.h \
                         e_mod_move_widget_object.c \
                         e_mod_move_widget_object.h \
                         e_mod_move_indicator_widget.c \
                         e_mod_move_indicator_widget.h \
                         e_mod_move_mini_apptray_widget.c \
                         e_mod_move_mini_apptray_widget.h
module_la_LIBADD       =
module_la_CFLAGS       = @ENLIGHTENMENT_CFLAGS@
module_la_LDFLAGS      = -module -avoid-version @ENLIGHTENMENT_LIBS@
module_la_DEPENDENCIES = $(top_builddir)/config.h
