plugin_name = cover_display
plugin_file = $(plugin_name).plugin

# Plugin UI file
cover_display_uidir = $(gtkpod_ui_dir)
cover_display_ui_DATA = $(plugin_name).ui

# Plugin Glade file
cover_display_gladedir = $(gtkpod_glade_dir)
cover_display_glade_DATA =  $(plugin_name).glade

# Plugin Icon file
cover_display_pixmapsdir = $(gtkpod_image_dir)
cover_display_pixmaps_DATA =

cover_display_plugindir = $(gtkpod_plugin_dir)
cover_display_plugin_DATA =

SUBDIRS = icons

include ../plugins.mk

# The plugin
plugin_lib = lib$(plugin_name).so
plugin_LTLIBRARIES = libcover_display.la

# Plugin sources
libcover_display_la_SOURCES = plugin.c plugin.h \
							  display_coverart.c display_coverart.h \
							  cover_display_context_menu.c cover_display_context_menu.h \
							  fetchcover.c fetchcover.h \
							  cover_display_preferences.c cover_display_preferences.h

libcover_display_la_LDFLAGS = $(GTKPOD_PLUGIN_LDFLAGS)

# Plugin dependencies
libcover_display_la_LIBADD = \
    $(LIBGTKPOD_LIBS) \
    $(LIBANJUTA_LIBS)

EXTRA_DIST = \
	$(plugin_file).in \
	$(cover_display_plugin_DATA) \
	$(cover_display_ui_DATA) \
	$(cover_display_glade_DATA) \
	$(cover_display_pixmaps_DATA)
