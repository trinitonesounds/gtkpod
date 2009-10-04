# Sample Makefile for a anjuta plugin.

# Plugin UI file
helloworld_uidir = $(gtkpod_ui_dir)
helloworld_ui_DATA =  helloworld.ui


# Plugin Glade file
helloworld_gladedir = $(gtkpod_glade_dir)
helloworld_glade_DATA =  helloworld.glade

# Plugin Icon file
helloworld_pixmapsdir = $(gtkpod_image_dir)
helloworld_pixmaps_DATA = helloworld.png

# Plugin description file
plugin_in_files = helloworld.plugin

helloworld_plugindir = $(gtkpod_plugin_dir)
helloworld_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

# Include paths
AM_CPPFLAGS = \
    -DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
    -DANJUTA_DATA_DIR=\"$(gtkpod_data_dir)\" \
    -DANJUTA_PLUGIN_DIR=\"$(gtkpod_plugin_dir)\" \
    -DANJUTA_IMAGE_DIR=\"$(gtkpod_image_dir)\" \
    -DANJUTA_GLADE_DIR=\"$(gtkpod_glade_dir)\" \
    -DANJUTA_UI_DIR=\"$(gtkpod_ui_dir)\" \
    -DPACKAGE_DATA_DIR=\"$(datadir)\" \
    -DPACKAGE_SRC_DIR=\"$(srcdir)\" \
    $(LIBANJUTA_CFLAGS)

# Where to install the plugin
plugindir = $(gtkpod_plugin_dir)

# The plugin
plugin_LTLIBRARIES = libhelloworld.la

# Plugin sources
libhelloworld_la_SOURCES = plugin.c plugin.h

libhelloworld_la_LDFLAGS = $(GTKPOD_PLUGIN_LDFLAGS)

# Plugin dependencies
libhelloworld_la_LIBADD = \
    $(GCONF_LIBS) \
    $(GLADE_LIBS) \
    $(VTE_LIBS) \
    $(LIBANJUTA_LIBS)

EXTRA_DIST = \
	$(plugin_in_files) \
	$(helloworld_plugin_DATA) \
	$(helloworld_ui_DATA) \
	$(helloworld_glade_DATA) \
	$(helloworld_pixmaps_DATA)
