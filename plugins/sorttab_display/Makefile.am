plugin_name = sorttab_display
plugin_lib = lib$(plugin_name).so
plugin_file = $(plugin_name).plugin

# Plugin UI file
sorttab_display_uidir = $(gtkpod_ui_dir)
sorttab_display_ui_DATA =  $(plugin_name).ui

# Plugin Glade file
sorttab_display_gladedir = $(gtkpod_glade_dir)
sorttab_display_glade_DATA =  $(plugin_name).glade

# Plugin Icon file
sorttab_display_pixmapsdir = $(gtkpod_image_dir)
sorttab_display_pixmaps_DATA =

# Plugin description file
plugin_in_files = $(plugin_file)

## I'm using %option outfile="lex.yy.c" in the parser files,
## so this should be portable
LEX_OUTPUT_ROOT = lex.yy

sorttab_display_plugindir = $(gtkpod_plugin_dir)
sorttab_display_plugin_DATA = $(plugin_file)

SUBDIRS =

# Include paths
AM_CPPFLAGS = \
    -DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
    -DGTKPOD_DATA_DIR=\"$(gtkpod_data_dir)\" \
    -DGTKPDO_PLUGIN_DIR=\"$(gtkpod_plugin_dir)\" \
    -DGTKPDO_IMAGE_DIR=\"$(gtkpod_image_dir)\" \
    -DGTKPOD_GLADE_DIR=\"$(gtkpod_glade_dir)\" \
    -DGTKPOD_UI_DIR=\"$(gtkpod_ui_dir)\" \
    -DPACKAGE_DATA_DIR=\"$(datadir)\" \
    -DPACKAGE_SRC_DIR=\"$(srcdir)\" \
    $(GTKPOD_CFLAGS)

# Where to install the plugin
plugindir = $(gtkpod_plugin_dir)

# The plugin
plugin_LTLIBRARIES = libsorttab_display.la

# Plugin sources
libsorttab_display_la_SOURCES = plugin.c plugin.h \
								sorttab_conversion.c sorttab_conversion.h \
								display_sorttabs.c display_sorttabs.h \
								date_parser2.l date_parser.l date_parser.h

libsorttab_display_la_LDFLAGS = $(GTKPOD_PLUGIN_LDFLAGS)

# Plugin dependencies
libsorttab_display_la_LIBADD = \
    $(LIBGTKPOD_LIBS) \
    $(LIBANJUTA_LIBS)

EXTRA_DIST = \
	$(plugin_in_files) \
	$(sorttab_display_plugin_DATA) \
	$(sorttab_display_ui_DATA) \
	$(sorttab_display_glade_DATA) \
	$(sorttab_display_pixmaps_DATA)
	
CLEANFILES = date_parser.c date_parser2.c


include ../../data/directories.mk

all-local: create-plugin-links create-ui-link create-glade-link

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
	if test -e $(gtkpod_ui_dir)/$(sorttab_display_ui_DATA); then \
	  # File already exists. Replacing ..." \
	  sudo rm -f $(gtkpod_ui_dir)/$(sorttab_display_ui_DATA); \
	fi; \
	# Creating link for $(sorttab_display_ui_DATA)" \
	sudo ln -s `pwd`/$(sorttab_display_ui_DATA) $(gtkpod_ui_dir)/$(sorttab_display_ui_DATA);

create-glade-link: create-glade-dir
	if test -e $(gtkpod_glade_dir)/$(sorttab_display_glade_DATA); then \
	  # File already exists. Replacing ..." \
	  sudo rm -f $(gtkpod_glade_dir)/$(sorttab_display_glade_DATA); \
	fi; \
	# Creating link for $(sorttab_display_glade_DATA)" \
	sudo ln -s `pwd`/$(sorttab_display_glade_DATA) $(gtkpod_glade_dir)/$(sorttab_display_glade_DATA);

# Clean up the links and files created purely for dev testing
clean-local: clean-plugin-files clean-ui-dir clean-glade-dir

clean-plugin-files:
	rm -f ../$(plugin_lib) ../$(plugin_file)

clean-ui-dir:
	if test -h $(gtkpod_ui_dir)/$(sorttab_display_ui_DATA); then \
	  # Symbolic link exists. Removing ..." \
	  sudo rm -f $(gtkpod_ui_dir)/$(sorttab_display_ui_DATA); \
	fi;

clean-glade-dir:
	if test -h $(gtkpod_glade_dir)/$(sorttab_display_glade_DATA); then \
	  # Symbolic link exists. Removing ..." \
	  sudo rm -f $(gtkpod_glade_dir)/$(sorttab_display_glade_DATA); \
	fi;
	