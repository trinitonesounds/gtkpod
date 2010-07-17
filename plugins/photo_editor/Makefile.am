plugin_name = photo_editor
plugin_lib = lib$(plugin_name).so
plugin_file = $(plugin_name).plugin

# Plugin UI file
details_editor_uidir = $(gtkpod_ui_dir)
details_editor_ui_DATA = $(plugin_name).ui

# Plugin Glade file
details_editor_gladedir = $(gtkpod_glade_dir)
details_editor_glade_DATA =  $(plugin_name).glade

# Plugin Icon file
details_editor_pixmapsdir = $(gtkpod_image_dir)
details_editor_pixmaps_DATA =

details_editor_plugindir = $(gtkpod_plugin_dir)
details_editor_plugin_DATA = $(plugin_file)

SUBDIRS = icons

include ../plugins.mk

# The plugin
plugin_LTLIBRARIES = libphoto_editor.la

# Plugin sources
libphoto_editor_la_SOURCES = 	plugin.c plugin.h \
								display_photo.c display_photo.h \
								photo_editor_actions.c photo_editor_actions.h \
								photo_editor_context_menu.c photo_editor_context_menu.h
							   
libphoto_editor_la_LDFLAGS = $(GTKPOD_PLUGIN_LDFLAGS)

# Plugin dependencies
libphoto_editor_la_LIBADD = \
    $(LIBGTKPOD_LIBS) \
    $(LIBANJUTA_LIBS)

EXTRA_DIST = \
	$(plugin_in_files) \
	$(photo_editor_plugin_DATA) \
	$(photo_editor_ui_DATA) \
	$(photo_editor_glade_DATA) \
	$(photo_editor_pixmaps_DATA)
