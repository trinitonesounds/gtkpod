if HAVE_MP4

plugin_name = filetype_mp4
plugin_lib = lib$(plugin_name).so
plugin_file = $(plugin_name).plugin

# Plugin UI file
filetype_mp4_uidir = $(gtkpod_ui_dir)
filetype_mp4_ui_DATA =

# Plugin Glade file
filetype_mp4_gladedir = $(gtkpod_glade_dir)
filetype_mp4_glade_DATA =

# Plugin Icon file
filetype_mp4_pixmapsdir = $(gtkpod_image_dir)
filetype_mp4_pixmaps_DATA =

filetype_mp4_plugindir = $(gtkpod_plugin_dir)
filetype_mp4_plugin_DATA = $(plugin_file)

SUBDIRS =

include ../plugins.mk

# The plugin
plugin_LTLIBRARIES = libfiletype_mp4.la

# Plugin sources
libfiletype_mp4_la_SOURCES = plugin.c plugin.h \
							  mp4file.c mp4file.h

libfiletype_mp4_la_LDFLAGS = $(GTKPOD_PLUGIN_LDFLAGS)

# Plugin dependencies
libfiletype_mp4_la_LIBADD = \
    $(LIBGTKPOD_LIBS) \
    $(LIBANJUTA_LIBS)

endif

EXTRA_DIST = \
	$(plugin_in_files) \
	$(filetype_mp4_plugin_DATA) \
	$(filetype_mp4_ui_DATA) \
	$(filetype_mp4_glade_DATA) \
	$(filetype_mp4_pixmaps_DATA)