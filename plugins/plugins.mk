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
    -I$(top_srcdir) \
    $(GTKPOD_CFLAGS) \
    $(LIBANJUTA_LIBS)

# Where to install the plugin
plugindir = $(gtkpod_plugin_dir)
plugin_DATA = $(plugin_file)

all-local: create-plugin-links create-gui-links
.PHONY: create-plugin-links create-gui-links

# Creating symbolic links in plugin root directory
create-plugin-links:
	if [ ! -e ../$(plugin_lib) ]; then \
		$(LN_S) `pwd`/.libs/$(plugin_lib) ../$(plugin_lib); \
	fi; \
	if [ ! -e ../$(plugin_file) ]; then \
		$(LN_S) `pwd`/$(plugin_file) ../$(plugin_file); \
	fi;

# Creating symbolic link to glade and ui files in installed directories
# Note: this will symlink to all xml files, inc. toolbar xml
#       files not just gtkbuilder files
create-gui-links:
	for file in $(plugin_name)*.xml $(plugin_name)*.glade; \
	do \
		if [ ! -e "$(top_srcdir)/data/glade/$$file" -a -e `pwd`/"$$file" ]; then \
			$(LN_S) `pwd`/"$$file" $(top_srcdir)/data/glade/"$$file"; \
		fi; \
	done;
	\
	for file in $(plugin_name)*.ui; \
	do \
		if [ ! -e $(top_srcdir)/data/ui/"$$file" -a -e `pwd`/"$$file" ]; then \
			$(LN_S) `pwd`/"$$file" $(top_srcdir)/data/ui/"$$file"; \
		fi; \
	done;

# Clean up the links and files created purely for development
clean-local: clean-dev-files
clean-local-check: clean-dev-files
.PHONY: clean-dev-files clean-local-check

clean-dev-files:
	for file in $(top_srcdir)/data/ui/$(plugin_name)*.ui \
				$(top_srcdir)/data/glade/$(plugin_name)* \
				$(top_srcdir)/plugins/$(plugin_lib) \
				$(top_srcdir)/plugins/$(plugin_file) \
				$(plugin_file); \
	do \
		if [ -e "$$file" ]; then \
			rm -f "$$file"; \
		fi; \
	done;

# List out the current language po files
PO_FILES=\
					$(top_srcdir)/po/ca.po \
					$(top_srcdir)/po/cs_CZ.po \
					$(top_srcdir)/po/de.po \
					$(top_srcdir)/po/es.po \
					$(top_srcdir)/po/fr.po \
					$(top_srcdir)/po/he.po \
					$(top_srcdir)/po/it.po \
					$(top_srcdir)/po/ja.po \
					$(top_srcdir)/po/nl.po \
					$(top_srcdir)/po/pt_BR.po \
					$(top_srcdir)/po/ro.po \
					$(top_srcdir)/po/ru.po \
					$(top_srcdir)/po/sv.po \
					$(top_srcdir)/po/zh_CN.po \
					$(top_srcdir)/po/zh_TW.po

# Create plugin description file with translations
build-plugin-file: $(INTLTOOL_MERGE) $(PO_FILES)
	$(INTLTOOL_MERGE) $(top_srcdir)/po $(plugin_name).plugin.in $(plugin_name).plugin -d -u -c $(top_builddir)/po/.intltool-merge-cache

