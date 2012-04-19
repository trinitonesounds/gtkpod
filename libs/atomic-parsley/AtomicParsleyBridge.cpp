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
#include <glib/gi18n-lib.h>
#include "libgtkpod/prefs.h"
#include "libgtkpod/charset.h"
#include "libgtkpod/gp_itdb.h"
#include "libgtkpod/gp_private.h"
}

#define ILST_FULL_ATOM "moov.udta.meta.ilst"
#define DATA "data"

#define TITLE "\251nam"
#define ARTIST "\251ART"
#define ALBUM_ARTIST "aART"
#define COMPOSER "\251wrt"
#define COMMENT "\251cmt"
#define YEAR "\251day"
#define ALBUM "\251alb"
#define TRACK_NUM_AND_TOTAL "trkn"
#define DISK_NUM_AND_TOTAL "disk"
#define GROUPING "\251grp"
#define DESCRIPTION "desc"
#define STANDARD_GENRE "gnre"
#define CUSTOM_GENRE "\251gen"
#define TEMPO "tmpo"
#define LYRICS "\251lyr"
#define KEYWORD "keyw"
#define TV_SHOW "tvsh"
#define TV_EPISODE "tven"
#define TV_EPISODE_NO "tves"
#define TV_NETWORK_NAME "tvnn"
#define TV_SEASON_NO "tvsn"
#define MEDIA_TYPE "stik"
#define COMPILATION "cpil"
#define CATEGORY "catg"
#define PODCAST_URL "purl"
#define ARTWORK "covr"
#define GAPLESS_FLAG "pgap"

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

static guint8 mediaTypeToMediaTypeTag(guint32 media_type) {
    switch (media_type) {
    case ITDB_MEDIATYPE_MOVIE: /* Movie */
        return 9;
    case ITDB_MEDIATYPE_AUDIO: /* Normal */
        return 1;
    case ITDB_MEDIATYPE_AUDIOBOOK: /* Audiobook */
        return 2;
    case ITDB_MEDIATYPE_MUSICVIDEO: /* Music Video */
        return 6;
    case ITDB_MEDIATYPE_TVSHOW: /* TV Show */
        return 10;
    case ITDB_MEDIATYPE_EPUB_BOOK: /* Booklet */
        return 11;
    case ITDB_MEDIATYPE_RINGTONE: /* Ringtone */
        return 14;
    }
    return 0;
}

static AtomicInfo *find_atom(const char *meta) {
    char atomName[100];

    sprintf(atomName, "%s.%s.%s", ILST_FULL_ATOM, meta, DATA);

    return APar_FindAtom(atomName, FALSE, VERSIONED_ATOM, 1);
}

static char* find_atom_value(const char* meta) {
    AtomicInfo *info = find_atom(meta);
    if (info) {
        return APar_ExtractDataAtom(info->AtomicNumber);
    }

    return NULL;
}

/**
 * Open and scan the metadata of the m4a/mp4/... file
 * and populate the given track.
 */
void AP_read_metadata(const char *filePath, Track *track) {
    FILE *mp4File;
    Trackage *trackage;
    uint8_t track_cur;
    gboolean audio_or_video_found = FALSE;

    APar_ScanAtoms(filePath, true);
    mp4File = openSomeFile(filePath, true);

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
        value = find_atom_value(TITLE);
        if (value) {
            track->title = charset_to_utf8(value);
            free(value);
        }

        // MP4 Artist
        value = find_atom_value(ARTIST);
        if (value) {
            track->artist = charset_to_utf8(value);
            free(value);
        }

        // MP4 Album Artist
        value = find_atom_value(ALBUM_ARTIST);
        if (value) {
            track->albumartist = charset_to_utf8(value);
            free(value);
        }

        // MP4 Composer
        value = find_atom_value(COMPOSER);
        if (value) {
            track->composer = charset_to_utf8(value);
            free(value);
        }

        // MP4 Comment
        value = find_atom_value(COMMENT);
        if (value) {
            track->comment = charset_to_utf8(value);
            free(value);
        }

        // MP4 Description
        value = find_atom_value(DESCRIPTION);
        if (value) {
            track->description = charset_to_utf8(value);
            free(value);
        }

        // MP4 Keywords
        value = find_atom_value(KEYWORD);
        if (value) {
            track->keywords = charset_to_utf8(value);
            free(value);
        }

        // MP4 Year
        value = find_atom_value(YEAR);
        if (value) {
            track->year = atoi(value);
            free(value);
        }

        // MP4 Album
        value = find_atom_value(ALBUM);
        if (value) {
            track->album = charset_to_utf8(value);
            free(value);
        }

        // MP4 Track No. and Total
        value = find_atom_value(TRACK_NUM_AND_TOTAL);
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
        value = find_atom_value(DISK_NUM_AND_TOTAL);
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
        value = find_atom_value(GROUPING);
        if (value) {
            track->grouping = charset_to_utf8(value);
            free(value);
        }

        // MP4 Genre - note: can be either a standard or custom genre
        // standard genre
        value = find_atom_value(STANDARD_GENRE);
        if (value) {
            track->genre = charset_to_utf8(value);
            // Should not free standard genres
        }
        else {
            // custom genre
            value = find_atom_value(CUSTOM_GENRE);
            if (value) {
                track->genre = charset_to_utf8(value);
                free(value);
            }
        }

        // MP4 Tempo / BPM
        value = find_atom_value(TEMPO);
        if (value) {
            track->BPM = atoi(value);
            free(value);
        }

        // MP4 Lyrics
        value = find_atom_value(LYRICS);
        if (value) {
            track->lyrics_flag = 0x01;
            free(value);
        }

        // MP4 TV Show
        value = find_atom_value(TV_SHOW);
        if (value) {
            track->tvshow = charset_to_utf8(value);
            free(value);
        }

        // MP4 TV Episode
        value = find_atom_value(TV_EPISODE);
        if (value) {
            track->tvepisode = charset_to_utf8(value);
            free(value);
        }

        // MP4 TV Episode No.
        value = find_atom_value(TV_EPISODE_NO);
        if (value) {
            track->episode_nr = atoi(value);
            free(value);
        }

        // MP4 TV Network
        value = find_atom_value(TV_NETWORK_NAME);
        if (value) {
            track->tvnetwork = charset_to_utf8(value);
            free(value);
        }

        // MP4 TV Season No.
        value = find_atom_value(TV_SEASON_NO);
        if (value) {
            track->season_nr = atoi(value);
            free(value);
        }

        // MP4 Media Type
        value = find_atom_value(MEDIA_TYPE);
        if (value) {
            track->mediatype = mediaTypeTagToMediaType(atoi(value));
            // Should not free standard media types
        }

        // MP4 Compilation flag
        value = find_atom_value(COMPILATION);
        if (value) {
            track->compilation = atoi(value);
            free(value);
        }

        // MP4 Category
        value = find_atom_value(CATEGORY);
        if (value) {
            track->category = charset_to_utf8(value);
            free(value);
        }

        // MP4 Podcast URL
        value = find_atom_value(PODCAST_URL);
        if (value) {
            track->podcasturl = g_strdup(value);
            free(value);
        }

        value = find_atom_value(GAPLESS_FLAG);
        if (value) {
            track->gapless_track_flag = atoi(value);
            free(value);
        }

        if (prefs_get_int("coverart_apic")) {
            gchar *tmp_file_prefix = g_build_filename(g_get_tmp_dir(), "ttt", NULL);
            gchar *tmp_file;

            AtomicInfo *info = find_atom("covr");
            if (info) {

                // Extract the data to a temporary file
                tmp_file = APar_ExtractAAC_Artwork(info->AtomicNumber, tmp_file_prefix, 1);
                g_free(tmp_file_prefix);

                if (tmp_file && g_file_test(tmp_file, G_FILE_TEST_EXISTS)) {

                    // Set the thumbnail using the tmp file
                    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(tmp_file, NULL);
                    if (pixbuf) {
                        itdb_track_set_thumbnails_from_pixbuf(track, pixbuf);
                        g_object_unref(pixbuf);
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

/**
 * Read any lyrics from the given file
 */
char *AP_read_lyrics(const char *filePath, GError **error) {
    APar_ScanAtoms(filePath, true);
    openSomeFile(filePath, true);

    char *value = find_atom_value(LYRICS);

    APar_FreeMemory();

    return value;
}

static void write_lyrics_internal(const char* lyrics, const char *filePath, GError **error) {
    gchar *atom = g_strdup_printf("%s.%s.%s", ILST_FULL_ATOM, LYRICS, DATA);

    if (!lyrics || strlen(lyrics) == 0)
        APar_RemoveAtom(atom, VERSIONED_ATOM, 0);
    else {
        short lyricsData_atom = APar_MetaData_atom_Init(atom, lyrics, AtomFlags_Data_Text);
        APar_Unified_atom_Put(lyricsData_atom, lyrics, UTF8_iTunesStyle_Unlimited, 0, 0);
    }

    g_free(atom);
}

void AP_write_lyrics(const char *lyrics, const char *filePath, GError **error) {
    APar_ScanAtoms(filePath);
    write_lyrics_internal(lyrics, filePath, error);
}

static void set_limited_text_atom_value(const char *meta, const char *value) {
    char atomName[100];

    sprintf(atomName, "%s.%s.%s", ILST_FULL_ATOM, meta, DATA);

    if (!value || strlen(value) == 0)
        APar_RemoveAtom(atomName, VERSIONED_ATOM, 0);
    else {
        short atom = APar_MetaData_atom_Init(atomName, value, AtomFlags_Data_Text);
        APar_Unified_atom_Put(atom, value, UTF8_iTunesStyle_256glyphLimited, 0, 0);
    }
}

/**
 * Using the given track, set the metadata of the target
 * file
 */
void AP_write_metadata(Track *track, const char *filePath, GError **error) {
    ExtraTrackData *etr;
    gchar *atom;
    gchar *value;

    g_return_if_fail(track);

    APar_ScanAtoms(filePath);

    if (metadata_style != ITUNES_STYLE) {
        gchar *fbuf = charset_to_utf8(filePath);
        gtkpod_log_error(error, g_strdup_printf(_("ERROR %s is not itunes style."), fbuf));
        g_free(fbuf);
        return;
    }

    // Title
    set_limited_text_atom_value(TITLE, track->title);

    // Artist
    set_limited_text_atom_value(ARTIST, track->artist);

    // Album Artist
    set_limited_text_atom_value(ALBUM_ARTIST, track->albumartist);

    // Album
    set_limited_text_atom_value(ALBUM, track->album);

    // Genre
    APar_MetaData_atomGenre_Set(track->genre);

    // Track Number and Total
    atom = g_strdup_printf("%s.%s.%s", ILST_FULL_ATOM, TRACK_NUM_AND_TOTAL, DATA);
    if (track->track_nr == 0) {
        APar_RemoveAtom(atom, VERSIONED_ATOM, 0);
    }
    else {
        gchar *track_info = g_strdup_printf("%d / %d", track->track_nr, track->tracks);
        short tracknumData_atom = APar_MetaData_atom_Init(atom, track_info, AtomFlags_Data_Binary);
        APar_Unified_atom_Put(tracknumData_atom, NULL, UTF8_iTunesStyle_256glyphLimited, 0, 16);
        APar_Unified_atom_Put(tracknumData_atom, NULL, UTF8_iTunesStyle_256glyphLimited, track->track_nr, 16);
        APar_Unified_atom_Put(tracknumData_atom, NULL, UTF8_iTunesStyle_256glyphLimited, track->tracks, 16);
        APar_Unified_atom_Put(tracknumData_atom, NULL, UTF8_iTunesStyle_256glyphLimited, 0, 16);
        g_free(track_info);
    }
    g_free(atom);

    // Disk Number and Total
    atom = g_strdup_printf("%s.%s.%s", ILST_FULL_ATOM, DISK_NUM_AND_TOTAL, DATA);
    if (track->cd_nr == 0) {
        APar_RemoveAtom(atom, VERSIONED_ATOM, 0);
    }
    else {
        gchar *disk_info = g_strdup_printf("%d / %d", track->cd_nr, track->cds);
        short disknumData_atom = APar_MetaData_atom_Init(atom, disk_info, AtomFlags_Data_Binary);
        APar_Unified_atom_Put(disknumData_atom, NULL, UTF8_iTunesStyle_256glyphLimited, 0, 16);
        APar_Unified_atom_Put(disknumData_atom, NULL, UTF8_iTunesStyle_256glyphLimited, track->cd_nr, 16);
        APar_Unified_atom_Put(disknumData_atom, NULL, UTF8_iTunesStyle_256glyphLimited, track->cds, 16);
        g_free(disk_info);
    }
    g_free(atom);

    // Comment
    set_limited_text_atom_value(COMMENT, track->comment);

    // Year
    gchar *yr = NULL;
    if (track->year > 0)
        yr = g_strdup_printf("%d", track->year);

    set_limited_text_atom_value(YEAR, yr);

    if (yr)
        g_free(yr);

    // Lyrics
    etr = (ExtraTrackData *) track->userdata;
    if (etr)
        write_lyrics_internal(etr->lyrics, filePath, error);

    // Composer
    set_limited_text_atom_value(COMPOSER, track->composer);

    // Grouping
    set_limited_text_atom_value(GROUPING, track->grouping);

    // Description
    set_limited_text_atom_value(DESCRIPTION, track->description);

    // TV Network
    set_limited_text_atom_value(TV_NETWORK_NAME, track->tvnetwork);

    // TV Show Name
    set_limited_text_atom_value(TV_SHOW, track->tvshow);

    // TV Episode
    set_limited_text_atom_value(TV_EPISODE, track->tvepisode);

    // Compilation
    atom = g_strdup_printf("%s.%s.%s", ILST_FULL_ATOM, COMPILATION, DATA);
    if (!track->compilation) {
        APar_RemoveAtom(atom, VERSIONED_ATOM, 0);
    }
    else {
        //compilation: [0, 0, 0, 0,   boolean_value]; BUT that first uint32_t is already accounted for in APar_MetaData_atom_Init
        value = g_strdup_printf("%d", track->compilation);
        short compilationData_atom = APar_MetaData_atom_Init(atom, value, AtomFlags_Data_UInt);
        APar_Unified_atom_Put(compilationData_atom, NULL, UTF8_iTunesStyle_256glyphLimited, 1, 8); //a hard coded uint8_t of: 1 is compilation
        g_free(value);
    }
    g_free(atom);

    // Tempo / BPM
    atom = g_strdup_printf("%s.%s.%s", ILST_FULL_ATOM, TEMPO, DATA);
    if (!track->BPM) {
        APar_RemoveAtom(atom, VERSIONED_ATOM, 0);
    }
    else {
        //bpm is [0, 0, 0, 0,   0, bpm_value]; BUT that first uint32_t is already accounted for in APar_MetaData_atom_Init
        value = g_strdup_printf("%d", track->BPM);
        short bpmData_atom = APar_MetaData_atom_Init(atom, value, AtomFlags_Data_UInt);
        APar_Unified_atom_Put(bpmData_atom, NULL, UTF8_iTunesStyle_256glyphLimited, track->BPM, 16);
        g_free(value);
    }
    g_free(atom);

    // Media Type
    atom = g_strdup_printf("%s.%s.%s", ILST_FULL_ATOM, MEDIA_TYPE, DATA);
    guint8 mediaTypeTag = mediaTypeToMediaTypeTag(track->mediatype);
    value = g_strdup_printf("%d", track->season_nr);
    short stikData_atom = APar_MetaData_atom_Init(atom, value, AtomFlags_Data_UInt);
    APar_Unified_atom_Put(stikData_atom, NULL, UTF8_iTunesStyle_256glyphLimited, mediaTypeTag, 8);
    g_free(value);
    g_free(atom);

    // TV Season No.
    atom = g_strdup_printf("%s.%s.%s", ILST_FULL_ATOM, TV_SEASON_NO, DATA);
    if (track->season_nr == 0) {
        APar_RemoveAtom(atom, VERSIONED_ATOM, 0);
    }
    else {
        value = g_strdup_printf("%d", track->season_nr);
        short tvseasonData_atom = APar_MetaData_atom_Init(atom, value, AtomFlags_Data_UInt);
        APar_Unified_atom_Put(tvseasonData_atom, NULL, UTF8_iTunesStyle_256glyphLimited, 0, 16);
        APar_Unified_atom_Put(tvseasonData_atom, NULL, UTF8_iTunesStyle_256glyphLimited, track->season_nr, 16);
        g_free(value);
    }
    g_free(atom);

    // TV Episode No.
    atom = g_strdup_printf("%s.%s.%s", ILST_FULL_ATOM, TV_EPISODE_NO, DATA);
    if (track->episode_nr == 0) {
        APar_RemoveAtom(atom, VERSIONED_ATOM, 0);
    }
    else {
        value = g_strdup_printf("%d", track->episode_nr);
        short tvepisodenumData_atom = APar_MetaData_atom_Init(atom, value, AtomFlags_Data_UInt);
        APar_Unified_atom_Put(tvepisodenumData_atom, NULL, UTF8_iTunesStyle_256glyphLimited, 0, 16);
        APar_Unified_atom_Put(tvepisodenumData_atom, NULL, UTF8_iTunesStyle_256glyphLimited, track->episode_nr, 16);
        g_free(value);
    }
    g_free(atom);

    // Keywords
    set_limited_text_atom_value(KEYWORD, track->keywords);

    // Podcast Category
    set_limited_text_atom_value(CATEGORY, track->category);

    // Podcast URL
    atom = g_strdup_printf("%s.%s.%s", ILST_FULL_ATOM, PODCAST_URL, DATA);
    if (!track->podcasturl || strlen(track->podcasturl) == 0) {
        APar_RemoveAtom(atom, VERSIONED_ATOM, 0);
    }
    else {
        short podcasturlData_atom = APar_MetaData_atom_Init(atom, track->podcasturl, AtomFlags_Data_Binary);
        APar_Unified_atom_Put(podcasturlData_atom, track->podcasturl, UTF8_iTunesStyle_Binary, 0, 0);
    }
    g_free(atom);

    // Gapless Playback
    atom = g_strdup_printf("%s.%s.%s", ILST_FULL_ATOM, GAPLESS_FLAG, DATA);
    if (!track->gapless_track_flag) {
        APar_RemoveAtom(atom, VERSIONED_ATOM, 0);
    }
    else {
        value = g_strdup_printf("%d", track->gapless_track_flag);
        short gaplessData_atom = APar_MetaData_atom_Init(atom, value, AtomFlags_Data_UInt);
        APar_Unified_atom_Put(gaplessData_atom, NULL, UTF8_iTunesStyle_256glyphLimited, 1, 8); //a hard coded uint8_t of: 1 is gapl
        g_free(value);
    }
    g_free(atom);

    if (prefs_get_int("coverart_apic")) {
        GdkPixbuf *pixbuf = (GdkPixbuf*) itdb_artwork_get_pixbuf(track->itdb->device, track->artwork, -1, -1);
        if (!pixbuf) {
            // Destroy any existing artwork if any
            APar_MetaData_atomArtwork_Set("REMOVE_ALL", NULL);
        }
        else {
            gchar *tmp_file = g_build_filename(g_get_tmp_dir(), "ttt.jpg", NULL);
            GError *pixbuf_err = NULL;

            gdk_pixbuf_save(pixbuf, tmp_file, "jpeg", &pixbuf_err, "quality", "100", NULL);

            if (!pixbuf_err) {
                APar_MetaData_atomArtwork_Set(tmp_file, NULL);
                g_remove(tmp_file);
            }
            else {
                gtkpod_log_error(error, g_strdup_printf(_("ERROR failed to change track file's artwork.")));
                g_error_free(pixbuf_err);
                return;
            }

            g_free(tmp_file);
            g_object_unref(pixbuf);
        }
    }

    // after all the modifications are enacted on the tree in memory
    // then write out the changes
    APar_DetermineAtomLengths();
    openSomeFile(filePath, true);
    APar_WriteFile(filePath, NULL, true);

    APar_FreeMemory();
}
