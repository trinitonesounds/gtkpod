/*
 |  Copyright (C) 2002-2011 Jorg Schuler <jcsjcs at users sourceforge net>
 |                                             Paul Richardson <phantom_sf at users.sourceforge.net>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "AtomicParsley.h"
#include "AP_AtomExtracts.h"
#include "AtomicParsleyBridge.h"

extern "C" {
#include <glib/gstdio.h>
#include "libgtkpod/prefs.h"
#include "libgtkpod/charset.h"
}

static guint32 mediaTypeTagToMediaType(guint8 media_type) {
    switch (media_type) {
    case 0: /* Movie */
        return ITDB_MEDIATYPE_MOVIE;
    case 1: /* Normal */
        break;
    case 2: /* Audiobook */
        return ITDB_MEDIATYPE_AUDIOBOOK;
    case 5: /* Whacked Bookmark */
        break;
    case 6: /* Music Video */
        return ITDB_MEDIATYPE_MUSICVIDEO;
    case 9: /* Short Film */
        break;
    case 10: /* TV Show */
        return ITDB_MEDIATYPE_TVSHOW;
    case 11: /* Booklet */
        break;
    }
    return 0;
}

static AtomicInfo *find_atom(const char *meta) {
    char atomName[100];

    sprintf(atomName, "moov.udta.meta.ilst.%s.data", meta);

    return APar_FindAtom(atomName, FALSE, VERSIONED_ATOM, 1);
}

static char* find_atom_value(const char* meta) {
    AtomicInfo *info = find_atom(meta);
    if (info) {
        return APar_ExtractDataAtom(info->AtomicNumber);
    }

    return NULL;
}

void AP_extract_metadata(const char *path, Track *track) {
    FILE *mp4File;
    Trackage *trackage;
    uint8_t track_cur;
    gboolean audio_or_video_found = FALSE;

    APar_ScanAtoms(path, true);
    mp4File = openSomeFile(path, true);

    trackage = APar_ExtractDetails(mp4File, SHOW_TRACK_INFO);

    for (track_cur = 0; track_cur < trackage->total_tracks; ++track_cur) {
        TrackInfo *info = trackage->infos[track_cur];

        // FIXME no chapter information implemented yet

        if (info->track_type && audio_or_video_found == FALSE
                && ((info->track_type & AUDIO_TRACK) || (info->track_type & VIDEO_TRACK)
                        || (info->track_type & DRM_PROTECTED_TRACK))) {

            track->tracklen = info->duration;
            track->bitrate = APar_calculate_bitrate(info);
            track->samplerate = info->media_sample_rate;
        }

        audio_or_video_found = TRUE;

    }

    if (prefs_get_int("readtags")) {
        char* value = NULL;

        // MP4 Title
        value = find_atom_value("\251nam");
        if (value) {
            track->title = charset_to_utf8(value);
            free(value);
        }

        // MP4 Artist
        value = find_atom_value("\251ART");
        if (value) {
            track->artist = charset_to_utf8(value);
            free(value);
        }

        // MP4 Album Artist
        value = find_atom_value("aART");
        if (value) {
            track->albumartist = charset_to_utf8(value);
            free(value);
        }

        // MP4 Composer
        value = find_atom_value("\251wrt");
        if (value) {
            track->composer = charset_to_utf8(value);
            free(value);
        }

        // MP4 Comment
        value = find_atom_value("\251cmt");
        if (value) {
            track->comment = charset_to_utf8(value);
            free(value);
        }

        // MP4 Year
        value = find_atom_value("\251day");
        if (value) {
            track->year = atoi(value);
            free(value);
        }

        // MP4 Album
        value = find_atom_value("\251alb");
        if (value) {
            track->album = charset_to_utf8(value);
            free(value);
        }

        // MP4 Track No. and Total
        value = find_atom_value("trkn");
        if (value) {
            const char* delimiter = " of ";
            char *result = NULL;
            result = strtok(value, delimiter);
            if (result)
                track->track_nr = atoi(result);

            result = strtok(NULL, delimiter);
            if (result)
                track->tracks = atoi(result);

            free(value);
        }

        // MP4 Disk No. and Total
        value = find_atom_value("disk");
        if (value) {
            const char* delimiter = " of ";
            char *result = NULL;
            result = strtok(value, delimiter);
            if (result)
                track->cd_nr = atoi(result);

            result = strtok(NULL, delimiter);
            if (result)
                track->cds = atoi(result);

            free(value);
        }

        // MP4 Grouping
        value = find_atom_value("\251grp");
        if (value) {
            track->grouping = charset_to_utf8(value);
            free(value);
        }

        // MP4 Genre - note: can be either a standard or custom genre
        // standard genre
        value = find_atom_value("gnre");
        if (value) {
            track->genre = charset_to_utf8(value);
            // Should not free standard genres
        } else {
            // custom genre
            value = find_atom_value("\251gen");
            if (value) {
                track->genre = charset_to_utf8(value);
                free(value);
            }
        }

        // MP4 Tempo / BPM
        value = find_atom_value("tmpo");
        if (value) {
            track->BPM = atoi(value);
            free(value);
        }

        // MP4 Lyrics
        value = find_atom_value("\251lyr");
        if (value) {
            track->lyrics_flag = 0x01;
            free(value);
        }

        // MP4 TV Show
        value = find_atom_value("tvsh");
        if (value) {
            track->tvshow = charset_to_utf8(value);
            free(value);
        }

        // MP4 TV Episode
        value = find_atom_value("tven");
        if (value) {
            track->tvepisode = charset_to_utf8(value);
            free(value);
        }

        // MP4 TV Episode No.
        value = find_atom_value("tves");
        if (value) {
            track->episode_nr = atoi(value);
            free(value);
        }

        // MP4 TV Network
        value = find_atom_value("tvnn");
        if (value) {
            track->tvnetwork = charset_to_utf8(value);
            free(value);
        }

        // MP4 TV Season No.
        value = find_atom_value("tvsn");
        if (value) {
            track->season_nr = atoi(value);
            free(value);
        }

        // MP4 Media Type
        value = find_atom_value("stik");
        if (value) {
            track->mediatype = mediaTypeTagToMediaType(atoi(value));
            // Should not free standard media types
        }

        // MP4 Compilation flag
        value = find_atom_value("cpil");
        if (value) {
            track->compilation = atoi(value);
            free(value);
        }

        fprintf(stdout, "Track title = %s\n", track->title);
        fprintf(stdout, "Track artist = %s\n", track->artist);
        fprintf(stdout, "Track album artist = %s\n", track->albumartist);
        fprintf(stdout, "Track composer = %s\n", track->composer);
        fprintf(stdout, "Track comment = %s\n", track->comment);
        fprintf(stdout, "Track year = %d\n", track->year);
        fprintf(stdout, "Track album = %s\n", track->album);
        fprintf(stdout, "Track no. = %d\n", track->track_nr);
        fprintf(stdout, "Track total = %d\n", track->tracks);
        fprintf(stdout, "Disk no. = %d\n", track->cd_nr);
        fprintf(stdout, "Disk total = %d\n", track->cds);
        fprintf(stdout, "Track Grouping = %s\n", track->grouping);
        fprintf(stdout, "Track genre = %s\n", track->genre);
        fprintf(stdout, "Track BPM = %d\n", track->BPM);
        fprintf(stdout, "Track Lyrics = %d\n", track->lyrics_flag);
        fprintf(stdout, "Track tv show = %s\n", track->tvshow);
        fprintf(stdout, "Track season no. = %d\n", track->season_nr);
        fprintf(stdout, "Track episode no. = %d\n", track->episode_nr);
        fprintf(stdout, "Track episode = %s\n", track->tvepisode);
        fprintf(stdout, "Track network name = %s\n", track->tvnetwork);
        fprintf(stdout, "Track media type = %d\n", track->mediatype);
        fprintf(stdout, "Track compilation = %d\n", track->compilation);

        if (prefs_get_int("coverart_apic")) {
            gchar *tmp_file_prefix = g_build_filename(g_get_tmp_dir(), "ttt", NULL);
            gchar *tmp_file;

            AtomicInfo *info = find_atom("covr");
            if (info) {

                // Extract the data to a temporary file
                tmp_file = APar_ExtractAAC_Artwork(info->AtomicNumber, tmp_file_prefix, 1);
                g_free(tmp_file_prefix);

                if (tmp_file && g_file_test (tmp_file, G_FILE_TEST_EXISTS)) {

                    // Set the thumbnail using the tmp file
                    itdb_track_set_thumbnails(track, tmp_file);

                    if (track->artwork) {
                        fprintf(stdout, "Track has artwork");
                    } else {
                        fprintf(stdout, "No artwork applied to track");
                    }

                    g_remove(tmp_file);
                }

                if (tmp_file)
                    g_free(tmp_file);
            }
        }
    }

    APar_FreeMemory();

}
