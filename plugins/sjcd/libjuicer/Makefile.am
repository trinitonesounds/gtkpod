noinst_LTLIBRARIES = libjuicer.la

libjuicer_la_SOURCES = \
	$(MARSHALFILES) \
	rb-gst-media-types.h \
	rb-gst-media-types.c \
	sj-structures.h \
	sj-structures.c \
	sj-error.h sj-error.c \
	sj-extractor.h \
	sj-extractor.c \
	sj-metadata.h \
	sj-metadata.c \
	sj-metadata-gvfs.h \
	sj-metadata-gvfs.c \
	sj-metadata-getter.c \
	sj-metadata-getter.h \
	sj-util.h sj-util.c

libjuicer_la_CPPFLAGS = \
	-DDATADIR=\""$(datadir)"\" \
	$(DISABLE_DEPRECATED_CFLAGS) \
	$(AM_CPPFLAGS)

libjuicer_la_CFLAGS = \
	$(WARN_CFLAGS)	\
	$(MUSICBRAINZ3_CFLAGS) \
	$(GSTREAMER_CFLAGS) \
	$(BURN_CFLAGS) \
	$(UI_CFLAGS) \
	$(CDIO_CFLAGS) \
	$(AM_CFLAGS)

libjuicer_la_LIBADD = \
	$(MUSICBRAINZ3_LIBS) \
	$(GSTREAMER_LIBS) \
	$(BURN_LIBS) \
	$(UI_LIBS) \
	$(CDIO_LIBS)

libjuicer_la_LDFLAGS = \
	-export-dynamic \
	$(AM_LDFLAGS)

if HAVE_MUSICBRAINZ3
libjuicer_la_SOURCES += \
	sj-metadata-musicbrainz3.h \
	sj-metadata-musicbrainz3.c
endif

#
# Build the GValue marshals
#

MARSHALFILES = sj-metadata-marshal.h sj-metadata-marshal.c
GLIB_GENMARSHAL =`pkg-config --variable=glib_genmarshal glib-2.0`

sj-metadata-marshal.h: sj-metadata-marshal.list
	$(AM_V_GEN) ( $(GLIB_GENMARSHAL) --prefix=metadata_marshal $< \
	--header > marshal-header.tmp \
	&& mv marshal-header.tmp $@ ) \
	|| ( rm -f marshal-header.tmp && exit 1 )

sj-metadata-marshal.c: sj-metadata-marshal.list sj-metadata-marshal.h
	$(AM_V_GEN) ( $(GLIB_GENMARSHAL) --prefix=metadata_marshal $< \
	--body > marshal-source.tmp \
	&& echo "#include \"sj-metadata-marshal.h\"" > $@ \
	&& cat marshal-source.tmp >> $@ \
	&& rm -f marshal-source.tmp ) \
	|| ( rm -f marshal-source.tmp && exit 1 )

BUILT_SOURCES = $(MARSHALFILES)
CLEANFILES = $(BUILT_SOURCES)
EXTRA_DIST = sj-metadata-marshal.list

-include $(top_srcdir)/git.mk
