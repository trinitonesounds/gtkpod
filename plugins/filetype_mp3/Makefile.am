plugin_name = filetype_mp3
plugin_file = $(plugin_name).plugin

# Plugin UI file
filetype_mp3_uidir = $(gtkpod_ui_dir)
filetype_mp3_ui_DATA =

# Plugin Glade file
filetype_mp3_gladedir = $(gtkpod_glade_dir)
filetype_mp3_glade_DATA =

# Plugin Icon file
filetype_mp3_pixmapsdir = $(gtkpod_image_dir)
filetype_mp3_pixmaps_DATA =

filetype_mp3_plugindir = $(gtkpod_plugin_dir)
filetype_mp3_plugin_DATA =

SUBDIRS =

include ../plugins.mk
filetype_mp3.plugin: build-plugin-file

# The plugin
plugin_lib = lib$(plugin_name).so
plugin_LTLIBRARIES = libfiletype_mp3.la

# Plugin sources
libfiletype_mp3_la_SOURCES = plugin.c plugin.h \
							  mp3file.c mp3file.h

libfiletype_mp3_la_LDFLAGS = $(GTKPOD_PLUGIN_LDFLAGS)

# Plugin dependencies
libfiletype_mp3_la_LIBADD = \
    $(GTKPOD_LIBS) \
    $(LIBANJUTA_LIBS)

EXTRA_DIST = \
	$(plugin_file).in \
	$(filetype_mp3_plugin_DATA) \
	$(filetype_mp3_ui_DATA) \
	$(filetype_mp3_glade_DATA) \
	$(filetype_mp3_pixmaps_DATA)
