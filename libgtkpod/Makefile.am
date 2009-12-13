lib_LTLIBRARIES = libgtkpod.la

libgtkpod_la_SOURCES =  gtkpod_app_iface.h gtkpod_app_iface.c \
                        itdb.h file_convert_info.h \
						gp_itdb.h gp_itdb.c \
						charset.h charset.c \
						sha1.h sha1.c \
						file.h file.c \
						file_itunesdb.c \
						file_convert.c file_convert.h \
						ipod_init.c ipod_init.h \
						misc_track.h misc_track.c \
						prefs.h prefs.c \
						syncdir.h syncdir.c \
						misc.h misc.c \
						misc_conversion.h misc_conversion.c \
						clientserver.h clientserver.c \
						directories.h directories.c \
						tools.c tools.h \
						misc_playlist.c

# Include paths
AM_CFLAGS = \
    -DPACKAGE_DATA_DIR=\""$(datadir)"\" \
    -DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
    -DPACKAGE_PLUGIN_DIR=\""$(libdir)"\" \
    -DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
    -DGTKPOD_PLUGIN_DIR=\""$(gtkpod_plugin_dir)"\" \
    -DGTKPOD_DATA_DIR=\""$(gtkpod_data_dir)"\" \
    -DGTKPOD_UI_DIR=\""$(gtkpod_ui_dir)"\" \
    -DGTKPOD_GLADE_DIR=\""$(gtkpod_glade_dir)"\" \
    -DGTKPOD_IMAGE_DIR=\""$(gtkpod_image_dir)"\" \
    $(LIBGTKPOD_CFLAGS)
 
libgtkpod_la_LDFLAGS = $(LIBGTKPOD_LDFLAGS) -version-info $(LIBGTKPOD_SO_VERSION) -no-undefined
libgtkpod_la_LIBADD = \
	$(LIBGTKPOD_LIBS)
	
libgtkpodincludebase = $(includedir)/gtkpod-1.0
libgtkpodincludedir = $(libgtkpodincludebase)/gtkpod
libgtkpodinclude_HEADERS = gp_itdb.h gtkpod_app_iface.h

INCLUDES=$(LIBGTKPOD_CFLAGS)
LIBS=$(LIBGTKPOD_LIBS) -lm
