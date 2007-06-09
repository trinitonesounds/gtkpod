SUBDIRS = icons

desktopdir = $(datadir)/applications
desktop_in_files = gtkpod.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

appdatadir = $(pkgdatadir)/data
appdata_DATA =				\
	gtkpod.glade			\
	cdshine.png			\
	cdshine_main.png		\
	default-cover.png		\
	gtkpod-add-dirs.png		\
	gtkpod-add-files.png		\
	gtkpod-add-playlists.png	\
	gtkpod-icon-32-2.png		\
	gtkpod-icon-32.png		\
	gtkpod-icon-48.png		\
	gtkpod-logo.png			\
	gtkpod-new-playlist.png		\
	gtkpod-read.png			\
	gtkpod-sync.png

EXTRA_DIST = 				\
	$(desktop_in_files)		\
	$(appdata_DATA)

CLEANFILES =				\
	$(desktop_DATA)			\
	gtkpod.gladep
