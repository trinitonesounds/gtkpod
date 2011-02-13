plugin_name = details_editor
plugin_file = $(plugin_name).plugin

# Plugin UI file
details_editor_uidir = $(gtkpod_ui_dir)
details_editor_ui_DATA = $(plugin_name).ui

# Plugin Glade file
details_editor_gladedir = $(gtkpod_glade_dir)
details_editor_glade_DATA =  $(plugin_name).xml

# Plugin Icon file
details_editor_pixmapsdir = $(gtkpod_image_dir)
details_editor_pixmaps_DATA =

details_editor_plugindir = $(gtkpod_plugin_dir)
details_editor_plugin_DATA =

SUBDIRS =

include ../plugins.mk
details_editor.plugin: build-plugin-file

# The plugin
plugin_lib = lib$(plugin_name).so
plugin_LTLIBRARIES = libdetails_editor.la

# Plugin sources
libdetails_editor_la_SOURCES = 	plugin.c plugin.h \
								details.c details.h \
								details_editor_actions.c details_editor_actions.h \
								fetchcover.c fetchcover.h
							   
libdetails_editor_la_LDFLAGS = $(GTKPOD_PLUGIN_LDFLAGS)

# Plugin dependencies
libdetails_editor_la_LIBADD = \
    $(LIBGTKPOD_LIBS) \
    $(LIBANJUTA_LIBS)

EXTRA_DIST = \
	$(plugin_file).in \
	$(details_editor_plugin_DATA) \
	$(details_editor_ui_DATA) \
	$(details_editor_glade_DATA) \
	$(details_editor_pixmaps_DATA)
