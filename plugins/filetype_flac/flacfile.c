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
#include "libgtkpod/gp_private.h"
#include "plugin.h"
#include "flacfile.h"

/* Info on how to implement new file formats: see mp3file.c for more info */

#include <sys/types.h>
#include <sys/param.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <FLAC/metadata.h>

Track *flac_get_file_info(const gchar *flacFileName, GError **error) {
    Track *track = NULL;
    FLAC__StreamMetadata stream_data;
    FLAC__StreamMetadata *tags;

    if (!FLAC__metadata_get_streaminfo(flacFileName, &stream_data)) {
        gchar *filename = NULL;
        filename = charset_to_utf8(flacFileName);

        gtkpod_log_error(error, g_strdup_printf(_("'%s' does not appear to be an FLAC audio file.\n"), filename));
        g_free(filename);
    }
    else {
        gboolean flac_metadata_ok = FALSE;

        track = gp_track_new();
        track->mediatype = ITDB_MEDIATYPE_AUDIO;

        if (prefs_get_int("readtags")) {
            if (!FLAC__metadata_get_tags(flacFileName, &tags)) {
                gchar *filename = NULL;
                filename = charset_to_utf8(flacFileName);
                gtkpod_log_error(error, g_strdup_printf(_("Error retrieving tags for '%s'.\n"), filename));
                g_free(filename);
            }
            else {
                gint i;

                if (tags->data.vorbis_comment.num_comments > 0) {
                    flac_metadata_ok = TRUE;
                }

                for (i = 0; i < tags->data.vorbis_comment.num_comments; i++) {
                    gchar *tag = (gchar*) tags->data.vorbis_comment.comments[i].entry;

                    if (g_ascii_strncasecmp("ARTIST=", tag, 7) == 0) {
                        track->artist = charset_to_utf8(tag + 7);
                    }
                    if (g_ascii_strncasecmp("ALBUM=", tag, 6) == 0) {
                        track->album = charset_to_utf8(tag + 6);
                    }
                    if (g_ascii_strncasecmp("TITLE=", tag, 6) == 0) {
                        track->title = charset_to_utf8(tag + 6);
                    }
                    if (g_ascii_strncasecmp("GENRE=", tag, 6) == 0) {
                        track->genre = charset_to_utf8(tag + 6);
                    }
                    if (g_ascii_strncasecmp("YEAR=", tag, 5) == 0) {
                        track->year = atoi(tag + 5);
                    }
                    if (g_ascii_strncasecmp("DATE=", tag, 5) == 0) {
                        /* The date field is supposed to be of the
                         format YYYY-MM-DD */
                        track->year = atoi(tag + 5);
                    }
                    /* track_nr/tracks tag handling */
                    if (g_ascii_strncasecmp("TRACKNUMBER=", tag, 12) == 0) {
                        gchar* string;
                        gchar* string2;
                        string = tag + 12;
                        string2 = strchr(string, '/');
                        if (string2) {
                            track->tracks = atoi(string2 + 1);
                            *string2 = '\0';
                        }
                        track->track_nr = atoi(string);
                    }
                    /* cd_nr/cds tag handling */
                    if (g_ascii_strncasecmp("DISCNUMBER=", tag, 11) == 0) {
                        gchar* string;
                        gchar* string2;
                        string = tag + 11;
                        string2 = strchr(string, '/');
                        if (string2) {
                            track->cds = atoi(string2 + 1);
                            *string2 = '\0';
                        }
                        track->cd_nr = atoi(string);
                    }
                    if (g_ascii_strncasecmp("COMPOSER=", tag, 9) == 0) {
                        track->composer = charset_to_utf8(tag + 9);
                    }
                    if (g_ascii_strncasecmp("COMMENT=", tag, 8) == 0) {
                        track->comment = charset_to_utf8(tag + 8);
                    }
                    if (g_ascii_strncasecmp("TRACKS=", tag, 7) == 0) {
                        track->tracks = atoi(tag + 7);
                    }
                    if (g_ascii_strncasecmp("CDNR=", tag, 5) == 0) {
                        track->cd_nr = atoi(tag + 5);
                    }
                    if (g_ascii_strncasecmp("CDS=", tag, 4) == 0) {
                        track->cds = atoi(tag + 4);
                    }
                    /* I'm not sure if "BPM=" is correct */
                    if (g_ascii_strncasecmp("BPM=", tag, 4) == 0) {
                        track->BPM = atoi(tag + 4);
                    }
                }
            }

            if (tags)
                FLAC__metadata_object_delete(tags);
        }

        if (track->description)
            g_free(track->description);

        track->description = g_strdup("FLAC audio file");

        track->bitrate = stream_data.data.stream_info.bits_per_sample / 1000;
        track->samplerate = stream_data.data.stream_info.sample_rate;
        track->tracklen = stream_data.data.stream_info.total_samples
                / (stream_data.data.stream_info.sample_rate / 1000);
    }

    return track;
}

gboolean flac_can_convert() {
    gchar *cmd = flac_get_conversion_cmd();
    return cmd && cmd[0];
}

gchar *flac_get_conversion_cmd() {
    return prefs_get_string("path_conv_flac");
}
