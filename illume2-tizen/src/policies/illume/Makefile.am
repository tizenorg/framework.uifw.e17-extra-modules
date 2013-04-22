MAINTAINERCLEANFILES = Makefile.in
MODULE = illume2-tizen
POLICY = illume

# the module .so file
INCLUDES = -I.. \
           -I$(top_srcdir) \
           -I$(top_srcdir)/src/ \
           -I$(top_srcdir)/src/policies/$(POLICY) \
           -I$(includedir) \
           @ENLIGHTENMENT_CFLAGS@

plugindir = $(libdir)/enlightenment/modules/$(MODULE)/policies

illumedir = $(plugindir)
illume_LTLIBRARIES = illume.la
illume_la_SOURCES = \
                  policy.h \
                  policy.c \
                  illume.h \
                  illume.c \
                  policy_util.h \
                  policy_util.c

illume_la_LIBADD       =
illume_la_CFLAGS       = @ENLIGHTENMENT_CFLAGS@
illume_la_LDFLAGS      = -module -avoid-version @ENLIGHTENMENT_LIBS@
illume_la_DEPENDENCIES = $(top_builddir)/config.h
illume_la_LIBTOOLFLAGS = --tag=disable-static

