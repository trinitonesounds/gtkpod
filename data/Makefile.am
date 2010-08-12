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
