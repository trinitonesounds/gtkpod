lib_LTLIBRARIES = libatomicparsley.la

libatomicparsley_la_SOURCES = AP_AtomDefinitions.h \
							  AP_AtomExtracts.h AP_AtomExtracts.cpp \
							  AP_commons.h AP_commons.cpp \
							  AP_iconv.h AP_iconv.cpp \
							  APar_sha1.h APar_sha1.cpp \
							  APar_uuid.h APar_uuid.cpp \
							  AtomicParsley_genres.h AtomicParsley_genres.cpp \
							  AtomicParsley.h AtomicParsley.cpp \
							  AtomicParsleyBridge.h AtomicParsleyBridge.cpp

# Include paths
AM_CPPFLAGS = \
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
    -I$(top_srcdir) \
    $(LIBATOMIC_CFLAGS)
 
libatomicparsley_la_LDFLAGS = $(LDFLAGS) -version-info $(LIBATOMIC_PARSLEY_SO_VERSION)
libatomicparsley_la_LIBADD = \
	$(GTKPOD_LIBS) \
	@LIBOBJS@
	
libatomicparsleyincludebase = $(includedir)/gtkpod
libatomicparsleyincludedir = $(libatomicparsleyincludebase)/gtkpod
libatomicparsleyinclude_HEADERS = AtomicParsleyBridge.h AtomicParsley.h

EXTRA_DIST = COPYING