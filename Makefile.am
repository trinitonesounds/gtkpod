## Process this file with automake to produce Makefile.in

SUBDIRS = libgtkpod src po scripts data icons doc plugins

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgtkpod-1.0.pc

EXTRA_DIST =			\
	autogen.sh		\
	intltool-extract.in	\
	intltool-merge.in	\
	intltool-update.in	\
	TROUBLESHOOTING		\
	TODOandBUGS.txt \
	libgtkpod-1.0.pc.in

DISTCLEANFILES =		\
	intltool-extract	\
	intltool-merge		\
	intltool-update

uninstall-hook:
	-rmdir --ignore-fail-on-non-empty $(DESTDIR)$(pkgdatadir)/data/ui
	-rmdir --ignore-fail-on-non-empty $(DESTDIR)$(pkgdatadir)/data/glade
	-rmdir --ignore-fail-on-non-empty $(DESTDIR)$(pkgdatadir)/data/icons
	-rmdir --ignore-fail-on-non-empty $(DESTDIR)$(pkgdatadir)/data
	-rmdir --ignore-fail-on-non-empty $(DESTDIR)$(pkgdatadir)/scripts
	-rmdir --ignore-fail-on-non-empty $(DESTDIR)$(pkgdatadir)

update-docs:
	cd doc && make update-docs
