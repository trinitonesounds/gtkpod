# Sample Makefile for a anjuta plugin.

plugin_name = helloworld
plugin_lib = lib$(plugin_name).so
plugin_file = $(plugin_name).plugin

# Plugin UI file
helloworld_uidir = $(gtkpod_ui_dir)
helloworld_ui_DATA =  $(plugin_name).ui

# Plugin Glade file
helloworld_gladedir = $(gtkpod_glade_dir)
helloworld_glade_DATA =  $(plugin_name).glade

# Plugin Icon file
helloworld_pixmapsdir = $(gtkpod_image_dir)
helloworld_pixmaps_DATA = $(plugin_name).png

# Plugin description file
plugin_in_files = $(plugin_file)

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

all-local:
	echo "Creating symbolic links in plugin root directory"
	if test ! -e ../$(plugin_lib); then \
	  ln -s `pwd`/.libs/$(plugin_lib) ../$(plugin_lib); \
	fi; \
	if test ! -e ../$(plugin_file); then \
	  ln -s `pwd`/$(plugin_file) ../$(plugin_file); \
	fi;

clean-local:
	rm -f ../$(plugin_lib) ../$(plugin_file)

