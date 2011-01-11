plugin_name = repository_editor
plugin_lib = lib$(plugin_name).so
plugin_file = $(plugin_name).plugin

# Plugin UI file
repository_editor_uidir = $(gtkpod_ui_dir)
repository_editor_ui_DATA = $(plugin_name).ui

# Plugin Glade file
repository_editor_gladedir = $(gtkpod_glade_dir)
repository_editor_glade_DATA =  $(plugin_name).glade

# Plugin Icon file
repository_editor_pixmapsdir = $(gtkpod_image_dir)
repository_editor_pixmaps_DATA =

repository_editor_plugindir = $(gtkpod_plugin_dir)
repository_editor_plugin_DATA =

SUBDIRS =

include ../plugins.mk

# The plugin
plugin_LTLIBRARIES = librepository_editor.la

# Plugin sources
librepository_editor_la_SOURCES = plugin.c plugin.h \
						 		  repository.c repository.h \
						 		  repository_actions.c repository_actions.h \
						 		  repository_init.c \
						 		  repository_editor.c \
						 		  repository_creator.c

librepository_editor_la_LDFLAGS = $(GTKPOD_PLUGIN_LDFLAGS)

# Plugin dependencies
librepository_editor_la_LIBADD = \
    $(LIBGTKPOD_LIBS) \
    $(LIBANJUTA_LIBS)

EXTRA_DIST = \
	$(plugin_in_files) \
	$(repository_editor_plugin_DATA) \
	$(repository_editor_ui_DATA) \
	$(repository_editor_glade_DATA) \
	$(repository_editor_pixmaps_DATA)
