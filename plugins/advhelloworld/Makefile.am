# Sample Makefile for a anjuta plugin.

plugin_name = advhelloworld
plugin_lib = lib$(plugin_name).so
plugin_file = $(plugin_name).plugin

# Plugin UI file
advhelloworld_uidir = $(gtkpod_ui_dir)
advhelloworld_ui_DATA =  $(plugin_name).ui

# Plugin Glade file
advhelloworld_gladedir = $(gtkpod_glade_dir)
advhelloworld_glade_DATA =  $(plugin_name).glade

# Plugin Icon file
advhelloworld_pixmapsdir = $(gtkpod_image_dir)
advhelloworld_pixmaps_DATA = $(plugin_name).png

# Plugin description file
plugin_in_files = $(plugin_file)

advhelloworld_plugindir = $(gtkpod_plugin_dir)
advhelloworld_plugin_DATA = $(plugin_file)

# Include paths
AM_CPPFLAGS = \
    -DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
    -DGTKPOD_DATA_DIR=\"$(gtkpod_data_dir)\" \
    -DGTKPDO_PLUGIN_DIR=\"$(gtkpod_plugin_dir)\" \
    -DGTKPDO_IMAGE_DIR=\"$(gtkpod_image_dir)\" \
    -DGTKPDO_GLADE_DIR=\"$(gtkpod_glade_dir)\" \
    -DGTKPOD_UI_DIR=\"$(gtkpod_ui_dir)\" \
    -DPACKAGE_DATA_DIR=\"$(datadir)\" \
    -DPACKAGE_SRC_DIR=\"$(srcdir)\" \
    $(LIBANJUTA_CFLAGS)

# Where to install the plugin
plugindir = $(gtkpod_plugin_dir)

# The plugin
plugin_LTLIBRARIES = libadvhelloworld.la

# Plugin sources
libadvhelloworld_la_SOURCES = plugin.c plugin.h

libadvhelloworld_la_LDFLAGS = $(GTKPOD_PLUGIN_LDFLAGS)

# Plugin dependencies
libadvhelloworld_la_LIBADD = \
    $(GCONF_LIBS) \
    $(GLADE_LIBS) \
    $(VTE_LIBS) \
    $(LIBANJUTA_LIBS)

EXTRA_DIST = \
	$(plugin_in_files) \
	$(advhelloworld_plugin_DATA) \
	$(advhelloworld_ui_DATA) \
	$(advhelloworld_glade_DATA) \
	$(advhelloworld_pixmaps_DATA)

all-local: create-plugin-links create-ui-link

create-plugin-links:
	echo "Creating symbolic links in plugin root directory"
	if test ! -e ../$(plugin_lib); then \
	  ln -s `pwd`/.libs/$(plugin_lib) ../$(plugin_lib); \
	fi; \
	if test ! -e ../$(plugin_file); then \
	  ln -s `pwd`/$(plugin_file) ../$(plugin_file); \
	fi;
	
create-ui-link: create-ui-dir
	if test -e $(gtkpod_ui_dir)/$(advhelloworld_ui_DATA); then \
	  sudo rm -f $(gtkpod_ui_dir)/$(advhelloworld_ui_DATA); \
	fi; \
	sudo ln -s `pwd`/$(advhelloworld_ui_DATA) $(gtkpod_ui_dir)/$(advhelloworld_ui_DATA);

create-ui-dir: create-data-dir
	if test ! -d $(gtkpod_ui_dir); then \
	  sudo mkdir $(gtkpod_ui_dir); \
	fi;

create-data-dir: create-base-data-dir
	if test ! -d $(gtkpod_data_dir); then \
	  sudo mkdir $(gtkpod_data_dir); \
	fi;

create-base-data-dir:
	if test ! -d $(datadir)/$(PACKAGE); then \
	  sudo mkdir $(datadir)/$(PACKAGE); \
	fi;


clean-local:
	rm -f ../$(plugin_lib) ../$(plugin_file)

