SUBDIRS = man ui glade

desktopdir = $(datadir)/applications
desktop_in_files = gtkpod.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

appdatadir = $(pkgdatadir)/data
appdata_DATA =				\
	gtkpod-icon-32-2.png	\
	gtkpod-icon-32.png		\
	gtkpod-icon-48.png		\
	gtkpod-logo.png			\
	anjuta.session \
	default.profile \
	dock-layout.xml

EXTRA_DIST = 				\
	$(desktop_in_files)		\
	$(appdata_DATA)

CLEANFILES =				\
	$(desktop_DATA)			\
	gtkpod.gladep

# Creating symbolic links in plugin root directory
COPYING: $(top_srcdir)/COPYING
	ln -s $(top_srcdir)/COPYING COPYING

AUTHORS: $(top_srcdir)/AUTHORS
	ln -s $(top_srcdir)/AUTHORS AUTHORS

all-local: COPYING AUTHORS
clean-local: clean-licence clean-authors

# Clean up the links and files created purely for developing
clean-licence:
	-rm -f COPYING

clean-authors:
	-rm -f AUTHORS

dist-hook: clean-licence clean-authors

.PHONY: clean-licence clean-authors
