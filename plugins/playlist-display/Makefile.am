plugin_name = playlist_display
plugin_lib = lib$(plugin_name).so
plugin_file = $(plugin_name).plugin

# Plugin UI file
playlist_display_uidir = $(gtkpod_ui_dir)
playlist_display_ui_DATA =  $(plugin_name).ui

# Plugin Glade file
playlist_display_gladedir = $(gtkpod_glade_dir)
playlist_display_glade_DATA =  $(plugin_name).glade

# Plugin Icon file
playlist_display_pixmapsdir = $(gtkpod_image_dir)
playlist_display_pixmaps_DATA = $(plugin_name).png

# Plugin description file
plugin_in_files = $(plugin_file)

playlist_display_plugindir = $(gtkpod_plugin_dir)
playlist_display_plugin_DATA = $(plugin_file)

# Include paths
AM_CPPFLAGS = \
    -DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
    -DGTKPOD_DATA_DIR=\"$(gtkpod_data_dir)\" \
    -DGTKPDO_PLUGIN_DIR=\"$(gtkpod_plugin_dir)\" \
    -DGTKPDO_IMAGE_DIR=\"$(gtkpod_image_dir)\" \
    -DGTKPDO_GLADE_DIR=\"$(gtkpod_glade_dir)\" \
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
								 display_playlists.c display_playlists.h

libplaylist_display_la_LDFLAGS = $(GTKPOD_PLUGIN_LDFLAGS)

# Plugin dependencies
libplaylist_display_la_LIBADD = \
    $(GCONF_LIBS) \
    $(GLADE_LIBS) \
    $(VTE_LIBS) \
    $(LIBANJUTA_LIBS)

EXTRA_DIST = \
	$(plugin_in_files) \
	$(playlist_display_plugin_DATA) \
	$(playlist_display_ui_DATA) \
	$(playlist_display_glade_DATA) \
	$(playlist_display_pixmaps_DATA)

all-local: create-plugin-links create-ui-link

# Creating symbolic links in plugin root directory
create-plugin-links:
	echo "Creating plugin links"
	if test ! -e ../$(plugin_lib); then \
	  ln -s `pwd`/.libs/$(plugin_lib) ../$(plugin_lib); \
	fi; \
	if test ! -e ../$(plugin_file); then \
	  ln -s `pwd`/$(plugin_file) ../$(plugin_file); \
	fi;
	
create-ui-link: create-ui-dir
	if test -e $(gtkpod_ui_dir)/$(playlist_display_ui_DATA); then \
	  # File already exists. Replacing ..." \
	  sudo rm -f $(gtkpod_ui_dir)/$(playlist_display_ui_DATA); \
	fi; \
	# Creating link for $(playlist_display_ui_DATA)" \
	sudo ln -s `pwd`/$(playlist_display_ui_DATA) $(gtkpod_ui_dir)/$(playlist_display_ui_DATA);

create-ui-dir: create-data-dir
	if test ! -d $(gtkpod_ui_dir); then \
	  sudo mkdir $(gtkpod_ui_dir); \
	fi;

create-data-dir: create-base-data-dir
	if test ! -d $(gtkpod_data_dir); then \
	  sudo mkdir $(gtkpod_data_dir); \
	fi;

create-base-data-dir:
	if test ! -d $(datadir)/$(PACKAGE); then \
	  sudo mkdir $(datadir)/$(PACKAGE); \
	fi;


clean-local: clean-ui-dir
	rm -f ../$(plugin_lib) ../$(plugin_file)

clean-ui-dir:
	if test -h $(gtkpod_ui_dir)/$(playlist_display_ui_DATA); then \
	  # Symbolic link exists. Removing ..." \
	  sudo rm -f $(gtkpod_ui_dir)/$(playlist_display_ui_DATA); \
	fi;

