/*
 |  Copyright (C) 2007 Jorg Schuler <jcsjcs at users sourceforge net>
 |  Part of the gtkpod project.
 |
 |  URL: http://www.gtkpod.org/
 |  URL: http://gtkpod.sourceforge.net/
 |
 |  Gtkpod is free software; you can redistribute it and/or modify
 |  it under the terms of the GNU General Public License as published by
 |  the Free Software Foundation; either version 2 of the License, or
 |  (at your option) any later version.
 |
 |  Gtkpod is distributed in the hope that it will be useful,
 |  but WITHOUT ANY WARRANTY; without even the implied warranty of
 |  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 |  GNU General Public License for more details.
 |
 |  You should have received a copy of the GNU General Public License
 |  along with gtkpod; if not, write to the Free Software
 |  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
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

#include "libgtkpod/charset.h"
#include "libgtkpod/gp_itdb.h"
#include "libgtkpod/prefs.h"
#include "plugin.h"
#include "m4afile.h"
#include "libs/atomic-parsley/AtomicParsleyBridge.h"

/* Info on how to implement new file formats: see mp3file.c for more info */

static void m4a_set_media_type(const gchar *m4aFileName, Track *track) {
    gchar *value;

    value = strrchr(m4aFileName, '.');
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
        else if (g_ascii_strcasecmp(value, ".m4a") == 0) {
            track->mediatype = ITDB_MEDIATYPE_MOVIE;
            track->movie_flag = 0x01;
            track->filetype = g_strdup(_("MP4 video file"));
        }
    }
}

Track *m4a_get_file_info(const gchar *m4aFileName, GError **error) {
    Track *track = NULL;

    g_return_val_if_fail(m4aFileName, NULL);
    g_message("Track Name: %s", m4aFileName);

    track = gp_track_new();

    m4a_set_media_type(m4aFileName, track);

    AP_extract_metadata(m4aFileName, track);

    return track;
}

gboolean m4a_write_file_info(const gchar *filename, Track *track, GError **error) {
    AP_write_metadata(track, filename, error);

    return error ? TRUE : FALSE;
}

gboolean m4a_read_lyrics(const gchar *m4aFileName, gchar **lyrics, GError **error) {
    gchar *value = AP_read_lyrics(m4aFileName, error);
    *lyrics = value;
    return error ? TRUE : FALSE;
}

gboolean m4a_write_lyrics(const gchar *m4aFileName, const gchar *lyrics, GError **error) {
    AP_write_lyrics(lyrics, m4aFileName, error);

    return error ? TRUE : FALSE;
}

gboolean m4a_read_gapless(const gchar *filename, Track *track, GError **error) {
    return FALSE;
}

gboolean m4a_can_convert() {
    gchar *cmd = m4a_get_conversion_cmd();
    /*
     * Return TRUE if
     * Command exists and fully formed
     * Target format is NOT set to AAC
     * convert_m4a preference is set to TRUE
     */
    return cmd && cmd[0] && (prefs_get_int("conversion_target_format") != TARGET_FORMAT_AAC)
            && prefs_get_int("convert_m4a");
}

gchar *m4a_get_conversion_cmd() {
    /* Convert an m4a to an mp3 */
    return prefs_get_string("path_conv_mp3");
}

gchar *m4a_get_gain_cmd() {
    return prefs_get_string("path_aacgain");
}
