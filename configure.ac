dnl Process this file with autoconf to produce a configure script.
m4_include([./version.m4])

# Require autoconf 2.55 or higher
AC_PREREQ(2.65)

AC_INIT([gtkpod], m4_defn([VERSION_NUMBER]))
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR(src/main.c)

dnl libtool versioning
# +1 : 0 : +1  == adds new functions to the interface
# +1 : 0 : 0   == changes or removes functions (changes include both
#                 changes to the signature and the semantic)
#  ? :+1 : ?   == just internal changes
# CURRENT : REVISION : AGE
LIBGTKPOD_SO_VERSION="1:0:0"

LIBGTKPOD_VERSION=`./version.sh`

AC_SUBST(LIBGTKPOD_SO_VERSION)
AC_SUBST(LIBGTKPOD_VERSION)

AM_INIT_AUTOMAKE([1.9 tar-ustar]) 
AM_MAINTAINER_MODE
AC_CANONICAL_HOST
AC_SEARCH_LIBS([strerror],[cposix])
LT_INIT
AC_PROG_CC
AC_PROG_CC
AC_DIAGNOSE([obsolete],[AM_PROG_CC_STDC:
	your code should no longer depend upon `am_cv_prog_cc_stdc', but upon
	`ac_cv_prog_cc_stdc'.  Remove this warning and the assignment when
	you adjust the code.  You can also remove the above call to
	AC_PROG_CC if you already called it elsewhere.])
am_cv_prog_cc_stdc=$ac_cv_prog_cc_stdc


IT_PROG_INTLTOOL([0.33])

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

dnl check if pkg-config exists -- if not print a meaningful error message
AC_PATH_TOOL([PKG_CONFIG], [pkg-config])
if test -z "$PKG_CONFIG"; then
    AC_MSG_FAILURE([*** pkg-config required (http://www.freedesktop.org/software/pkgconfig)])
else
    ac_cv_env_PKG_CONFIG_set=set
fi

dnl check for required libs that are managed with pkg-config
PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.12.0,, [AC_MSG_FAILURE([*** $GTK_PKG_ERRORS])])
PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.15.0,, [AC_MSG_FAILURE([*** $GLIB_PKG_ERRORS])])
PKG_CHECK_MODULES(GMODULE, gmodule-2.0,, [AC_MSG_FAILURE([*** $GMODULE_PKG_ERRORS])])
PKG_CHECK_MODULES(GTHREAD, gthread-2.0 >= 0.14.0,, [AC_MSG_FAILURE([*** $GTHREAD_PKG_ERRORS])])
PKG_CHECK_MODULES(LIBGLADE, libglade-2.0 >= 2.4.0,, [AC_MSG_FAILURE([*** $LIBGLADE_PKG_ERRORS])])
PKG_CHECK_MODULES(LIBGPOD, libgpod-1.0 >= 0.7.0,, [AC_MSG_FAILURE([*** $LIBGPOD_PKG_ERRORS])])
PKG_CHECK_MODULES(LIBANJUTA, libanjuta-1.0 >= 2.30.0,, [AC_MSG_FAILURE([*** $LIBANJUTA_PKG_ERRORS])])
PKG_CHECK_MODULES(LIBGDL, gdl-1.0 >= 2.26.0,, [AC_MSG_FAILURE([*** $LIBGDL_PKG_ERRORS])])
PKG_CHECK_MODULES(GIO, gio-2.0 >= 2.15.0,, [AC_MSG_FAILURE([*** $GIO_PKG_ERRORS])])

dnl Check if GDK has GSEAL for GdkDragContext
PKG_CHECK_MODULES(GSEALED_GDK, gdk-2.0 >= 2.21.0, have_gsealed_gdk=yes, have_gsealed_gdk=no)

AM_CONDITIONAL(HAVE_GSEALED_GDK, test "x$have_gsealed_gdk" = "xyes")
if test "x$have_gsealed_gdk" = "xyes"; then
    AC_DEFINE(HAVE_GSEALED_GDK, 1, [Define if GDK has GSEAL])
    dnl Preparing for GTK3 transition
	GTK_CLEANLINESS_FLAGS="-DG_DISABLE_SINGLE_INCLUDES -DGDK_PIXBUF_DISABLE_SINGLE_INCLUDES -DGTK_DISABLE_SINGLE_INCLUDES"
	GTK_CLEANLINESS_FLAGS="$GTK_CLEANLINESS_FLAGS -DG_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED"
	GTK_CLEANLINESS_FLAGS="$GTK_CLEANLINESS_FLAGS -DGSEAL_ENABLE"
fi

dnl Force C99 mode (no implicit int declarations)
CFLAGS="$CFLAGS -std=gnu99 -Werror-implicit-function-declaration"

CFLAGS="$CFLAGS $GTK_CFLAGS $GLIB_CFLAGS $GMODULE_CFLAGS $GTHREAD_CFLAGS $LIBGLADE_CFLAGS $LIBGPOD_CFLAGS $LIBANJUTA_CFLAGS $LIBGDL_CFLAGS $GTK_CLEANLINESS_FLAGS"
LIBS="$LIBS $GTK_LIBS $GLIB_LIBS $GMODULE_LIBS $GTHREAD_LIBS $LIBGLADE_LIBS $LIBGPOD_LIBS $LIBANJUTA_LIBS $LIBGDL_LIBS"

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

GTKPOD_PLUGIN_LDFLAGS="-module -avoid-version -no-undefined"
AC_SUBST(GTKPOD_PLUGIN_LDFLAGS)

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
ALL_LINGUAS="ca de es fr he it ja ru ro sv zh_CN"
AM_GLIB_GNU_GETTEXT

dnl Check if flock() is available (seems to be missing on some solaris versions)
AC_CHECK_FUNCS(flock)

dnl Check if statvfs() is available (otherwise we fall back on 'df' to determine
dnl free space on the iPod
AC_CHECK_FUNCS(statvfs)

dnl Check for libid3tag
AC_SEARCH_LIBS([id3_frame_field], ["id3tag" "id3tag -lz"],,
    AC_MSG_ERROR([*** id3tag >= 0.15 lib not found (0.14 will not work!)]))

dnl Check for libcurl
AC_ARG_WITH(curl, AS_HELP_STRING([--without-curl],[Disable coverart download support]))
if test "x$with_curl" != "xno"; then
    PKG_CHECK_MODULES(CURL, [libcurl >= 7.10.0], have_curl=yes, have_curl=no)
    if test "x$with_curl" = "xyes" -a "x$have_curl" = "xno"; then
        AC_MSG_ERROR([curl support explicitly requested but curl couldn't be found])
    fi
fi
AM_CONDITIONAL(HAVE_CURL, test "x$have_curl" = "xyes")
if test "x$have_curl" = "xyes"; then
    AC_DEFINE(HAVE_CURL, 1, [Define if you have curl support])
    have_curl="yes -- will build with coverart download support"
    CPPFLAGS="$CPPFLAGS $CURL_CFLAGS"
    CFLAGS="$CFLAGS $CURL_CFLAGS"
    LIBS="$LIBS $CURL_LIBS"
else
    have_curl="*no -- will build without coverart download support"
fi

dnl Check for webkit
dnl -------------------------------------------------------------

AC_ARG_ENABLE(plugin-coverweb,
	AS_HELP_STRING([--disable-plugin-coverweb],[Disable coverweb plugin for finding coverart in gtkpod]), 
	[ if test "$enableval" = "no"; then
		user_disabled_coverweb=1
	  fi ],
	[ user_disabled_coverweb=0 ])
AC_MSG_CHECKING(if coverweb plugin is disabled)
if test "$user_disabled_coverweb" = 1; then
	AC_MSG_RESULT(yes)
	coverweb="no"
else
	AC_MSG_RESULT(no)
	
	PKG_CHECK_MODULES(WEBKIT,
		[webkit-1.0 >= 1.1],
		[coverweb="yes"],
		[coverweb="no"])
fi

AM_CONDITIONAL(HAVE_PLUGIN_COVERWEB, [test "x$coverweb" = "xyes"])

dnl Check for gstreamer
dnl -------------------------------------------------------------

AC_ARG_ENABLE(plugin-media-player,
	AS_HELP_STRING([--disable-plugin-media-player],[Disable media player plugin for playing tracks in gtkpod]), 
	[ if test "$enableval" = "no"; then
		user_disabled_media_player=1
	  fi ],
	[ user_disabled_media_player=0 ])
AC_MSG_CHECKING(if media player plugin is disabled)
if test "$user_disabled_media_player" = 1; then
	AC_MSG_RESULT(yes)
	media_player="no"
else
	AC_MSG_RESULT(no)
	
	PKG_CHECK_MODULES(GSTREAMER,
		[gstreamer-0.10 >= 0.10.25],
		[media_player="yes"],
		[media_player="no"])
	
	PKG_CHECK_MODULES(GSTREAMER_INTERFACES,
		[gstreamer-interfaces-0.10 >= 0.10.25],
		[media_player="yes"],
		[media_player="no"])
fi

AM_CONDITIONAL(HAVE_PLUGIN_MEDIA_PLAYER, [test "x$media_player" = "xyes"])

dnl Check for libvorbisfile and libvorbis
AC_ARG_WITH(ogg, AS_HELP_STRING([--without-ogg],[Disable Ogg/Vorbis support]))
if test "x$with_ogg" != "xno"; then
    AC_SEARCH_LIBS(ov_open, ["vorbisfile"],
        [AC_CHECK_HEADER(vorbis/vorbisfile.h, have_ogg=yes, have_ogg_header=no)], have_ogg=no)
    if test "x$have_ogg_header" = "xno"; then
        have_ogg="no"
        AC_MSG_WARN([vorbis/vorbisfile.h cannot be found or is unusable. Check your ogg/vorbis installation.])
    fi
    if test "x$have_ogg" = "xyes"; then
        AC_SEARCH_LIBS(vorbis_comment_query, ["vorbis"],
            [AC_CHECK_HEADER(vorbis/codec.h, have_ogg=yes, have_ogg_header=no)], have_ogg=no)
        if test "x$have_ogg_header" = "xno"; then
            have_ogg="no"
            AC_MSG_WARN([vorbis/codec.h cannot be found or is unusable. Check your ogg/vorbis installation.])
        fi
    fi
    if test "x$with_ogg" = "xyes" -a "x$have_ogg" = "xno"; then
        AC_MSG_ERROR([Ogg/Vorbis support explicitly requested but ogg/vorbis libs couldn't be found])
    fi
fi

if test "x$have_ogg" = "xyes"; then
    AC_DEFINE(HAVE_LIBVORBISFILE, 1, [Define if you have the ogg/vorbis library])
fi
AM_CONDITIONAL(HAVE_LIBVORBISFILE, test "x$have_ogg" = "xyes")

dnl Check for FLAC
AC_ARG_WITH(flac, AS_HELP_STRING([--without-flac],[Disable FLAC support]))
if test "x$with_flac" != "xno"; then
    AC_SEARCH_LIBS(FLAC__metadata_get_streaminfo, ["FLAC"],
        [AC_CHECK_HEADER(FLAC/metadata.h, have_flac=yes, have_flac_header=no)], have_flac=no)
    if test "x$have_flac_header" = "xno"; then
        have_flac="no"
        AC_MSG_WARN([FLAC/metadata.h cannot be found or is unusable. Check your FLACinstallation.])
    fi
    if test "xwith_flac" = "xyes" -a "x$have_flac" = "xno"; then
        AC_MSG_ERROR([FLAC support explicitly requested but flac libs couldn't be found])
    fi
fi

if test "x$have_flac" = "xyes"; then
    AC_DEFINE(HAVE_FLAC, 1, [Define if you have the flac library])
fi
AM_CONDITIONAL(HAVE_FLAC, test "x$have_flac" = "xyes")

dnl Check for MP4 Headers
AC_CHECK_HEADERS([mp4v2/platform.h], have_mp4=yes, have_mp4=no)
AC_CHECK_HEADERS([mp4v2/itmf_tags.h], [], [],
		 [#ifdef HAVE_MP4V2_PLATFORM_H
		  # include <mp4v2/platform.h>
		  #endif
		  typedef void* MP4FileHandle;
		 ])
AM_CONDITIONAL(HAVE_MP4, test "x$have_mp4" = "xyes")

dnl Additional libs maybe needed when compiling under solaris
AC_SEARCH_LIBS(bind, ["socket" "nsl" "socket -lnsl"])

dnl Check for Linux-specific headers (so we can compile Linux-specific
dnl stuff only when compiling under Linux)
AC_CHECK_HEADERS(linux/cdrom.h scsi/sg.h scsi/scsi.h scsi/scsi_ioctl.h)

libgpod_version=`$PKG_CONFIG libgpod-1.0 --modversion`
AC_DEFINE_UNQUOTED(LIBGPOD_VERSION, "${libgpod_version}", [Set this to the libgpod version])

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
libgtkpod-1.0.pc
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
])
AC_OUTPUT

echo "
Core Configuration for $PACKAGE $VERSION :
--------------------------------

 Host System Type .....: $host
 Install path .........: $prefix
 GTK2 version .........: `$PKG_CONFIG gtk+-2.0 --modversion`
 GLib2/GThread version : `$PKG_CONFIG gthread-2.0 --modversion`
 libgpod version ......: $libgpod_version
 Preprocessor .........: $CC $CPPFLAGS
 Compiler .............: $CC $CFLAGS
 Linker ...............: $CC $LDFLAGS $LIBS
"

echo "
Plugin Configuration :
---------------------------------"
echo " CoverWeb Browser           .....: $coverweb"
echo " Media Player               .....: $media_player"
echo " MP4 File Type              .....: $have_mp4"
echo " M4A File Type              .....: $have_mp4"
echo " Flac File Type             .....: $have_flac"
echo " Ogg File Type              .....: $have_ogg"

echo " Support for cover download .....: $have_curl"

echo "
 Now type 'make' to build $PACKAGE $VERSION,
 and then 'make install' for installation.
"
