## Process this file with automake to produce Makefile.in

INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@PACKAGE_CFLAGS@

bin_PROGRAMS = gtkpod

gtkpod_SOURCES = \
	charset.c charset.h \
	confirmation.c confirmation.h \
	context_menus.c context_menus.h \
	date_parser.l date_parser.h \
	dirbrowser.c dirbrowser.h \
	display.c display_playlists.c display_sorttabs.c display_songs.c \
	display.h display_private.h \
        file.c file.h \
	file_export.c file_export.h \
	id3_tag.c id3_tag.h genres.h \
	interface.c interface.h \
	itunesdb.c itunesdb.h \
	main.c \
	md5.c md5.h \
	misc.c misc.h \
	mp3file.c mp3file.h \
	playlist.c playlist.h \
	prefs.c prefs.h \
	prefs_window.c prefs_window.h \
	song.c song.h \
	support.c support.h \
	callbacks.c callbacks.h

gtkpod_LDADD = @PACKAGE_LIBS@ $(INTLLIBS) @LIBOBJS@

EXTRA_DIST = getopt.h
