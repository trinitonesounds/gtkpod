plugin_name = mserv
plugin_lib = lib$(plugin_name).so
plugin_file = $(plugin_name).plugin

# Plugin description file
plugin_in_files = $(plugin_file)

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
plugindir = $(gtkpod_plugin_dir)
mserv_plugindir = $(gtkpod_plugin_dir)
mserv_plugin_DATA = $(plugin_file)

SUBDIRS = icons

# The plugin
plugin_LTLIBRARIES = libmserv.la

AM_CPPFLAGS = \
    -DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
    -DGTKPOD_DATA_DIR=\"$(gtkpod_data_dir)\" \
    -DGTKPOD_PLUGIN_DIR=\"$(gtkpod_plugin_dir)\" \
    -DGTKPOD_IMAGE_DIR=\"$(gtkpod_image_dir)\" \
    -DGTKPOD_GLADE_DIR=\"$(gtkpod_glade_dir)\" \
    -DGTKPOD_UI_DIR=\"$(gtkpod_ui_dir)\" \
    -DPACKAGE_DATA_DIR=\"$(datadir)\" \
    -DPACKAGE_SRC_DIR=\"$(srcdir)\" \
    $(GTKPOD_CFLAGS)
	
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
    
all-local: create-plugin-links create-ui-link create-glade-link

# Creating symbolic links in plugin root directory
create-plugin-links:
	echo "Creating plugin links"
	if [ ! -e ../$(plugin_lib) ]; then \
		ln -s `pwd`/.libs/$(plugin_lib) ../$(plugin_lib); \
	fi; \
	if [ ! -e ../$(plugin_file) ]; then \
		ln -s `pwd`/$(plugin_file) ../$(plugin_file); \
	fi;

# Creating symbolic link to ui file in installed ui directory
create-ui-link:
	if [ ! -e ../../data/ui/$(plugin_name).ui ]; then \
		ln -s `pwd`/$(plugin_name).ui ../../data/ui/$(plugin_name).ui; \
	fi;

create-glade-link:
	if  [ -e `pwd`/$(plugin_name).xml ]; then \
		if  [ ! -e ../../data/glade/$(plugin_name).xml ]; then \
			ln -s `pwd`/$(plugin_name).xml ../../data/glade/$(plugin_name).xml; \
		fi; \
	fi;

# Clean up the links and files created purely for dev  [ing
clean-local: clean-plugin-files clean-ui-dir clean-glade-dir

clean-plugin-files:
	if [ -h ../$(plugin_file) ]; then \
		rm -f ../$(plugin_lib) ../$(plugin_file); \
	fi;

clean-ui-dir:
	if [ -h ../../data/ui/$(plugin_name).ui ]; then \
		rm -f ../../data/ui/$(plugin_name).ui; \
	fi;

clean-glade-dir:
	if  [ -h $(gtkpod_glade_dir)/$(plugin_name).xml ]; then \
		rm -f ../../data/glade/$(plugin_name).xml; \
	fi;

EXTRA_DIST = \
	$(plugin_in_files) \
	$(mserv_plugin_DATA) \
	$(mserv_ui_DATA) \
	$(mserv_glade_DATA) \
	$(mserv_pixmaps_DATA)
