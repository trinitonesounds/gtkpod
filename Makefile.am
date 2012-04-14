## Process this file with automake to produce Makefile.in

SUBDIRS = libgtkpod libs src po scripts data icons doc plugins

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgtkpod-1.1.0.pc

aboutdir = $(pkgdatadir)/data
about_DATA = COPYING \
			 AUTHORS

EXTRA_DIST =			\
	autogen.sh		\
	CMakeLists.txt \
	cmake \
	generate-ChangeLog.sh \
	intltool-extract.in	\
	intltool-merge.in	\
	intltool-update.in	\
	TROUBLESHOOTING		\
	TODO \
	libgtkpod-1.1.0.pc.in \
	version.sh \
	version

DISTCLEANFILES =		\
	intltool-extract	\
	intltool-merge		\
	intltool-update

dist-hook: configure.ac
	sh $(top_srcdir)/generate-ChangeLog.sh $(top_srcdir) $(distdir)

uninstall-hook:
	-rm -rf $(DESTDIR)$(pkgdatadir)

update-docs:
	cd doc && make update-docs

