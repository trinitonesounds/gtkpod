plugin_name = sorttab_display
plugin_lib = lib$(plugin_name).so
plugin_file = $(plugin_name).plugin

# Plugin UI file
sorttab_display_uidir = $(gtkpod_ui_dir)
sorttab_display_ui_DATA =  $(plugin_name).ui

# Plugin Glade file
sorttab_display_gladedir = $(gtkpod_glade_dir)
sorttab_display_glade_DATA =  $(plugin_name).glade

# Plugin Icon file
sorttab_display_pixmapsdir = $(gtkpod_image_dir)
sorttab_display_pixmaps_DATA =

## I'm using %option outfile="lex.yy.c" in the parser files,
## so this should be portable
LEX_OUTPUT_ROOT = lex.yy

sorttab_display_plugindir = $(gtkpod_plugin_dir)
sorttab_display_plugin_DATA = $(plugin_file)

SUBDIRS =

include ../plugins.mk

# The plugin
plugin_LTLIBRARIES = libsorttab_display.la

# Plugin sources
libsorttab_display_la_SOURCES = plugin.c plugin.h \
								sorttab_conversion.c sorttab_conversion.h \
								sorttab_display_actions.c sorttab_display_actions.h \
								display_sorttabs.c display_sorttabs.h \
								date_parser2.l date_parser.l date_parser.h

libsorttab_display_la_LDFLAGS = $(GTKPOD_PLUGIN_LDFLAGS)

# Plugin dependencies
libsorttab_display_la_LIBADD = \
    $(LIBGTKPOD_LIBS) \
    $(LIBANJUTA_LIBS)

EXTRA_DIST = \
	$(plugin_in_files) \
	$(sorttab_display_plugin_DATA) \
	$(sorttab_display_ui_DATA) \
	$(sorttab_display_glade_DATA) \
	$(sorttab_display_pixmaps_DATA)
	
CLEANFILES = date_parser.c date_parser2.c
