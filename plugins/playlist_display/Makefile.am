plugin_name = playlist_display
plugin_file = $(plugin_name).plugin

# Plugin UI file
playlist_display_uidir = $(gtkpod_ui_dir)
playlist_display_ui_DATA = $(plugin_name).ui

# Plugin Glade file - No glade/xml file
playlist_display_gladedir = $(gtkpod_glade_dir)
playlist_display_glade_DATA = $(plugin_name).xml

# Plugin Toolbar file
playlist_display_toolbardir = $(gtkpod_glade_dir)
playlist_display_toolbar_DATA =  $(plugin_name)_toolbar.xml

# Plugin Icon file
playlist_display_pixmapsdir = $(gtkpod_image_dir)
playlist_display_pixmaps_DATA =

playlist_display_plugindir = $(gtkpod_plugin_dir)
playlist_display_plugin_DATA =

SUBDIRS = icons

include ../plugins.mk
playlist_display.plugin: build-plugin-file

# The plugin
plugin_lib = lib$(plugin_name).so
plugin_LTLIBRARIES = libplaylist_display.la

# Plugin sources
libplaylist_display_la_SOURCES = plugin.c plugin.h \
								 display_playlists.c display_playlists.h \
								 playlist_display_actions.c playlist_display_actions.h \
								 playlist_display_context_menu.c playlist_display_context_menu.h \
								 playlist_display_preferences.c playlist_display_preferences.h \
								 playlist_display_spl.c playlist_display_spl.h

libplaylist_display_la_LDFLAGS = $(GTKPOD_PLUGIN_LDFLAGS)

# Plugin dependencies
libplaylist_display_la_LIBADD = \
    $(GTKPOD_LIBS) \
    $(LIBANJUTA_LIBS)

EXTRA_DIST = \
	$(plugin_file).in \
	$(playlist_display_plugin_DATA) \
	$(playlist_display_ui_DATA) \
	$(playlist_display_glade_DATA) \
	$(playlist_display_toolbar_DATA) \
	$(playlist_display_pixmaps_DATA)
