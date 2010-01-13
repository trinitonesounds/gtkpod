SUBDIRS = icons man ui glade

desktopdir = $(datadir)/applications
desktop_in_files = gtkpod.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

appdatadir = $(pkgdatadir)/data
appdata_DATA =				\
	default-cover.png		\
	gtkpod-icon-32-2.png	\
	gtkpod-icon-32.png		\
	gtkpod-icon-48.png		\
	gtkpod-logo.png			\
	photo-toolbar-album.png \
	photo-toolbar-photos.png

EXTRA_DIST = 				\
	$(desktop_in_files)		\
	$(appdata_DATA)

CLEANFILES =				\
	$(desktop_DATA)			\
	gtkpod.gladep

include directories.mk

all-local: create-profile-link

create-profile-link: create-data-dir
	if test -e $(gtkpod_data_dir)/default.profile; then \
      # File already exists. Replacing ..." \
      sudo rm -rf $(gtkpod_data_dir)/default.profile; \
	fi; \
	# Creating link for default.profile" \
	sudo ln -s `pwd`/default.profile $(gtkpod_data_dir)/default.profile; \
	if test -e $(gtkpod_data_dir)/anjuta.session; then \
      # File already exists. Replacing ..." \
      sudo rm -rf $(gtkpod_data_dir)/anjuta.session; \
	fi; \
	# Creating link for anjuta.session" \
	sudo ln -s `pwd`/anjuta.session $(gtkpod_data_dir)/anjuta.session; \
	if test -e $(gtkpod_data_dir)/dock-layout.xml; then \
      # File already exists. Replacing ..." \
      sudo rm -rf $(gtkpod_data_dir)/dock-layout.xml; \
	fi; \
	# Creating link for dock-layout.xml" \
	sudo ln -s `pwd`/dock-layout.xml $(gtkpod_data_dir)/dock-layout.xml;

# Clean up the links and files created purely for dev testing
clean-local: clean-profile-link

clean-profile-link:
	if test -h $(gtkpod_data_dir)/default.profile; then \
      # Symbolic link exists. Removing ..." \
      sudo rm -f $(gtkpod_data_dir)/default.profile; \
	fi;