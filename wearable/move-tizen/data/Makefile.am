MAINTAINERCLEANFILES = Makefile.in

EDJE_CC = @EDJE_CC@
EDJE_FLAGS = -v

filesdir = /usr/lib/enlightenment/modules/move-tizen
files_DATA = \
effect.edj

EXTRA_DIST = \
effect.edc

shadow.edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/effect.edc \
	$(top_builddir)/data/effect.edj

clean-local:
	rm -f *.edj
