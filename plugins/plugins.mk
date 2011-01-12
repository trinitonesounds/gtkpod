# Include paths
AM_CPPFLAGS = \
    -DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
    -DGTKPOD_DATA_DIR=\"$(gtkpod_data_dir)\" \
    -DGTKPOD_PLUGIN_DIR=\"$(gtkpod_plugin_dir)\" \
    -DGTKPOD_IMAGE_DIR=\"$(gtkpod_image_dir)\" \
    -DGTKPOD_GLADE_DIR=\"$(gtkpod_glade_dir)\" \
    -DGTKPOD_SCRIPT_DIR=\"$(gtkpod_script_dir)\" \
    -DGTKPOD_UI_DIR=\"$(gtkpod_ui_dir)\" \
    -DPACKAGE_DATA_DIR=\"$(datadir)\" \
    -DPACKAGE_SRC_DIR=\"$(srcdir)\" \
    -I$(top_srcdir)\
    $(GTKPOD_CFLAGS)

# Where to install the plugin
plugindir = $(gtkpod_plugin_dir)

all-local: create-plugin-links create-ui-link create-glade-link

# Creating symbolic links in plugin root directory
create-plugin-links:
	echo "Creating plugin links"
	if [ ! -e ../$(plugin_lib) ]; then \
		ln -s `pwd`/.libs/$(plugin_lib) ../$(plugin_lib); \
	fi; \
	if [ ! -e ../$(plugin_file) ]; then \
		ln -sf `pwd`/$(plugin_file) ../$(plugin_file); \
	fi;

# Creating symbolic link to ui file in installed ui directory
create-ui-link:
	if [ -e `pwd`/$(plugin_name).ui ] && [ ! -e ../../data/ui/$(plugin_name).ui ]; then \
		ln -s `pwd`/$(plugin_name).ui ../../data/ui/$(plugin_name).ui; \
	fi;

create-glade-link:
	if  [ -e `pwd`/$(plugin_name).glade ]; then \
		if  [ ! -e ../../data/glade/$(plugin_name).glade ]; then \
			ln -s `pwd`/$(plugin_name).glade ../../data/glade/$(plugin_name).glade; \
		fi; \
	fi; \
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
	if  [ -h ../../data/glade/$(plugin_name).glade ]; then \
		rm -f ../../data/glade/$(plugin_name).glade; \
	fi; \
	if  [ -h ../../data/glade/$(plugin_name).xml ]; then \
		rm -f ../../data/glade/$(plugin_name).xml; \
	fi;

# Create plugin description file with translations
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache
