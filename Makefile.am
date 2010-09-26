## Process this file with automake to produce Makefile.in

SUBDIRS = libgtkpod src po scripts data icons doc plugins

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgtkpod-1.0.pc

aboutdir = $(pkgdatadir)/data
about_DATA = COPYING \
			 AUTHORS

EXTRA_DIST =			\
	autogen.sh		\
	CMakeLists.txt \
	cmake \
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
	-rm -rf $(DESTDIR)$(pkgdatadir)

update-docs:
	cd doc && make update-docs
	
all-local: copy-licence copy-authors

# Creating symbolic links in plugin root directory
copy-licence:
	echo "Copying licence file"
	if [ ! -e data/COPYING ]; then \
		ln -s `pwd`/COPYING data/COPYING; \
	fi;

copy-authors:
	echo "Copying authors file"
	if [ ! -e data/AUTHORS ]; then \
		ln -s `pwd`/AUTHORS data/AUTHORS; \
	fi;