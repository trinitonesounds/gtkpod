## Process this file with automake to produce Makefile.in

AM_CFLAGS = @PACKAGE_CFLAGS@

INCLUDES = @PACKAGE_CFLAGS@ \
	   -DMOUNT_BIN=\""@MOUNT@\"" \
	   -DUMOUNT_BIN=\""@UMOUNT@\"" \
	   -DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	   -DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" 

bin_PROGRAMS = gtkpod itdb

## I'm using %option outfile="lex.yy.c" in the parser files,
## so this should be portable
LEX_OUTPUT_ROOT = lex.yy

itdb_SOURCES = \
	itdb_main.c \
	itdb.h \
	itdb_itunesdb.c \
	itdb_playlist.c \
	itdb_private.h \
	itdb_track.c

itdb_LDADD = @PACKAGE_LIBS@ $(INTLLIBS) @LIBOBJS@

gtkpod_SOURCES = \
    info.c info.h \
    interface.c interface.h \
    itdb_itunesdb.c itdb_playlist.c  itdb_track.c itdb_private.h itdb.h \
    main.c \
    md5.c md5.h \
    misc.c misc.h \
    misc_track.c misc_track.h \
    misc_confirm.c misc_conversion.c misc_input.c misc_playlist.c \
    mp3file.c mp3file.h \
    mp4file.c mp4file.h \
    prefs.c prefs.h \
    prefs_window.c prefs_window.h \
    tools.c tools.h \
    support.c support.h \
    wavfile.c wavfile.h \
    charset.c charset.h \
    clientserver.c clientserver.h \
    confirmation.c confirmation.h \
    context_menus.c context_menus.h \
    date_parser.l date_parser2.l date_parser.h \
    dirbrowser.c dirbrowser.h \
    display_itdb.c display_itdb.h \
    display.c display_playlists.c display_sorttabs.c \
    display_songs.c display_spl.c \
    display.h display_private.h \
    file.c file.h file_export.c \
    file_itunesdb.c \
    callbacks.c callbacks.h

gtkpod_LDADD = @PACKAGE_LIBS@ $(INTLLIBS) @LIBOBJS@

EXTRA_DIST = getopt.h
