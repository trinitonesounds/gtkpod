plugin_name = clarity
plugin_file = $(plugin_name).plugin

# Plugin UI file
clarity_uidir = $(gtkpod_ui_dir)
clarity_ui_DATA = $(plugin_name).ui

# Plugin Glade file
clarity_gladedir = $(gtkpod_glade_dir)
clarity_glade_DATA =  $(plugin_name).xml

# Plugin Icon file
clarity_pixmapsdir = $(gtkpod_image_dir)
clarity_pixmaps_DATA =

clarity_plugindir = $(gtkpod_plugin_dir)
clarity_plugin_DATA =

SUBDIRS = icons

if HAVE_PLUGIN_CLARITY

include ../plugins.mk
clarity.plugin: build-plugin-file

# The plugin
plugin_lib = lib$(plugin_name).so
plugin_LTLIBRARIES = libclarity.la

# Plugin sources
libclarity_la_SOURCES = plugin.c plugin.h \
							  album_model.c album_model.h \
							  clarity_cover.c clarity_cover.h \
							  clarity_canvas.c clarity_canvas.h \
							  clarity_widget.c clarity_widget.h \
							  clarity_preview.c clarity_preview.h \
							  clarity_utils.c clarity_utils.h \
							  clarity_context_menu.c clarity_context_menu.h \
							  fetchcover.c fetchcover.h
libclarity_la_CFLAGS = $(CLUTTER_GTK_CFLAGS)
libclarity_la_LDFLAGS = $(GTKPOD_PLUGIN_LDFLAGS)

# Plugin dependencies
libclarity_la_LIBADD = \
    $(LIBGTKPOD_LIBS) \
    $(LIBANJUTA_LIBS) \
    $(CLUTTER_GTK_LIBS)

endif

EXTRA_DIST = \
	$(plugin_file).in \
	$(clarity_plugin_DATA) \
	$(clarity_ui_DATA) \
	$(clarity_glade_DATA) \
	$(clarity_pixmaps_DATA)
