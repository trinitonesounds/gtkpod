plugin_name = external_player
plugin_file = $(plugin_name).plugin

# Plugin UI file
external_player_uidir = $(gtkpod_ui_dir)
external_player_ui_DATA = $(plugin_name).ui

# Plugin Glade file
external_player_gladedir = $(gtkpod_glade_dir)
external_player_glade_DATA =  $(plugin_name).xml

# Plugin Icon file
external_player_pixmapsdir = $(gtkpod_image_dir)
external_player_pixmaps_DATA =

# Where to install the plugin
external_player_plugindir = $(gtkpod_plugin_dir)
external_player_plugin_DATA =

SUBDIRS =

include ../plugins.mk
external_player.plugin: build-plugin-file

# The plugin
plugin_lib = lib$(plugin_name).so
plugin_LTLIBRARIES = libexternal_player.la

# Plugin sources
libexternal_player_la_SOURCES = plugin.c plugin.h \
						 	    external_player.c external_player.h
						 	 
libexternal_player_la_LDFLAGS = $(GTKPOD_PLUGIN_LDFLAGS)

# Plugin dependencies
libexternal_player_la_LIBADD = \
    $(GTKPOD_LIBS) \
    $(LIBANJUTA_LIBS)

EXTRA_DIST = \
	$(plugin_file).in \
	$(external_player_plugin_DATA) \
	$(external_player_ui_DATA) \
	$(external_player_glade_DATA) \
	$(external_player_pixmaps_DATA)
