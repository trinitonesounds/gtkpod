plugin_name = info_display
plugin_file = $(plugin_name).plugin

# Plugin UI file
info_display_uidir = $(gtkpod_ui_dir)
info_display_ui_DATA = $(plugin_name).ui

# Plugin Glade file
info_display_gladedir = $(gtkpod_glade_dir)
info_display_glade_DATA =

# Plugin Icon file
info_display_pixmapsdir = $(gtkpod_image_dir)
info_display_pixmaps_DATA =

info_display_plugindir = $(gtkpod_plugin_dir)
info_display_plugin_DATA =

SUBDIRS =

include ../plugins.mk
info_display.plugin: build-plugin-file

# The plugin
plugin_lib = lib$(plugin_name).so
plugin_LTLIBRARIES = libinfo_display.la

# Plugin sources
libinfo_display_la_SOURCES = plugin.c plugin.h \
							 info.c info.h \
							 infoview.c infoview.h
							 

libinfo_display_la_LDFLAGS = $(GTKPOD_PLUGIN_LDFLAGS)

# Plugin dependencies
libinfo_display_la_LIBADD = \
    $(LIBGTKPOD_LIBS) \
    $(LIBANJUTA_LIBS)

EXTRA_DIST = \
	$(plugin_file).in \
	$(info_display_plugin_DATA) \
	$(info_display_ui_DATA) \
	$(info_display_glade_DATA) \
	$(info_display_pixmaps_DATA)
