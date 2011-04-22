plugin_name = filetype_ogg
plugin_file = $(plugin_name).plugin

# Plugin UI file
filetype_ogg_uidir = $(gtkpod_ui_dir)
filetype_ogg_ui_DATA =

# Plugin Glade file
filetype_ogg_gladedir = $(gtkpod_glade_dir)
filetype_ogg_glade_DATA =

# Plugin Icon file
filetype_ogg_pixmapsdir = $(gtkpod_image_dir)
filetype_ogg_pixmaps_DATA =

# Where to install the plugin
filetype_ogg_plugindir = $(gtkpod_plugin_dir)
filetype_ogg_plugin_DATA =

SUBDIRS =

if HAVE_LIBVORBISFILE

include ../plugins.mk
filetype_ogg.plugin: build-plugin-file

# The plugin
plugin_lib = lib$(plugin_name).so
plugin_LTLIBRARIES = libfiletype_ogg.la

# Plugin sources
libfiletype_ogg_la_SOURCES = plugin.c plugin.h \
							  oggfile.c oggfile.h
libfiletype_ogg_la_CFLAGS = $(VORBISFILE_CFLAGS)
libfiletype_ogg_la_LDFLAGS = $(GTKPOD_PLUGIN_LDFLAGS)

# Plugin dependencies
libfiletype_ogg_la_LIBADD = \
    $(GTKPOD_LIBS) \
    $(LIBANJUTA_LIBS) \
    $(VORBISFILE_LIBS)

endif

EXTRA_DIST = \
	$(plugin_file).in \
	$(filetype_ogg_plugin_DATA) \
	$(filetype_ogg_ui_DATA) \
	$(filetype_ogg_glade_DATA) \
	$(filetype_ogg_pixmaps_DATA)
