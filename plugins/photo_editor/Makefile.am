plugin_name = photo_editor
plugin_file = $(plugin_name).plugin

# Plugin UI file
photo_editor_uidir = $(gtkpod_ui_dir)
photo_editor_ui_DATA = $(plugin_name).ui

# Plugin Glade file
photo_editor_gladedir = $(gtkpod_glade_dir)
photo_editor_glade_DATA =  $(plugin_name).glade

# Plugin Icon file
photo_editor_pixmapsdir = $(gtkpod_image_dir)
photo_editor_pixmaps_DATA =

photo_editor_plugindir = $(gtkpod_plugin_dir)
photo_editor_plugin_DATA =

SUBDIRS = icons

include ../plugins.mk
photo_editor.plugin: build-plugin-file

# The plugin
plugin_lib = lib$(plugin_name).so
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
	$(plugin_file).in \
	$(photo_editor_plugin_DATA) \
	$(photo_editor_ui_DATA) \
	$(photo_editor_glade_DATA) \
	$(photo_editor_pixmaps_DATA)
