dnl Process this file with autoconf to produce a configure script.
m4_include([./version.m4])

# Require autoconf 2.55 or higher
AC_PREREQ(2.65)
AC_INIT([gtkpod], VERSION_NUMBER)
AC_CONFIG_SRCDIR(src/main.c)

AM_CONFIG_HEADER([config.h])

dnl libtool versioning
# +1 : 0 : +1  == adds new functions to the interface
# +1 : 0 : 0   == changes or removes functions (changes include both
#                 changes to the signature and the semantic)
#  ? :+1 : ?   == just internal changes
# CURRENT : REVISION : AGE
LIBGTKPOD_SO_VERSION="2:0:1"
LIBATOMIC_PARSLEY_SO_VERSION="0:1:0"

LIBGTKPOD_VERSION=`./version.sh`

AC_SUBST(LIBGTKPOD_SO_VERSION)
AC_SUBST(LIBGTKPOD_VERSION)
AC_SUBST(LIBATOMIC_PARSLEY_SO_VERSION)

AM_INIT_AUTOMAKE([1.9 tar-ustar]) 

AM_MAINTAINER_MODE

AC_PROG_CC
AC_PROG_CXX

LT_PATH_LD
AC_HEADER_STDC
AC_PROG_INSTALL
LT_INIT
AC_CANONICAL_HOST
AC_SEARCH_LIBS([strerror],[cposix])

AC_DIAGNOSE([obsolete],[AM_PROG_CC_STDC:
	your code should no longer depend upon am_cv_prog_cc_stdc, but upon
	ac_cv_prog_cc_stdc.  Remove this warning and the assignment when
	you adjust the code.  You can also remove the above call to
	AC_PROG_CC if you already called it elsewhere.])
am_cv_prog_cc_stdc=$ac_cv_prog_cc_stdc

AM_PROG_LIBTOOL
IT_PROG_INTLTOOL([0.33])

dnl Check for ln -s
AC_PROG_LN_S

dnl Only use -Wall if we have gcc

if test "x$GCC" = "xyes"; then
    if test -z "`echo "$CFLAGS" | grep "\-Wall" 2> /dev/null`" ; then
        CFLAGS="$CFLAGS -Wall"
    fi
    # gcc < 4.0 does not know '-Wno-pointer-sign'. Try to find out
    # whether we can set this option or not.
    CFLAGS_orig=$CFLAGS
    CFLAGS="$CFLAGS -Wno-pointer-sign"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <stdio.h>]])],
                      [], [CFLAGS=$CFLAGS_orig])
fi

GETTEXT_PACKAGE=gtkpod
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE",["Gettext package name"])

dnl GSettings
GLIB_GSETTINGS

dnl check if pkg-config exists -- if not print a meaningful error message
AC_PATH_TOOL([PKG_CONFIG], [pkg-config])
if test -z "$PKG_CONFIG"; then
    AC_MSG_FAILURE([*** pkg-config required (http://www.freedesktop.org/software/pkgconfig)])
else
    ac_cv_env_PKG_CONFIG_set=set
fi

dnl check for required libs that are managed with pkg-config
PKG_CHECK_MODULES(GTK, gtk+-3.0 >= 3.0.8,, [AC_MSG_FAILURE([*** $GTK_PKG_ERRORS])])
PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.31,, [AC_MSG_FAILURE([*** $GLIB_PKG_ERRORS])])
PKG_CHECK_MODULES(GDK, gdk-3.0 >= 3.0.11,, [AC_MSG_FAILURE([*** $GDK_PKG_ERRORS])])
PKG_CHECK_MODULES(GMODULE, gmodule-2.0 >= 2.31,, [AC_MSG_FAILURE([*** $GMODULE_PKG_ERRORS])])
PKG_CHECK_MODULES(GTHREAD, gthread-2.0 >= 2.31,, [AC_MSG_FAILURE([*** $GTHREAD_PKG_ERRORS])])
PKG_CHECK_MODULES(LIBGPOD, libgpod-1.0 >= 0.7.0,, [AC_MSG_FAILURE([*** $LIBGPOD_PKG_ERRORS])])
PKG_CHECK_MODULES(LIBANJUTA, libanjuta-3.0 >= 2.91,, [AC_MSG_FAILURE([*** $LIBANJUTA_PKG_ERRORS])])
PKG_CHECK_MODULES(LIBGDL, gdl-3.0 >= 3.0.0,, [AC_MSG_FAILURE([*** $LIBGDL_PKG_ERRORS])])
PKG_CHECK_MODULES(GIO, gio-2.0 >= 2.31,, [AC_MSG_FAILURE([*** $GIO_PKG_ERRORS])])
PKG_CHECK_MODULES(LIBXML, libxml-2.0 >= 2.7.7,, [AC_MSG_FAILURE([*** $LIBXML_PKG_ERRORS])])
PKG_CHECK_MODULES(ID3TAG, id3tag >= 0.15, [have_id3="yes"], [AC_MSG_FAILURE([*** $ID3TAG_PKG_ERRORS])])

dnl --- optional dependencies ---
echo "Checking optional dependencies"
PKG_CHECK_MODULES(CLUTTER_GTK, [clutter-gtk-1.0 >= 1.2], [have_clutter_gtk="yes"], [have_clutter_gtk="no"])
PKG_CHECK_MODULES(CURL, [libcurl >= 7.10.0], [have_curl="yes"], [have_curl="no"])
PKG_CHECK_MODULES(WEBKIT, [webkitgtk-3.0 >= 1.3], [have_webkit="yes"], [have_webkit="no"])
PKG_CHECK_MODULES(GSTREAMER, gstreamer-1.0 >= 1.0 gstreamer-base-1.0 >= 1.0 gstreamer-plugins-base-1.0 >= 1.0 gstreamer-pbutils-1.0 >= 1.0, [have_gstreamer="yes"], [have_gstreamer="no"])
PKG_CHECK_MODULES(VORBISFILE, vorbisfile >= 1.3.1, [have_vorbis="yes"], [have_vorbis="no"])
PKG_CHECK_MODULES(FLAC, flac >= 1.2.1, [have_flac="yes"], [have_flac="no"])
PKG_CHECK_MODULES(BRASERO, libbrasero-media3 >= 3.0, [have_brasero="yes"], [have_brasero="no"])		
PKG_CHECK_MODULES(MUSICBRAINZ5, libmusicbrainz5 >= 5.0.1, [have_mb5="yes"], [have_mb5="no"])
PKG_CHECK_MODULES(DISCID, libdiscid >= 0.2.2, [have_discid="yes"], [have_discid="no"])

GTK_CLEANLINESS_FLAGS="-DG_DISABLE_SINGLE_INCLUDES -DGDK_PIXBUF_DISABLE_SINGLE_INCLUDES -DGTK_DISABLE_SINGLE_INCLUDES -DGSEAL_ENABLE"

dnl Force C99 mode (no implicit int declarations)
CFLAGS="$CFLAGS -std=gnu99 -Werror-implicit-function-declaration -Werror=format-security"

CFLAGS="$CFLAGS $GTK_CFLAGS $GLIB_CFLAGS $GDK_CFLAGS $GMODULE_CFLAGS $GTHREAD_CFLAGS $LIBGLADE_CFLAGS $LIBGPOD_CFLAGS $LIBANJUTA_CFLAGS $LIBGDL_CFLAGS $LIBXML_CFLAGS $ID3TAG_CFLAGS $GTK_CLEANLINESS_FLAGS"
LIBS="$LIBS $GTK_LIBS $GLIB_LIBS $GDK_LIBS $GMODULE_LIBS $GTHREAD_LIBS $LIBGLADE_LIBS $LIBGPOD_LIBS $LIBANJUTA_LIBS $LIBGDL_LIBS $LIBXML_LIBS $ID3TAG_LIBS"

dnl we need 'flex'
AM_PROG_LEX
case "$LEX" in
flex|*/flex|lex|*/lex)
	;;
*)
        AC_MSG_FAILURE([*** flex or lex required])
	;;
esac

dnl Retrieve the path of mount and umount binaries
AC_PATH_PROG(MOUNT, mount)
AC_SUBST(MOUNT)
AC_PATH_PROG(UMOUNT, umount)
AC_SUBST(UMOUNT)


dnl Add the languages which your application supports here.
ALL_LINGUAS="ca cs_CZ de es fr he it ja nl pt_BR ro ru sv zh_CN zh_TW"
AM_GLIB_GNU_GETTEXT


dnl Check if flock() is available (seems to be missing on some solaris versions)
AC_CHECK_FUNCS(flock)


dnl Check if statvfs() is available (otherwise we fall back on 'df' to determine
dnl free space on the iPod
AC_CHECK_FUNCS(statvfs)

dnl Add clutter gtk if installed
dnl -------------------------------------------------------------
if test "x$have_clutter_gtk" = "xyes"; then		
	AC_DEFINE(HAVE_CLUTTER_GTK, 1, [Define if you have clutter gtk support])
    dnl Need to add to CFLAGS and LIBS so main can init correctly
    CFLAGS="$CFLAGS $CLUTTER_GTK_CFLAGS"
    LIBS="$LIBS $CLUTTER_GTK_LIBS"
fi
AM_CONDITIONAL(HAVE_CLUTTER_GTK, [test "x$have_clutter_gtk" = "xyes"])

dnl Add gstreamer if installed
dnl -------------------------------------------------------------
if test "x$have_gstreamer" = "xyes"; then
	AC_DEFINE(HAVE_GSTREAMER, 1, [Define if you have gstreamer support])
	dnl Need to add to CFLAGS and LIBS so main can init correctly
    CFLAGS="$CFLAGS $GSTREAMER_CFLAGS"
    LIBS="$LIBS $GSTREAMER_LIBS"
fi

dnl Add brasero if installed
dnl -------------------------------------------------------------
if test "x$have_brasero" = "xyes"; then
	AC_DEFINE(HAVE_BRASERO, 1, [Define if you have brasero support])
	dnl Need to add to CFLAGS and LIBS so main can init correctly
    CFLAGS="$CFLAGS $BRASERO_CFLAGS"
    LIBS="$LIBS $BRASERO_LIBS"
fi

dnl Check for libcurl
dnl -------------------------------------------------------------
AC_ARG_WITH(curl, AS_HELP_STRING([--without-curl], [Disable coverart download support]), [], [with_curl=yes])
if test "x$with_curl" != "xno" -a "x$have_curl" = "xno"; then
	AC_MSG_ERROR([The curl library could not be found. Explicity disable support by rerunning with --without-curl])
fi

AM_CONDITIONAL(HAVE_CURL, test "x$have_curl" = "xyes")
if test "x$have_curl" = "xyes"; then
    AC_DEFINE(HAVE_CURL, 1, [Define if you have curl support])
    have_curl="yes -- will build with coverart download support"
else
    have_curl="*no -- will build without coverart download support"
fi


dnl Checks for coverweb plugin
dnl -------------------------------------------------------------
have_coverweb="no"

AC_ARG_ENABLE(plugin-coverweb,
	AS_HELP_STRING([--disable-plugin-coverweb],[Disable coverweb plugin for finding coverart in gtkpod]), 
	[ if test "$enableval" = "no"; then
		user_disabled_coverweb=1
	  fi ],
	[ user_disabled_coverweb=0 ])
AC_MSG_CHECKING(if coverweb plugin is disabled)
if test "$user_disabled_coverweb" = 1; then
	AC_MSG_RESULT(yes)	
else
	AC_MSG_RESULT(no)
	AC_MSG_CHECKING(coverweb plugin dependencies)
	if test "x$have_webkit" = "xyes"; then
		have_coverweb="yes"
		AC_MSG_RESULT(yes)
	else
		AC_MSG_RESULT(no)
	fi
fi
AM_CONDITIONAL(HAVE_PLUGIN_COVERWEB, [test "x$have_coverweb" = "xyes"])
if test "x$have_coverweb" = "xyes"; then
    AC_DEFINE(HAVE_PLUGIN_COVERWEB, 1, [Define if you have coverweb support])
fi


dnl Checks for clarity plugin
dnl -------------------------------------------------------------
have_clarity="no"

AC_ARG_ENABLE(plugin-clarity,
	AS_HELP_STRING([--disable-plugin-clarity],[Disable clarity plugin for displaying coverart in gtkpod]), 
	[ if test "$enableval" = "no"; then
		user_disabled_clarity=1
	  fi ],
	[ user_disabled_clarity=0 ])
AC_MSG_CHECKING(if clarity plugin is disabled)
if test "$user_disabled_clarity" = 1; then
	AC_MSG_RESULT(yes)
else
	AC_MSG_RESULT(no)
	AC_MSG_CHECKING(clarity plugin dependencies)
	if test "x$have_clutter_gtk" = "xyes"; then
		have_clarity="yes"
		AC_MSG_RESULT(yes)
	else
		AC_MSG_RESULT(no)
	fi
fi
AM_CONDITIONAL(HAVE_PLUGIN_CLARITY, [test "x$have_clarity" = "xyes"])	

dnl Checks for media player plugin
dnl -------------------------------------------------------------
have_media_player="no"

AC_ARG_ENABLE(plugin-media-player,
	AS_HELP_STRING([--disable-plugin-media-player],[Disable media player plugin for playing tracks in gtkpod]), 
	[ if test "$enableval" = "no"; then
		user_disabled_media_player=1
	  fi ],
	[ user_disabled_media_player=0 ])
AC_MSG_CHECKING(if media player plugin is disabled)
if test "$user_disabled_media_player" = 1; then
	AC_MSG_RESULT(yes)
else
	AC_MSG_RESULT(no)
	AC_MSG_CHECKING(media player plugin dependencies)
	if test "x$have_gstreamer" = "xyes"; then
		have_media_player="yes"
		AC_MSG_RESULT(yes)
	else
		AC_MSG_RESULT(no)
	fi
fi
AM_CONDITIONAL(HAVE_PLUGIN_MEDIA_PLAYER, [test "x$have_media_player" = "xyes"])
if test "x$have_media_player" = "xyes"; then
    AC_DEFINE(HAVE_PLUGIN_MEDIA_PLAYER, 1, [Define if you have media player support])
fi

dnl Checks for filetype_ogg plugin
dnl -------------------------------------------------------------
AC_ARG_WITH(ogg, AS_HELP_STRING([--without-ogg], [Disable Ogg/Vorbis support]), [], [with_ogg=yes])
if test "x$with_ogg" != "xno" -a "x$have_vorbis" = "xno"; then
		AC_MSG_ERROR([Ogg/Vorbis libraries could not be found. Explicitly disable support by rerunning with --without-ogg])
fi

AM_CONDITIONAL(HAVE_PLUGIN_FILETYPE_OGG, test "x$have_vorbis" = "xyes")
if test "x$have_vorbis" = "xyes"; then
    AC_DEFINE(HAVE_PLUGIN_FILETYPE_OGG, 1, [Define if you have the ogg/vorbis support])
fi


dnl Checks for filetype_flac plugin
dnl -------------------------------------------------------------
AC_ARG_WITH(flac, AS_HELP_STRING([--without-flac], [Disable FLAC support]), [], [with_flac=yes])
if test "x$with_flac" != "xno" -a "x$have_flac" = "xno"; then
		AC_MSG_ERROR([The FLAC library could not be found. Explicitly disable support by rerunning with --without-flac])
fi
AM_CONDITIONAL(HAVE_PLUGIN_FILETYPE_FLAC, test "x$have_flac" = "xyes")
if test "x$have_flac" = "xyes"; then
    AC_DEFINE(HAVE_PLUGIN_FILETYPE_FLAC, 1, [Define if you have flac support])
fi


dnl Checks for MP4 and M4A plugins
dnl -------------------------------------------------------------
AC_ARG_WITH(mp4, AS_HELP_STRING([--without-mp4],[Disable mp4/m4a support]))
have_mp4="yes"
if test "xwith_mp4" = "xno"; then
	have_mp4="no"
fi
AM_CONDITIONAL(HAVE_PLUGIN_FILETYPE_MP4, test "x$have_mp4" = "xyes")
if test "x$have_mp4" = "xyes"; then
    AC_DEFINE(HAVE_PLUGIN_FILETYPE_MP4, 1, [Define if you have mp4 support])
fi

dnl Check for faad for use filetype_m4a plugin
dnl -------------------------------------------------------------
have_faad="no"

AC_CHECK_PROG(FAAD, faad, yes, no)
if test "x$FAAD" = "xyes"; then
	have_faad="yes"
else
    AC_MSG_WARN(["Cannot find faad. Conversion from m4a to mp3 not possible"])
fi

dnl Set up variables for M4A plugin
dnl -------------------------------------------------------------

if test "x$have_mp4" = "xyes" -a "x$have_faad" = "xyes"; then
	have_m4a="yes"
else
	have_m4a="no"
fi
AM_CONDITIONAL(HAVE_PLUGIN_FILETYPE_M4A, test "x$have_m4a" = "xyes")


dnl Checks for sjcd plugin
dnl -------------------------------------------------------------
have_sjcd="no"

AC_ARG_ENABLE(plugin-sjcd,
	AS_HELP_STRING([--disable-plugin-sjcd],[Disable sound juicer plugin for extracting tracks from cd]), 
	[ if test "$enableval" = "no"; then
		user_disabled_sjcd=1
	  fi ],
	[ user_disabled_sjcd=0 ])
AC_MSG_CHECKING(if sjcd plugin is disabled)
if test "$user_disabled_sjcd" = 1; then
	AC_MSG_RESULT(yes)
else
	AC_MSG_RESULT(no)
	AC_MSG_CHECKING(sjcd plugin dependencies)
	if test "x$have_gstreamer" = "xyes" -a "x$have_brasero" = "xyes" -a "x$have_mb5" = "xyes" -a "x$have_discid" = "xyes"; then

		dnl Used within code for determining whether musicbrainz5 gtype is available
		AC_DEFINE([HAVE_MUSICBRAINZ5], 1, [Whether libmusicbrainz5 is available])
  
		# ISO-CODES
		PKG_CHECK_MODULES(ISO_CODES, [iso-codes])
		if $PKG_CONFIG --variable=domains iso-codes | grep 3166 >/dev/null ; then
			AC_DEFINE_UNQUOTED([ISO_CODES_PREFIX],["`$PKG_CONFIG --variable=prefix iso-codes`"],[ISO codes prefix])
		else
			AC_MSG_ERROR([iso-codes database does not support iso3166 country codes])
		fi

		AC_MSG_RESULT(yes)
		have_sjcd="yes"
		
		dnl Checks for components of gstreamer
		GST_ELEMENT_CHECK(vorbisenc,1.0,,AC_MSG_WARN([The 'vorbisenc' element was not found. This will cause encoding to Ogg Vorbis to fail.]))
		GST_ELEMENT_CHECK(flacenc,1.0,,AC_MSG_WARN([The 'flacenc' element was not found. This will cause encoding to FLAC to fail.]))
		GST_ELEMENT_CHECK(wavenc,1.0,,AC_MSG_WARN([The 'wavenc' element was not found. This will cause encoding to Wave to fail.]))
		GST_ELEMENT_CHECK(giosink,1.0,,AC_MSG_WARN([The 'giosink' element was not found. This will cause Sound Juicer to fail at runtime.]))
	else
		AC_MSG_RESULT(no)
	fi
fi

AM_CONDITIONAL(HAVE_PLUGIN_SJCD, [test "x$have_sjcd" = "xyes"])
if test "x$have_sjcd" = "xyes"; then
    AC_DEFINE(HAVE_PLUGIN_SJCD, 1, [Define if you have sound juicer support])
fi


# Check that we have endian.h
AC_CHECK_HEADERS(endian.h, have_endian=yes, have_endian=no)
if test "x$have_endian" = "xno"; then
	AC_CHECK_HEADERS(sys/endian.h, have_endian=yes, have_endian=no)
fi
if test "x$have_endian" = "xyes"; then
	AC_DEFINE(HAVE_ENDIAN_H, 1, [Define if you have the endian header])
fi


dnl Additional libs maybe needed when compiling under solaris
AC_SEARCH_LIBS(bind, ["socket" "nsl" "socket -lnsl"])


dnl Check for Linux-specific headers (so we can compile Linux-specific
dnl stuff only when compiling under Linux)
AC_CHECK_HEADERS(linux/cdrom.h scsi/sg.h scsi/scsi.h scsi/scsi_ioctl.h)


libgpod_version=`$PKG_CONFIG libgpod-1.0 --modversion`
AC_DEFINE_UNQUOTED(LIBGPOD_VERSION, "${libgpod_version}", [Set this to the libgpod version])


dnl Setup Atomic Parsley library flags
dnl --------------------------
LIBATOMIC_CFLAGS="$ATOMIC_CFLAGS -Wall"
LIBATOMIC_CFLAGS="$ATOMIC_CFLAGS $GTK_CFLAGS $GLIB_CFLAGS $GDK_CFLAGS $GMODULE_CFLAGS $GTHREAD_CFLAGS $LIBGPOD_CFLAGS $GTK_CLEANLINESS_FLAGS"
AC_SUBST(LIBATOMIC_CFLAGS)

dnl Setup Gtkpod flags for building library
dnl --------------------------
LIBGTKPOD_CFLAGS="$CFLAGS"
LIBGTKPOD_LIBS="$LIBS"
AC_SUBST(LIBGTKPOD_CFLAGS)
AC_SUBST(LIBGTKPOD_LIBS)

dnl Setup Gtkpod flags for building platform
dnl --------------------------
LIBGTKPOD_STATIC='$(top_builddir)/libgtkpod/libgtkpod.la'
GTKPOD_LIBS="$LIBGTKPOD_STATIC"
GTKPOD_CFLAGS="$CFLAGS"
AC_SUBST(GTKPOD_CFLAGS)
AC_SUBST(GTKPOD_LIBS)

LDFLAGS="$LDFLAGS -no-undefined -Wl,--as-needed"
GTKPOD_PLUGIN_LDFLAGS="$LDFLAGS -module -avoid-version"
AC_SUBST(GTKPOD_PLUGIN_LDFLAGS)


dnl Setup Plugin directories
dnl ------------------------
gtkpod_plugin_dir='$(libdir)/$(PACKAGE)'
gtkpod_data_dir='$(datadir)/$(PACKAGE)/data'
gtkpod_doc_dir='$(datadir)/$(PACKAGE)/doc'
gtkpod_ui_dir='$(gtkpod_data_dir)/ui'
gtkpod_glade_dir='$(gtkpod_data_dir)/glade'
gtkpod_image_dir='$(datadir)/$(PACKAGE)/icons'
gtkpod_script_dir='$(datadir)/$(PACKAGE)/scripts'
AC_SUBST(gtkpod_plugin_dir)
AC_SUBST(gtkpod_data_dir)
AC_SUBST(gtkpod_doc_dir)
AC_SUBST(gtkpod_ui_dir)
AC_SUBST(gtkpod_glade_dir)
AC_SUBST(gtkpod_image_dir)
AC_SUBST(gtkpod_script_dir)

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_CONFIG_FILES([
Makefile
libgtkpod-1.1.0.pc
src/Makefile
po/Makefile.in
scripts/Makefile
data/Makefile
icons/Makefile
icons/16x16/Makefile
icons/22x22/Makefile
icons/24x24/Makefile
icons/32x32/Makefile
icons/48x48/Makefile
icons/64x64/Makefile
icons/scalable/Makefile
icons/hicolor/Makefile
icons/hicolor/64x64/Makefile
data/man/Makefile
data/man/gtkpod.1
data/glade/Makefile
data/ui/Makefile
doc/Makefile
doc/figures/Makefile
libgtkpod/Makefile
libs/Makefile
libs/atomic-parsley/Makefile
plugins/Makefile
plugins/filetype_mp3/Makefile
plugins/filetype_ogg/Makefile
plugins/filetype_flac/Makefile
plugins/filetype_wav/Makefile
plugins/filetype_m4a/Makefile
plugins/filetype_mp4/Makefile
plugins/filetype_video/Makefile
plugins/exporter/Makefile
plugins/playlist_display/Makefile
plugins/playlist_display/icons/Makefile
plugins/playlist_display/icons/hicolor/Makefile
plugins/playlist_display/icons/hicolor/16x16/Makefile
plugins/playlist_display/icons/hicolor/16x16/places/Makefile
plugins/playlist_display/icons/hicolor/22x22/Makefile
plugins/playlist_display/icons/hicolor/22x22/places/Makefile
plugins/playlist_display/icons/hicolor/24x24/Makefile
plugins/playlist_display/icons/hicolor/24x24/places/Makefile
plugins/playlist_display/icons/hicolor/32x32/Makefile
plugins/playlist_display/icons/hicolor/32x32/places/Makefile
plugins/playlist_display/icons/hicolor/48x48/Makefile
plugins/playlist_display/icons/hicolor/48x48/places/Makefile
plugins/playlist_display/icons/hicolor/scalable/Makefile
plugins/playlist_display/icons/hicolor/scalable/places/Makefile
plugins/repository_editor/Makefile
plugins/sorttab_display/Makefile
plugins/sorttab_display/icons/Makefile
plugins/sorttab_display/icons/hicolor/Makefile
plugins/sorttab_display/icons/hicolor/16x16/Makefile
plugins/sorttab_display/icons/hicolor/16x16/places/Makefile
plugins/sorttab_display/icons/hicolor/22x22/Makefile
plugins/sorttab_display/icons/hicolor/22x22/places/Makefile
plugins/sorttab_display/icons/hicolor/24x24/Makefile
plugins/sorttab_display/icons/hicolor/24x24/places/Makefile
plugins/sorttab_display/icons/hicolor/32x32/Makefile
plugins/sorttab_display/icons/hicolor/32x32/places/Makefile
plugins/sorttab_display/icons/hicolor/48x48/Makefile
plugins/sorttab_display/icons/hicolor/48x48/places/Makefile
plugins/track_display/Makefile
plugins/track_display/icons/Makefile
plugins/track_display/icons/hicolor/Makefile
plugins/track_display/icons/hicolor/16x16/Makefile
plugins/track_display/icons/hicolor/16x16/places/Makefile
plugins/track_display/icons/hicolor/22x22/Makefile
plugins/track_display/icons/hicolor/22x22/places/Makefile
plugins/track_display/icons/hicolor/24x24/Makefile
plugins/track_display/icons/hicolor/24x24/places/Makefile
plugins/track_display/icons/hicolor/32x32/Makefile
plugins/track_display/icons/hicolor/32x32/places/Makefile
plugins/track_display/icons/hicolor/48x48/Makefile
plugins/track_display/icons/hicolor/48x48/places/Makefile
plugins/details_editor/Makefile
plugins/core_preferences/Makefile
plugins/core_preferences/icons/Makefile
plugins/core_preferences/icons/hicolor/Makefile
plugins/core_preferences/icons/hicolor/16x16/Makefile
plugins/core_preferences/icons/hicolor/16x16/places/Makefile
plugins/core_preferences/icons/hicolor/22x22/Makefile
plugins/core_preferences/icons/hicolor/22x22/places/Makefile
plugins/core_preferences/icons/hicolor/24x24/Makefile
plugins/core_preferences/icons/hicolor/24x24/places/Makefile
plugins/core_preferences/icons/hicolor/32x32/Makefile
plugins/core_preferences/icons/hicolor/32x32/places/Makefile
plugins/core_preferences/icons/hicolor/48x48/Makefile
plugins/core_preferences/icons/hicolor/48x48/places/Makefile
plugins/info_display/Makefile
plugins/cover_display/Makefile
plugins/cover_display/icons/Makefile
plugins/cover_display/icons/hicolor/Makefile
plugins/cover_display/icons/hicolor/16x16/Makefile
plugins/cover_display/icons/hicolor/16x16/places/Makefile
plugins/cover_display/icons/hicolor/22x22/Makefile
plugins/cover_display/icons/hicolor/22x22/places/Makefile
plugins/cover_display/icons/hicolor/24x24/Makefile
plugins/cover_display/icons/hicolor/24x24/places/Makefile
plugins/cover_display/icons/hicolor/32x32/Makefile
plugins/cover_display/icons/hicolor/32x32/places/Makefile
plugins/cover_display/icons/hicolor/48x48/Makefile
plugins/cover_display/icons/hicolor/48x48/places/Makefile
plugins/cover_display/icons/hicolor/240x240/Makefile
plugins/cover_display/icons/hicolor/240x240/places/Makefile
plugins/photo_editor/Makefile
plugins/photo_editor/icons/Makefile
plugins/photo_editor/icons/hicolor/Makefile
plugins/photo_editor/icons/hicolor/16x16/Makefile
plugins/photo_editor/icons/hicolor/16x16/places/Makefile
plugins/photo_editor/icons/hicolor/22x22/Makefile
plugins/photo_editor/icons/hicolor/22x22/places/Makefile
plugins/photo_editor/icons/hicolor/24x24/Makefile
plugins/photo_editor/icons/hicolor/24x24/places/Makefile
plugins/photo_editor/icons/hicolor/32x32/Makefile
plugins/photo_editor/icons/hicolor/32x32/places/Makefile
plugins/photo_editor/icons/hicolor/48x48/Makefile
plugins/photo_editor/icons/hicolor/48x48/places/Makefile
plugins/coverweb/Makefile
plugins/coverweb/icons/Makefile
plugins/coverweb/icons/hicolor/Makefile
plugins/coverweb/icons/hicolor/16x16/Makefile
plugins/coverweb/icons/hicolor/16x16/places/Makefile
plugins/coverweb/icons/hicolor/22x22/Makefile
plugins/coverweb/icons/hicolor/22x22/places/Makefile
plugins/coverweb/icons/hicolor/24x24/Makefile
plugins/coverweb/icons/hicolor/24x24/places/Makefile
plugins/coverweb/icons/hicolor/32x32/Makefile
plugins/coverweb/icons/hicolor/32x32/places/Makefile
plugins/coverweb/icons/hicolor/48x48/Makefile
plugins/coverweb/icons/hicolor/48x48/places/Makefile
plugins/coverweb/icons/hicolor/128x128/Makefile
plugins/coverweb/icons/hicolor/128x128/places/Makefile
plugins/media_player/Makefile
plugins/media_player/icons/Makefile
plugins/media_player/icons/hicolor/Makefile
plugins/media_player/icons/hicolor/16x16/Makefile
plugins/media_player/icons/hicolor/16x16/places/Makefile
plugins/media_player/icons/hicolor/22x22/Makefile
plugins/media_player/icons/hicolor/22x22/places/Makefile
plugins/media_player/icons/hicolor/24x24/Makefile
plugins/media_player/icons/hicolor/24x24/places/Makefile
plugins/media_player/icons/hicolor/32x32/Makefile
plugins/media_player/icons/hicolor/32x32/places/Makefile
plugins/media_player/icons/hicolor/48x48/Makefile
plugins/media_player/icons/hicolor/48x48/places/Makefile
plugins/mserv/Makefile
plugins/mserv/icons/Makefile
plugins/mserv/icons/hicolor/Makefile
plugins/mserv/icons/hicolor/16x16/Makefile
plugins/mserv/icons/hicolor/16x16/places/Makefile
plugins/mserv/icons/hicolor/22x22/Makefile
plugins/mserv/icons/hicolor/22x22/places/Makefile
plugins/mserv/icons/hicolor/24x24/Makefile
plugins/mserv/icons/hicolor/24x24/places/Makefile
plugins/mserv/icons/hicolor/32x32/Makefile
plugins/mserv/icons/hicolor/32x32/places/Makefile
plugins/mserv/icons/hicolor/48x48/Makefile
plugins/mserv/icons/hicolor/48x48/places/Makefile
plugins/clarity/Makefile
plugins/clarity/icons/Makefile
plugins/clarity/icons/hicolor/Makefile
plugins/clarity/icons/hicolor/16x16/Makefile
plugins/clarity/icons/hicolor/16x16/places/Makefile
plugins/clarity/icons/hicolor/22x22/Makefile
plugins/clarity/icons/hicolor/22x22/places/Makefile
plugins/clarity/icons/hicolor/24x24/Makefile
plugins/clarity/icons/hicolor/24x24/places/Makefile
plugins/clarity/icons/hicolor/32x32/Makefile
plugins/clarity/icons/hicolor/32x32/places/Makefile
plugins/clarity/icons/hicolor/48x48/Makefile
plugins/clarity/icons/hicolor/48x48/places/Makefile
plugins/clarity/icons/hicolor/240x240/Makefile
plugins/clarity/icons/hicolor/240x240/places/Makefile
plugins/sjcd/Makefile
plugins/sjcd/data/Makefile
plugins/sjcd/libjuicer/Makefile
plugins/sjcd/icons/Makefile
plugins/sjcd/icons/hicolor/Makefile
plugins/sjcd/icons/hicolor/16x16/Makefile
plugins/sjcd/icons/hicolor/16x16/places/Makefile
plugins/sjcd/icons/hicolor/22x22/Makefile
plugins/sjcd/icons/hicolor/22x22/places/Makefile
plugins/sjcd/icons/hicolor/24x24/Makefile
plugins/sjcd/icons/hicolor/24x24/places/Makefile
plugins/sjcd/icons/hicolor/32x32/Makefile
plugins/sjcd/icons/hicolor/32x32/places/Makefile
plugins/sjcd/icons/hicolor/48x48/Makefile
plugins/sjcd/icons/hicolor/48x48/places/Makefile
plugins/sjcd/icons/hicolor/scalable/Makefile
plugins/sjcd/icons/hicolor/scalable/places/Makefile
plugins/external_player/Makefile
])
AC_OUTPUT

echo "
Core Configuration for $PACKAGE $VERSION :
--------------------------------

 Host System Type .....: $host
 Install path .........: $prefix
 GTK3 version .........: `$PKG_CONFIG gtk+-3.0 --modversion`
 GLib2/GThread version : `$PKG_CONFIG gthread-2.0 --modversion`
 libgpod version ......: $libgpod_version
 Preprocessor .........: $CC $CPPFLAGS
 Compiler .............: $CC $CFLAGS
 Linker ...............: $CC $LDFLAGS $LIBS
"

echo "
Plugin Configuration :
---------------------------------"
echo " CoverWeb Browser           .....: $have_coverweb"
echo " Media Player               .....: $have_media_player"
echo " MP4 File Type              .....: $have_mp4"
echo " M4A File Type              .....: $have_m4a"
echo " Flac File Type             .....: $have_flac"
echo " Ogg File Type              .....: $have_vorbis"
echo " Clarity Display Widget     .....: $have_clarity"
echo " Sound Juicer Widget        .....: $have_sjcd"

echo " Support for cover download .....: $have_curl"

echo "
 Now type 'make' to build $PACKAGE $VERSION,
 and then 'make install' for installation.
"
