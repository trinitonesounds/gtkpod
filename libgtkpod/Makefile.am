lib_LTLIBRARIES = libgtkpod.la

libgtkpod_la_SOURCES =  gtkpod_app_iface.h gtkpod_app_iface.c \
						filetype_iface.h filetype_iface.c \
						track_command_iface.h track_command_iface.c \
						exporter_iface.h exporter_iface.c \
						repository_editor_iface.h repository_editor_iface.c \
						details_editor_iface.h details_editor_iface.c \
						lyrics_editor_iface.h lyrics_editor_iface.c \
						photo_editor_iface.h photo_editor_iface.c \
						itdb.h file_convert_info.h \
						gp_private.h gp_private.c \
						gp_itdb.h gp_itdb.c \
						charset.h charset.c \
						sha1.h sha1.c \
						file.h file.c \
						file_itunesdb.c \
						file_convert.c file_convert.h \
						fileselection.c fileselection.h \
						misc_track.h misc_track.c \
						prefs.h prefs.c \
						syncdir.h syncdir.c \
						misc.h misc.c \
						misc_conversion.h misc_conversion.c \
						clientserver.h clientserver.c \
						directories.h directories.c \
						tools.c tools.h \
						misc_playlist.c misc_playlist.h \
						stock_icons.c stock_icons.h \
						gtkpod_app-marshallers.c gtkpod_app-marshallers.h \
						tool_menu_action.c tool_menu_action.h \
						context_menus.c context_menus.h \
						gp_spl.c gp_spl.h \
						autodetection.c autodetection.h

# Include paths
AM_CFLAGS = \
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
    -DGTKPOD_SCRIPT_DIR=\""$(gtkpod_script_dir)"\" \
    $(LIBGTKPOD_CFLAGS)
 
libgtkpod_la_LDFLAGS = $(LIBGTKPOD_LDFLAGS) -version-info $(LIBGTKPOD_SO_VERSION) -no-undefined
libgtkpod_la_LIBADD = \
	$(LIBGTKPOD_LIBS) \
	@LIBOBJS@
	
libgtkpodincludebase = $(includedir)/gtkpod
libgtkpodincludedir = $(libgtkpodincludebase)/gtkpod
libgtkpodinclude_HEADERS = gp_itdb.h gtkpod_app_iface.h

INCLUDES=$(LIBGTKPOD_CFLAGS)
LIBS=$(LIBGTKPOD_LIBS) -lm

EXTRA_DIST = gtkpod_app-marshallers.list

all-local: gtkpod_app-marshallers.h gtkpod_app-marshallers.c

gtkpod_app-marshallers.c: gtkpod_app-marshallers.list
	glib-genmarshal --prefix _gtkpod_app_marshal --body $< > $@

gtkpod_app-marshallers.h: gtkpod_app-marshallers.list
	glib-genmarshal --prefix _gtkpod_app_marshal --header $< > $@

