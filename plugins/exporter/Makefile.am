plugin_name = exporter
plugin_lib = lib$(plugin_name).so
plugin_file = $(plugin_name).plugin

# Plugin UI file
exporter_uidir = $(gtkpod_ui_dir)
exporter_ui_DATA = $(plugin_name).ui

# Plugin Glade file
exporter_gladedir = $(gtkpod_glade_dir)
exporter_glade_DATA =  $(plugin_name).xml

# Plugin Icon file
exporter_pixmapsdir = $(gtkpod_image_dir)
exporter_pixmaps_DATA =

exporter_plugindir = $(gtkpod_plugin_dir)
exporter_plugin_DATA = $(plugin_file)

SUBDIRS =

include ../plugins.mk

# The plugin
plugin_LTLIBRARIES = libexporter.la

# Plugin sources
libexporter_la_SOURCES = plugin.c plugin.h \
						 file_export.c file_export.h \
						 exporter_actions.c exporter_actions.h

libexporter_la_LDFLAGS = $(GTKPOD_PLUGIN_LDFLAGS)

# Plugin dependencies
libexporter_la_LIBADD = \
    $(LIBGTKPOD_LIBS) \
    $(LIBANJUTA_LIBS)

EXTRA_DIST = \
	$(plugin_in_files) \
	$(exporter_plugin_DATA) \
	$(exporter_ui_DATA) \
	$(exporter_glade_DATA) \
	$(exporter_pixmaps_DATA)
