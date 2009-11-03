## Process this file with automake to produce Makefile.in

SUBDIRS = libgtkpod src po scripts data doc plugins

EXTRA_DIST =			\
	autogen.sh		\
	intltool-extract.in	\
	intltool-merge.in	\
	intltool-update.in	\
	TROUBLESHOOTING		\
	TODOandBUGS.txt

DISTCLEANFILES =		\
	intltool-extract	\
	intltool-merge		\
	intltool-update

uninstall-hook:
	-rmdir --ignore-fail-on-non-empty $(DESTDIR)$(pkgdatadir)/data
	-rmdir --ignore-fail-on-non-empty $(DESTDIR)$(pkgdatadir)/scripts
	-rmdir --ignore-fail-on-non-empty $(DESTDIR)$(pkgdatadir)

update-docs:
	cd doc && make update-docs
