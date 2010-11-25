#ifdef HAVE_LIBVORBISFILE

plugin_name = filetype_ogg
plugin_lib = lib$(plugin_name).so
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

filetype_ogg_plugindir = $(gtkpod_plugin_dir)
filetype_ogg_plugin_DATA = $(plugin_file)

SUBDIRS =

include ../plugins.mk

# The plugin
plugin_LTLIBRARIES = libfiletype_ogg.la

# Plugin sources
libfiletype_ogg_la_SOURCES = plugin.c plugin.h \
							  oggfile.c oggfile.h

libfiletype_ogg_la_LDFLAGS = $(GTKPOD_PLUGIN_LDFLAGS)

# Plugin dependencies
libfiletype_ogg_la_LIBADD = \
    $(LIBGTKPOD_LIBS) \
    $(LIBANJUTA_LIBS)

EXTRA_DIST = \
	$(plugin_in_files) \
	$(filetype_ogg_plugin_DATA) \
	$(filetype_ogg_ui_DATA) \
	$(filetype_ogg_glade_DATA) \
	$(filetype_ogg_pixmaps_DATA)

#endif