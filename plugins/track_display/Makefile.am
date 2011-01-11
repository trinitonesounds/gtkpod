plugin_name = track_display
plugin_lib = lib$(plugin_name).so
plugin_file = $(plugin_name).plugin

# Plugin UI file
track_display_uidir = $(gtkpod_ui_dir)
track_display_ui_DATA =  $(plugin_name).ui

# Plugin Glade file
track_display_gladedir = $(gtkpod_glade_dir)
track_display_glade_DATA =  $(plugin_name).glade

# Plugin Icon file
track_display_pixmapsdir = $(gtkpod_image_dir)
track_display_pixmaps_DATA =

track_display_plugindir = $(gtkpod_plugin_dir)
track_display_plugin_DATA =

SUBDIRS = icons

include ../plugins.mk

# The plugin
plugin_LTLIBRARIES = libtrack_display.la

# Plugin sources
libtrack_display_la_SOURCES = 	plugin.c plugin.h \
								track_display_actions.c track_display_actions.h \
								track_display_context_menu.c track_display_context_menu.h \
								track_display_preferences.c track_display_preferences.h \
								display_tracks.c display_tracks.h \
								rb_cell_renderer_rating.c rb_cell_renderer_rating.h \
								rb_rating_helper.c rb_rating_helper.h

libtrack_display_la_LDFLAGS = $(GTKPOD_PLUGIN_LDFLAGS)

# Plugin dependencies
libtrack_display_la_LIBADD = \
    $(LIBGTKPOD_LIBS) \
    $(LIBANJUTA_LIBS)
    
EXTRA_DIST = \
	$(plugin_in_files) \
	$(track_display_plugin_DATA) \
	$(track_display_ui_DATA) \
	$(track_display_glade_DATA) \
	$(track_display_pixmaps_DATA)
	
