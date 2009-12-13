SUBDIRS = icons man ui

desktopdir = $(datadir)/applications
desktop_in_files = gtkpod.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

appdatadir = $(pkgdatadir)/data
appdata_DATA =				\
	gtkpod.glade			\
	default-cover.png		\
	gtkpod-add-dirs.png		\
	gtkpod-add-files.png		\
	gtkpod-add-playlists.png	\
	gtkpod-icon-32-2.png		\
	gtkpod-icon-32.png		\
	gtkpod-icon-48.png		\
	gtkpod-logo.png			\
	gtkpod-read.png			\
	gtkpod-read-16.png		\
	photo-toolbar-album.png \
	photo-toolbar-photos.png

EXTRA_DIST = 				\
	$(desktop_in_files)		\
	$(appdata_DATA)

CLEANFILES =				\
	$(desktop_DATA)			\
	gtkpod.gladep

all-local: create-profile-link

create-profile-link: create-data-dir
	if test -e $(gtkpod_data_dir)/default.profile; then \
      # File already exists. Replacing ..." \
      sudo rm -rf $(gtkpod_data_dir)/default.profile; \
	fi; \
	# Creating link for default.profile" \
	sudo ln -s `pwd`/default.profile $(gtkpod_data_dir)/default.profile;
	

create-data-dir: create-base-data-dir
	if test ! -d $(gtkpod_data_dir); then \
      sudo mkdir $(gtkpod_data_dir); \
	fi;

create-base-data-dir:
	if test ! -d $(datadir)/$(PACKAGE); then \
      sudo mkdir $(datadir)/$(PACKAGE); \
	fi;

# Clean up the links and files created purely for dev testing
clean-local: clean-profile-link

clean-profile-link:
	if test -h $(gtkpod_data_dir)/default.profile; then \
      # Symbolic link exists. Removing ..." \
      sudo rm -f $(gtkpod_data_dir)/default.profile; \
	fi;