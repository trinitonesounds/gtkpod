plugin_name = playlist_display
plugin_lib = lib$(plugin_name).so
plugin_file = $(plugin_name).plugin

# Plugin UI file
playlist_display_uidir = $(gtkpod_ui_dir)
playlist_display_ui_DATA = $(plugin_name).ui

# Plugin Glade file
playlist_display_gladedir = $(gtkpod_glade_dir)
playlist_display_glade_DATA =  $(plugin_name).glade

# Plugin Icon file
playlist_display_pixmapsdir = $(gtkpod_image_dir)
playlist_display_pixmaps_DATA =

# Plugin description file
plugin_in_files = $(plugin_file)

playlist_display_plugindir = $(gtkpod_plugin_dir)
playlist_display_plugin_DATA = $(plugin_file)

SUBDIRS = icons

# Include paths
AM_CPPFLAGS = \
    -DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
    -DGTKPOD_DATA_DIR=\"$(gtkpod_data_dir)\" \
    -DGTKPDO_PLUGIN_DIR=\"$(gtkpod_plugin_dir)\" \
    -DGTKPDO_IMAGE_DIR=\"$(gtkpod_image_dir)\" \
    -DGTKPOD_GLADE_DIR=\"$(gtkpod_glade_dir)\" \
    -DGTKPOD_UI_DIR=\"$(gtkpod_ui_dir)\" \
    -DPACKAGE_DATA_DIR=\"$(datadir)\" \
    -DPACKAGE_SRC_DIR=\"$(srcdir)\" \
    $(GTKPOD_CFLAGS)

# Where to install the plugin
plugindir = $(gtkpod_plugin_dir)

# The plugin
plugin_LTLIBRARIES = libplaylist_display.la

# Plugin sources
libplaylist_display_la_SOURCES = plugin.c plugin.h \
								 display_playlists.c display_playlists.h \
								 file_export.c file_export.h \
								 playlist_display_actions.c playlist_display_actions.h

libplaylist_display_la_LDFLAGS = $(GTKPOD_PLUGIN_LDFLAGS)

# Plugin dependencies
libplaylist_display_la_LIBADD = \
    $(LIBGTKPOD_LIBS) \
    $(LIBANJUTA_LIBS)

EXTRA_DIST = \
	$(plugin_in_files) \
	$(playlist_display_plugin_DATA) \
	$(playlist_display_ui_DATA) \
	$(playlist_display_glade_DATA) \
	$(playlist_display_pixmaps_DATA)

include ../../data/directories.mk

all-local: create-plugin-links create-ui-link create-glade-link copy-icons-dir

# Creating symbolic links in plugin root directory
create-plugin-links:
	echo "Creating plugin links"
	if test ! -e ../$(plugin_lib); then \
	  ln -s `pwd`/.libs/$(plugin_lib) ../$(plugin_lib); \
	fi; \
	if test ! -e ../$(plugin_file); then \
	  ln -s `pwd`/$(plugin_file) ../$(plugin_file); \
	fi;
	
# Creating symbolic link to ui file in installed ui directory
create-ui-link: create-ui-dir
	if test -e $(gtkpod_ui_dir)/$(playlist_display_ui_DATA); then \
	  # File already exists. Replacing ..." \
	  sudo rm -f $(gtkpod_ui_dir)/$(playlist_display_ui_DATA); \
	fi; \
	# Creating link for $(playlist_display_ui_DATA)" \
	sudo ln -s `pwd`/$(playlist_display_ui_DATA) $(gtkpod_ui_dir)/$(playlist_display_ui_DATA);

create-glade-link: create-glade-dir
	if test -e $(gtkpod_glade_dir)/$(playlist_display_glade_DATA); then \
	  # File already exists. Replacing ..." \
	  sudo rm -f $(gtkpod_glade_dir)/$(playlist_display_glade_DATA); \
	fi; \
	# Creating link for $(playlist_display_glade_DATA)" \
	sudo ln -s `pwd`/$(playlist_display_glade_DATA) $(gtkpod_glade_dir)/$(playlist_display_glade_DATA);

# Copy icons
copy-icons-dir: create-icons-dir
	sudo cp -rf `pwd`/icons/* $(gtkpod_image_dir)/; \
	sudo chmod -R 755 $(gtkpod_image_dir)/*; \
	sudo find $(gtkpod_image_dir) -name 'Makefile*' -delete;

# Clean up the links and files created purely for dev testing
clean-local: clean-plugin-files clean-ui-dir clean-glade-dir clean-image-dir

clean-plugin-files:
	rm -f ../$(plugin_lib) ../$(plugin_file)

clean-ui-dir:
	if test -h $(gtkpod_ui_dir)/$(playlist_display_ui_DATA); then \
	  # Symbolic link exists. Removing ..." \
	  sudo rm -f $(gtkpod_ui_dir)/$(playlist_display_ui_DATA); \
	fi;

clean-glade-dir:
	if test -h $(gtkpod_glade_dir)/$(playlist_display_glade_DATA); then \
	  # Symbolic link exists. Removing ..." \
	  sudo rm -f $(gtkpod_glade_dir)/$(playlist_display_glade_DATA); \
	fi;

clean-image-dir:
	if test -d $(gtkpod_image_dir); then \
	  sudo find $(gtkpod_image_dir) -name 'playlist_display*' -delete; \
	fi;