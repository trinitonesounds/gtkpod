/* Time-stamp: <2009-08-01 17:46:51 jcs>
 |
 |  Copyright (C) 2002-2005 Jorg Schuler <jcsjcs at users sourceforge net>
 |  Part of the gtkpod project.
 |
 |  URL: http://www.gtkpod.org/
 |  URL: http://gtkpod.sourceforge.net/
 |
 |  This program is free software; you can redistribute it and/or modify
 |  it under the terms of the GNU General Public License as published by
 |  the Free Software Foundation; either version 2 of the License, or
 |  (at your option) any later version.
 |
 |  This program is distributed in the hope that it will be useful,
 |  but WITHOUT ANY WARRANTY; without even the implied warranty of
 |  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 |  GNU General Public License for more details.
 |
 |  You should have received a copy of the GNU General Public License
 |  along with this program; if not, write to the Free Software
 |  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 |
 |  iTunes and iPod are trademarks of Apple
 |
 |  This product is not supported/written/published by Apple!
 |
 |  $Id$
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

/* ------------------------------------------------------------

 Info on how to implement new file formats.

 You need to supply a function

 Track *xxx_get_file_info (gchar *filename)

 that returns a new Track structure with as many of the following
 fields filled in (in UTF8):

 gchar   *album;            /+ album (utf8)          +/
 gchar   *artist;           /+ artist (utf8)         +/
 gchar   *title;            /+ title (utf8)          +/
 gchar   *genre;            /+ genre (utf8)          +/
 gchar   *comment;          /+ comment (utf8)        +/
 gchar   *composer;         /+ Composer (utf8)       +/
 gchar   *filetype;         /+ Format description (utf8)   +/
 gchar   *charset;          /+ charset used for tags       +/
 gchar   *description;      /+ Description text (podcasts) +/
 gchar   *podcasturl;       /+ URL/Title (podcasts)        +/
 gchar   *podcastrss;       /+ Podcast RSS                 +/
 gchar   *subtitle;         /+ Subtitle (podcasts)         +/
 guint32 time_released;     /+ For podcasts: release date as
 displayed next to the title in the
 Podcast playlist  +/
 gint32  cd_nr;             /+ CD number             +/
 gint32  cds;               /+ number of CDs         +/
 gint32  track_nr;          /+ track number          +/
 gint32  tracks;            /+ number of tracks      +/
 gint32  year;              /+ year                  +/
 gint32  tracklen;          /+ Length of track in ms +/
 gint32  bitrate;           /+ bitrate in kbps       +/
 guint16 samplerate;        /+ e.g.: CD is 44100     +/
 guint32 peak_signal;	      /+ LAME Peak Signal * 0x800000         +/
 gboolean compilation;      /+ Track is part of a compilation CD   +/
 gboolean lyrics_flag;
 gint16 bpm;

 If prefs_get_int("readtags") returns FALSE you only should fill in
 tracklen, bitrate, samplerate, soundcheck and filetype

 If prefs_get_int("coverart_apic") returns TRUE you should try to
 read APIC coverart data from the tags and set it with
 gp_set_thumbnails_from_data().

 Please note that the iPod will only play as much of the track as
 specified in "tracklen".

 You don't have to fill in the value for charset if you use the
 default charset (i.e. you use charset_to_utf8() to convert to
 UTF8). Otherwise please specify the charset used.

 When an error occurs, the function returns NULL and logs an error
 message using gtkpod_warning().

 You also have to write a function to write TAGs back to the
 file. That function should be named

 gboolean xxx_write_file_info (gchar *filename, Track *track)

 and return TRUE on success or FALSE on error. In that case it
 should log an error message using gtkpod_warning().

 Finally, you may want to provide a function that can
 read and set the soundcheck field:

 gboolean xxx_read_soundcheck (gchar *filename, Track *track)

 and return TRUE when the soundcheck value could be determined.

 ------------------------------------------------------------ */

#include <sys/types.h>
#include <sys/param.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include "libgtkpod/charset.h"
#include "libgtkpod/misc.h"
#include "libgtkpod/prefs.h"
#include "libgtkpod/gp_private.h"
#include "mp4file.h"
#include "libs/atomic-parsley/AtomicParsleyBridge.h"

static void mp4_set_media_type(const gchar *mp4FileName, Track *track) {
    gchar *value;

    value = strrchr(mp4FileName, '.');
    if (value) {
        if (g_ascii_strcasecmp(value, ".m4a") == 0) {
            track->mediatype = ITDB_MEDIATYPE_AUDIO;
            track->filetype = g_strdup(_("AAC audio file"));
        }
        else if (g_ascii_strcasecmp(value, ".m4p") == 0) {
            track->mediatype = ITDB_MEDIATYPE_AUDIO;
            track->filetype = g_strdup(_("Protected AAC audio file"));
        }
        else if (g_ascii_strcasecmp(value, ".m4b") == 0) {
            track->mediatype = ITDB_MEDIATYPE_AUDIOBOOK;
            track->filetype = g_strdup(_("AAC audio book file"));
        }
        else if (g_ascii_strcasecmp(value, ".mp4") == 0) {
            track->mediatype = ITDB_MEDIATYPE_MOVIE;
            track->movie_flag = 0x01;
            track->filetype = g_strdup(_("MP4 video file"));
        }
    }
}

Track *mp4_get_file_info(const gchar *mp4FileName, GError **error) {
    Track *track = NULL;

    g_return_val_if_fail(mp4FileName, NULL);

    track = gp_track_new();

    mp4_set_media_type(mp4FileName, track);
    AP_read_metadata(mp4FileName, track);

    return track;
}

gboolean mp4_write_file_info(const gchar *mp4FileName, Track *track, GError **error) {
    AP_write_metadata(track, mp4FileName, error);

    return error ? TRUE : FALSE;
}

gboolean mp4_read_lyrics(const gchar *mp4FileName, gchar **lyrics, GError **error) {
    char *value = AP_read_lyrics(mp4FileName, error);

    *lyrics = value;

    return error ? TRUE : FALSE;
}

gboolean mp4_write_lyrics(const gchar *mp4FileName, const gchar *lyrics, GError **error) {
    AP_write_lyrics(lyrics, mp4FileName, error);

    return error ? TRUE : FALSE;
}
