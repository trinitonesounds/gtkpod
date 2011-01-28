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
plugin_DATA = $(plugin_file)

all-local: create-plugin-links create-ui-link create-glade-link
.PHONY: create-plugin-links create-ui-link create-glade-link

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
clean-local-check: clean-plugin-files
.PHONY: clean-plugin-files clean-ui-dir clean-glade-dir clean-local-check

clean-plugin-files:
	if [ -h ../$(plugin_file) ]; then \
		rm -f ../$(plugin_lib) ../$(plugin_file) $(plugin_file); \
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

# List out the current language po files
PO_FILES=\
				 $(top_srcdir)/po/ca.po \
				 $(top_srcdir)/po/es.po \
				 $(top_srcdir)/po/he.po \
				 $(top_srcdir)/po/ja.po \
				 $(top_srcdir)/po/ru.po \
				 $(top_srcdir)/po/zh_CN.po \
				 $(top_srcdir)/po/de.po \
				 $(top_srcdir)/po/fr.po \
				 $(top_srcdir)/po/it.po \
				 $(top_srcdir)/po/ro.po \
				 $(top_srcdir)/po/sv.po \
				 $(top_srcdir)/po/zh_TW.po

# Create plugin description file with translations
build-plugin-file: $(INTLTOOL_MERGE) $(PO_FILES)
	$(INTLTOOL_MERGE) $(top_srcdir)/po $(plugin_name).plugin.in $(plugin_name).plugin -d -u -c $(top_builddir)/po/.intltool-merge-cache

