/*
 |  Copyright (C) 2002-2007 Jorg Schuler <jcsjcs at users sourceforge net>
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
#include <string.h>
#include <glib/gstdio.h>
#include "charset.h"
#include "file.h"
#include "itdb.h"
#include "sha1.h"
#include "misc.h"
#include "misc_track.h"
#include "prefs.h"
#include "syncdir.h"
#include "gp_itdb.h"
#include "file_convert.h"
#include "tools.h"
#include "gtkpod_app_iface.h"
#include <glib/gi18n-lib.h>

#define _TO_STR(x) #x
#define TO_STR(x) _TO_STR(x)
#define debug(s) printf(__FILE__":" TO_STR(__LINE__) ":" s)
#define debugx(s,...) printf(__FILE__":" TO_STR(__LINE__) ":" s,__VA_ARGS__)

#define WRITE_EXTENDED_INFO TRUE
/* #define WRITE_EXTENDED_INFO prefs_get_int("write_extended_info") */

/*------------------------------------------------------------------*\
 *                                                                  *
 *      Handle Import of iTunesDB                                   *
 *                                                                  *
 \*------------------------------------------------------------------*/

/* only used when reading extended info from file */
/* see definition of ExtraTrackData in display_itdb.h for explanations */
struct track_extended_info {
    guint ipod_id;
    gchar *pc_path_locale;
    gchar *pc_path_utf8;
    time_t mtime;
    gchar *thumb_path_locale;
    gchar *thumb_path_utf8;
    gchar *converted_file;
    gchar *sha1_hash;
    gchar *charset;
    gchar *hostname;
    gchar *ipod_path;
    guint64 local_itdb_id;
    guint64 local_track_dbid;
    gboolean transferred;
    gchar *lyrics;
};

typedef struct {
    GMutex *mutex; /* mutex for this struct          */
    gboolean abort; /* TRUE = abort                   */
    GCond *finished_cond; /* used to signal end of thread   */
    gboolean finished; /* background thread has finished */
    Track *track; /* Current track                  */
    const gchar *filename; /* Filename to copy/remove        */
    /* Widgets for progress dialog */
    GtkWidget *dialog;
    GtkWidget *textlabel;
    GtkProgressBar *progressbar;
} TransferData;

/* Used to keep the "extended information" until the iTunesDB is loaded */
static GHashTable *extendedinfohash = NULL;
static GHashTable *extendedinfohash_sha1 = NULL;
static GList *extendeddeletion = NULL;
static float extendedinfoversion = 0.0;

/* Some declarations */
static gboolean gp_write_itdb(iTunesDB *itdb);

/* fills in extended info if available */
/* num/total are used to give updates in case the sha1 checksums have
 to be matched against the files which is very time consuming */
void fill_in_extended_info(Track *track, gint32 total, gint32 num) {
    gint ipod_id = 0;
    ExtraTrackData *etr;
    struct track_extended_info *sei = NULL;

    g_return_if_fail (track);
    etr = track->userdata;
    g_return_if_fail (etr);

    if (extendedinfohash && track->id) {
        /* copy id to gint value -- needed for the hash table functions */
        ipod_id = track->id;
        sei = g_hash_table_lookup(extendedinfohash, &ipod_id);
    }
    if (!sei && extendedinfohash_sha1) {
        gtkpod_statusbar_message(_("Matching SHA1 checksum for file %d/%d"), num, total);
        while (widgets_blocked && gtk_events_pending())
            gtk_main_iteration();

        if (!etr->sha1_hash) {
            gchar *filename = get_file_name_from_source(track, SOURCE_IPOD);
            etr->sha1_hash = sha1_hash_on_filename(filename, FALSE);
            g_free(filename);
        }
        if (etr->sha1_hash) {
            sei = g_hash_table_lookup(extendedinfohash_sha1, etr->sha1_hash);
        }
    }
    if (sei) /* found info for this id! */
    {
        etr->lyrics = NULL;
        sei->lyrics = NULL;
        if (sei->pc_path_locale && !etr->pc_path_locale) {
            etr->pc_path_locale = g_strdup(sei->pc_path_locale);
            etr->mtime = sei->mtime;
        }
        if (sei->pc_path_utf8 && !etr->pc_path_utf8)
            etr->pc_path_utf8 = g_strdup(sei->pc_path_utf8);
        if (sei->thumb_path_locale && !etr->thumb_path_locale)
            etr->thumb_path_locale = g_strdup(sei->thumb_path_locale);
        if (sei->thumb_path_utf8 && !etr->thumb_path_utf8)
            etr->thumb_path_utf8 = g_strdup(sei->thumb_path_utf8);
        if (sei->sha1_hash && !etr->sha1_hash)
            etr->sha1_hash = g_strdup(sei->sha1_hash);
        if (sei->charset && !etr->charset)
            etr->charset = g_strdup(sei->charset);
        if (sei->hostname && !etr->hostname)
            etr->hostname = g_strdup(sei->hostname);
        if (sei->converted_file && !etr->converted_file)
            etr->converted_file = g_strdup(sei->converted_file);
        etr->local_itdb_id = sei->local_itdb_id;
        etr->local_track_dbid = sei->local_track_dbid;
        track->transferred = sei->transferred;
        /* don't remove the sha1-hash -- there may be duplicates... */
        if (extendedinfohash)
            g_hash_table_remove(extendedinfohash, &ipod_id);
    }
}

/* Used to free the memory of hash data */
static void hash_delete(gpointer data) {
    struct track_extended_info *sei = data;

    if (sei) {
        g_free(sei->pc_path_locale);
        g_free(sei->pc_path_utf8);
        g_free(sei->thumb_path_locale);
        g_free(sei->thumb_path_utf8);
        g_free(sei->sha1_hash);
        g_free(sei->charset);
        g_free(sei->hostname);
        g_free(sei->converted_file);
        g_free(sei->ipod_path);
        g_free(sei);
    }
}

static void destroy_extendedinfohash(void) {
    if (extendedinfohash) {
        g_hash_table_destroy(extendedinfohash);
        extendedinfohash = NULL;
    }
    if (extendedinfohash_sha1) {
        g_hash_table_destroy(extendedinfohash_sha1);
        extendedinfohash_sha1 = NULL;
    }
    if (extendeddeletion) {
        g_list_foreach(extendeddeletion, (GFunc) itdb_track_free, NULL);
        g_list_free(extendeddeletion);
        extendeddeletion = NULL;
    }
    extendedinfoversion = 0.0;
}

/* Read extended info from "name" and check if "itunes" is the
 corresponding iTunesDB (using the itunes_hash value in "name").
 The data is stored in a hash table with the ipod_id as key.  This
 hash table is used by fill_in_extended_info() (called from
 gp_import_itdb()) to fill in missing information */
/* Return TRUE on success, FALSE otherwise */
static gboolean read_extended_info(gchar *name, gchar *itunes) {
    gchar *sha1, buf[PATH_MAX];
    gboolean success = TRUE;
    gboolean expect_hash, hash_matched = FALSE;
    gint len;
    struct track_extended_info *sei = NULL;
    FILE *fp;

    g_return_val_if_fail (itunes, FALSE);

    if (!name) { /* name can be NULL if it does not exist on the iPod */
        return FALSE;
    }

    fp = fopen(name, "r");

    if (!fp) {
        /* Ideally, we'd only warn when we know we've written the extended info
         * previously...
         gtkpod_warning (_("Could not open \"%s\" for reading extended info.\n"),
         name);
         */
        return FALSE;
    }
    sha1 = sha1_hash_on_filename(itunes, FALSE);
    if (!sha1) {
        gtkpod_warning(_("Could not create hash value from itunesdb\n"));
        fclose(fp);
        return FALSE;
    }
    /* Create hash table */
    destroy_extendedinfohash();
    expect_hash = TRUE; /* next we expect the hash value (checksum) */
    while (success && fgets(buf, PATH_MAX, fp)) {
        gchar *line, *arg, *bufp;

        /* allow comments */
        if ((buf[0] == ';') || (buf[0] == '#'))
            continue;
        arg = strchr(buf, '=');
        if (!arg || (arg == buf)) {
            gtkpod_warning(_("Error while reading extended info: %s\n"), buf);
            continue;
        }
        /* skip whitespace (isblank() is a GNU extension... */
        bufp = buf;
        while ((*bufp == ' ') || (*bufp == 0x09))
            ++bufp;
        line = g_strndup(buf, arg - bufp);
        ++arg;
        len = strlen(arg); /* remove newline */
        if ((len > 0) && (arg[len - 1] == 0x0a))
            arg[len - 1] = 0;
        if (expect_hash) {
            if (g_ascii_strcasecmp(line, "itunesdb_hash") == 0) {
                if (strcmp(arg, sha1) != 0) {
                    hash_matched = FALSE;
                    gtkpod_warning(_("iTunesDB '%s' does not match checksum in extended information file '%s'\ngtkpod will try to match the information using SHA1 checksums. This may take a long time.\n\n"), itunes, name);
                    while (widgets_blocked && gtk_events_pending())
                        gtk_main_iteration();
                }
                else {
                    hash_matched = TRUE;
                }
                expect_hash = FALSE;
            }
            else {
                gtkpod_warning(_("%s:\nExpected \"itunesdb_hash=\" but got:\"%s\"\n"), name, buf);
                success = FALSE;
                g_free(line);
                break;
            }
        }
        else if (g_ascii_strcasecmp(line, "id") == 0) { /* found new id */
            if (sei) {
                if (sei->ipod_id != 0) { /* normal extended information */
                    sei->lyrics = NULL;
                    if (hash_matched) {
                        if (!extendedinfohash) {
                            extendedinfohash = g_hash_table_new_full(g_int_hash, g_int_equal, NULL, hash_delete);
                        }
                        g_hash_table_insert(extendedinfohash, &sei->ipod_id, sei);
                    }
                    else if (sei->sha1_hash) {
                        if (!extendedinfohash_sha1) {
                            extendedinfohash_sha1 = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, hash_delete);
                        }
                        g_hash_table_insert(extendedinfohash_sha1, sei->sha1_hash, sei);
                    }
                    else {
                        hash_delete((gpointer) sei);
                    }
                }
                else { /* this is a deleted track that hasn't yet been
                 removed from the iPod's hard drive */
                    Track *track = gp_track_new();
                    track->ipod_path = g_strdup(sei->ipod_path);
                    extendeddeletion = g_list_append(extendeddeletion, track);
                    hash_delete((gpointer) sei); /* free sei */
                }
                sei = NULL;
            }
            if (strcmp(arg, "xxx") != 0) {
                sei = g_malloc0(sizeof(struct track_extended_info));
                sei->ipod_id = atoi(arg);
                sei->lyrics = NULL;
            }
        }
        else if (g_ascii_strcasecmp(line, "version") == 0) { /* found version */
            extendedinfoversion = g_ascii_strtod(arg, NULL);
        }
        else if (sei == NULL) {
            gtkpod_warning(_("%s:\nFormat error: %s\n"), name, buf);
            success = FALSE;
            g_free(line);
            break;
        }
        else if (g_ascii_strcasecmp(line, "hostname") == 0)
            sei->hostname = g_strdup(arg);
        else if (g_ascii_strcasecmp(line, "converted_file") == 0)
            sei->converted_file = g_strdup(arg);
        else if (g_ascii_strcasecmp(line, "filename_locale") == 0)
            sei->pc_path_locale = g_strdup(arg);
        else if (g_ascii_strcasecmp(line, "filename_utf8") == 0)
            sei->pc_path_utf8 = g_strdup(arg);
        else if (g_ascii_strcasecmp(line, "thumbnail_locale") == 0)
            sei->thumb_path_locale = g_strdup(arg);
        else if (g_ascii_strcasecmp(line, "thumbnail_utf8") == 0)
            sei->thumb_path_utf8 = g_strdup(arg);
        else if ((g_ascii_strcasecmp(line, "md5_hash") == 0) || (g_ascii_strcasecmp(line, "sha1_hash") == 0)) { /* only accept hash value if version is >= 0.53 or
         PATH_MAX is 4096 -- in 0.53 I changed the MD5 hash
         routine to using blocks of 4096 Bytes in
         length. Before it was PATH_MAX, which might be
         different on different architectures. */
            if ((extendedinfoversion >= 0.53) || (PATH_MAX == 4096))
                sei->sha1_hash = g_strdup(arg);
        }
        else if (g_ascii_strcasecmp(line, "charset") == 0)
            sei->charset = g_strdup(arg);
        else if (g_ascii_strcasecmp(line, "transferred") == 0)
            sei->transferred = atoi(arg);
        else if (g_ascii_strcasecmp(line, "filename_ipod") == 0)
            sei->ipod_path = g_strdup(arg);
        else if (g_ascii_strcasecmp(line, "pc_mtime") == 0)
            sei->mtime = (time_t) g_ascii_strtoull(arg, NULL, 10);
        else if (g_ascii_strcasecmp(line, "local_itdb_id") == 0)
            sei->local_itdb_id = (guint64) g_ascii_strtoull(arg, NULL, 10);
        else if (g_ascii_strcasecmp(line, "local_track_dbid") == 0)
            sei->local_track_dbid = (guint64) g_ascii_strtoull(arg, NULL, 10);
        g_free(line);
    }
    g_free(sha1);
    fclose(fp);
    if (success && !hash_matched && !extendedinfohash_sha1) {
        gtkpod_warning(_("No SHA1 checksums on individual tracks are available.\n\nTo avoid this situation in the future either switch on duplicate detection (will provide SHA1 checksums) or avoid using the iPod with programs other than gtkpod.\n\n"), itunes, name);
        success = FALSE;
    }
    if (!success)
        destroy_extendedinfohash();
    return success;
}

/**
 * load_photodb:
 *
 * Using the info in the provided itunes db, load the photo db
 * from the ipod if there is one present. Reference it in the
 * extra itunes db data structure for later use.
 *
 * @ itdb: itunes database
 *
 */
static void load_photodb(iTunesDB *itdb) {
    ExtraiTunesDBData *eitdb;
    PhotoDB *db;
    const gchar *mp;
    GError *error = NULL;

    g_return_if_fail (itdb);

    if (!itdb_device_supports_photo(itdb->device))
        return;

    eitdb = itdb->userdata;
    g_return_if_fail (eitdb);
    g_return_if_fail (eitdb->photodb == NULL);

    mp = itdb_get_mountpoint(itdb);
    db = itdb_photodb_parse(mp, &error);
    if (error) {
        gtkpod_warning(_("Error reading iPod photo database (%s).\n"), error->message);
        g_error_free(error);
        error = NULL;
    }
    else if (db == NULL) {
        gtkpod_warning(_("Error reading iPod photo database. (No error message)\n"));
    }
    else {
        /* Set the reference to the photo database */
        eitdb->photodb = db;
    }
}

/* Import an iTunesDB and return an iTunesDB structure.
 * If @old_itdb is set, it will be merged into the newly imported
 * one. @old_itdb will not be changed.
 * @type: GP_ITDB_TYPE_LOCAL/IPOD (bitwise flags!)
 * @mp: mount point of iPod (if reading an iPod iTunesDB)
 * @name_off: name of the iTunesDB in offline mode
 * @name_loc: name of iTunesDB (if reading a local file browser) */
/* Return value: a new iTunesDB structure or NULL in case of an error */
iTunesDB *gp_import_itdb(iTunesDB *old_itdb, const gint type, const gchar *mp, const gchar *name_off, const gchar *name_loc) {
    gchar *cfgdir;
    GList *gl;
    Playlist *pod_pl;
    ExtraiTunesDBData *eitdb;
    iTunesDB *itdb = NULL;
    GError *error = NULL;
    gint32 total, num;
    gboolean offline;

    g_return_val_if_fail (!(type & GP_ITDB_TYPE_LOCAL) || name_loc, NULL);
    g_return_val_if_fail (!(type & GP_ITDB_TYPE_IPOD) ||
            (mp && name_off), NULL);

    cfgdir = prefs_get_cfgdir();
    g_return_val_if_fail (cfgdir, NULL);

    if (old_itdb)
        offline = get_offline(old_itdb);
    else
        offline = FALSE;

    block_widgets();
    if (offline || (type & GP_ITDB_TYPE_LOCAL)) { /* offline or local database - requires extended info */
        gchar *name_ext;
        gchar *name_db;

        if (type & GP_ITDB_TYPE_LOCAL) {
            name_ext = g_strdup_printf("%s.ext", name_loc);
            name_db = g_strdup(name_loc);
        }
        else {
            name_ext = g_strdup_printf("%s.ext", name_off);
            name_db = g_strdup(name_off);
        }

        if (g_file_test(name_db, G_FILE_TEST_EXISTS)) {
            if (WRITE_EXTENDED_INFO) {
                if (!read_extended_info(name_ext, name_db)) {
                    gchar
                            *msg =
                                    g_strdup_printf(_("The repository %s does not have a readable extended database.\n"), name_db);
                    msg
                            = g_strconcat(msg, _("This database identifies the track on disk with the track data in the repository database."), _("Any tracks already in the database cannot be transferred between repositories without the extended database."), _("A new extended database will be created upon saving but existing tracks will need to be reimported to be linked to the file on disk."), NULL);

                    gtkpod_warning(msg);
                }
            }
            itdb = itdb_parse_file(name_db, &error);
            if (itdb && !error) {
                if (type & GP_ITDB_TYPE_IPOD)
                    gtkpod_statusbar_message(_("Offline iPod database successfully imported"));
                else
                    gtkpod_statusbar_message(_("Local database successfully imported"));
            }
            else {
                if (error) {
                    if (type & GP_ITDB_TYPE_IPOD)
                        gtkpod_warning(_("Offline iPod database import failed: '%s'\n\n"), error->message);
                    else
                        gtkpod_warning(_("Local database import failed: '%s'\n\n"), error->message);
                }
                else {
                    if (type & GP_ITDB_TYPE_IPOD)
                        gtkpod_warning(_("Offline iPod database import failed: \n\n"));
                    else
                        gtkpod_warning(_("Local database import failed: \n\n"));
                }
            }
        }
        else {
            gtkpod_warning(_("'%s' does not exist. Import aborted.\n\n"), name_db);
        }
        g_free(name_ext);
        g_free(name_db);
    }
    else { /* GP_ITDB_TYPE_IPOD _and_ iPod is connected */
        gchar *name_ext = NULL, *name_db = NULL;

        name_db = itdb_get_itunesdb_path(mp);
        if (name_db) {
            name_ext = g_strdup_printf("%s.ext", name_db);

            if (WRITE_EXTENDED_INFO) {
                if (!read_extended_info(name_ext, name_db)) {
                    gtkpod_warning(_("Extended info will not be used.\n"));
                }
            }
            itdb = itdb_parse(mp, &error);
            if (itdb && !error) {
                gtkpod_statusbar_message(_("iPod Database Successfully Imported"));
            }
            else {
                if (error) {
                    gtkpod_warning(_("iPod Database Import Failed: '%s'\n\n"), error->message);
                }
                else {
                    gtkpod_warning(_("iPod Database Import Failed.\n\n"));
                }
            }
        }
        else {
            gchar *name = g_build_filename(mp, "iPod_Control", "iTunes", "iTunesDB", NULL);
            gtkpod_warning(_("'%s' (or similar) does not exist. Import aborted.\n\n"), name);
            g_free(name);
        }
        g_free(name_ext);
        g_free(name_db);
    }
    g_free(cfgdir);

    if (!itdb) {
        release_widgets();
        return NULL;
    }

    /* add Extra*Data */
    gp_itdb_add_extra_full(itdb);

    eitdb = itdb->userdata;
    g_return_val_if_fail (eitdb, (release_widgets(), NULL));

    eitdb->offline = offline;

    /* fill in additional info */
    itdb->usertype = type;
    if (type & GP_ITDB_TYPE_IPOD) {
        if (offline) {
            itdb_set_mountpoint(itdb, mp);
            g_free(itdb->filename);
            itdb->filename = NULL;
        }
        eitdb->offline_filename = g_strdup(name_off);
    }

    total = g_list_length(itdb->tracks);
    num = 1;
    /* validate all tracks and fill in extended info */
    for (gl = itdb->tracks; gl; gl = gl->next) {
        Track *track = gl->data;

        ExtraTrackData *etr;
        g_return_val_if_fail (track, (release_widgets(), NULL));
        etr = track->userdata;
        g_return_val_if_fail (etr, (release_widgets(), NULL));
        fill_in_extended_info(track, total, num);
        gp_track_validate_entries(track);
        /* properly set value for has_artwork */
        if ((track->has_artwork == 0x00) || ((track->has_artwork == 0x02) && (extendedinfoversion > 0.0)
                && (extendedinfoversion <= 0.99))) { /* if has_artwork is not set (0x00), or it has been
         (potentially wrongly) set to 0x02 by gtkpod V0.99 or
         smaller, determine the correct(?) value */
            if (itdb_track_has_thumbnails(track))
                track->has_artwork = 0x01;
            else
                track->has_artwork = 0x02;
        }

        /* set mediatype to audio if unset (important only for iPod Video) */
        if (track->mediatype == 0)
            track->mediatype = 0x00000001;
        /* restore deleted thumbnails */
        if ((!itdb_track_has_thumbnails(track)) && (strlen(etr->thumb_path_locale) != 0)) {
            /* !! gp_track_set_thumbnails() writes on
             etr->thumb_path_locale, so we need to g_strdup()
             first !! */
            gchar *filename = g_strdup(etr->thumb_path_locale);
            gp_track_set_thumbnails(track, filename);
            g_free(filename);
        }

        /* add to filename hash */
        gp_itdb_pc_path_hash_add_track(track);

        ++num;
    }
    /* take over the pending deletion information */
    while (extendeddeletion) {
        Track *track = extendeddeletion->data;
        g_return_val_if_fail (track, (release_widgets(), NULL));
        mark_track_for_deletion(itdb, track);
        extendeddeletion = g_list_delete_link(extendeddeletion, extendeddeletion);
    }

    /* delete hash information (if present) */
    destroy_extendedinfohash();

    /* find duplicates and create sha1 hash*/
    gp_sha1_hash_tracks_itdb(itdb);

    /* mark the data as unchanged */
    data_unchanged(itdb);
    /* set mark that this itdb struct contains an imported iTunesDB */
    eitdb->itdb_imported = TRUE;

    if (old_itdb) {
        /* this table holds pairs of old_itdb-tracks/new_itdb/tracks */
        ExtraiTunesDBData *old_eitdb = old_itdb->userdata;
        GHashTable *track_hash = g_hash_table_new(g_direct_hash, g_direct_equal);
        Playlist *mpl = itdb_playlist_mpl(itdb);
        g_return_val_if_fail (mpl, (release_widgets(), NULL));
        g_return_val_if_fail (old_eitdb, (release_widgets(), NULL));

        /* add tracks from @old_itdb to new itdb */
        for (gl = old_itdb->tracks; gl; gl = gl->next) {
            Track *duptr, *addtr;
            Track *track = gl->data;
            g_return_val_if_fail (track, (release_widgets(), NULL));
            duptr = itdb_track_duplicate(track);
            /* add to database -- if duplicate detection is on and the
             same track already exists in the database, the already
             existing track is returned and @duptr is freed */
            addtr = gp_track_add(itdb, duptr);
            g_hash_table_insert(track_hash, track, addtr);
            if (addtr == duptr) { /* Add to MPL */
                itdb_playlist_add_track(mpl, addtr, -1);
            }
        }
        /* add playlists */
        gl = old_itdb->playlists;
        while (gl && gl->next) {
            GList *glm;
            Playlist *duppl;
            Playlist *pl = gl->next->data; /* skip MPL */
            g_return_val_if_fail (pl, (release_widgets(), NULL));
            duppl = itdb_playlist_duplicate(pl);
            /* switch members */
            for (glm = duppl->members; glm; glm = glm->next) {
                Track *newtr;
                g_return_val_if_fail (glm->data, (release_widgets(), NULL));
                newtr = g_hash_table_lookup(track_hash, glm->data);
                g_return_val_if_fail (newtr, (release_widgets(), NULL));
                glm->data = newtr;
            }
            /* if it's the podcasts list, don't add the list again if
             it already exists, but only the members. */
            if (itdb_playlist_is_podcasts(duppl) && itdb_playlist_podcasts(itdb)) {
                Playlist *podcasts = itdb_playlist_podcasts(itdb);
                for (glm = duppl->members; glm; glm = glm->next) {
                    g_return_val_if_fail (glm->data, (release_widgets(), NULL));
                    itdb_playlist_add_track(podcasts, glm->data, -1);
                }
                itdb_playlist_free(duppl);
            }
            else {
                itdb_playlist_add(itdb, duppl, -1);
            }
            gl = gl->next;
        }
        g_hash_table_destroy(track_hash);
        /* copy data_changed flag */
        eitdb->data_changed = old_eitdb->data_changed;
    }

    /* Repair old iTunesDB where we didn't add podcasts to the MPL */
    pod_pl = itdb_playlist_podcasts(itdb);
    if (pod_pl) {
        Playlist *mpl = itdb_playlist_mpl(itdb);
        for (gl = pod_pl->members; gl; gl = gl->next) {
            Track *tr = gl->data;
            g_return_val_if_fail (tr, NULL);
            if (!itdb_playlist_contains_track(mpl, tr)) { /* track contained in Podcasts playlist but not in MPL
             -> add to MPL */
                itdb_playlist_add_track(mpl, tr, -1);
            }
        }
    }

    /* Add photo database */
    load_photodb(itdb);

    release_widgets();

    return itdb;
}

/* attempts to import all iPod databases */
void gp_load_ipods(void) {
    struct itdbs_head *itdbs_head;
    GList *gl;

    itdbs_head = gp_get_itdbs_head();
    g_return_if_fail (itdbs_head);

    for (gl = itdbs_head->itdbs; gl; gl = gl->next) {
        iTunesDB *itdb = gl->data;
        ExtraiTunesDBData *eitdb;
        g_return_if_fail (itdb);
        eitdb = itdb->userdata;
        g_return_if_fail (eitdb);
        if ((itdb->usertype & GP_ITDB_TYPE_IPOD) && !eitdb->itdb_imported) {
            gp_load_ipod(itdb);
        }
    }
}

/*
 * Merges @old_itdb with a newly imported one, then replaces @old_itdb
 * in the itdbs-list and the display.
 *
 * old_itdb->usertype, ->mountpoint, ->filename,
 * ->eitdb->offline_filename must be set according to usertype and
 * will be used to read the new itdb
 *
 * Return value: pointer to the new repository
 */
static iTunesDB *gp_merge_itdb(iTunesDB *old_itdb) {
    ExtraiTunesDBData *old_eitdb;
    iTunesDB *new_itdb;

    g_return_val_if_fail (old_itdb, NULL);
    old_eitdb = old_itdb->userdata;
    g_return_val_if_fail (old_eitdb, NULL);

    if (old_itdb->usertype & GP_ITDB_TYPE_LOCAL) {
        g_return_val_if_fail (old_itdb->filename, NULL);

        new_itdb = gp_import_itdb(old_itdb, old_itdb->usertype, NULL, NULL, old_itdb->filename);
    }
    else if (old_itdb->usertype & GP_ITDB_TYPE_IPOD) {
        const gchar *mountpoint = itdb_get_mountpoint(old_itdb);
        g_return_val_if_fail (mountpoint, NULL);
        g_return_val_if_fail (old_eitdb->offline_filename, NULL);

        new_itdb = gp_import_itdb(old_itdb, old_itdb->usertype, mountpoint, old_eitdb->offline_filename, NULL);
    }
    else {
        g_return_val_if_reached (NULL);
    }

    if (new_itdb) {
        gp_replace_itdb(old_itdb, new_itdb);
        /* take care of autosync... */
        sync_all_playlists(new_itdb);

        /* update all live SPLs */
        itdb_spl_update_live(new_itdb);
    }

    gtkpod_tracks_statusbar_update();

    return new_itdb;
}

/**
 * gp_load_ipod: loads the contents of an iPod into @itdb. If data
 * already exists in @itdb, data is merged.
 *
 * If new countrate and rating information is available, this
 * information is adjusted within the local databases as well if the
 * track can be identified in the local databases.
 *
 * @itdb: repository to load iPod contents into. mountpoint must be
 * set, and the iPod must not be loaded already
 * (eitdb->itdb_imported).
 *
 * Return value: the new repository holding the contents of the iPod.
 */
iTunesDB *gp_load_ipod(iTunesDB *itdb) {
    ExtraiTunesDBData *eitdb;
    iTunesDB *new_itdb = NULL;
    gchar *mountpoint;
    gchar *itunesdb;
    gboolean ok_to_load = TRUE;

    g_return_val_if_fail (itdb, NULL);
    g_return_val_if_fail (itdb->usertype & GP_ITDB_TYPE_IPOD, NULL);
    eitdb = itdb->userdata;
    g_return_val_if_fail (eitdb, NULL);
    g_return_val_if_fail (eitdb->itdb_imported == FALSE, NULL);

    mountpoint = get_itdb_prefs_string(itdb, KEY_MOUNTPOINT);
    call_script("gtkpod.load", mountpoint, NULL);

    /* read preferences keys from the iPod if available */
    load_ipod_prefs(itdb, mountpoint);

    itdb_device_set_mountpoint(itdb->device, mountpoint);

    itunesdb = itdb_get_itunesdb_path(mountpoint);
    if (!itunesdb) {
        gchar *displaymp = g_uri_unescape_string(mountpoint, NULL);
        gchar
                *str =
                        g_strdup_printf(_("Could not find iPod directory structure at '%s'.\n\nIf you are sure that the iPod is properly mounted at '%s', it may not be initialized for use. In this case, gtkpod can initialize it for you.\n\nDo you want to create the directory structure now?"), displaymp, displaymp);

        gint
                result =
                        gtkpod_confirmation_simple(GTK_MESSAGE_WARNING, _("iPod directory structure not found"), str, _("Create directory structure"));

        g_free(str);
        g_free(displaymp);

        if (result == GTK_RESPONSE_OK) {
            ok_to_load = gtkpod_init_repository(itdb);
        }
        else {
            ok_to_load = FALSE;
        }
    }
    g_free(itunesdb);
    g_free(mountpoint);

    if (ok_to_load) {
        gchar *prefs_model = get_itdb_prefs_string(itdb, KEY_IPOD_MODEL);
        gchar *ipod_model = itdb_device_get_sysinfo(itdb->device, "ModelNumStr");
        if (!prefs_model && ipod_model) { /* let's believe what the iPod says */
            set_itdb_prefs_string(itdb, KEY_IPOD_MODEL, ipod_model);
        }
        else if (prefs_model && !ipod_model) { /* verify with the user if the model is correct --
         * incorrect mdoel information can result in loss of
         * Artwork */
            gtkpod_populate_repository_model(itdb, prefs_model);
            /* write out new SysInfo file -- otherwise libpod won't
             use it. Ignore error for now. */
            itdb_device_write_sysinfo(itdb->device, NULL);
        }
        else if (!prefs_model && !ipod_model) {
            /* ask the user to set the model information */
            gtkpod_populate_repository_model(itdb, NULL);
            /* write out new SysInfo file -- otherwise libpod won't
             use it. Ignore error for now. */
            itdb_device_write_sysinfo(itdb->device, NULL);
        }
        else { /* prefs_model && ipod_model are set */
            const gchar *prefs_ptr = prefs_model;
            const gchar *ipod_ptr = ipod_model;
            /* Normalize model number */
            if (isalpha (prefs_model[0]))
                ++prefs_ptr;
            if (isalpha (ipod_model[0]))
                ++ipod_ptr;
            if (strcmp(prefs_ptr, ipod_ptr) != 0) { /* Model number is different -- confirm */
                gtkpod_populate_repository_model(itdb, ipod_model);
                /* write out new SysInfo file -- otherwise libpod won't
                 use it. Ignore error for now. */
                itdb_device_write_sysinfo(itdb->device, NULL);
            }
        }
        g_free(prefs_model);
        g_free(ipod_model);

        new_itdb = gp_merge_itdb(itdb);

        if (new_itdb) {
            GList *gl;
            gchar *new_model = itdb_device_get_sysinfo(new_itdb->device, "ModelNumStr");

            if (!new_model) { /* Something went wrong with setting the ipod model
             * above */
                prefs_model = get_itdb_prefs_string(new_itdb, KEY_IPOD_MODEL);
                if (prefs_model) {
                    itdb_device_set_sysinfo(new_itdb->device, "ModelNumStr", prefs_model);
                }
                else { /* ask again... */
                    gtkpod_populate_repository_model(new_itdb, NULL);
                }
                g_free(prefs_model);
            }
            g_free(new_model);

            /* adjust rating and playcount in local databases */
            for (gl = new_itdb->tracks; gl; gl = gl->next) {
                Track *itr = gl->data;
                g_return_val_if_fail (itr, new_itdb);
                if ((itr->recent_playcount != 0) || (itr->app_rating != itr->rating)) {
                    GList *gl;
                    GList *tracks = gp_itdb_find_same_tracks_in_local_itdbs(itr);
                    for (gl = tracks; gl; gl = gl->next) {
                        Track *ltr = gl->data;
                        g_return_val_if_fail (ltr, new_itdb);

                        if (itr->recent_playcount != 0) {
                            ltr->playcount += itr->recent_playcount;
                            ltr->recent_playcount += itr->recent_playcount;
                        }
                        if (itr->rating != itr->app_rating) {
                            ltr->app_rating = ltr->rating;
                            ltr->rating = itr->rating;
                        }
                        gtkpod_track_updated(ltr);
                        data_changed(ltr->itdb);
                    }
                    g_list_free(tracks);
                }
            }
        }
    }

    return new_itdb;
}

/**
 * gp_eject_ipod: store @itdb and call ~/.gtkpod/gtkpod.eject with the
 * mountpoint as parameter. Then @itdb is deleted and replaced with an
 * empty version. eitdb->ejected is set.
 *
 * @itdb: must be an iPod itdb (eject does not make sense otherwise)
 *
 * Return value: TRUE if saving was successful, FALSE otherwise.
 */
gboolean gp_eject_ipod(iTunesDB *itdb) {

    g_return_val_if_fail (itdb, FALSE);
    g_return_val_if_fail (itdb->usertype & GP_ITDB_TYPE_IPOD, FALSE);

    if (gp_save_itdb(itdb)) {
        gint index;
        gchar *mountpoint;

        mountpoint = get_itdb_prefs_string(itdb, KEY_MOUNTPOINT);

        /* save prefs relevant for this iPod to the iPod */
        save_ipod_prefs(itdb, mountpoint);

        call_script("gtkpod.eject", mountpoint, FALSE);
        g_free(mountpoint);

        index = get_itdb_index(itdb);

        if (itdb->usertype & GP_ITDB_TYPE_AUTOMATIC) { /* remove itdb from display */
            remove_itdb_prefs(itdb);
            gp_itdb_remove(itdb);
            gp_itdb_free(itdb);
        }
        else { /* switch to an empty itdb */
            iTunesDB *new_itdb = setup_itdb_n(index);
            if (new_itdb) {
                ExtraiTunesDBData *new_eitdb;

                new_eitdb = new_itdb->userdata;
                g_return_val_if_fail (new_eitdb, TRUE);

                gp_replace_itdb(itdb, new_itdb);

                new_eitdb->ipod_ejected = TRUE;
            }
        }
        return TRUE;
    }
    return FALSE;
}

/**
 * gp_save_itdb: Save a repository after updating smart playlists. If
 * the repository is an iPod, contacts, notes and calendar are also
 * updated.
 *
 * @itdb: repository to save
 *
 * return value: TRUE on succes, FALSE when an error occurred.
 */
gboolean gp_save_itdb(iTunesDB *itdb) {
    Playlist *pl;
    gboolean success;
    g_return_val_if_fail (itdb, FALSE);

    if (itdb->usertype & GP_ITDB_TYPE_IPOD) { /* handle conversions for this repository with priority */
        file_convert_itdb_first(itdb);
    }

    /* update smart playlists before writing */
    itdb_spl_update_live(itdb);
    pl = gtkpod_get_current_playlist();
    if (pl && (pl->itdb == itdb) && pl->is_spl && pl->splpref.liveupdate) { /* Update display if necessary */
        gtkpod_set_current_playlist(pl);
    }

    success = gp_write_itdb(itdb);

    if (itdb->usertype & GP_ITDB_TYPE_IPOD) {
        if (get_itdb_prefs_int(itdb, "concal_autosync")) {
            tools_sync_all(itdb);
        }
    }

    return success;
}

/*------------------------------------------------------------------*\
 *                                                                  *
 *      Functions concerning deletion of tracks                      *
 *                                                                  *
 \*------------------------------------------------------------------*/

/* Fills in @size with the size in bytes taken on the iPod or the
 * local harddisk with files to be deleted, @num with the number of
 * tracks to be deleted. */
void gp_info_deleted_tracks(iTunesDB *itdb, gdouble *size, guint32 *num) {
    ExtraiTunesDBData *eitdb;
    GList *gl;

    if (size)
        *size = 0;
    if (num)
        *num = 0;

    g_return_if_fail (itdb);
    eitdb = itdb->userdata;
    g_return_if_fail (eitdb);

    for (gl = eitdb->pending_deletion; gl; gl = gl->next) {
        ExtraTrackData *etr;
        Track *tr = gl->data;
        g_return_if_fail (tr);
        etr = tr->userdata;
        g_return_if_fail (tr);

        if (size)
            *size += tr->size;
        if (num)
            *num += 1;
    }
}

/* Adds @track to the list of tracks to be deleted. The following
 information is required:
 - userdata with ExtraTrackData
 - size of track to be deleted
 - either track->ipod_path or etr->pc_path_local */
void mark_track_for_deletion(iTunesDB *itdb, Track *track) {
    ExtraiTunesDBData *eitdb;
    g_return_if_fail (itdb && itdb->userdata && track);
    g_return_if_fail (track->itdb == NULL);
    eitdb = itdb->userdata;

    eitdb->pending_deletion = g_list_append(eitdb->pending_deletion, track);
}

/*------------------------------------------------------------------*\
 *                                                                  *
 *      Handle Export of iTunesDB                                   *
 *                                                                  *
 \*------------------------------------------------------------------*/

/* Writes extended info (sha1 hash, PC-filename...) of @itdb into file
 * @itdb->filename+".ext". @itdb->filename will also be used to
 * calculate the sha1 checksum of the corresponding iTunesDB */
static gboolean write_extended_info(iTunesDB *itdb) {
    ExtraiTunesDBData *eitdb;
    FILE *fp;
    gchar *sha1;
    GList *gl;
    gchar *name;

    g_return_val_if_fail (itdb, FALSE);
    g_return_val_if_fail (itdb->filename, FALSE);
    eitdb = itdb->userdata;
    g_return_val_if_fail (eitdb, FALSE);

    name = g_strdup_printf("%s.ext", itdb->filename);
    fp = fopen(name, "w");
    if (!fp) {
        gtkpod_warning(_("Could not open \"%s\" for writing extended info.\n"), name);
        g_free(name);
        return FALSE;
    }
    g_free(name);
    name = NULL;
    sha1 = sha1_hash_on_filename(itdb->filename, FALSE);
    if (sha1) {
        fprintf(fp, "itunesdb_hash=%s\n", sha1);
        g_free(sha1);
    }
    else {
        gtkpod_warning(_("Aborted writing of extended info.\n"));
        fclose(fp);
        return FALSE;
    }
    fprintf(fp, "version=%s\n", VERSION);
    for (gl = itdb->tracks; gl; gl = gl->next) {
        Track *track = gl->data;
        ExtraTrackData *etr;
        g_return_val_if_fail (track, (fclose (fp), FALSE));
        etr = track->userdata;
        g_return_val_if_fail (etr, (fclose (fp), FALSE));
        fprintf(fp, "id=%d\n", track->id);
        if (etr->hostname)
            fprintf(fp, "hostname=%s\n", etr->hostname);
        if (etr->converted_file)
            fprintf(fp, "converted_file=%s\n", etr->converted_file);
        if (etr->pc_path_locale && *etr->pc_path_locale)
            fprintf(fp, "filename_locale=%s\n", etr->pc_path_locale);
        if (etr->pc_path_utf8 && *etr->pc_path_utf8)
            fprintf(fp, "filename_utf8=%s\n", etr->pc_path_utf8);
        if (etr->thumb_path_locale && *etr->thumb_path_locale)
            fprintf(fp, "thumbnail_locale=%s\n", etr->thumb_path_locale);
        if (etr->thumb_path_utf8 && *etr->thumb_path_utf8)
            fprintf(fp, "thumbnail_utf8=%s\n", etr->thumb_path_utf8);
        /* this is just for convenience for people looking for a track
         on the ipod away from gktpod/itunes etc. */
        if (track->ipod_path && strlen(track->ipod_path) != 0)
            fprintf(fp, "filename_ipod=%s\n", track->ipod_path);
        if (etr->sha1_hash && *etr->sha1_hash)
            fprintf(fp, "sha1_hash=%s\n", etr->sha1_hash);
        if (etr->charset && *etr->charset)
            fprintf(fp, "charset=%s\n", etr->charset);
        if (etr->mtime)
            fprintf(fp, "pc_mtime=%llu\n", (unsigned long long) etr->mtime);
        if (etr->local_itdb_id)
            fprintf(fp, "local_itdb_id=%" G_GUINT64_FORMAT "\n", etr->local_itdb_id);
        if (etr->local_track_dbid)
            fprintf(fp, "local_track_dbid=%" G_GUINT64_FORMAT "\n", etr->local_track_dbid);
        fprintf(fp, "transferred=%d\n", track->transferred);
        while (widgets_blocked && gtk_events_pending())
            gtk_main_iteration();
    }
    if (get_offline(itdb)) { /* we are offline and also need to export the list of tracks that
     are to be deleted */
        for (gl = eitdb->pending_deletion; gl; gl = gl->next) {
            Track *track = gl->data;
            g_return_val_if_fail (track, (fclose (fp), FALSE));

            fprintf(fp, "id=000\n"); /* our sign for tracks pending
             deletion */
            fprintf(fp, "filename_ipod=%s\n", track->ipod_path);
            while (widgets_blocked && gtk_events_pending())
                gtk_main_iteration();
        }
    }
    fprintf(fp, "id=xxx\n");
    fclose(fp);
    return TRUE;
}

TransferData *transfer_data_new(void) {
    TransferData *transfer_data;
    transfer_data = g_new0 (TransferData, 1);
    transfer_data->mutex = g_mutex_new ();
    transfer_data->finished_cond = g_cond_new ();
    return transfer_data;
}

void transfer_data_free(TransferData *transfer_data) {
    if (transfer_data->mutex)
        g_mutex_free (transfer_data->mutex);
    if (transfer_data->finished_cond)
        g_cond_free (transfer_data->finished_cond);
    g_free(transfer_data);
}

/* Threaded remove file */
/* returns: int result (of remove()) */
static gpointer th_remove(gpointer userdata) {
    TransferData *td = userdata;
    gint result;

    result = g_remove(td->filename);
    g_mutex_lock (td->mutex);
    td->finished = TRUE; /* signal that thread will end */
    g_cond_signal (td->finished_cond);
    g_mutex_unlock (td->mutex);
    return GINT_TO_POINTER(result);
}

/* This function is called when the user presses the abort button
 * during transfer_tracks() or delete_tracks() */
static void file_dialog_abort(TransferData *transfer_data) {
    g_return_if_fail (transfer_data);

    g_mutex_lock (transfer_data->mutex);

    transfer_data->abort = TRUE;

    g_mutex_unlock (transfer_data->mutex);
}

/* This function is called when the user closes the window */
static gboolean file_dialog_delete(TransferData *transfer_data) {
    file_dialog_abort(transfer_data);
    return TRUE; /* don't close the window -- let our own code take care of this */
}

/* check if iPod directory stucture is present */
static gboolean ipod_dirs_present(const gchar *mountpoint) {
    gchar *file;
    gchar *dir;
    gboolean result = TRUE;

    g_return_val_if_fail (mountpoint, FALSE);

    dir = itdb_get_music_dir(mountpoint);
    if (!dir)
        return FALSE;

    file = itdb_get_path(dir, "F00");
    if (!file || !g_file_test(file, G_FILE_TEST_IS_DIR))
        result = FALSE;
    g_free(file);
    g_free(dir);

    dir = itdb_get_itunes_dir(mountpoint);
    if (!dir || !g_file_test(dir, G_FILE_TEST_IS_DIR))
        result = FALSE;
    g_free(dir);

    return result;
}

static GtkWidget *create_transfer_information_dialog(TransferData *td) {
    GladeXML *dialog_xml;
    GtkWidget *dialog, *widget;

    dialog_xml = gtkpod_xml_new(gtkpod_get_glade_xml(), "file_transfer_information_dialog");
    glade_xml_signal_autoconnect(dialog_xml);

    dialog = gtkpod_xml_get_widget(dialog_xml, "file_transfer_information_dialog");
    g_return_val_if_fail (dialog, NULL);

    /* the window itself */
    td->dialog = dialog;

    /* text label */
    td->textlabel = gtkpod_xml_get_widget(dialog_xml, "textlabel");

    /* progress bar */
    td->progressbar = GTK_PROGRESS_BAR (
            gtkpod_xml_get_widget (dialog_xml, "progressbar"));

    /* Indicate that user wants to abort */
    widget = gtkpod_xml_get_widget(dialog_xml, "abortbutton");
    g_signal_connect_swapped (GTK_OBJECT (widget), "clicked",
            G_CALLBACK (file_dialog_abort),
            td);

    /* User tried to close the window */
    g_signal_connect_swapped (GTK_OBJECT (dialog), "delete-event",
            G_CALLBACK (file_dialog_delete),
            td);

    /* Set the dialog parent */
    gtk_window_set_transient_for(GTK_WINDOW (dialog), GTK_WINDOW (gtkpod_app));

    return dialog;
}

static void set_progressbar(GtkProgressBar *progressbar, time_t start, gint n, gint count, gint init_count) {
    gchar *progtext;
    const gchar *progtext_old;
    gdouble fraction, fraction_old;

    g_return_if_fail (progressbar);

    if (n == 0) {
        fraction = 1;
    }
    else {
        fraction = (gdouble) count / n;
    }

    if (count - init_count == 0) {
        progtext = g_strdup_printf(_("%d%%"), (gint) (fraction * 100));
    }
    else {
        time_t diff, fullsecs, hrs, mins, secs;

        diff = time(NULL) - start;
        fullsecs = (diff * (n - init_count) / (count - init_count)) - diff + 5;
        hrs = fullsecs / 3600;
        mins = (fullsecs % 3600) / 60;
        secs = ((fullsecs % 60) / 5) * 5;
        /* don't bounce up too quickly (>10% change only) */
        /* left = ((mins < left) || (100*mins >= 110*left)) ? mins : left;*/
        progtext
                = g_strdup_printf(_("%d%% (%d/%d  %d:%02d:%02d left)"), (gint) (fraction * 100), count, n, (gint) hrs, (gint) mins, (gint) secs);
    }

    progtext_old = gtk_progress_bar_get_text(progressbar);
    if (!progtext_old || (strcmp(progtext_old, progtext) != 0)) { /* only update progressbar text if it has changed */
        gtk_progress_bar_set_text(progressbar, progtext);
    }

    fraction_old = gtk_progress_bar_get_fraction(progressbar);
    if (fraction_old != fraction) { /* only update progressbar fraction if it has changed */
        gtk_progress_bar_set_fraction(progressbar, fraction);
    }

    g_free(progtext);
}

/* Removes all tracks that were marked for deletion from the iPod or
 the local harddisk (for itdb->usertype == GP_ITDB_TYPE_LOCAL) */
/* Returns TRUE on success, FALSE if some error occurred and not all
 files were removed */
static gboolean delete_files(iTunesDB *itdb, TransferData *td) {
    gboolean result = TRUE;
    gint n, count;
    time_t start;
    ExtraiTunesDBData *eitdb;
    GThread *thread = NULL;

    g_return_val_if_fail (td, FALSE);

    g_return_val_if_fail (itdb, FALSE);
    eitdb = itdb->userdata;
    g_return_val_if_fail (eitdb, FALSE);

    if (!eitdb->pending_deletion) {
        return TRUE;
    }

    if (itdb->usertype & GP_ITDB_TYPE_IPOD) {
        g_return_val_if_fail (itdb_get_mountpoint (itdb), FALSE);
    }

    /* stop file transfer while we're deleting to avoid time-consuming
     interference with slow access harddisks */
    file_transfer_activate(itdb, FALSE);

    gtk_label_set_text(GTK_LABEL (td->textlabel), _("Status: Deleting File"));

    n = g_list_length(eitdb->pending_deletion);
    count = 0; /* number of tracks removed */
    start = time(NULL); /* start time for progress bar */

    /* lets clean up those pending deletions */
    while (!td->abort && eitdb->pending_deletion) {
        gchar *filename = NULL;
        Track *track = eitdb->pending_deletion->data;
        g_return_val_if_fail (track, FALSE);

        track->itdb = itdb;
        if (itdb->usertype & GP_ITDB_TYPE_IPOD) {
            filename = get_file_name_from_source(track, SOURCE_IPOD);
        }
        if (itdb->usertype & GP_ITDB_TYPE_LOCAL) {
            filename = get_file_name_from_source(track, SOURCE_LOCAL);
        }
        track->itdb = NULL;

        if (filename) {
            gint rmres;

            td->finished = FALSE;
            td->filename = filename;

            g_mutex_lock (td->mutex);

            thread = g_thread_create (th_remove, td, TRUE, NULL);

            do {
                GTimeVal gtime;

                set_progressbar(td->progressbar, start, n, count, 0);

                g_mutex_unlock (td->mutex);

                while (widgets_blocked && gtk_events_pending())
                    gtk_main_iteration();

                g_mutex_lock (td->mutex);

                /* wait a maximum of 20 ms or until cond is signaled */
                g_get_current_time(&gtime);
                g_time_val_add(&gtime, 20000);
                g_cond_timed_wait (td->finished_cond,
                        td->mutex, &gtime);
            }
            while (!td->finished);

            g_mutex_unlock (td->mutex);

            rmres = GPOINTER_TO_INT(g_thread_join (thread));

            if (rmres == -1) {
                gtkpod_warning(_("Could not remove the following file: '%s'\n\n"), filename);

                while (widgets_blocked && gtk_events_pending())
                    gtk_main_iteration();
            }

            g_free(filename);
        }
        ++count;
        itdb_track_free(track);
        eitdb->pending_deletion = g_list_delete_link(eitdb->pending_deletion, eitdb->pending_deletion);
    }

    set_progressbar(td->progressbar, start, n, count, 0);

    while (widgets_blocked && gtk_events_pending())
        gtk_main_iteration();

    if (td->abort)
        result = FALSE;

    file_transfer_reset(itdb);

    return result;
}

/* Reschedule tracks that failed during transfer. This is a hack as
 * the @itdb could have been removed in the meanwhile. The clean
 * solution would be to integrate the error display into the
 * file_convert.c framework */
static void transfer_reschedule(gpointer user_data1, gpointer user_data2) {
    struct itdbs_head *ihead = gp_get_itdbs_head(gtkpod_app);
    iTunesDB *itdb = user_data1;
    GList *gl;

    g_return_if_fail (itdb && ihead);

    for (gl = ihead->itdbs; gl; gl = gl->next) {
        iTunesDB *it = gl->data;
        g_return_if_fail (it);
        if (it == itdb) { /* itdb is still valid --> reschedule tracks */
            file_transfer_reschedule(itdb);
            break;
        }
    }
}

/* Show an error message that not all tracks were transferred */
static void transfer_tracks_show_failed(iTunesDB *itdb, TransferData *td) {
    GString *string_transfer, *string_convert, *string;
    gint failed_transfer, failed_conversion;
    GList *tracks, *gl;

    g_return_if_fail (itdb && td);

    gtk_widget_hide(td->dialog);

    string_transfer = g_string_sized_new(1000);
    string_convert = g_string_sized_new(1000);
    string = g_string_sized_new(1000);
    failed_transfer = 0;
    failed_conversion = 0;

    tracks = file_transfer_get_failed_tracks(itdb);
    /* since failed_num is not 0, tracks cannot be empty */
    g_return_if_fail (tracks);
    /* Add information about failed tracks to the respective
     string */
    for (gl = tracks; gl; gl = gl->next) {
        ExtraTrackData *etr;
        gchar *buf;
        Track *tr = gl->data;
        g_return_if_fail (tr && tr->userdata);

        etr = tr->userdata;
        etr->lyrics = NULL;

        buf = get_track_info(tr, FALSE);

        switch (etr->conversion_status) {
        case FILE_CONVERT_INACTIVE:
        case FILE_CONVERT_CONVERTED:
            /* This track was converted successfully (or did not
             * neeed conversion) and failed during transfer */
            ++failed_transfer;
            g_string_append_printf(string_transfer, "%s\n", buf);
            break;
        default:
            /* These tracks failed during conversion */
            ++failed_conversion;
            g_string_append_printf(string_convert, "%s\n", buf);
            break;
        }
        g_free(buf);
    }

    if (failed_conversion != 0) {
        g_string_append(string, ngettext("The following track could not be converted successfully:\n\n", "The following tracks could not be converted successfully:\n\n", failed_conversion));
        g_string_append(string, string_convert->str);
        g_string_append(string, "\n\n");
    }

    if (failed_transfer != 0) {
        g_string_append(string, ngettext("The following track could not be transferred successfully:\n\n", "The following tracks could not be transferred successfully:\n\n", failed_transfer));
        g_string_append(string, string_transfer->str);
        g_string_append(string, "\n\n");
    }

    gtkpod_confirmation(CONF_ID_TRANSFER, /* ID     */
    FALSE, /* modal, */
    _("Warning"), /* title  */
    _("The iPod could not be ejected. Please fix the problems mentioned below and then eject the iPod again. Pressing 'OK' will re-schedule the failed tracks for conversion and transfer."), string->str, /* text to be displayed */
    NULL, 0, NULL, /* option 1 */
    NULL, 0, NULL, /* option 2 */
    TRUE, /* gboolean confirm_again, */
    NULL, /* confirm_again_key, */
    transfer_reschedule, /* ConfHandler ok_handler,*/
    NULL, /* don't show "Apply" */
    CONF_NULL_HANDLER, /* cancel_handler,*/
    itdb, /* gpointer user_data1,*/
    NULL); /* gpointer user_data2,*/

    g_string_free(string_transfer, TRUE);
    g_string_free(string_convert, TRUE);
    g_string_free(string, TRUE);
}

/* Initiates and waits for transfer of tracks to the iPod */
static gboolean transfer_tracks(iTunesDB *itdb, TransferData *td) {
    gint to_convert_num, converting_num, to_transfer_num;
    gint transferred_num, failed_num, transferred_init;
    gboolean result = TRUE;
    FileTransferStatus status;
    ExtraiTunesDBData *eitdb;
    time_t start;

    g_return_val_if_fail (itdb && td, FALSE);
    eitdb = itdb->userdata;
    g_return_val_if_fail (eitdb, FALSE);

    /* make sure background transfer is running */
    file_transfer_activate(itdb, TRUE);
    file_transfer_continue(itdb);
    /* reschedule previously failed tracks */
    file_transfer_reschedule(itdb);

    /* find out how many tracks have already been processed */
    file_transfer_get_status(itdb, NULL, NULL, NULL, &transferred_num, &failed_num);
    transferred_init = transferred_num + failed_num;

    start = time(NULL);

    do {
        gchar *buf;
        const gchar *buf_old;

        status
                = file_transfer_get_status(itdb, &to_convert_num, &converting_num, &to_transfer_num, &transferred_num, &failed_num);

        set_progressbar(td->progressbar, start, to_convert_num + to_transfer_num + failed_num + transferred_num, transferred_num
                + failed_num, transferred_init);

        if (to_transfer_num > 0) {
            buf = g_strdup_printf(_("Status: Copying track"));
        }
        else {
            if ((to_convert_num + converting_num) > 0) {
                buf = g_strdup_printf(_("Status: Waiting for conversion to complete"));
            }
            else {
                buf = g_strdup_printf(_("Status: Finished transfer"));
            }
        }

        /*	buf = g_strdup_printf (_("Status: %d. To convert: %d. To transfer: %d\n"
         "Transferred: %d. Failed: %d"),
         status, to_convert_num, to_transfer_num,
         transferred_num, failed_num);*/
        buf_old = gtk_label_get_text(GTK_LABEL(td->textlabel));
        if (!buf_old || (strcmp(buf_old, buf) != 0)) { /* only set label if it has changed */
            gtk_label_set_text(GTK_LABEL(td->textlabel), buf);
        }
        g_free(buf);

        if ((to_convert_num != 0) && (converting_num == 0)) { /* Force the conversion to continue. Not sure if this scenario
         * is likely to happen, but better be safe then sorry */
            file_convert_continue();
        }

        while (widgets_blocked && gtk_events_pending())
            gtk_main_iteration();

        /* sleep 20 ms */
        g_usleep(20 * 1000);
    }
    while (!td->abort && (status != FILE_TRANSFER_DISK_FULL) && (to_convert_num + to_transfer_num) > 0);

    /* reset background transfer to value in prefs */
    file_transfer_reset(itdb);

    if (td->abort) {
        result = FALSE;
    }
    else if (status == FILE_TRANSFER_DISK_FULL) {
        gchar *buf;
        GtkWidget *dialog;

        gtk_widget_hide(td->dialog);

        buf
                = g_strdup_printf(ngettext (
                        "One track could not be transferred because your iPod is full. Either delete some tracks or otherwise create space on the iPod before ejecting the iPod again.",
                        "%d tracks could not be transferred because your iPod is full. Either delete some tracks or otherwise create space on the iPod before ejecting the iPod again.", to_transfer_num), to_transfer_num);

        dialog
                = gtk_message_dialog_new(GTK_WINDOW (gtkpod_app), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "%s", buf);
        gtk_dialog_run(GTK_DIALOG (dialog));
        gtk_widget_destroy(dialog);
        g_free(buf);
        result = FALSE;
    }
    else if (failed_num != 0) /* one error message is enough -> else{... */
    {
        transfer_tracks_show_failed(itdb, td);
        result = FALSE;
    }

    if (result == TRUE) {
        /* remove transferred tracks from list so they won't be removed
         when deleting the itdb */
        file_transfer_ack_itdb(itdb);
    }

    return result;
}

static gboolean gp_write_itdb(iTunesDB *itdb) {
    gchar *cfgdir;
    gboolean success = TRUE;
    ExtraiTunesDBData *eitdb;
    GtkWidget *dialog;
    Playlist *mpl;
    TransferData *transferdata;
    GList *it;

    g_return_val_if_fail (itdb, FALSE);
    eitdb = itdb->userdata;
    g_return_val_if_fail (eitdb, FALSE);

    cfgdir = prefs_get_cfgdir();
    g_return_val_if_fail (cfgdir, FALSE);

    mpl = itdb_playlist_mpl(itdb);
    g_return_val_if_fail (mpl, FALSE);

    if (!eitdb->itdb_imported) { /* No iTunesDB was read but user wants to export current
     data. If an iTunesDB is present on the iPod or in cfgdir,
     this is most likely an error. We should tell the user */
        gchar *tunes = NULL;
        /* First check if we can find an existing iTunesDB. */
        if (itdb->usertype & GP_ITDB_TYPE_LOCAL) {
            tunes = g_strdup(itdb->filename);
        }
        else if (itdb->usertype & GP_ITDB_TYPE_IPOD) {
            if (get_offline(itdb)) {
                tunes = g_strdup(eitdb->offline_filename);
            }
            else {
                const gchar *mountpoint = itdb_get_mountpoint(itdb);
                g_return_val_if_fail (mountpoint, FALSE);
                tunes = itdb_get_itunesdb_path(mountpoint);
            }
        }
        else {
            g_free(cfgdir);
            g_return_val_if_reached (FALSE);
        }
        if (g_file_test(tunes, G_FILE_TEST_EXISTS)) {
            gchar
                    *str =
                            g_strdup_printf(_("You did not import the existing iTunesDB ('%s'). This is most likely incorrect and will result in the loss of the existing database.\n\nIf you skip storing, you can import the existing database before calling this function again.\n"), tunes);

            gint
                    result =
                            gtkpod_confirmation_hig(GTK_MESSAGE_WARNING, _("Existing iTunes database not imported"), str, _("Proceed anyway"), _("Skip storing"), NULL, NULL);

            g_free(str);

            if (result == GTK_RESPONSE_CANCEL) {
                g_free(cfgdir);
                return FALSE;
            }
        }
    }

    block_widgets();

    transferdata = transfer_data_new();
    dialog = create_transfer_information_dialog(transferdata);
    gtk_widget_show(dialog);

    if ((itdb->usertype & GP_ITDB_TYPE_IPOD) && !get_offline(itdb)) {
        const gchar *mountpoint = itdb_get_mountpoint(itdb);
        g_return_val_if_fail (mountpoint, FALSE);
        /* check if iPod directories are present */
        if (!ipod_dirs_present(mountpoint)) { /* no -- create them */
            gtkpod_init_repository(itdb);
            /* if still not present abort */
            if (!ipod_dirs_present(mountpoint)) {
                gtkpod_warning(_("iPod directory structure must be present before synching to the iPod can be performed.\n"));
                success = FALSE;
            }
        }
        if (success) { /* remove deleted files */
            success = delete_files(itdb, transferdata);
            if (!success) {
                gtkpod_warning(_("Some tracks could not be deleted from the iPod. Export aborted!"));
            }
        }
        if (success) {
            /* write tracks to iPod */
            success = transfer_tracks(itdb, transferdata);
        }
    }

    if (itdb->usertype & GP_ITDB_TYPE_LOCAL) {
        success = delete_files(itdb, transferdata);
    }

    if (success) {
        gchar *buf;
        buf = g_strdup_printf(_("Now writing database '%s'. Please wait..."), mpl->name);
        gtk_label_set_text(GTK_LABEL (transferdata->textlabel), buf);
        g_free(buf);

        while (widgets_blocked && gtk_events_pending())
            gtk_main_iteration();
    }

    for (it = itdb->tracks; it != NULL; it = it->next) {
        gp_track_cleanup_empty_strings((Itdb_Track *) it->data);
    }

    if (success && !get_offline(itdb) && (itdb->usertype & GP_ITDB_TYPE_IPOD)) { /* write to the iPod */
        GError *error = NULL;
        if (!itdb_write(itdb, &error)) { /* an error occurred */
            success = FALSE;
            if (error && error->message)
                gtkpod_warning("%s\n\n", error->message);
                else
                g_warning ("error->message == NULL!\n");
            g_error_free(error);
            error = NULL;
        }

        if (success) { /* write shuffle data */
            if (!itdb_shuffle_write(itdb, &error)) { /* an error occurred */
                success = FALSE;
                if (error && error->message)
                    gtkpod_warning("%s\n\n", error->message);
                    else
                    g_warning ("error->message == NULL!\n");
                g_error_free(error);
                error = NULL;
            }
        }
        if (success) {
            if (WRITE_EXTENDED_INFO) { /* write extended information */
                success = write_extended_info(itdb);
            }
            else { /* delete extended information if present */
                gchar *ext = g_strdup_printf("%s.ext", itdb->filename);
                if (g_file_test(ext, G_FILE_TEST_EXISTS)) {
                    if (remove(ext) != 0) {
                        gtkpod_statusbar_message(_("Extended information file not deleted: '%s\'"), ext);
                    }
                }
                g_free(ext);
            }
        }
        if (success) { /* copy to cfgdir */
            GError *error = NULL;
            if (!g_file_test(eitdb->offline_filename, G_FILE_TEST_EXISTS)) {
                /* Possible that plugging an ipod into a different pc can lead to an offline filename
                 * that does not exist. This results in the save process failing as it cannot write a
                 * backup. Attempt to mitigate this situation with a reserve backup path.
                 */
                gchar *ipod_model = get_itdb_prefs_string(itdb, KEY_IPOD_MODEL);
                gchar *backup_name = g_strconcat("backupDB_", ipod_model, NULL);
                g_free(ipod_model);
                eitdb->offline_filename = g_build_filename(cfgdir, backup_name, NULL);
                g_free(backup_name);
                gtkpod_statusbar_message("Backup database could not be found so backing up database to %s\n", eitdb->offline_filename);
            }

            if (!itdb_cp(itdb->filename, eitdb->offline_filename, &error)) {
                success = FALSE;
                if (error && error->message)
                    gtkpod_warning("%s\n\n", error->message);
                    else
                    g_warning ("error->message == NULL!\n");
                g_error_free(error);
                error = NULL;
            }
            if (WRITE_EXTENDED_INFO) {
                gchar *from, *to;
                from = g_strdup_printf("%s.ext", itdb->filename);
                to = g_strdup_printf("%s.ext", eitdb->offline_filename);
                if (!itdb_cp(from, to, &error)) {
                    success = FALSE;
                    if (error && error->message)
                        gtkpod_warning("%s\n\n", error->message);
                        else
                        g_warning ("error->message == NULL!\n");
                    g_error_free(error);
                }
                g_free(from);
                g_free(to);
            }
        }
    }

    if (success && get_offline(itdb) && (itdb->usertype & GP_ITDB_TYPE_IPOD)) { /* write to cfgdir */
        GError *error = NULL;
        if (!itdb_write_file(itdb, eitdb->offline_filename, &error)) { /* an error occurred */
            success = FALSE;
            if (error && error->message)
                gtkpod_warning("%s\n\n", error->message);
                else
                g_warning ("error->message == NULL!\n");
            g_error_free(error);
            error = NULL;
        }
        if (success && WRITE_EXTENDED_INFO) { /* write extended information */
            success = write_extended_info(itdb);
        }
    }

    if (success && (itdb->usertype & GP_ITDB_TYPE_LOCAL)) { /* write to cfgdir */
        GError *error = NULL;
        if (!itdb_write_file(itdb, NULL, &error)) { /* an error occurred */
            success = FALSE;
            if (error && error->message)
                gtkpod_warning("%s\n\n", error->message);
                else
                g_warning ("error->message == NULL!\n");
            g_error_free(error);
            error = NULL;
        }
        if (success) { /* write extended information */
            success = write_extended_info(itdb);
        }
    }

    for (it = itdb->tracks; it != NULL; it = it->next) {
        gp_track_validate_entries((Itdb_Track *) it->data);
    }

    /* If the ipod supports photos and the photo_data_changed
     * flag has been set to true then wrtie the photo database
     */
    if (success && (itdb->usertype & GP_ITDB_TYPE_IPOD) && itdb_device_supports_photo(itdb->device) && eitdb->photodb
            != NULL && eitdb->photo_data_changed == TRUE) {
        GError *error = NULL;
        if (!itdb_photodb_write(eitdb->photodb, &error)) {
            success = FALSE;
            if (error && error->message)
                gtkpod_warning("%s\n\n", error->message);
                else
                g_warning ("error->message == NULL!\n");

            g_error_free(error);
            error = NULL;
        }
    }

    /* indicate that files and/or database is saved */
    if (success) {
        data_unchanged(itdb);
        if (itdb->usertype & GP_ITDB_TYPE_IPOD) {
            gtkpod_statusbar_message(_("%s: Database saved"), mpl->name);
        }
        else {
            gtkpod_statusbar_message(_("%s: Changes saved"), mpl->name);
        }
    }

    g_free(cfgdir);

    gtk_widget_destroy(dialog);
    transfer_data_free(transferdata);

    release_widgets();

    return success;
}

/* used to handle export of database */
/* ATTENTION: directly used as callback in gtkpod.glade -- if you
 change the arguments of this function make sure you define a
 separate callback for gtkpod.glade */
void handle_export(void) {
    GList *gl;
    gboolean success = TRUE;
    struct itdbs_head *itdbs_head;

    g_return_if_fail (gtkpod_app);

    itdbs_head = gp_get_itdbs_head();
    g_return_if_fail (itdbs_head);

    block_widgets(); /* block user input */

    /* read offline playcounts -- in case we added some tracks we can
     now handle */
    parse_offline_playcount();

    for (gl = itdbs_head->itdbs; gl; gl = gl->next) {
        ExtraiTunesDBData *eitdb;
        iTunesDB *itdb = gl->data;
        g_return_if_fail (itdb);
        eitdb = itdb->userdata;
        g_return_if_fail (eitdb);

        if (eitdb->data_changed) {
            success &= gp_save_itdb(itdb);
        }
    }

    release_widgets();
}

/* indicate that data was changed and update the free space indicator,
 * as well as the changed indicator in the playlist view */
void data_changed(iTunesDB *itdb) {
    ExtraiTunesDBData *eitdb;
    g_return_if_fail (itdb);
    eitdb = itdb->userdata;
    g_return_if_fail (eitdb);

    if ((itdb->usertype & GP_ITDB_TYPE_IPOD) && !eitdb->itdb_imported) {
        /* don't do anything for non-imported iPod
         repositories. Marking them as "changed" allows the empty
         repository to be saved back to the iPod, overwriting data
         there */
        return;
    }
    else {
        eitdb->data_changed = TRUE;
        gtkpod_notify_data_changed(itdb);
    }
}

/* indicate that data was changed and update the free space indicator,
 * as well as the changed indicator in the playlist view */
void data_unchanged(iTunesDB *itdb) {
    ExtraiTunesDBData *eitdb;
    g_return_if_fail (itdb);
    eitdb = itdb->userdata;
    g_return_if_fail (eitdb);

    eitdb->data_changed = FALSE;
    if (eitdb->photo_data_changed == TRUE)
        eitdb->photo_data_changed = FALSE;

    gtkpod_notify_data_unchanged(itdb);
}

/* Check if all files are saved (i.e. none of the itdbs has the
 * data_changed flag set */
gboolean files_are_saved(void) {
    struct itdbs_head *itdbs_head;
    gboolean changed = FALSE;
    GList *gl;

    g_return_val_if_fail (gtkpod_app, TRUE);
    itdbs_head = gp_get_itdbs_head();
    g_return_val_if_fail (itdbs_head, TRUE);
    for (gl = itdbs_head->itdbs; gl; gl = gl->next) {
        iTunesDB *itdb = gl->data;
        ExtraiTunesDBData *eitdb;
        g_return_val_if_fail (itdb, !changed);
        eitdb = itdb->userdata;
        g_return_val_if_fail (eitdb, !changed);
        /* printf ("itdb: %p, changed: %d, imported: %d\n",
         itdb, eitdb->data_changed, eitdb->itdb_imported);*/
        changed |= eitdb->data_changed;
    }
    return !changed;
}
