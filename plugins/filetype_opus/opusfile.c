/*
 |
 |  Copyright (C) 2019 Curtis Hildebrand <info (at) curtis hildebrand .com>
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
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "libgtkpod/charset.h"
#include "libgtkpod/gp_itdb.h"
#include "libgtkpod/prefs.h"
#include "libgtkpod/gp_private.h"
#include "plugin.h"
#include "opusfile.h"

/* Info on how to implement new file formats: see mp3file.c for more info */

/* Opus library (www.xiph.org) */
#include <sys/types.h>
#include <sys/param.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
//#include "opus/codec.h"
#include "opus/opusfile.h"

/*
 * The VorbisComment recommendation is too vague on the meaning and
 * content of the fields: http://xiph.org/vorbis/doc/v-comment.html
 *
 * A widely used suggested usage of the fields can be found here:
 * http://wiki.xiph.org/index.php/VorbisComment
 */
Track *opus_get_file_info(const gchar *opusFileName, GError **error) {
    Track *track = NULL;
    FILE *file = NULL;
    int err = NULL;

    file = fopen(opusFileName, "rb");

    if (file == NULL) {
        gchar *filename = charset_to_utf8(opusFileName);
        gtkpod_log_error(error, g_strdup_printf(_("Could not open '%s' for reading.\n"), filename));
        g_free(filename);
    }
    else {
        fclose(file);
        OggOpusFile *opusFile = op_open_file(opusFileName, &err);
        if (err != 0) {
            gchar *filename = NULL;
            filename = charset_to_utf8(opusFileName);
            gtkpod_log_error(error, g_strdup_printf(_("'%s' does not appear to be an Opus audio file.\n"), filename));
            g_free(filename);
            op_free(opusFile);
        }
        else {
            track = gp_track_new();
            track->description = g_strdup(_("Opus audio file"));
            track->mediatype = ITDB_MEDIATYPE_AUDIO;
            const OpusTags *tags = op_tags(opusFile, -1);
            if (tags == NULL) {
                gchar *filename = NULL;
                filename = charset_to_utf8(opusFileName);
                gtkpod_log_error(error, g_strdup_printf(_("'%s' does not appear to have an Opus Tag.\n"), filename));
                g_free(filename);
                g_free(tags);
                op_free(opusFile);
            }
            else {
                track->bitrate = op_bitrate(opusFile, -1);
                track->samplerate = 48000;
                track->tracklen = (op_pcm_total(opusFile, -1)) / 48000; /* in seconds */
                if (prefs_get_int("readtags")) {
                    const char *str;
                    if ((str = opus_tags_query(tags, "artist", 0)) != NULL) {
                        track->artist = charset_to_utf8(str);
                    }
                    if ((str = opus_tags_query(tags, "album", 0)) != NULL) {
                        track->album = charset_to_utf8(str);
                    }
                    if ((str = opus_tags_query(tags, "title", 0)) != NULL) {
                        track->title = charset_to_utf8(str);
                    }
                    if ((str = opus_tags_query(tags, "genre", 0)) != NULL) {
                        track->genre = charset_to_utf8(str);
                    }
                    if ((str = opus_tags_query(tags, "year", 0)) != NULL) {
                        track->year = atoi(str);
                    }
                    if ((str = opus_tags_query(tags, "date", 0)) != NULL) {
                        /* Expected format is YYYY-MM-DDTHH:MM:SS+TS
                         * The fields are optional from right to
                         * left. Year must always be present. Atoi()
                         * will always stop parsing at the first dash
                         * and return the year. */
                        track->year = atoi(str);
                    }
                    if ((str = opus_tags_query(tags, "tracknumber", 0)) != NULL) {
                        track->track_nr = atoi(str);
                    }
                    if ((str = opus_tags_query(tags, "composer", 0)) != NULL) {
                        track->composer = charset_to_utf8(str);
                    }
                    if ((str = opus_tags_query(tags, "comment", 0)) != NULL) {
                        track->comment = charset_to_utf8(str);
                    }
                    if ((str = opus_tags_query(tags, "tracks", 0)) != NULL) {
                        track->tracks = atoi(str);
                    }
                    if ((str = opus_tags_query(tags, "cdnr", 0)) != NULL) {
                        track->cd_nr = atoi(str);
                    }
                    if ((str = opus_tags_query(tags, "cds", 0)) != NULL) {
                        track->cds = atoi(str);
                    }
                    /* I'm not sure if "bpm" is correct */
                    if ((str = opus_tags_query(tags, "bpm", 0)) != NULL) {
                        track->BPM = atoi(str);
                    }
                }
            }
            op_free(opusFile); /* performs the fclose(file); */
        }
    }

    return track;
}

gboolean opus_can_convert() {
    gchar *cmd = opus_get_conversion_cmd();
    return cmd && cmd[0];
}

gchar *opus_get_conversion_cmd() {
    return prefs_get_string("path_conv_opus");
}
