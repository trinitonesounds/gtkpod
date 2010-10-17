plugin_name = playlist_display
plugin_lib = lib$(plugin_name).so
plugin_file = $(plugin_name).plugin

# Plugin UI file
playlist_display_uidir = $(gtkpod_ui_dir)
playlist_display_ui_DATA = $(plugin_name).ui

# Plugin Glade file - No glade/xml file
playlist_display_gladedir = $(gtkpod_glade_dir)
playlist_display_glade_DATA = $(plugin_name).glade

# Plugin Toolbar file
playlist_display_toolbardir = $(gtkpod_glade_dir)
playlist_display_toolbar_DATA =  $(plugin_name)_toolbar.xml

# Plugin Icon file
playlist_display_pixmapsdir = $(gtkpod_image_dir)
playlist_display_pixmaps_DATA =

playlist_display_plugindir = $(gtkpod_plugin_dir)
playlist_display_plugin_DATA = $(plugin_file)

SUBDIRS = icons

# Plugin description file
plugin_in_files = $(plugin_file)

# Include paths
AM_CPPFLAGS = \
    -DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
    -DGTKPOD_DATA_DIR=\"$(gtkpod_data_dir)\" \
    -DGTKPOD_PLUGIN_DIR=\"$(gtkpod_plugin_dir)\" \
    -DGTKPOD_IMAGE_DIR=\"$(gtkpod_image_dir)\" \
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
								 playlist_display_actions.c playlist_display_actions.h \
								 playlist_display_context_menu.c playlist_display_context_menu.h \
								 playlist_display_preferences.c playlist_display_preferences.h

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
	$(playlist_display_toolbar_DATA) \
	$(playlist_display_pixmaps_DATA)

all-local: create-plugin-links create-ui-link create-glade-link

# Creating symbolic links in plugin root directory
create-plugin-links:
	echo "Creating plugin links"
	if [ ! -e ../$(plugin_lib) ]; then \
		ln -s `pwd`/.libs/$(plugin_lib) ../$(plugin_lib); \
	fi; \
	if [ ! -e ../$(plugin_file) ]; then \
		ln -s `pwd`/$(plugin_file) ../$(plugin_file); \
	fi;

# Creating symbolic link to ui file in installed ui directory
create-ui-link:
	if [ ! -e ../../data/ui/$(plugin_name).ui ]; then \
		ln -s `pwd`/$(plugin_name).ui ../../data/ui/$(plugin_name).ui; \
	fi;
	
create-glade-link:
	if  [ -e `pwd`/$(plugin_name).glade ]; then \
		if  [ ! -e ../../data/glade/$(plugin_name).glade ]; then \
			ln -s `pwd`/$(plugin_name).glade ../../data/glade/$(plugin_name).glade; \
		fi; \
	fi; \
	if  [ -e `pwd`/$(plugin_name).xml ]; then \
		if  [ ! -e ../../data/glade/$(plugin_name).xml ]; then \
			ln -s `pwd`/$(plugin_name).xml ../../data/glade/$(plugin_name).xml; \
		fi; \
	fi; \
	if  [ -e `pwd`/$(plugin_name)_toolbar.xml ]; then \
		if  [ ! -e ../../data/glade/$(plugin_name)_toolbar.xml ]; then \
			ln -s `pwd`/$(plugin_name)_toolbar.xml ../../data/glade/$(plugin_name)_toolbar.xml; \
		fi; \
	fi;

# Clean up the links and files created purely for dev  [ing
clean-local: clean-plugin-files clean-ui-dir clean-glade-dir

clean-plugin-files:
	if [ -h ../$(plugin_file) ]; then \
		rm -f ../$(plugin_lib) ../$(plugin_file); \
	fi;

clean-ui-dir:
	if [ -h ../../data/ui/$(plugin_name).ui ]; then \
		rm -f ../../data/ui/$(plugin_name).ui; \
	fi;
	
clean-glade-dir:
	if  [ -h ../../data/glade/$(plugin_name).glade ]; then \
		rm -f ../../data/glade/$(plugin_name).glade; \
	fi; \
		if  [ -h ../../data/glade/$(plugin_name).xml ]; then \
		rm -f ../../data/glade/$(plugin_name).xml; \
	fi; \
	if  [ -h ../../data/glade/$(plugin_name)_toolbar.xml ]; then \
		rm -f ../../data/glade/$(plugin_name)_toolbar.xml; \
	fi;
