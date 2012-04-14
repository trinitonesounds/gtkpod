plugin_name = filetype_m4a
plugin_file = $(plugin_name).plugin

# Plugin UI file
filetype_m4a_uidir = $(gtkpod_ui_dir)
filetype_m4a_ui_DATA =

# Plugin Glade file
filetype_m4a_gladedir = $(gtkpod_glade_dir)
filetype_m4a_glade_DATA =

# Plugin Icon file
filetype_m4a_pixmapsdir = $(gtkpod_image_dir)
filetype_m4a_pixmaps_DATA =

# Where to install the plugin
filetype_m4a_plugindir = $(gtkpod_plugin_dir)
filetype_m4a_plugin_DATA =

SUBDIRS =

if HAVE_PLUGIN_FILETYPE_M4A

include ../plugins.mk
filetype_m4a.plugin: build-plugin-file

# The plugin
plugin_lib = lib$(plugin_name).so
plugin_LTLIBRARIES = libfiletype_m4a.la

# Plugin sources
libfiletype_m4a_la_SOURCES = plugin.c plugin.h \
							  m4afile.c m4afile.h

libfiletype_m4a_la_CFLAGS = -I $(top_srcdir)/libs/atomic-parsley
libfiletype_m4a_la_LDFLAGS = $(GTKPOD_PLUGIN_LDFLAGS)

# Plugin dependencies
libfiletype_m4a_la_LIBADD = \
    $(GTKPOD_LIBS) \
    $(LIBANJUTA_LIBS) \
    $(top_builddir)/libs/atomic-parsley/libatomicparsley.la

endif

EXTRA_DIST = \
	$(plugin_file).in \
	$(filetype_m4a_plugin_DATA) \
	$(filetype_m4a_ui_DATA) \
	$(filetype_m4a_glade_DATA) \
	$(filetype_m4a_pixmaps_DATA)
