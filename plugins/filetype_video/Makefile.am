plugin_name = filetype_video
plugin_lib = lib$(plugin_name).so
plugin_file = $(plugin_name).plugin

# Plugin UI file
filetype_video_uidir = $(gtkpod_ui_dir)
filetype_video_ui_DATA =

# Plugin Glade file
filetype_video_gladedir = $(gtkpod_glade_dir)
filetype_video_glade_DATA =

# Plugin Icon file
filetype_video_pixmapsdir = $(gtkpod_image_dir)
filetype_video_pixmaps_DATA =

filetype_video_plugindir = $(gtkpod_plugin_dir)
filetype_video_plugin_DATA =

SUBDIRS =

include ../plugins.mk

# The plugin
plugin_LTLIBRARIES = libfiletype_video.la

# Plugin sources
libfiletype_video_la_SOURCES = plugin.c plugin.h \
							  videofile.c videofile.h

libfiletype_video_la_LDFLAGS = $(GTKPOD_PLUGIN_LDFLAGS)

# Plugin dependencies
libfiletype_video_la_LIBADD = \
    $(LIBGTKPOD_LIBS) \
    $(LIBANJUTA_LIBS)

EXTRA_DIST = \
	$(plugin_in_files) \
	$(filetype_video_plugin_DATA) \
	$(filetype_video_ui_DATA) \
	$(filetype_video_glade_DATA) \
	$(filetype_video_pixmaps_DATA)
	