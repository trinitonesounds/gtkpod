## Process this file with automake to produce Makefile.in

AM_CFLAGS = @PACKAGE_CFLAGS@

INCLUDES = @PACKAGE_CFLAGS@ \
	   -DMOUNT_BIN=\""@MOUNT@\"" \
	   -DUMOUNT_BIN=\""@UMOUNT@\"" \
	   -DEJECT_BIN=\""@EJECT@\"" \
	   -DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	   -DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" 

bin_PROGRAMS = gtkpod

BUILT_SOURCES = \
        ../pixmaps/gtkpod.glade \
        ../pixmaps/gtkpod.gladep

../pixmaps/gtkpod.glade:
	ln -s ../gtkpod.glade ../pixmaps/gtkpod.glade

../pixmaps/gtkpod.gladep:
	ln -s ../gtkpod.gladep ../pixmaps/gtkpod.gladep

## I'm using %option outfile="lex.yy.c" in the parser files,
## so this should be portable
LEX_OUTPUT_ROOT = lex.yy


gtkpod_SOURCES = \
    charset.c charset.h \
    clientserver.c clientserver.h \
    confirmation.c confirmation.h \
    context_menus.c context_menus.h \
    date_parser.l date_parser2.l date_parser.h \
    details.c details.h \
    display_coverart.c display_coverart.h \
    display.c display_playlists.c display_sorttabs.c \
    display.h display_private.h \
    display_itdb.c display_itdb.h \
    display_spl.c display_tracks.c \
    fetchcover.c fetchcover.h \
    file.c file.h file_export.c \
    file_convert.c file_convert.h \
    file_itunesdb.c \
    fileselection.h fileselection.c \
    flacfile.c flacfile.h \
    info.c info.h \
    ipod_init.c ipod_init.h \
    itdb.h \
    main.c \
    misc.c misc.h \
    misc_confirm.c misc_conversion.c misc_input.c \
    misc_playlist.c \
    misc_track.c misc_track.h \
    mp3file.c mp3file.h \
    mp4file.c mp4file.h \
    oggfile.c oggfile.h \
    podcast.c podcast.h \
    prefs.c prefs.h \
    prefs_window.c prefs_window.h \
    repository.c repository.h \
    sha1.c sha1.h \
    syncdir.c syncdir.h \
    tools.c tools.h \
    wavfile.c wavfile.h

gtkpod_LDADD = @PACKAGE_LIBS@ $(INTLLIBS) @LIBOBJS@

EXTRA_DIST = getopt.h
