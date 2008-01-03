## Process this file with automake to produce Makefile.in

INCLUDES = -DMOUNT_BIN=\""@MOUNT@\"" \
	   -DUMOUNT_BIN=\""@UMOUNT@\"" \
	   -DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	   -DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" 

bin_PROGRAMS = gtkpod

## I'm using %option outfile="lex.yy.c" in the parser files,
## so this should be portable
LEX_OUTPUT_ROOT = lex.yy


gtkpod_SOURCES = \
    autodetection.c autodetection.h \
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
    display_photo.c display_photo.h \
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
    sort_window.c sort_window.h \
    repository.c repository.h \
    sha1.c sha1.h \
    stock_icons.c stock_icons.h \
    syncdir.c syncdir.h \
    tools.c tools.h \
    wavfile.c wavfile.h \
    help.c \
    help.h \
    prefsdlg.c \
    infodlg.c \
    infodlg.h

gtkpod_LDADD = $(LIBS) $(INTLLIBS) @LIBOBJS@

EXTRA_DIST = getopt.h

CLEANFILES = date_parser.c date_parser2.c
