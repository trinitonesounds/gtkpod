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
       -DGTKPOD_DOC_DIR=\""$(gtkpod_doc_dir)"\" \
       -DGTKPOD_UI_DIR=\""$(gtkpod_ui_dir)"\" \
       -DGTKPOD_GLADE_DIR=\""$(gtkpod_glade_dir)"\" \
       -DGTKPOD_IMAGE_DIR=\""$(gtkpod_image_dir)"\" \
       -DGTKPOD_SCRIPT_DIR=\""$(gtkpod_script_dir)"\"
       


bin_PROGRAMS = gtkpod

## I'm using %option outfile="lex.yy.c" in the parser files,
## so this should be portable
LEX_OUTPUT_ROOT = lex.yy


gtkpod_SOURCES = \
    anjuta-about.c anjuta-about.h \
    anjuta-action-callbacks.c anjuta-action-callbacks.h \
    anjuta-actions.h \
    anjuta-window.c anjuta-window.h \
    gtkpod.c gtkpod.h \
    main.c

gtkpod_LDADD = $(GTKPOD_LIBS) $(INTLLIBS) -ldl

### gschema settings in line with anjuta architecture ###

prefs_name = org.gtkpod

# gsettings_SCHEMAS is a list of all the schemas you want to install
schema_in_files = $(prefs_name).gschema.xml.in
gsettings_SCHEMAS = $(schema_in_files:.gschema.xml.in=.gschema.xml)

# include the appropriate makefile rules for schema handling
@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

EXTRA_DIST = \
	$(schema_in_files)

CLEANFILES = $(gsettings_SCHEMAS)
