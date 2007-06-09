## Process this file with automake to produce Makefile.in

SUBDIRS = src po scripts data

EXTRA_DIST = \
	autogen.sh \
	TROUBLESHOOTING \
	TODOandBUGS.txt

uninstall-hook:
	-rmdir --ignore-fail-on-non-empty $(DESTDIR)$(pkgdatadir)/data
	-rmdir --ignore-fail-on-non-empty $(DESTDIR)$(pkgdatadir)/scripts
	-rmdir --ignore-fail-on-non-empty $(DESTDIR)$(pkgdatadir)
