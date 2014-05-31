MAINTAINERCLEANFILES = Makefile.in

EDJE_CC = @EDJE_CC@
EDJE_FLAGS = -v \
-id $(top_srcdir)/data/images

pkgdatadir = $(datadir)
filesdir = $(pkgdatadir)/themes
files_DATA = \
e_mod_devmode.edj

EXTRA_DIST = \
e_mod_devmode.edc

e_mod_devmode.edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/e_mod_devmode.edc \
	$(top_builddir)/data/e_mod_devmode.edj

clean-local:
	rm -f *.edj
