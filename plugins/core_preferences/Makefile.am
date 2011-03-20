plugin_name = core_prefs
plugin_file = $(plugin_name).plugin

# Plugin UI file
core_core_prefsdir = $(gtkpod_ui_dir)
core_core_prefs_DATA = $(plugin_name).ui

# Plugin Glade file
core_prefs_gladedir = $(gtkpod_glade_dir)
core_prefs_glade_DATA =  $(plugin_name).xml

# Plugin Icon file
core_prefs_pixmapsdir = $(gtkpod_image_dir)
core_prefs_pixmaps_DATA =

core_prefs_plugindir = $(gtkpod_plugin_dir)
core_prefs_plugin_DATA =

SUBDIRS = icons

include ../plugins.mk
core_prefs.plugin: build-plugin-file

# The plugin
plugin_lib = lib$(plugin_name).so
plugin_LTLIBRARIES = libcore_prefs.la

# Plugin sources
libcore_prefs_la_SOURCES = plugin.c plugin.h \
						 core_prefs.c core_prefs.h

libcore_prefs_la_LDFLAGS = $(GTKPOD_PLUGIN_LDFLAGS)

# Plugin dependencies
libcore_prefs_la_LIBADD = \
    $(GTKPOD_LIBS) \
    $(LIBANJUTA_LIBS)

EXTRA_DIST = \
	$(plugin_file).in \
	$(core_prefs_plugin_DATA) \
	$(core_core_prefs_DATA) \
	$(core_prefs_glade_DATA) \
	$(core_prefs_pixmaps_DATA)

