ACLOCAL_AMFLAGS = -I m4
MAINTAINERCLEANFILES = Makefile.in aclocal.m4 config.guess config.h.in \
		       config.sub configure depcomp install-sh ltmain.sh \
		       missing module.desktop config.rpath mkinstalldirs

SUBDIRS = src \
data

filesdir = $(datadir)
files_DATA = module.desktop \
	     $(wildcard images/*.png)

EXTRA_DIST = module.desktop.in

clean-local:
	rm -rf module.desktop *~

uninstall:
	rm -rf $(DESTDIR)$(datadir)

