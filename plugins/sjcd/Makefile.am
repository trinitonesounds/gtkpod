plugin_name = sjcd
plugin_file = $(plugin_name).plugin

# Plugin UI file
sjcd_uidir = $(gtkpod_ui_dir)
sjcd_ui_DATA = $(plugin_name).ui

# Plugin Glade file
sjcd_gladedir = $(gtkpod_glade_dir)
sjcd_glade_DATA =  $(plugin_name).xml

# Plugin Icon file
sjcd_pixmapsdir = $(gtkpod_image_dir)
sjcd_pixmaps_DATA =

sjcd_plugindir = $(gtkpod_plugin_dir)
sjcd_plugin_DATA =

SUBDIRS = data libjuicer

include ../plugins.mk
sjcd.plugin: build-plugin-file

# The plugin
plugin_lib = lib$(plugin_name).so
plugin_LTLIBRARIES = libsjcd.la

# Plugin sources
libsjcd_la_SOURCES = plugin.c plugin.h

libsjcd_la_LDFLAGS = $(GTKPOD_PLUGIN_LDFLAGS)

# Plugin dependencies
libsjcd_la_LIBADD = \
    $(GTKPOD_LIBS) \
    $(LIBANJUTA_LIBS)

EXTRA_DIST = \
	$(plugin_file).in \
	$(sjcd_plugin_DATA) \
	$(sjcd_ui_DATA) \
	$(sjcd_glade_DATA) \
	$(sjcd_pixmaps_DATA)
