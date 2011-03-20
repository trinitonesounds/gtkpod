plugin_name = filetype_wav
plugin_file = $(plugin_name).plugin

# Plugin UI file
filetype_wav_uidir = $(gtkpod_ui_dir)
filetype_wav_ui_DATA =

# Plugin Glade file
filetype_wav_gladedir = $(gtkpod_glade_dir)
filetype_wav_glade_DATA =

# Plugin Icon file
filetype_wav_pixmapsdir = $(gtkpod_image_dir)
filetype_wav_pixmaps_DATA =

filetype_wav_plugindir = $(gtkpod_plugin_dir)
filetype_wav_plugin_DATA =

SUBDIRS =

include ../plugins.mk
filetype_wav.plugin: build-plugin-file

# The plugin
plugin_lib = lib$(plugin_name).so
plugin_LTLIBRARIES = libfiletype_wav.la

# Plugin sources
libfiletype_wav_la_SOURCES = plugin.c plugin.h \
							  wavfile.c wavfile.h

libfiletype_wav_la_LDFLAGS = $(GTKPOD_PLUGIN_LDFLAGS)

# Plugin dependencies
libfiletype_wav_la_LIBADD = \
    $(GTKPOD_LIBS) \
    $(LIBANJUTA_LIBS)

EXTRA_DIST = \
	$(plugin_file).in \
	$(filetype_wav_plugin_DATA) \
	$(filetype_wav_ui_DATA) \
	$(filetype_wav_glade_DATA) \
	$(filetype_wav_pixmaps_DATA)
	