## Process this file with automake to produce Makefile.in

AM_CFLAGS = \
        $(GTKPOD_CFLAGS) \
       -I$(top_srcdir)\
       -I.. -I. \
       -DMOUNT_BIN=\""@MOUNT@\"" \
	   -DUMOUNT_BIN=\""@UMOUNT@\"" \
	   -DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	   -DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	   -DPACKAGE_PLUGIN_DIR=\""$(libdir)"\" \
       -DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
       -DGTKPOD_PLUGIN_DIR=\""$(gtkpod_plugin_dir)"\" \
       -DGTKPOD_DATA_DIR=\""$(gtkpod_data_dir)"\" \
       -DGTKPOD_UI_DIR=\""$(gtkpod_ui_dir)"\" \
       -DGTKPOD_GLADE_DIR=\""$(gtkpod_glade_dir)"\" \
       -DGTKPOD_IMAGE_DIR=\""$(gtkpod_image_dir)"\"
       


bin_PROGRAMS = gtkpod

## I'm using %option outfile="lex.yy.c" in the parser files,
## so this should be portable
LEX_OUTPUT_ROOT = lex.yy


gtkpod_SOURCES = \
    anjuta-about.c anjuta-about.h \
    anjuta-action-callbacks.c anjuta-action-callbacks.h \
    anjuta-actions.h \
    anjuta-app.c anjuta-app.h \
    gtkpod.c gtkpod.h \
    main.c

gtkpod_LDADD = $(GTKPOD_LIBS) $(INTLLIBS) @LIBOBJS@
    
EXTRA_DIST = getopt.h

CLEANFILES = date_parser.c date_parser2.c
