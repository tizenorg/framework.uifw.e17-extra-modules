bin_PROGRAMS = elogwatcher

elogwatcher_CPPFLAGS = \
-I. \
@ELOGWATCHER_CFLAGS@
elogwatcher_CFLAGS = \
-fPIE

elogwatcher_LDFLAGS = \
-pie

elogwatcher_LDADD = \
@ELOGWATCHER_LIBS@

elogwatcher_SOURCES = \
elogwatcher.c

MAINTAINERCLEANFILES = \
Makefile.in 
