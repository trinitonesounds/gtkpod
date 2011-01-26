plugin_name = mserv
plugin_file = $(plugin_name).plugin

# Plugin UI file
mserv_uidir = $(gtkpod_ui_dir)
mserv_ui_DATA = $(plugin_name).ui

# Plugin Glade file
mserv_gladedir = $(gtkpod_glade_dir)
mserv_glade_DATA =  $(plugin_name).xml

# Plugin Icon file
mserv_pixmapsdir = $(gtkpod_image_dir)
mserv_pixmaps_DATA =

# Where to install the plugin
mserv_plugindir = $(gtkpod_plugin_dir)
mserv_plugin_DATA =

SUBDIRS = icons

include ../plugins.mk
mserv.plugin: build-plugin-file

# The plugin
plugin_lib = lib$(plugin_name).so
plugin_LTLIBRARIES = libmserv.la

# Plugin sources
libmserv_la_SOURCES = plugin.c plugin.h \
						 mserv.c mserv.h \
						 mserv_actions.c mserv_actions.h \
						 mserv_preferences.c mserv_preferences.h
	
libmserv_la_LDFLAGS = $(GTKPOD_PLUGIN_LDFLAGS)

# Plugin dependencies
libmserv_la_LIBADD = \
    $(LIBGTKPOD_LIBS) \
    $(LIBANJUTA_LIBS)

EXTRA_DIST = \
	$(plugin_file).in \
	$(mserv_plugin_DATA) \
	$(mserv_ui_DATA) \
	$(mserv_glade_DATA) \
	$(mserv_pixmaps_DATA)
