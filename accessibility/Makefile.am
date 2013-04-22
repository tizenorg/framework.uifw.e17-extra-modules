# maintainer-clean removes everything
MAINTAINERCLEANFILES = aclocal.m4 compile config.sub config.guess config.h.in \
		       configure depcomp install-sh ltmain.sh Makefile.in missing

SUBDIRS = src

filesdir = $(datadir)
files_DATA = module.desktop

EXTRA_DIST = module.desktop.in

clean-local:
	rm -rf module.desktop

uninstall:
	rm -rf $(DESTDIR)$(datadir)


