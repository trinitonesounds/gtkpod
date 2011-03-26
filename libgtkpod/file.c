/*
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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>

#include "charset.h"
#include "clientserver.h"
#include "file.h"
#include "file_convert.h"
#include "itdb.h"
#include "sha1.h"
#include "misc.h"
#include "misc_track.h"
#include "prefs.h"
#include "misc_conversion.h"
#include "filetype_iface.h"

#define UNKNOWN_ERROR _("Unknown error")

/* The uppercase version of these extensions is tried as well. */
static const gchar *imageext[] =
    { ".jpg", ".jpeg", ".png", ".pbm", ".pgm", ".ppm", ".tif", ".tiff", ".gif", NULL };

/*
 * Struct to hold the track added signal and
 * hashtable of itdb to count of tracks added
 * so far
 */
typedef struct {
    gulong file_added_signal_id;
    GHashTable *itdb_tracks_count;
} TrackMonitor;

/*
 * Pair struct for insertion into the TrackMonitor
 * hashtable
 */
typedef struct {
    guint64 *id;
    gint count;
} TrackMonitorPair;

/* Single TrackMonitor instance */
static TrackMonitor *trkmonitor = NULL;

/**
 * Callback fired when a new track is added to an itdb.
 * Will be fired from playlist, directory and file functions
 */
static void file_track_added_cb(GtkPodApp *app, gpointer tk, gpointer data) {
    Track *track = tk;
    if (!track)
        return;

    g_return_if_fail(track->itdb);
    g_return_if_fail(trkmonitor);

    guint64 *key = &track->itdb->id;
    TrackMonitorPair *value = g_hash_table_lookup(trkmonitor->itdb_tracks_count, key);
    if (!value) {
        value = g_new0(TrackMonitorPair, 1);
        value->id = key;
        value->count = 1;
    } else {
        value->count++;
    }

    /* save every ${file_threshold} files but do at least ${file_theshold} first*/
    int threshold = prefs_get_int("file_saving_threshold");
    if (value->count >= threshold) {
        gp_save_itdb(track->itdb);
        gtkpod_tracks_statusbar_update();
        // Reset the count
        value->count = 0;
    }

    g_hash_table_replace(trkmonitor->itdb_tracks_count, key, value);
}

/**
 * Initialise the file added signal with the callback
 */
static void init_file_added_signal() {
    if (! trkmonitor) {
        trkmonitor = g_new0(TrackMonitor, 1);
        trkmonitor->itdb_tracks_count = g_hash_table_new (g_int64_hash, g_int64_equal);
        trkmonitor->file_added_signal_id = g_signal_connect (gtkpod_app, SIGNAL_TRACK_ADDED, G_CALLBACK (file_track_added_cb), NULL);
    }
}

/* Determine the type of a file.
 *
 * Currently this is done by checking the suffix of the filename. An improved
 * version should probably only fall back to that method if everything else
 * fails.
 * -jlt
 */

FileType *determine_filetype(const gchar *path) {
    gchar *path_utf8, *suf;
    FileType *type = NULL;

    g_return_val_if_fail (path, type);

    if (g_file_test(path, G_FILE_TEST_IS_DIR))
        return type;

    path_utf8 = charset_to_utf8(path);
    suf = g_strrstr(path_utf8, ".");
    if (!suf)
        return type;

    suf = suf + 1;
    type = gtkpod_get_filetype(suf);

    g_free(path_utf8);
    return type;
}

/** check a filename against the "excludes file mask" from the preferences
 *  and return TRUE if it should be excluded based on the mask
 */
static gboolean excludefile(gchar *filename) {
    gboolean matched = FALSE;
    gchar **masks, *prefs_masks;

    prefs_masks = prefs_get_string("exclude_file_mask");

    if (prefs_masks == NULL)
        return FALSE;

    masks = g_strsplit(prefs_masks, ";", -1);

    if (masks != NULL) {
        gint i;
        for (i = 0; !matched && masks[i]; i++) {
            matched = g_pattern_match_simple(g_strstrip(masks[i]), filename);
        }
        g_strfreev(masks);
    }

    g_free(prefs_masks);

    return matched;
}

/*------------------------------------------------------------------*\
 *                                                                  *
 *      Add Playlists                                               *
 *                                                                  *
 \*------------------------------------------------------------------*/

/* Add all files specified in playlist @plfile. Will create a new
 * playlist with the name "basename (plfile)", even if one of the same
 * name already exists (if @plitem is != NULL, all tracks will be added
 * to @plitem and no new playlist will be created).
 * It will then add all tracks listed in @plfile. If set in the prefs,
 * duplicates will be detected (and the track already present in the
 * database will be added to the playlist instead). */
/* @addtrackfunc: if != NULL this will be called instead of
 "add_track_to_playlist () -- used for dropping tracks at a specific
 position in the track view */
/* Return value: playlist to which the files were added to or NULL on
 * error */
Playlist *
add_playlist_by_filename(iTunesDB *itdb, gchar *plfile, Playlist *plitem, gint plitem_pos, AddTrackFunc addtrackfunc, gpointer data) {
    gchar *bufp, *plfile_utf8;
    gchar *dirname = NULL, *plname = NULL;
    gchar buf[PATH_MAX];
    FileType *type = NULL; /* type of playlist file */
    gint line, tracks;
    FILE *fp;
    gboolean error;

    g_return_val_if_fail (plfile, FALSE);
    g_return_val_if_fail (itdb, FALSE);

    if (g_file_test(plfile, G_FILE_TEST_IS_DIR)) {
        gtkpod_warning(_("'%s' is a directory, not a playlist file.\n\n"), plfile);
        return FALSE; /* definitely not! */
    }

    plfile_utf8 = charset_to_utf8(plfile);

    plname = g_path_get_basename(plfile_utf8);
    g_free(plfile_utf8);

    bufp = g_strrstr(plname, "."); /* find last occurence of '.' */
    if (bufp) {
        *bufp = 0; /* truncate playlist name */
        type = determine_filetype(plfile);
        if (!filetype_is_playlist_filetype(type)) {
            gtkpod_warning(_("'%s' is a not a known playlist file.\n\n"), plfile);
            g_free(plname);
            return FALSE;
        }
    }

    /* attempt to open playlist file */
    if (!(fp = fopen(plfile, "r"))) {
        gtkpod_warning(_("Could not open '%s' for reading.\n"), plfile);
        g_free(plname);
        return FALSE; /* definitely not! */
    }
    /* create playlist (if none is specified) */
    if (!plitem)
        plitem = gp_playlist_add_new(itdb, plname, FALSE, plitem_pos);
    C_FREE (plname);
    g_return_val_if_fail (plitem, NULL);

    /* need dirname if playlist file contains relative paths */
    dirname = g_path_get_dirname(plfile);

    /* for now assume that all playlist files will be line-based
     all of these are line based -- add different code for different
     playlist files */
    line = -1; /* nr of line being read */
    tracks = 0; /* nr of tracks added */
    error = FALSE;
    while (!error && fgets(buf, PATH_MAX, fp)) {
        gchar *bufp = buf;
        gchar *filename = NULL;
        gint len = strlen(bufp); /* remove newline */

        ++line;
        if (len == 0)
            continue; /* skip empty lines */

        /* remove linux / windows newline characters */
        if (bufp[len - 1] == 0x0a) {
            bufp[len - 1] = 0;
            --len;
        }

        /* remove windows carriage return
           to support playlist files created on Windows */
        if (bufp[len -1] == 0x0d) {
            bufp[len - 1] = 0;
            --len;
        }

        if (!filetype_is_playlist_filetype(type)) {
            /* skip whitespace */
            while (isspace (*bufp))
                ++bufp;
            /* assume comments start with ';' or '#' */
            if ((*bufp == ';') || (*bufp == '#'))
                continue;
            /* assume the rest of the line is a filename */
            filename = concat_dir_if_relative(dirname, bufp);
        }
        else {
            if (filetype_is_m3u_filetype(type)) {
                /* comments start with '#' */
                if (*bufp == '#')
                    continue;
                /* assume the rest of the line is a filename */
                filename = concat_dir_if_relative(dirname, bufp);
            }
            else if (filetype_is_pls_filetype(type)) {
                /* I don't know anything about pls playlist files and just
                 looked at one example produced by xmms -- please
                 correct the code if you know more */
                if (line == 0) { /* check for "[playlist]" */
                    if (strncasecmp(bufp, "[playlist]", 10) != 0)
                        error = TRUE;
                }
                else if (strncasecmp(bufp, "File", 4) == 0) { /* looks like a file entry */
                    bufp = strchr(bufp, '=');
                    if (bufp) {
                        ++bufp;
                        filename = concat_dir_if_relative(dirname, bufp);
                    }
                }
            }
        }

        if (filename) {
            /* do not allow to add directories! */
            if (g_file_test(filename, G_FILE_TEST_IS_DIR)) {
                gtkpod_warning(_("Skipping '%s' because it is a directory.\n"), filename);
            }
            /* do not allow to add playlist file recursively */
            else if (strcmp(plfile, filename) == 0) {
                gtkpod_warning(_("Skipping '%s' to avoid adding playlist file recursively\n"), filename);
            }
            else {
                add_track_by_filename(itdb, filename, plitem, prefs_get_int("add_recursively"), addtrackfunc, data);
            }
            g_free(filename);
        }
    }
    fclose(fp);
    C_FREE (dirname);

    /* I don't think it's too interesting to pop up the list of
     duplicates -- but we should reset the list. */
    gp_duplicate_remove(NULL, (void *) -1);

    if (!error)
        return plitem;
    return NULL;
}

/*------------------------------------------------------------------*\
 *                                                                  *
 *      Add Dir                                                     *
 *                                                                  *
 \*------------------------------------------------------------------*/

/*
 * Add all files in directory and subdirectories.
 *
 * If @name is a regular file, just add that.
 * If @plitem != NULL, add tracks also to Playlist @plitem
 * @descend:    TRUE: add recursively
 *                      FALSE: don't enter subdirectories
 * @addtrackfunc:
 *                      if != NULL this will be called instead of
 *                      "add_track_to_playlist () -- used for dropping
 *                      tracks at a specific position in the track view
 *
 * return:
 *              value indicating number of added tracks.
 */
/* */
gint add_directory_by_name(iTunesDB *itdb, gchar *name, Playlist *plitem, gboolean descend, AddTrackFunc addtrackfunc, gpointer data) {
    gint result = 0;

    g_return_val_if_fail (itdb, 0);
    g_return_val_if_fail (name, 0);

    if (g_file_test(name, G_FILE_TEST_IS_DIR)) {
        GDir *dir = g_dir_open(name, 0, NULL);
        block_widgets();
        if (dir != NULL) {
            G_CONST_RETURN gchar *next;
            do {
                next = g_dir_read_name(dir);
                if (next != NULL) {
                    gchar *nextfull = g_build_filename(name, next, NULL);
                    if (descend || !g_file_test(nextfull, G_FILE_TEST_IS_DIR)) {
                        result += add_directory_by_name(itdb, nextfull, plitem, descend, addtrackfunc, data);
                    }
                    g_free(nextfull);
                }
            }
            while (next != NULL);

            g_dir_close(dir);
        }
        release_widgets();
    }
    else {
        if (add_track_by_filename(itdb, name, plitem, descend, addtrackfunc, data))
            result++;
    }
    return result;
}

/*------------------------------------------------------------------*\
 *                                                                  *
 *      Fill in track struct with data from file                     *
 *                                                                  *
 \*------------------------------------------------------------------*/

/* parse the file with name @filename (UTF8) and fill extract the tags
 * to @trackas specified in @template. @track can be NULL if you just
 * want to verify the template */
static gboolean parse_filename_with_template(Track *track, gchar *filename, const gchar *template) {
    GList *tokens = NULL, *gl;
    gchar *tpl, *fn;
    gchar *sps, *sp, *str;
    gboolean parse_error = FALSE;

    if (!template || !filename)
        return FALSE;

#ifdef DEBUG
    printf ("fn: '%s', tpl: '%s'\n", filename, template);
#endif

    /* If the template starts with a '%.' (except for '%%') we add a
     '/' in front so that the field has a boundary */
    if ((template[0] == '%') && (template[1] != '%'))
        tpl = g_strdup_printf("%c%s", G_DIR_SEPARATOR, template);
    else
        tpl = g_strdup(template);

    fn = g_strdup(filename);

    /* We split the template into tokens. Each token starting with a
     '%' and two chars long specifies a field ('%.'), otherwise it's
     normal text (used to separate two fields).
     "/%a - %t.mp3" would become ".mp3"  "%t"  " - "  "%a"  "/" */
    sps = tpl;
    while ((sp = strchr(sps, '%'))) {
        if (sps != sp)
            tokens = g_list_prepend(tokens, g_strndup(sps, sp - sps));
        if (sp[1] != '%') {
            tokens = g_list_prepend(tokens, g_strndup(sp, 2));
        }
        else {
            tokens = g_list_prepend(tokens, g_strdup("%"));
        }
        if (!sp[1])
            break;
        sps = sp + 2;
    }
    /* add what's left */
    if (sps[0] != 0)
        tokens = g_list_prepend(tokens, g_strdup(sps));

    /* If the "last" token does not contain a '.' (like in ".mp3"),
     remove the filename extension ("somefile.mp3" -> "somefile")
     because no extension was given in the template */
    str = g_list_nth_data(tokens, 0);
    if (str && (strchr(str, '.') == NULL)) {
        gchar *str = strrchr(fn, '.');
        if (str)
            str[0] = 0;
    }

#ifdef DEBUG
    puts (tpl);
    for (gl=tokens; gl; gl=gl->next)
    puts (gl->data);
    puts (fn);
#endif

    /* use the tokens to parse the filename */
    gl = tokens;
    while (gl) {
        gchar *token = gl->data;
        /* remember: all tokens starting with '%' and two chars long
         specify a field */
        if ((token[0] == '%') && (strlen(token) == 2)) { /* handle tag item */
            GList *gln = gl->next;
            if (gln) {
                gchar *itm;
                gchar *next_token = gln->data;
                /* find next token so we can determine where the
                 current field ends */
                gchar *fnp = g_strrstr(fn, next_token);

                if (!fnp) {
                    parse_error = TRUE;
                    break;
                }

                /* truncate the filename (for the next token) */
                fnp[0] = 0;
                /* adjust fnp to point to the start of the field */
                fnp = fnp + strlen(next_token);
#ifdef DEBUG
                printf ("%s: '%s'\n", token, fnp);
#endif
                itm = g_strstrip (g_strdup (fnp));
                switch (token[1]) {
                case 'a': /* artist */
                    if (track && (!track->artist || prefs_get_int("parsetags_overwrite"))) {
                        g_free(track->artist);
                        track->artist = itm;
                    }
                    break;
                case 'A': /* album */
                    if (track && (!track->album || prefs_get_int("parsetags_overwrite"))) {
                        g_free(track->album);
                        track->album = itm;
                    }
                    break;
                case 'c': /* composer */
                    if (track && (!track->composer || prefs_get_int("parsetags_overwrite"))) {
                        g_free(track->composer);
                        track->composer = itm;
                    }
                    break;
                case 't': /* title */
                    if (track && (!track->title || prefs_get_int("parsetags_overwrite"))) {
                        g_free(track->title);
                        track->title = itm;
                    }
                    break;
                case 'g': /* genre */
                case 'G': /* genre */
                    if (track && (!track->genre || prefs_get_int("parsetags_overwrite"))) {
                        g_free(track->genre);
                        track->genre = itm;
                    }
                    break;
                case 'T': /* track */
                    if (track && ((track->track_nr == 0) || prefs_get_int("parsetags_overwrite")))
                        track->track_nr = atoi(itm);
                    g_free(itm);
                    break;
                case 'C': /* CD */
                    if (track && (track->cd_nr == 0 || prefs_get_int("parsetags_overwrite")))
                        track->cd_nr = atoi(itm);
                    g_free(itm);
                    break;
                case 'Y': /* year */
                    if (track && (track->year == 0 || prefs_get_int("parsetags_overwrite")))
                        track->year = atoi(itm);
                    g_free(itm);
                    break;
                case '*': /* placeholder to skip a field */
                    g_free(itm);
                    break;
                default:
                    g_free(itm);
                    gtkpod_warning(_("Unknown token '%s' in template '%s'\n"), token, template);
                    parse_error = TRUE;
                    break;
                }
                if (parse_error)
                    break;
                gl = gln->next;
            }
            else { /* if (gln)...else... */
                break;
            }
        }
        else { /* skip text */
            gchar *fnp = g_strrstr(fn, token);
            if (!fnp) {
                parse_error = TRUE;
                break; /* could not match */
            }
            if (fnp - fn + strlen(fnp) != strlen(fn)) {
                parse_error = TRUE;
                break; /* not at the last position */
            }
            fnp[0] = 0;
            gl = gl->next;
        }
    }

    g_free(fn);
    g_free(tpl);
    g_list_foreach(tokens, (GFunc) g_free, NULL);
    g_list_free(tokens);
    return (!parse_error);
}

/* parse the filename of @track and extract the tags as specified in
 prefs_get_string("parsetags_template"). Several templates can be separated
 with the "," character. */
static void parse_filename(Track *track) {
    ExtraTrackData *etr;
    gchar *template;
    gchar **templates, **tplp;

    g_return_if_fail (track);
    etr = track->userdata;
    g_return_if_fail (etr);

    template = prefs_get_string("parsetags_template");
    if (!template)
        return;
    templates = g_strsplit(template, ";", 0);
    tplp = templates;
    while (*tplp) {
        if (parse_filename_with_template(NULL, etr->pc_path_utf8, *tplp))
            break;
        ++tplp;
    }
    if (*tplp)
        parse_filename_with_template(track, etr->pc_path_utf8, *tplp);
    g_strfreev(templates);
    g_free(template);
}

/* Set entry "column" (TM_COLUMN_TITLE etc) according to filename */
/* TODO: make the TAG extraction more intelligent -- if possible, this
 should be user configurable. */
static void set_entry_from_filename(Track *track, gint column) {
    ExtraTrackData *etr;

    g_return_if_fail (track);
    etr = track->userdata;
    g_return_if_fail (etr);

    if (prefs_get_int_index("tag_autoset", column) && etr->pc_path_utf8 && strlen(etr->pc_path_utf8)) {
        switch (column) {
        case TM_COLUMN_TITLE:
            g_free(track->title);
            track->title = g_path_get_basename(etr->pc_path_utf8);
            break;
        case TM_COLUMN_ALBUM:
            g_free(track->album);
            track->album = g_path_get_basename(etr->pc_path_utf8);
            break;
        case TM_COLUMN_ARTIST:
            g_free(track->artist);
            track->artist = g_path_get_basename(etr->pc_path_utf8);
            break;
        case TM_COLUMN_GENRE:
            g_free(track->genre);
            track->genre = g_path_get_basename(etr->pc_path_utf8);
            break;
        case TM_COLUMN_COMPOSER:
            g_free(track->composer);
            track->composer = g_path_get_basename(etr->pc_path_utf8);
            break;
        }
    }
}

static void set_unset_entries_from_filename(Track *track) {
    /* try to fill tags from filename */
    if (prefs_get_int("parsetags"))
        parse_filename(track);
    /* fill up what is left unset */
    if (!track->album && prefs_get_int_index("tag_autoset", TM_COLUMN_ALBUM))
        set_entry_from_filename(track, TM_COLUMN_ALBUM);
    if (!track->artist && prefs_get_int_index("tag_autoset", TM_COLUMN_ARTIST))
        set_entry_from_filename(track, TM_COLUMN_ARTIST);
    if (!track->title && prefs_get_int_index("tag_autoset", TM_COLUMN_TITLE))
        set_entry_from_filename(track, TM_COLUMN_TITLE);
    if (!track->genre && prefs_get_int_index("tag_autoset", TM_COLUMN_GENRE))
        set_entry_from_filename(track, TM_COLUMN_GENRE);
    if (!track->composer && prefs_get_int_index("tag_autoset", TM_COLUMN_COMPOSER)) {
        set_entry_from_filename(track, TM_COLUMN_COMPOSER);
    }
}

/* update the track->charset info with the currently used charset */
void update_charset_info(Track *track) {
    const gchar *charset = prefs_get_string("charset");
    ExtraTrackData *etr;

    g_return_if_fail (track);
    etr = track->userdata;
    g_return_if_fail (etr);

    C_FREE (etr->charset);
    if (!charset || !strlen(charset)) { /* use standard locale charset */
        g_get_charset(&charset);
    }
    /* only set charset if it's not GTKPOD_JAPAN_AUTOMATIC */
    if (charset && (strcmp(charset, GTKPOD_JAPAN_AUTOMATIC) != 0)) {
        etr->charset = g_strdup(charset);
    }
}

/* Copy "new" info read from file to an old Track structure. */
/* Return value: TRUE: at least one item was changed. FALSE: track was
 unchanged */
static gboolean copy_new_info(Track *from, Track *to) {
    ExtraTrackData *efrom, *eto;
    T_item item;
    gboolean changed = FALSE;

    g_return_val_if_fail (from, FALSE);
    g_return_val_if_fail (to, FALSE);
    efrom = from->userdata;
    eto = to->userdata;
    g_return_val_if_fail (efrom, FALSE);
    g_return_val_if_fail (eto, FALSE);

    for (item = 0; item < T_ITEM_NUM; ++item) {
        switch (item) {
        case T_ALBUM:
        case T_ARTIST:
        case T_TITLE:
        case T_GENRE:
        case T_COMMENT:
        case T_COMPOSER:
        case T_FILETYPE:
        case T_DESCRIPTION:
        case T_PODCASTURL:
        case T_PODCASTRSS:
        case T_SUBTITLE:
        case T_TV_SHOW:
        case T_TV_EPISODE:
        case T_TV_NETWORK:
        case T_THUMB_PATH:
        case T_PC_PATH:
        case T_ALBUMARTIST:
        case T_SORT_ARTIST:
        case T_SORT_TITLE:
        case T_SORT_ALBUM:
        case T_SORT_ALBUMARTIST:
        case T_SORT_COMPOSER:
        case T_SORT_TVSHOW:
        case T_YEAR:
        case T_TRACK_NR:
        case T_TRACKLEN:
        case T_STARTTIME:
        case T_STOPTIME:
        case T_SIZE:
        case T_BITRATE:
        case T_SAMPLERATE:
        case T_BPM:
        case T_TIME_ADDED:
        case T_TIME_MODIFIED:
        case T_TIME_RELEASED:
        case T_SOUNDCHECK:
        case T_CD_NR:
        case T_COMPILATION:
        case T_MEDIA_TYPE:
        case T_SEASON_NR:
        case T_EPISODE_NR:
        case T_GROUPING:
        case T_LYRICS:
            changed |= track_copy_item(from, to, item);
            break;
        case T_CATEGORY:
            /* not implemented from tags */
            break;
        case T_RATING:
        case T_REMEMBER_PLAYBACK_POSITION:
        case T_SKIP_WHEN_SHUFFLING:
        case T_CHECKED:
        case T_TIME_PLAYED:
        case T_IPOD_PATH:
        case T_ALL:
        case T_IPOD_ID:
        case T_TRANSFERRED:
        case T_PLAYCOUNT:
        case T_VOLUME:
        case T_GAPLESS_TRACK_FLAG:
            /* not applicable */
            break;
        case T_ITEM_NUM:
            g_return_val_if_reached (FALSE);
        }
    }

    if ((eto->charset == NULL) || (strcmp(efrom->charset, eto->charset) != 0)) {
        g_free(eto->charset);
        eto->charset = g_strdup(efrom->charset);
        changed = TRUE;
    }

    if (from->chapterdata) {
        itdb_chapterdata_free(to->chapterdata);
        to->chapterdata = itdb_chapterdata_duplicate(from->chapterdata);
        changed = TRUE; /* FIXME: probably should actually compare chapters for changes */
    }

    itdb_artwork_free(to->artwork);
    to->artwork = itdb_artwork_duplicate(from->artwork);
    if ((to->artwork_size != from->artwork_size) || (to->artwork_count != from->artwork_count) || (to->has_artwork
            != from->has_artwork)) { /* FIXME -- artwork might have changed nevertheless */
        changed = TRUE;
        to->artwork_size = from->artwork_size;
        to->artwork_count = from->artwork_count;
        to->has_artwork = from->has_artwork;
    }

    if ((to->lyrics_flag != from->lyrics_flag) || (to->movie_flag != from->movie_flag)) {
        changed = TRUE;
        to->lyrics_flag = from->lyrics_flag;
        to->movie_flag = from->movie_flag;
    }

    if ((to->pregap != from->pregap) || (to->postgap != from->postgap) || (to->samplecount != from->samplecount)
            || (to->gapless_data != from->gapless_data) || (to->gapless_track_flag != from->gapless_track_flag)) {
        changed = TRUE;
        to->pregap = from->pregap;
        to->postgap = from->postgap;
        to->samplecount = from->samplecount;
        to->gapless_data = from->gapless_data;
        to->gapless_track_flag = from->gapless_track_flag;
    }

    if (eto->mtime != efrom->mtime) {
        changed = TRUE;
        eto->mtime = efrom->mtime;
    }

    return changed;
}

/* ensure that a cache directory is available for video thumbnail generation.
 * largely copied from conversion_setup_cachedir, only slightly simplified
 * and modified to return the path to the caller for convenience.
 */
static gchar* video_thumbnail_setup_cache() {
    gchar *cachedir = NULL;
    gchar *cfgdir = prefs_get_cfgdir();

    if (!cfgdir)
        return NULL;

    cachedir = g_build_filename(cfgdir, "video_thumbnail_cache", NULL);
    g_free(cfgdir);

    if (!g_file_test(cachedir, G_FILE_TEST_IS_DIR) && (g_mkdir(cachedir, 0777) == -1)) {
        gtkpod_warning(_("Could not create '%s'"), cachedir);
        g_free(cachedir);
        cachedir = NULL;
    }

    return cachedir;
}

/*
 * automatically generate a thumbnail for video input files using
 * an external program (the totem thumbnailer by default).
 * @input is the path of the video file to be thumbnailed.
 * Returns a path to a temporary file containing the thumbnail.  The
 * temporary file is stored in a dedicated cache directory.
 */
static gchar* create_video_thumbnail(gchar* input) {
    GString *cmd = NULL;
    GError *err = NULL;
    int fd, retval, forkstatus;
    gchar *thumbnailer = NULL, *tmp_fn = NULL, *tmp = NULL, *p = NULL;
    /* find (and set up if necessary) the thumbnail cache dir */
    gchar *tdir = video_thumbnail_setup_cache();

    if (!tdir) {
        return NULL;
    }

    /* safely generate a temporary file.  we don't actually need the fd
     * so we close it on succesful generation */
    tmp_fn = g_build_filename(tdir, "thumb.XXXXXX", NULL);
    g_free(tdir);
    tdir = NULL;

    if ((fd = g_mkstemp(tmp_fn)) == -1 || close(fd)) {
        gtkpod_warning(_("Error creating thumbnail file"));
        g_free(tmp_fn);
        return NULL;
    }

    /* get the string containing the (template) command for thumbnailing */
    thumbnailer = prefs_get_string("video_thumbnailer_prog");
    /* copy it into cmd, expanding any format characters */
    cmd = g_string_new("");
    p = thumbnailer;

    while (*p) {
        if (*p == '%') {
            ++p;

            switch (*p) {
            /* %f: input file */
            case 'f':
                tmp = g_shell_quote(input);
                break;
                /* %o: temporary output file */
            case 'o':
                tmp = g_shell_quote(tmp_fn);
                break;
            case '%':
                cmd = g_string_append_c (cmd, '%');
                break;
            default:
                gtkpod_warning(_("Unknown token '%%%c' in template '%s'"), *p, thumbnailer);
                break;
            }

            if (tmp) {
                cmd = g_string_append(cmd, tmp);
                g_free(tmp);
                tmp = NULL;
            }
        }
        else {
            cmd = g_string_append_c (cmd, *p);
        }

        ++p;
    }
    /* run the thumbnailing program */
    forkstatus = g_spawn_command_line_sync(cmd->str, NULL, NULL, &retval, &err);

    if (!forkstatus) {
        gtkpod_warning(_("Unable to start video thumbnail generator\n(command line was: '%s')"), cmd->str);
    }
    else if (retval) {
        gtkpod_warning(_("Thumbnail generator returned status %d"), retval);
    }

    g_string_free(cmd, TRUE);
    /* thumbnail is in tmp_fn */
    return tmp_fn;
}

/* look for a picture specified by coverart_template  */
static void add_coverart(Track *tr) {
    ExtraTrackData *etr;
    gchar *full_template;
    gchar **templates, **tplp;
    gchar *dirname;
    gchar *filename_local = NULL;
    FileType *filetype;
    gint vid_thumbnailer;

    g_return_if_fail (tr);
    etr = tr->userdata;
    g_return_if_fail (etr);

    dirname = g_path_get_dirname(etr->pc_path_utf8);

    full_template = prefs_get_string("coverart_template");
    vid_thumbnailer = prefs_get_int("video_thumbnailer");

    templates = g_strsplit(full_template, ";", 0);
    tplp = templates;
    while (*tplp && !filename_local) {
        gchar *filename_utf8;
        gchar *fname = get_string_from_template(tr, *tplp, FALSE, FALSE);
        if (fname) {
            if (strchr(*tplp, '.') != NULL) { /* if template has an extension, try if it is valid */
                if (fname[0] == '/') /* allow absolute paths in template */
                    filename_utf8 = g_build_filename("", fname, NULL);
                else
                    filename_utf8 = g_build_filename(dirname, fname, NULL);
                filename_local = charset_from_utf8(filename_utf8);
                g_free(filename_utf8);
                if (!g_file_test(filename_local, G_FILE_TEST_EXISTS)) {
                    g_free(filename_local);
                    filename_local = NULL;
                }
            }
            if (!filename_local) { /* if no filename is found try out different extensions */
                const gchar **extp = imageext;
                while (*extp && !filename_local) {
                    gchar *ffname = g_strconcat(fname, *extp, NULL);
                    if (ffname[0] == '/') /* allow absolute paths in template */
                        filename_utf8 = g_build_filename("", ffname, NULL);
                    else
                        filename_utf8 = g_build_filename(dirname, ffname, NULL);
                    g_free(ffname);
                    filename_local = charset_from_utf8(filename_utf8);
                    g_free(filename_utf8);
                    if (!g_file_test(filename_local, G_FILE_TEST_EXISTS)) {
                        g_free(filename_local);
                        filename_local = NULL;
                    }
                    ++extp;
                }
                extp = imageext;
                while (*extp && !filename_local) { /* try uppercase version of extension */
                    gchar *upper_ext = g_ascii_strup(*extp, -1);
                    gchar *ffname = g_strconcat(fname, upper_ext, NULL);
                    g_free(upper_ext);
                    if (ffname[0] == '/') /* allow absolute paths in template */
                        filename_utf8 = g_build_filename("", ffname, NULL);
                    else
                        filename_utf8 = g_build_filename(dirname, ffname, NULL);
                    g_free(ffname);
                    filename_local = charset_from_utf8(filename_utf8);
                    g_free(filename_utf8);
                    if (!g_file_test(filename_local, G_FILE_TEST_EXISTS)) {
                        g_free(filename_local);
                        filename_local = NULL;
                    }
                    ++extp;
                }
            }
        }
        g_free(fname);
        ++tplp;
    }

    if (filename_local) {
        gp_track_set_thumbnails(tr, filename_local);
    }
    else if (vid_thumbnailer) {
        filetype = determine_filetype(etr->pc_path_utf8);
        /* if a template match was not made, and we're dealing with a video
         * file, generate an arbitrary thumbnail if appropriate */
        if (filetype_is_video_filetype(filetype)) {
            filename_local = create_video_thumbnail(etr->pc_path_utf8);
            gp_track_set_thumbnails(tr, filename_local);
        }
    }

    g_strfreev(templates);
    g_free(full_template);
    g_free(dirname);
}

/* Fills the supplied @orig_track with data from the file @name. If
 * @orig_track is NULL, a new track struct is created. The entries
 * pc_path_utf8 and pc_path_locale are not changed if an entry already
 * exists. time_added is not modified if already set. */
/* Returns NULL on error, a pointer to the Track otherwise */
Track *get_track_info_from_file(gchar *name, Track *orig_track) {
    Track *track = NULL;
    Track *nti = NULL;
    FileType *filetype;
    gint len;
    gchar *name_utf8 = NULL;
    GError *error = NULL;

    g_return_val_if_fail (name, NULL);

    if (g_file_test(name, G_FILE_TEST_IS_DIR))
        return NULL;

    name_utf8 = charset_to_utf8(name);

    if (!g_file_test(name, G_FILE_TEST_EXISTS)) {
        gtkpod_warning(_("The following track could not be processed (file does not exist): '%s'\n"), name_utf8);
        g_free(name_utf8);
        return NULL;
    }

    /* reset the auto detection charset (see explanation in charset.c) */
    charset_reset_auto();

    /* check for filetype */
    len = strlen(name);
    if (len < 4)
        return NULL;

    filetype = determine_filetype(name);
    if (!filetype) {
        gtkpod_warning(
                _("The filetype '%s' is not currently supported.\n\n"
                        "If you have a plugin that supports this filetype then please enable it."), name_utf8);
        return NULL;
    }

    nti = filetype_get_file_info(filetype, name, &error);
    if (error || !nti) {
        gtkpod_warning(_("No track information could be retrieved from the file %s due to the following error:\n\n%s"), name_utf8, error->message);
        g_error_free(error);
        error = NULL;
        g_free(name_utf8);
        return NULL;
    } else if (!nti) {
        gtkpod_warning(_("No track information could be retrieved from the file %s due to the following error:\n\nAn error was not returned."), name_utf8);
        g_free(name_utf8);
        return NULL;
    }

    switch (nti->mediatype) {
    case ITDB_MEDIATYPE_AUDIOBOOK:
    case ITDB_MEDIATYPE_PODCAST:
    case ITDB_MEDIATYPE_PODCAST | ITDB_MEDIATYPE_MOVIE: /* Video podcast */
        /* For audiobooks and podcasts, default to remember playback position
         * and skip when shuffling. */
        nti->skip_when_shuffling = 1;
        nti->remember_playback_position = 1;
        break;
    }

    ExtraTrackData *enti = nti->userdata;
    struct stat filestat;

    g_return_val_if_fail (enti, NULL);

    if (enti->charset == NULL) { /* Fill in currently used charset. Try if auto_charset is
     * set first. If not, use the currently set charset. */
        enti->charset = charset_get_auto();
        if (enti->charset == NULL)
            update_charset_info(nti);
    }
    /* set path file information */
    enti->pc_path_utf8 = charset_to_utf8(name);
    enti->pc_path_locale = g_strdup(name);
    enti->lyrics = NULL;
    /* set length of file */
    stat(name, &filestat);
    nti->size = filestat.st_size; /* get the filesize in bytes */
    enti->mtime = filestat.st_mtime; /* get the modification date */
    if (nti->bitrate == 0) { /* estimate bitrate */
        if (nti->tracklen)
            nti->bitrate = nti->size * 8 / nti->tracklen;
    }
    /* Set unset strings (album...) from filename */
    set_unset_entries_from_filename(nti);

    /* Set coverart */
    if (prefs_get_int("coverart_file")) {
        /* APIC data takes precedence */
        if (!itdb_track_has_thumbnails(nti))
            add_coverart(nti);
    }

    /* Set modification date to the files modified date */
    nti->time_modified = enti->mtime;
    /* Set added date to *now* (unless orig_track is present) */
    if (orig_track) {
        nti->time_added = orig_track->time_added;
    }
    else {
        nti->time_added = time(NULL);
    }

    /* Make sure all strings are initialized -- that way we don't
     have to worry about it when we are handling the
     strings. Also, validate_entries() will fill in the utf16
     strings if that hasn't already been done. */
    /* exception: sha1_hash, charset and hostname: these may be
     * NULL. */

    gp_track_validate_entries(nti);

    if (orig_track) { /* we need to copy all information over to the original
     * track */
        ExtraTrackData *eorigtr = orig_track->userdata;

        g_return_val_if_fail (eorigtr, NULL);

        eorigtr->tchanged = copy_new_info(nti, orig_track);

        track = orig_track;
        itdb_track_free(nti);
        nti = NULL;
    }
    else { /* just use nti */
        track = nti;
        nti = NULL;
    }

    while (widgets_blocked && gtk_events_pending())
        gtk_main_iteration();

    g_free(name_utf8);

    return track;
}

/*------------------------------------------------------------------*\
 *                                                                  *
 *      Update Track Data from File                                  *
 *                                                                  *
 \*------------------------------------------------------------------*/

/* reads info from file and updates the ID3 tags of
 @selected_tracks. */
void update_tracks(GList *selected_tracks) {
    GList *gl;
    iTunesDB *itdb = NULL;

    if (selected_tracks == NULL) {
        gtkpod_statusbar_message(_("Nothing to update"));
        return;
    }

    block_widgets();
    for (gl = selected_tracks; gl; gl = gl->next) {
        Track *track = gl->data;
        g_return_if_fail (track);

        /* update_track_from_file() may possibly remove tracks from
         the database, so we need to check if the track we are
         referencing to is still valid. To do so we first have to
         secure a valid pointer to an itdb. Since the first track in
         selected_tracks is always valid, we take that one. */
        if (!itdb)
            itdb = track->itdb;
        g_return_if_fail (itdb);
        if (g_list_find(itdb->tracks, track)) {
            gchar *buf = get_track_info(track, TRUE);
            gtkpod_statusbar_message(_("Updating %s"), buf);
            g_free(buf);
            update_track_from_file(track->itdb, track);
        }
    }
    release_widgets();
    /* display log of non-updated tracks */
    display_non_updated(NULL, NULL);
    /* display log of updated tracks */
    display_updated(NULL, NULL);
    /* display log of detected duplicates */
    gp_duplicate_remove(NULL, NULL);
    gtkpod_statusbar_message(_("Updated selected tracks with info from file."));
}

/* Logs tracks (@track) that could not be updated from file for some
 reason. Pop up a window with the log by calling with @track = NULL,
 or remove the log by calling with @track = -1.
 @txt (if available)w is added as an explanation as to why it was
 impossible to update the track */
void display_non_updated(Track *track, gchar *txt) {
    gchar *buf;
    static gint track_nr = 0;
    static GString *str = NULL;

    if ((track == NULL) && str) {
        if (prefs_get_int("show_non_updated") && str->len) { /* Some tracks have not been updated. Print a notice */
            buf
                    = g_strdup_printf(ngettext("The following track could not be updated", "The following %d tracks could not be updated", track_nr), track_nr);
            gtkpod_confirmation(-1, /* gint id, */
            FALSE, /* gboolean modal, */
            _("Failed Track Update"), /* title */
            buf, /* label */
            str->str, /* scrolled text */
            NULL, 0, NULL, /* option 1 */
            NULL, 0, NULL, /* option 2 */
            TRUE, /* gboolean confirm_again, */
            "show_non_updated",/* confirm_again_key,*/
            CONF_NULL_HANDLER, /* ConfHandler ok_handler,*/
            NULL, /* don't show "Apply" button */
            NULL, /* cancel_handler,*/
            NULL, /* gpointer user_data1,*/
            NULL); /* gpointer user_data2,*/
            g_free(buf);
        }
        display_non_updated((void *) -1, NULL);
    }

    if (track == (void *) -1) { /* clean up */
        if (str)
            g_string_free(str, TRUE);
        str = NULL;
        track_nr = 0;
        gtkpod_tracks_statusbar_update();
    }
    else if (prefs_get_int("show_non_updated") && track) {
        /* add info about it to str */
        buf = get_track_info(track, TRUE);
        if (!str) {
            track_nr = 0;
            str = g_string_sized_new(2000); /* used to keep record of
             * non-updated tracks */
        }
        if (txt)
            g_string_append_printf(str, "%s (%s)\n", buf, txt);
        else
            g_string_append_printf(str, "%s\n", buf);
        g_free(buf);
        ++track_nr; /* count tracks */
    }
}

/* Logs tracks (@track) that could be updated from file. Pop up a window
 with the log by calling with @track = NULL, or remove the log by
 calling with @track = -1.
 @txt (if available)w is added as an explanation as to why it was
 impossible to update the track */
void display_updated(Track *track, gchar *txt) {
    gchar *buf;
    static gint track_nr = 0;
    static GString *str = NULL;

    if (prefs_get_int("show_updated") && (track == NULL) && str) {
        if (str->len) { /* Some tracks have been updated. Print a notice */
            buf
                    = g_strdup_printf(ngettext("The following track has been updated", "The following %d tracks have been updated", track_nr), track_nr);
            gtkpod_confirmation(-1, /* gint id, */
            FALSE, /* gboolean modal, */
            _("Successful Track Update"), /* title */
            buf, /* label */
            str->str, /* scrolled text */
            NULL, 0, NULL, /* option 1 */
            NULL, 0, NULL, /* option 2 */
            TRUE, /* gboolean confirm_again, */
            "show_updated",/* confirm_again_key,*/
            CONF_NULL_HANDLER, /* ConfHandler ok_handler,*/
            NULL, /* don't show "Apply" button */
            NULL, /* cancel_handler,*/
            NULL, /* gpointer user_data1,*/
            NULL); /* gpointer user_data2,*/
            g_free(buf);
        }
        display_updated((void *) -1, NULL);
    }

    if (track == (void *) -1) { /* clean up */
        if (str)
            g_string_free(str, TRUE);
        str = NULL;
        track_nr = 0;
        gtkpod_tracks_statusbar_update();
    }
    else if (prefs_get_int("show_updated") && track) {
        /* add info about it to str */
        buf = get_track_info(track, TRUE);
        if (!str) {
            track_nr = 0;
            str = g_string_sized_new(2000); /* used to keep record of
             * non-updated tracks */
        }
        if (txt)
            g_string_append_printf(str, "%s (%s)\n", buf, txt);
        else
            g_string_append_printf(str, "%s\n", buf);
        g_free(buf);
        ++track_nr; /* count tracks */
    }
}

/* Update information of @track from data in original file. If the
 * original filename is not available, information will be updated
 * from the copy on the iPod and a warning is printed.

 A list of non-updated tracks can be displayed by calling
 display_non_updated (NULL, NULL). This list can be deleted by
 calling display_non_updated ((void *)-1, NULL);

 It is also possible that duplicates get detected in the process --
 a list of those can be displayed by calling "gp_duplicate_remove
 (NULL, NULL)", that list can be deleted by calling
 "gp_duplicate_remove (NULL, (void *)-1)"*/
void update_track_from_file(iTunesDB *itdb, Track *track) {
    ExtraTrackData *oetr;
    Track *oldtrack;
    gchar *prefs_charset = NULL;
    gchar *trackpath = NULL;
    gint32 oldsize = 0;
    gboolean charset_set;

    g_return_if_fail (itdb);
    g_return_if_fail (track);
    oetr = track->userdata;
    g_return_if_fail (oetr);
    /* remember size of track on iPod */
    if (track->transferred)
        oldsize = track->size;
    else
        oldsize = 0;

    /* remember if charset was set */
    if (oetr->charset)
        charset_set = TRUE;
    else
        charset_set = FALSE;

    if (!prefs_get_int("update_charset") && charset_set) { /* we should use the initial charset for the update */
        prefs_charset = prefs_get_string("charset");
        /* use the charset used when first importing the track */
        prefs_set_string("charset", oetr->charset);
    }

    trackpath = get_file_name_from_source(track, SOURCE_PREFER_LOCAL);

    if (!(oetr->pc_path_locale && *oetr->pc_path_locale)) { /* no path available */
        if (trackpath) {
            display_non_updated(track, _("no local filename available, file on the iPod will be used instead"));
        }
        else {
            if (itdb->usertype & GP_ITDB_TYPE_IPOD) {
                display_non_updated(track, _("no local filename available and copy on iPod cannot be found"));
            }
            else {
                display_non_updated(track, _("no local filename available"));
            }
        }
    }
    else if (!g_file_test(oetr->pc_path_locale, G_FILE_TEST_EXISTS)) {
        if (trackpath) {
            display_non_updated(track, _("local file could not be found, file on the iPod will be used instead"));
        }
        else {
            if (itdb->usertype & GP_ITDB_TYPE_IPOD) {
                display_non_updated(track, _("local file as well as copy on the iPod cannot be found"));
            }
            else {
                display_non_updated(track, _("no local filename available"));
            }
        }
    }

    if (trackpath && get_track_info_from_file(trackpath, track)) { /* update successful */
        ExtraTrackData *netr = track->userdata;

        /* remove track from sha1 hash and reinsert it
         (hash value may have changed!) */
        gchar *oldhash = oetr->sha1_hash;

        sha1_track_remove(track);
        /* need to remove the old value manually! */
        oetr->sha1_hash = NULL;
        oldtrack = sha1_track_exists_insert(itdb, track);
        if (oldtrack) { /* track exists, remove old track
         and register the new version */
            sha1_track_remove(oldtrack);
            gp_duplicate_remove(track, oldtrack);
            sha1_track_exists_insert(itdb, track);
        }

        if (itdb->usertype & GP_ITDB_TYPE_IPOD) { /* track may have to be copied to iPod on next export */
            gchar *name_on_ipod;
            gboolean transfer_again = FALSE;

            name_on_ipod = get_file_name_from_source(track, SOURCE_IPOD);
            if (name_on_ipod && (strcmp(name_on_ipod, trackpath) != 0)) { /* trackpath is not on the iPod */
                if (oldhash && oetr->sha1_hash) { /* do we really have to copy the track again? */
                    if (strcmp(oldhash, oetr->sha1_hash) != 0) {
                        transfer_again = TRUE;
                    }
                }
                else { /* no hash available -- copy! */
                    transfer_again = TRUE;
                }
            }
            else {
                data_changed(itdb);
            }

            if (transfer_again) { /* We need to copy the track back to the iPod. That's done
             marking a copy of the original track for deletion and
             then adding the original track to the
             conversion/transfer list */
                Track *new_track = gp_track_new();
                ExtraTrackData *new_etr = new_track->userdata;
                g_return_if_fail (new_etr);

                new_track->size = oldsize;
                new_track->ipod_path = track->ipod_path;
                track->ipod_path = g_strdup("");
                track->transferred = FALSE;

                /* cancel conversion/transfer of track */
                file_convert_cancel_track(track);
                /* mark the track for deletion on the ipod */
                mark_track_for_deletion(itdb, new_track);
                /* reschedule conversion/transfer of track */
                file_convert_add_track(track);

                netr->tchanged = TRUE;
            }

            g_free(name_on_ipod);
        }

        /* Set this flag to true to ensure artwork is reread from file */
        netr->tartwork_changed = TRUE;

        /* notify display model */
        gtkpod_track_updated(track);
        if (netr->tchanged) {
            data_changed(itdb);
            netr->tchanged = FALSE;
        }

        display_updated(track, NULL);
        g_free(oldhash);
    }
    else if (trackpath) { /* update not successful -- log this track for later display */
        display_non_updated(track, _("update failed (format not supported?)"));
    }

    if (!prefs_get_int("update_charset") && charset_set) { /* reset charset */
        prefs_set_string("charset", prefs_charset);
    }

    g_free(trackpath);
    g_free(prefs_charset);

    while (widgets_blocked && gtk_events_pending())
        gtk_main_iteration();
}

/*------------------------------------------------------------------*\
 *                                                                  *
 *      Add File                                                    *
 *                                                                  *
 \*------------------------------------------------------------------*/

/* Append file @fname to the list of tracks.
 @fname is in the current locale
 @plitem: if != NULL, add track to plitem as well (unless it's the MPL)
 descend: TRUE:  add directories recursively
 FALSE: add contents of directories passed but don't descend
 into its subdirectories */
/* @addtrackfunc: if != NULL this will be called instead of
 "add_track_to_playlist () -- used for dropping tracks at a specific
 position in the track view */
gboolean add_track_by_filename(iTunesDB *itdb, gchar *fname, Playlist *plitem, gboolean descend, AddTrackFunc addtrackfunc, gpointer data) {
    Track *oldtrack;
    gchar str[PATH_MAX];
    gchar *basename;
    Playlist *mpl;
    gboolean result = TRUE;
    FileType *filetype;

    g_return_val_if_fail (fname, FALSE);
    g_return_val_if_fail (itdb, FALSE);
    mpl = itdb_playlist_mpl(itdb);
    g_return_val_if_fail (mpl, FALSE);

    init_file_added_signal();

    if (!plitem)
        plitem = mpl;

    if (g_file_test(fname, G_FILE_TEST_IS_DIR)) {
        return add_directory_by_name(itdb, fname, plitem, descend, addtrackfunc, data);
    }

    /* check if file is a playlist */
    filetype = determine_filetype(fname);
    if (filetype_is_playlist_filetype(filetype)) {
        if (add_playlist_by_filename(itdb, fname, plitem, -1, addtrackfunc, data))
            return TRUE;
        return FALSE;
    }

    if (!filetype_is_audio_filetype(filetype) && !filetype_is_video_filetype(filetype)) {
        return FALSE;
    }

    /* print a message about which file is being processed */
    basename = g_path_get_basename(fname);
    if (basename) {
        gchar *bn_utf8 = charset_to_utf8(basename);
        gtkpod_statusbar_message(_("Processing '%s'..."), bn_utf8);
        while (widgets_blocked && gtk_events_pending())
            gtk_main_iteration();
        g_free(bn_utf8);

        if (excludefile(basename)) {
            gtkpod_warning(_("Skipping '%s' because it matches exclude masks.\n"), basename);
            while (widgets_blocked && gtk_events_pending())
                gtk_main_iteration();
            g_free(basename);
            return FALSE;
        }
    }
    C_FREE (basename);

    /* Check if there exists already a track with the same filename */
    oldtrack = gp_track_by_filename(itdb, fname);
    /* If a track already exists in the database, either update it or
     just add it to the current playlist (if it's not already there) */
    if (oldtrack) {
        if (prefs_get_int("update_existing")) { /* update the information */
            update_track_from_file(itdb, oldtrack);
        }
        /* add to current playlist if it's not already in there */
        if (!itdb_playlist_is_mpl(plitem)) {
            if (!itdb_playlist_contains_track(plitem, oldtrack)) {
                if (addtrackfunc)
                    addtrackfunc(plitem, oldtrack, data);
                else
                    gp_playlist_add_track(plitem, oldtrack, TRUE);
            }
        }
    }
    else /* oldtrack == NULL */
    { /* OK, the same filename does not already exist */
        Track *track = get_track_info_from_file(fname, NULL);
        if (track) {
            Track *added_track = NULL;
            ExtraTrackData *etr = track->userdata;
            g_return_val_if_fail (etr, FALSE);

            track->id = 0;
            track->transferred = FALSE;

            /* is 'fname' on the iPod? -- if yes mark as transfered, if
             * it's in the music directory */
            if (itdb->usertype & GP_ITDB_TYPE_IPOD) {
                const gchar *mountpoint = itdb_get_mountpoint(itdb);
                g_return_val_if_fail (mountpoint, FALSE);
                if (strstr(fname, mountpoint) == fname) { /* Yes */
                    /* is 'fname' in the iPod's Music directory? */
                    gchar *music_dir = itdb_get_music_dir(mountpoint);
                    if (music_dir) {
                        gchar *cdir = g_strdup_printf("%s%c", music_dir, G_DIR_SEPARATOR);
                        /* TODO: Use GIO for file/directory operations */
                        if (g_ascii_strncasecmp(fname, cdir, strlen(cdir)) == 0) { /* Yes */
                            gchar *fname_i = fname + strlen(mountpoint);
                            if (*fname_i == G_DIR_SEPARATOR)
                                ++fname_i;
                            track->transferred = TRUE;
                            track->ipod_path = g_strdup_printf("%c%s", G_DIR_SEPARATOR, fname_i);
                            itdb_filename_fs2ipod(track->ipod_path);
                        }
                        g_free(music_dir);
                        g_free(cdir);
                    }
                }
            }

            if (gethostname(str, PATH_MAX - 2) == 0) {
                str[PATH_MAX - 1] = 0;
                etr->hostname = g_strdup(str);
            }
            /* add_track may return pointer to a different track if an
             identical one (SHA1 checksum) was found */
            added_track = gp_track_add(itdb, track);
            g_return_val_if_fail (added_track, FALSE);

            /* set flags to 'podcast' if adding to podcast list */
            if (itdb_playlist_is_podcasts(plitem))
                gp_track_set_flags_podcast(added_track);

            if (itdb_playlist_is_mpl(plitem)) { /* add track to master playlist if it wasn't a
             duplicate */
                if (added_track == track) {
                    if (addtrackfunc)
                        addtrackfunc(plitem, added_track, data);
                    else
                        gp_playlist_add_track(plitem, added_track, TRUE);
                }
            }
            else {
#if 0 /* initially iTunes didn't add podcasts to the MPL */
                /* add track to master playlist if it wasn't a
                 * duplicate and plitem is not the podcasts playlist
                 */
                if (added_track == track)
                {
                    if (!itdb_playlist_is_podcasts (plitem))
                    gp_playlist_add_track (mpl, added_track, TRUE);
                }
#else
                if (added_track == track) {
                    gp_playlist_add_track(mpl, added_track, TRUE);
                }
#endif
                /* add track to specified playlist -- unless adding
                 * to podcasts list and track already exists there */
                if (itdb_playlist_is_podcasts(plitem) && g_list_find(plitem->members, added_track)) {
                    gchar *buf = get_track_info(added_track, FALSE);
                    gtkpod_warning(_("Podcast already present: '%s'\n\n"), buf);
                    g_free(buf);
                }
                else {
                    if (addtrackfunc)
                        addtrackfunc(plitem, added_track, data);
                    else
                        gp_playlist_add_track(plitem, added_track, TRUE);
                }
            }

            /* indicate that non-transferred files exist */
            data_changed(itdb);
        }
        else { /* !track */
            result = FALSE;
        }
    }

    while (widgets_blocked && gtk_events_pending())
        gtk_main_iteration();
    return result;
}

/*------------------------------------------------------------------*\
 *                                                                  *
 *      Write Tags                                                  *
 *                                                                  *
 \*------------------------------------------------------------------*/

/* Call the correct tag writing function for the filename @name */
static gboolean file_write_info(gchar *name, Track *track, GError *error) {
    FileType *filetype;

    g_return_val_if_fail (name, FALSE);
    g_return_val_if_fail (track, FALSE);

    filetype = determine_filetype(name);
    return filetype_write_file_info(filetype, name, track, &error);
}

/* Write tags to file */
gboolean write_tags_to_file(Track *track) {
    ExtraTrackData *etr;
    iTunesDB *itdb;
    gchar *ipod_fullpath;
    gchar *prefs_charset = NULL;
    Track *oldtrack;
    gboolean track_charset_set;
    GError *error = NULL;

    g_return_val_if_fail (track, FALSE);
    etr = track->userdata;
    g_return_val_if_fail (etr, FALSE);
    itdb = track->itdb;
    g_return_val_if_fail (itdb, FALSE);

    /* if we are to use the charset used when first importing
     the track, change the prefs settings temporarily */
    if (etr->charset)
        track_charset_set = TRUE;
    else
        track_charset_set = FALSE;
    if (!prefs_get_int("write_charset") && track_charset_set) { /* we should use the initial charset for the update */
        prefs_charset = prefs_get_string("charset");
        /* use the charset used when first importing the track */
        prefs_set_string("charset", etr->charset);
    }
    else { /* we should update the track->charset information */
        update_charset_info(track);
    }

    if (etr->pc_path_locale && (strlen(etr->pc_path_locale) > 0)) {
        if (! file_write_info(etr->pc_path_locale, track, error)) {
            gchar *msg = g_strdup_printf(_("Couldn't change tags of file: %s"), etr->pc_path_locale);
            if (error) {
                gtkpod_warning("%s\n%s", msg, error->message);
                g_error_free(error);
                error = NULL;
            }
            else {
                gtkpod_warning("%s\n%s", msg, UNKNOWN_ERROR);
            }
            g_free(msg);
        }
    }
    if (!get_offline(itdb) && track->transferred && track->ipod_path && (g_utf8_strlen(track->ipod_path, -1) > 0)) {
        /* need to get ipod filename */
        ipod_fullpath = get_file_name_from_source(track, SOURCE_IPOD);
        if (!file_write_info(ipod_fullpath, track, error)) {
            gchar *msg = g_strdup_printf(_("Couldn't change tags of file: %s\n"), ipod_fullpath);
            if (error) {
                gtkpod_warning("%s\n%s", msg, error->message);
                g_error_free(error);
                error = NULL;
            }
            else {
                gtkpod_warning("%s\n%s", msg, UNKNOWN_ERROR);
            }
            g_free(msg);
        }
        g_free(ipod_fullpath);
    }
    /* remove track from sha1 hash and reinsert it (hash value has changed!) */
    sha1_track_remove(track);
    C_FREE (etr->sha1_hash); /* need to remove the old value manually! */
    oldtrack = sha1_track_exists_insert(itdb, track);
    if (oldtrack) { /* track exists, remove and register the new version */
        sha1_track_remove(oldtrack);
        gp_duplicate_remove(track, oldtrack);
        sha1_track_exists_insert(itdb, track);
    }

    if (!prefs_get_int("write_charset") && track_charset_set) { /* reset charset */
        prefs_set_string("charset", prefs_charset);
    }
    g_free(prefs_charset);
    return TRUE;
}

/* Get file name from source @source */
/* File is guaranteed to exist, otherwise NULL is returned. */
gchar *get_file_name_from_source(Track *track, FileSource source) {
    gchar *result = NULL;
    ExtraTrackData *etr;

    g_return_val_if_fail (track, NULL);
    etr = track->userdata;
    g_return_val_if_fail (etr, NULL);

    switch (source) {
    case SOURCE_PREFER_LOCAL:
        result = get_file_name_from_source(track, SOURCE_LOCAL);
        if (!result) {
            if (track->itdb && (track->itdb->usertype & GP_ITDB_TYPE_IPOD)) {
                result = get_file_name_from_source(track, SOURCE_IPOD);
            }
        }
        break;
    case SOURCE_PREFER_IPOD:
        result = get_file_name_from_source(track, SOURCE_IPOD);
        if (!result)
            result = get_file_name_from_source(track, SOURCE_LOCAL);
        break;
    case SOURCE_LOCAL:
        if (etr->pc_path_locale && (*etr->pc_path_locale)) {
            if (g_file_test(etr->pc_path_locale, G_FILE_TEST_EXISTS)) {
                result = g_strdup(etr->pc_path_locale);
            }
        }
        break;
    case SOURCE_IPOD:
        if (track && !get_offline(track->itdb)) {
            result = itdb_filename_on_ipod(track);
        }
        break;
    }
    return result;
}

/* ------------------------------------------------------------

 Reading of offline playcount file

 ------------------------------------------------------------ */

/* Read the ~/.gtkpod/offline_playcount file and adjust the
 playcounts. The tracks will first be matched by their sha1 sum, if
 that fails, by their filename.
 If tracks could not be matched, the user will be queried whether to
 forget about them or write them back into the offline_playcount
 file. */
void parse_offline_playcount(void) {
    gchar *cfgdir = prefs_get_cfgdir();
    gchar *offlplyc = g_strdup_printf("%s%c%s", cfgdir, G_DIR_SEPARATOR, "offline_playcount");

    if (g_file_test(offlplyc, G_FILE_TEST_EXISTS)) {
        FILE *file = fopen(offlplyc, "r+");
        size_t len = 0; /* how many bytes are written */
        gchar *buf;
        GString *gstr, *gstr_filenames;
        if (!file) {
            gtkpod_warning(_("Could not open '%s' for reading and writing.\n"), offlplyc);
            g_free(offlplyc);
            return;
        }
        if (flock(fileno(file), LOCK_EX) != 0) {
            gtkpod_warning(_("Could not obtain lock on '%s'.\n"), offlplyc);
            fclose(file);
            g_free(offlplyc);
            return;
        }
        buf = g_malloc(2 * PATH_MAX);
        gstr = g_string_sized_new(PATH_MAX);
        gstr_filenames = g_string_sized_new(PATH_MAX);
        while (fgets(buf, 2 * PATH_MAX, file)) {
            gchar *buf_utf8 = charset_to_utf8(buf);
            gchar *sha1 = NULL;
            gchar *filename = NULL;
            gchar *ptr1, *ptr2;
            /* skip strings that do not start with "PLCT:" */
            if (strncmp(buf, SOCKET_PLYC, strlen(SOCKET_PLYC)) != 0) {
                gtkpod_warning(_("Malformed line in '%s': %s\n"), offlplyc, buf);
                goto cont;
            }
            /* start of SHA1 string */
            ptr1 = buf + strlen(SOCKET_PLYC);
            /* end of SHA1 string */
            ptr2 = strchr(ptr1, ' ');
            if (ptr2 == NULL) { /* error! */
                gtkpod_warning(_("Malformed line in '%s': %s\n"), offlplyc, buf_utf8);
                goto cont;
            }
            if (ptr1 != ptr2)
                sha1 = g_strndup(ptr1, ptr2 - ptr1);
            /* start of filename */
            ptr1 = ptr2 + 1;
            /* end of filename string */
            ptr2 = strchr(ptr1, '\n');
            if (ptr2 == NULL) { /* error! */
                gtkpod_warning(_("Malformed line in '%s': %s\n"), offlplyc, buf_utf8);
                goto cont;
            }
            if (ptr1 != ptr2) {
                filename = g_strndup(ptr1, ptr2 - ptr1);
            }
            else { /* error! */
                gtkpod_warning(_("Malformed line in '%s': %s\n"), offlplyc, buf_utf8);
                goto cont;
            }
            if (gp_increase_playcount(sha1, filename, 1) == FALSE) { /* didn't find the track -> store */
                gchar *filename_utf8 = charset_to_utf8(filename);
                g_string_append(gstr_filenames, filename_utf8);
                g_string_append(gstr_filenames, "\n");
                g_free(filename_utf8);
                g_string_append(gstr, buf);
            }
            cont: g_free(buf_utf8);
            g_free(sha1);
            g_free(filename);
        }

        /* rewind file pointer to beginning */
        rewind(file);
        if (gstr->len != 0) {
            gint result = 0;
            result
                    = gtkpod_confirmation(-1, /* gint id, */
                    TRUE, /* gboolean modal, */
                    _("Remove offline playcounts?"), /* title */
                    _("Some tracks played offline could not be found in the iTunesDB. Press 'OK' to remove them from the offline playcount file, 'Cancel' to keep them."), /* label */
                    gstr_filenames->str, /* scrolled text */
                    NULL, 0, NULL, /* option 1 */
                    NULL, 0, NULL, /* option 2 */
                    TRUE, /* confirm_again, */
                    NULL, /* confirm_again_key,*/
                    CONF_NULL_HANDLER, /* ConfHandler ok_handler,*/
                    NULL, /* don't show "Apply" button */
                    CONF_NULL_HANDLER, /* cancel_handler,*/
                    NULL, /* gpointer user_data1,*/
                    NULL); /* gpointer user_data2,*/

            if (result != GTK_RESPONSE_OK) {
                len = fwrite(gstr->str, sizeof(gchar), gstr->len, file);
                if (len != gstr->len) {
                    gtkpod_warning(_("Error writing to '%s'.\n"), offlplyc);
                }
            }
        }
        ftruncate(fileno(file), len);
        fclose(file);
        g_string_free(gstr, TRUE);
        g_string_free(gstr_filenames, TRUE);
        g_free(buf);
    }
    g_free(cfgdir);
    g_free(offlplyc);
}

/* ------------------------------------------------------------

 Reading of gain tags

 ------------------------------------------------------------ */
/**
 * Read the soundcheck value for @track.
 *
 * Return value: TRUE, if gain could be read
 */
gboolean read_soundcheck(Track *track) {
    gchar *path;
    FileType *filetype;
    gboolean result = FALSE;
    GError *error = NULL;
    gchar *msg = g_strdup_printf(_("Failed to read sound check from track because"));

    g_return_val_if_fail (track, FALSE);

    path = get_file_name_from_source(track, SOURCE_PREFER_LOCAL);
    filetype = determine_filetype(path);
    if (! filetype) {
        gtkpod_warning(_("%s\n\nfiletype of %s is not recognised."), msg, path);
    }
    else {
        if (!filetype_read_soundcheck(filetype, path, track, &error)) {
            if (error) {
                gtkpod_warning(_("%s\n\n%s"), msg, error->message);
            } else {
                gtkpod_warning(_("%s\n\n%s"), msg, UNKNOWN_ERROR);
            }
        } else {
            // track read successfully
            result = TRUE;
        }
    }

    g_free(path);
    g_free(msg);
    return result;
}

/* Get lyrics from file */
gboolean read_lyrics_from_file(Track *track, gchar **lyrics) {
    gchar *path;
    gboolean result = FALSE;
    ExtraTrackData *etr;
    FileType *filetype;
    GError *error = NULL;

    g_return_val_if_fail (track, FALSE);
    etr = track->userdata;
    g_return_val_if_fail (etr,FALSE);
    path = get_file_name_from_source(track, SOURCE_PREFER_IPOD);
    if (path) {
        filetype = determine_filetype(path);
        if (!filetype) {
            *lyrics = g_strdup_printf(_("Error: Could not determine filetype for file at path: %s.\n\n"), path);
        }
        else {
            result = filetype_read_lyrics(filetype, path, lyrics, &error);
            if (!result) {
                if (error) {
                    *lyrics = g_strdup_printf(_("Error: Failed to read lyrics because:\n\n%s"), error->message);
                    g_error_free(error);
                    error = NULL;
                }
                else
                    *lyrics = g_strdup_printf(_("Error: Failed to read lyrics because:\n\n%s"), UNKNOWN_ERROR);
            }
        }
    } else {
        *lyrics = g_strdup_printf(_("Error: Unable to get filename from path"));
    }

    g_free(path);

    if (result) {
        if (!*lyrics)
            *lyrics = g_strdup("");
        if (etr->lyrics)
            g_free(etr->lyrics);
        etr->lyrics = g_strdup(*lyrics);
    }
    return result;
}

/* Write lyrics to file */
gboolean write_lyrics_to_file(Track *track) {
    gchar *path = NULL;
    gchar *buf;
    Track *oldtrack;
    gboolean result = FALSE;
    gboolean warned = FALSE;
    ExtraTrackData *etr;
    iTunesDB *itdb;
    FileType *filetype;
    GError *error = NULL;

    g_return_val_if_fail (track, FALSE);
    etr = track->userdata;
    g_return_val_if_fail (etr,FALSE);

    if (g_str_has_prefix(etr->lyrics, _("Error:"))) {
        /* Not writing lyrics as there are only errors */
        return FALSE;
    }

    itdb = track->itdb;
    g_return_val_if_fail (itdb, FALSE);
    path = get_file_name_from_source(track, SOURCE_IPOD);
    if (!path) {
        if (prefs_get_int("id3_write")) {
            path = get_file_name_from_source(track, SOURCE_LOCAL);
        }
        else {
            buf = get_track_info(track, FALSE);
            gtkpod_warning(_("iPod File not available and ID3 saving disabled in options, cannot save lyrics to: %s.\n\n"), buf);
            g_free(buf);
            warned = TRUE;
        }
    }

    filetype = determine_filetype(path);
    if (!filetype) {
        if (!warned) {
            gtkpod_warning(_("Lyrics not written, file type cannot be determined (%s).\n\n"), path);
        }
    }
    else {
        result = filetype_write_lyrics(filetype, path, etr->lyrics, &error);
        if (!result) {
            if (error) {
                gtkpod_warning(_("Lyrics not written due to the error:\n\n%s"), error->message);
                g_error_free(error);
                error = NULL;
            }
            else
                gtkpod_warning(_("Lyrics not written due to the error:\n%s"), UNKNOWN_ERROR);
        }
    }

    g_free(path);

    if (!result || !etr->lyrics || (strlen(etr->lyrics) == 0)) {
        track->lyrics_flag = 0x00;
    }
    else {
        track->lyrics_flag = 0x01;
    }
    if (!etr->lyrics) {
        etr->lyrics = g_strdup("");
    }

    if (result) {
        /* remove track from sha1 hash and reinsert it (hash value has changed!) */
        sha1_track_remove(track);
        C_FREE (etr->sha1_hash); /* need to remove the old value manually! */
        oldtrack = sha1_track_exists_insert(itdb, track);
        if (oldtrack) { /* track exists, remove the old track and register the new version */
            sha1_track_remove(oldtrack);
            gp_duplicate_remove(track, oldtrack);
            sha1_track_exists_insert(itdb, track);
        }
    }
    return result;
}
