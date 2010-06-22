if HAVE_PLUGIN_COVERWEB

plugin_name = coverweb
plugin_lib = lib$(plugin_name).so
plugin_file = $(plugin_name).plugin

# Plugin description file
plugin_in_files = $(plugin_file)

# Plugin UI file
coverweb_uidir = $(gtkpod_ui_dir)
coverweb_ui_DATA = $(plugin_name).ui

# Plugin Glade file
coverweb_gladedir = $(gtkpod_glade_dir)
coverweb_glade_DATA =  $(plugin_name).glade

# Plugin Icon file
coverweb_pixmapsdir = $(gtkpod_image_dir)
coverweb_pixmaps_DATA =

# Where to install the plugin
plugindir = $(gtkpod_plugin_dir)
coverweb_plugindir = $(gtkpod_plugin_dir)
coverweb_plugin_DATA = $(plugin_file)

SUBDIRS = icons

# Even though this is located in the plugin directory, it will be 
# included from plugin sub-directories.
include ../../data/directories.mk

# The plugin
plugin_LTLIBRARIES = libcoverweb.la

AM_CPPFLAGS = \
    -DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
    -DGTKPOD_DATA_DIR=\"$(gtkpod_data_dir)\" \
    -DGTKPOD_PLUGIN_DIR=\"$(gtkpod_plugin_dir)\" \
    -DGTKPOD_IMAGE_DIR=\"$(gtkpod_image_dir)\" \
    -DGTKPOD_GLADE_DIR=\"$(gtkpod_glade_dir)\" \
    -DGTKPOD_UI_DIR=\"$(gtkpod_ui_dir)\" \
    -DPACKAGE_DATA_DIR=\"$(datadir)\" \
    -DPACKAGE_SRC_DIR=\"$(srcdir)\" \
    $(GTKPOD_CFLAGS) \
	  $(WEBKIT_CFLAGS)
	
# Plugin sources
libcoverweb_la_SOURCES = plugin.c plugin.h \
						 coverweb.c coverweb.h \
						 coverweb_preferences.c coverweb_preferences.h
	
libcoverweb_la_LDFLAGS = $(GTKPOD_PLUGIN_LDFLAGS)

# Plugin dependencies
libcoverweb_la_LIBADD = \
    $(LIBGTKPOD_LIBS) \
    $(LIBANJUTA_LIBS) \
    $(WEBKIT_LIBS)
    
all-local: create-plugin-links create-ui-link create-glade-link copy-icons-dir

# Creating symbolic links in plugin root directory
create-plugin-links:
	echo "Creating plugin links"
	if test ! -e ../$(plugin_lib); then \
		ln -s `pwd`/.libs/$(plugin_lib) ../$(plugin_lib); \
	fi; \
	if test ! -e ../$(plugin_file); then \
		ln -s `pwd`/$(plugin_file) ../$(plugin_file); \
	fi;

# Creating symbolic link to ui file in installed ui directory
create-ui-link: create-ui-dir
	if test -e $(gtkpod_ui_dir)/$(plugin_name).ui; then \
		# File already exists. Replacing ..." \
		sudo rm -f $(gtkpod_ui_dir)/$(plugin_name).ui; \
	fi; \
	# Creating link for $(plugin_name).ui" \
	sudo ln -s `pwd`/$(plugin_name).ui $(gtkpod_ui_dir)/$(plugin_name).ui;

create-glade-link: create-glade-dir
	if test -e `pwd`/$(plugin_name).glade; then \
		if test -e $(gtkpod_glade_dir)/$(plugin_name).glade; then \
			# File already exists. Replacing ..." \
			sudo rm -f $(gtkpod_glade_dir)/$(plugin_name).glade; \
		fi; \
		# Creating link for $(plugin_name).glade" \
		sudo ln -s `pwd`/$(plugin_name).glade $(gtkpod_glade_dir)/$(plugin_name).glade; \
	fi; \
	if test -e `pwd`/$(plugin_name).xml; then \
		if test -e $(gtkpod_glade_dir)/$(plugin_name).xml; then \
			# File already exists. Replacing ..." \
			sudo rm -f $(gtkpod_glade_dir)/$(plugin_name).xml; \
		fi; \
		# Creating link for $(plugin_name).xml" \
		sudo ln -s `pwd`/$(plugin_name).xml $(gtkpod_glade_dir)/$(plugin_name).xml; \
	fi;
	
# Copy icons
copy-icons-dir: create-icons-dir
	if test -e `pwd`/icons; then \
		sudo cp -rf `pwd`/icons/* $(gtkpod_image_dir)/; \
		sudo chmod -R 755 $(gtkpod_image_dir)/*; \
		sudo find $(gtkpod_image_dir) -name 'Makefile*' -delete; \
	fi;

# Clean up the links and files created purely for dev testing
clean-local: clean-plugin-files clean-ui-dir clean-glade-dir clean-image-dir

clean-plugin-files:
	rm -f ../$(plugin_lib) ../$(plugin_file)

clean-ui-dir:
	if test -h $(gtkpod_ui_dir)/$(plugin_name).ui; then \
		# Symbolic link exists. Removing ..." \
		sudo rm -f $(gtkpod_ui_dir)/$(plugin_name).ui; \
	fi;

clean-glade-dir:
	if test -h $(gtkpod_glade_dir)/$(plugin_name).glade; then \
		# Symbolic link exists. Removing ..." \
		sudo rm -f $(gtkpod_glade_dir)/$(plugin_name).glade; \
	fi;
	
clean-image-dir:
	if test -d $(gtkpod_image_dir); then \
	  sudo find $(gtkpod_image_dir) -name '$(plugin_name)*' -delete; \
	fi;

endif

EXTRA_DIST = \
	$(plugin_in_files) \
	$(coverweb_plugin_DATA) \
	$(coverweb_ui_DATA) \
	$(coverweb_glade_DATA) \
	$(coverweb_pixmaps_DATA)
