## Process this file with automake to produce Makefile.in

INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@PACKAGE_CFLAGS@

bin_PROGRAMS = gtkpod

gtkpod_SOURCES = \
	main.c \
	support.c support.h \
	interface.c interface.h \
	callbacks.c callbacks.h \
	prefs.c prefs.h \
	misc.c misc.h \
	song.c song.h \
	playlist.c playlist.h \
        display.c display.h \
	itunesdb.c itunesdb.h \
	id3_tag.c id3_tag.h genres.h \
	mp3file.c mp3file.h \
	dirbrowser.c dirbrowser.h \
	md5.c md5.h \
	prefs_window.c prefs_window.h \
	delete_window.c delete_window.h

gtkpod_LDADD = @PACKAGE_LIBS@ $(INTLLIBS)

