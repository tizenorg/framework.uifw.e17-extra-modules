MAINTAINERCLEANFILES = Makefile.in

EDJE_CC = @EDJE_CC@
EDJE_FLAGS = -v \
-id $(top_srcdir)/data/images

filesdir = /usr/share/enlightenment/data/themes
files_DATA = \
shadow.edj

EXTRA_DIST = \
shadow.edc

shadow.edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/shadow.edc \
	$(top_builddir)/data/shadow.edj

clean-local:
	rm -f *.edj
