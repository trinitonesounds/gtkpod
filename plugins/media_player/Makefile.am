plugin_name = media_player
plugin_file = $(plugin_name).plugin

# Plugin UI file
media_player_uidir = $(gtkpod_ui_dir)
media_player_ui_DATA = $(plugin_name).ui

# Plugin Glade file
media_player_gladedir = $(gtkpod_glade_dir)
media_player_glade_DATA =  $(plugin_name).glade

# Plugin Icon file
media_player_pixmapsdir = $(gtkpod_image_dir)
media_player_pixmaps_DATA =

# Where to install the plugin
media_player_plugindir = $(gtkpod_plugin_dir)
media_player_plugin_DATA =

SUBDIRS = icons

if HAVE_PLUGIN_MEDIA_PLAYER

include ../plugins.mk

# The plugin
plugin_lib = lib$(plugin_name).so
plugin_LTLIBRARIES = libmedia_player.la

# Plugin sources
libmedia_player_la_SOURCES = plugin.c plugin.h \
						 	 media_player.c media_player.h
libmedia_player_la_CFLAGS = $(GSTREAMER_CFLAGS)
libmedia_player_la_LDFLAGS = $(GTKPOD_PLUGIN_LDFLAGS)

# Plugin dependencies
libmedia_player_la_LIBADD = \
    $(LIBGTKPOD_LIBS) \
    $(LIBANJUTA_LIBS) \
    $(GSTREAMER_LIBS)
endif

EXTRA_DIST = \
	$(plugin_file).in \
	$(media_player_plugin_DATA) \
	$(media_player_ui_DATA) \
	$(media_player_glade_DATA) \
	$(media_player_pixmaps_DATA)
