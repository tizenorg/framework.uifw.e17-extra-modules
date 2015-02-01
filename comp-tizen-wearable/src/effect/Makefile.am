MAINTAINERCLEANFILES = Makefile.in
MODULE = comp-tizen
SUB_MODULE = effect

#EDJE_CC = @edje_cc@
#EDJE_FLAGS = -v \
#           @EDJE_DEF@

# data files for the module
filesdir = $(libdir)/enlightenment/modules/$(MODULE)/$(SUB_MODULE)

EXTRA_DIST = $(files_DATA)

# the module .so file
INCLUDES               = -I. \
                         -I$(top_srcdir) \
                         -I$(top_srcdir)/src \
                         -I$(top_srcdir)/src/effect \
                         @HWC_CFLAGS@ \
			 @ENLIGHTENMENT_CFLAGS@

pkgdir                 = $(filesdir)
pkg_LTLIBRARIES        = common.la micro.la mobile.la
common_la_SOURCES      = effect.h \
                         effect.c \
                         effect_image_launch.h \
                         effect_image_launch.c \
                         effect_win_rotation.h \
                         effect_win_rotation.c

common_la_LIBADD       = @ENLIGHTENMENT_LIBS@ @HWC_LIBS@ @dlopen_libs@
common_la_LDFLAGS      = -module -avoid-version
common_la_DEPENDENCIES = $(top_builddir)/config.h

micro_la_SOURCES      = effect.h \
                       effect.c \
                       effect_pos_animation.h \
                       effect_pos_animation.c \
                       effect_image_launch.h \
                       effect_image_launch.c \
                       effect_win_rotation.h \
                       effect_win_rotation.c

micro_la_LIBADD       = @ENLIGHTENMENT_LIBS@ @HWC_LIBS@ @dlopen_libs@
micro_la_LDFLAGS      = -module -avoid-version
micro_la_DEPENDENCIES = $(top_builddir)/config.h

mobile_la_SOURCES      = effect.h \
                       effect.c \
                       effect_image_launch.h \
                       effect_image_launch.c \
                       effect_win_rotation.h \
                       effect_win_rotation.c

mobile_la_LIBADD       = @ENLIGHTENMENT_LIBS@ @HWC_LIBS@ @dlopen_libs@
mobile_la_LDFLAGS      = -module -avoid-version
mobile_la_DEPENDENCIES = $(top_builddir)/config.h
