plugin_name = coverweb
plugin_file = $(plugin_name).plugin

# Plugin UI file
coverweb_uidir = $(gtkpod_ui_dir)
coverweb_ui_DATA = $(plugin_name).ui

# Plugin Glade file
coverweb_gladedir = $(gtkpod_glade_dir)
coverweb_glade_DATA =  $(plugin_name).xml

# Plugin Icon file
coverweb_pixmapsdir = $(gtkpod_image_dir)
coverweb_pixmaps_DATA =

# Where to install the plugin
coverweb_plugindir = $(gtkpod_plugin_dir)
coverweb_plugin_DATA =

SUBDIRS = icons

if HAVE_PLUGIN_COVERWEB

include ../plugins.mk
coverweb.plugin: build-plugin-file

# The plugin
plugin_lib = lib$(plugin_name).so
plugin_LTLIBRARIES = libcoverweb.la

# Plugin sources
libcoverweb_la_SOURCES = plugin.c plugin.h \
						 coverweb.c coverweb.h \
						 coverweb_preferences.c coverweb_preferences.h
libcoverweb_la_CFLAGS = $(WEBKIT_CFLAGS)
libcoverweb_la_LDFLAGS = $(GTKPOD_PLUGIN_LDFLAGS)

# Plugin dependencies
libcoverweb_la_LIBADD = \
    $(GTKPOD_LIBS) \
    $(LIBANJUTA_LIBS) \
    $(WEBKIT_LIBS)
endif

EXTRA_DIST = \
	$(plugin_file).in \
	$(coverweb_plugin_DATA) \
	$(coverweb_ui_DATA) \
	$(coverweb_glade_DATA) \
	$(coverweb_pixmaps_DATA)
