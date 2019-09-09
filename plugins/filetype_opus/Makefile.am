plugin_name = filetype_opus
plugin_file = $(plugin_name).plugin

# Plugin UI file
filetype_opus_uidir = $(gtkpod_ui_dir)
filetype_opus_ui_DATA =

# Plugin Glade file
filetype_opus_gladedir = $(gtkpod_glade_dir)
filetype_opus_glade_DATA =

# Plugin Icon file
filetype_opus_pixmapsdir = $(gtkpod_image_dir)
filetype_opus_pixmaps_DATA =

# Where to install the plugin
filetype_opus_plugindir = $(gtkpod_plugin_dir)
filetype_opus_plugin_DATA =

SUBDIRS =

if HAVE_PLUGIN_FILETYPE_OPUS

include ../plugins.mk
filetype_opus.plugin: build-plugin-file

# The plugin
plugin_lib = lib$(plugin_name).so
plugin_LTLIBRARIES = libfiletype_opus.la

# Plugin sources
libfiletype_opus_la_SOURCES = plugin.c plugin.h \
							  opusfile.c opusfile.h
libfiletype_opus_la_CFLAGS = $(OPUSFILE_CFLAGS)
libfiletype_opus_la_LDFLAGS = $(GTKPOD_PLUGIN_LDFLAGS)

# Plugin dependencies
libfiletype_opus_la_LIBADD = \
    $(GTKPOD_LIBS) \
    $(LIBANJUTA_LIBS) \
    $(OPUSFILE_LIBS)

endif

EXTRA_DIST = \
	$(plugin_file).in \
	$(filetype_opus_plugin_DATA) \
	$(filetype_opus_ui_DATA) \
	$(filetype_opus_glade_DATA) \
	$(filetype_opus_pixmaps_DATA)
