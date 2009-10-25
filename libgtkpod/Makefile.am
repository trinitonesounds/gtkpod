lib_LTLIBRARIES = libgtkpod.la

libgtkpod_la_SOURCES =  itdb.h file_convert_info.h \
						gp_itdb.h gp_itdb.c \
						charset.h charset.c \
						sha1.h sha1.c \
						file.h file.c \
						misc_track.h misc_track.c \
						prefs.h prefs.c \
						syncdir.h syncdir.c \
						misc.h misc.c \
						types.h types.c \
						clientserver.h clientserver.c

# Include paths
AM_CPPFLAGS = \
    -DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
    -DGTKPOD_DATA_DIR=\"$(gtkpod_data_dir)\" \
    -DGTKPDO_PLUGIN_DIR=\"$(gtkpod_plugin_dir)\" \
    -DGTKPDO_IMAGE_DIR=\"$(gtkpod_image_dir)\" \
    -DGTKPDO_GLADE_DIR=\"$(gtkpod_glade_dir)\" \
    -DGTKPOD_UI_DIR=\"$(gtkpod_ui_dir)\" \
    -DPACKAGE_DATA_DIR=\"$(datadir)\" \
    -DPACKAGE_SRC_DIR=\"$(srcdir)\" \
    $(GTKPOD_CFLAGS)
 
libgtkpod_la_LDFLAGS = $(GTKPOD_LDFLAGS) -version-info $(LIBGTKPOD_SO_VERSION) -no-undefined
libgtkpod_la_LIBADD = \
	$(GTKPOD_LIBS)
	
libgtkpodincludebase = $(includedir)/gtkpod-1.0
libgtkpodincludedir = $(libgtkpodincludebase)/gtkpod
libgtkpodinclude_HEADERS = gp_itdb.h

INCLUDES=$(GTKPOD_CFLAGS)
LIBS=$(GTKPOD_LIBS) -lm