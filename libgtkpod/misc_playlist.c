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

#include <string.h>
#include <glib/gi18n-lib.h>
#include "charset.h"
#include "itdb.h"
#include "sha1.h"
#include "misc.h"
#include "misc_playlist.h"
#include "misc_track.h"
#include "prefs.h"
#include "file_convert.h"
#include "gtkpod_app_iface.h"

#define DEBUG_MISC 0

/*------------------------------------------------------------------*\
 *                                                                  *
 *             Add new playlist asking user for name                *
 *                                                                  *
 \*------------------------------------------------------------------*/

/* Add a new playlist at position @position. The name for the new
 * playlist is queried from the user. A default (@dflt) name can be
 * provided.
 * Return value: the new playlist or NULL if the dialog was
 * cancelled. */
Playlist *add_new_pl_user_name(iTunesDB *itdb, gchar *dflt, gint32 position) {
    ExtraiTunesDBData *eitdb;
    Playlist *result = NULL;
    gchar *name;

    g_return_val_if_fail (itdb, NULL);

    eitdb = itdb->userdata;
    g_return_val_if_fail (eitdb, NULL);

    if (!eitdb->itdb_imported) {
        gtkpod_warning_simple(_("Please load the iPod before adding playlists."));
        return NULL;
    }

    name
            = get_user_string(_("New Playlist"), _("Please enter a name for the new playlist"), dflt ? dflt : _("New Playlist"), NULL, NULL, GTK_STOCK_ADD);
    if (name) {
        result = gp_playlist_add_new(itdb, name, FALSE, position);
        gtkpod_tracks_statusbar_update ();
    }
    return result;
}

/*------------------------------------------------------------------*\
 *                                                                  *
 *                     Special Playlist Stuff                       *
 *                                                                  *
 \*------------------------------------------------------------------*/

/* used for special playlist creation */
typedef gboolean (*PL_InsertFunc)(Track *track, gpointer userdata);

/* generate_category_playlists: Create a playlist for each category
 @cat (T_ARTIST, T_ALBUM, T_GENRE, T_COMPOSER) */
void generate_category_playlists(iTunesDB *itdb, T_item cat) {
    Playlist *master_pl;
    gchar *qualifier;
    GList *gl;

    g_return_if_fail (itdb);

    /* Initialize the "qualifier". It is used to indicate the category of
     automatically generated playlists */
    switch (cat) {
    case T_ARTIST:
        qualifier = _("AR:");
        break;
    case T_ALBUM:
        qualifier = _("AL:");
        break;
    case T_GENRE:
        qualifier = _("GE:");
        break;
    case T_COMPOSER:
        qualifier = _("CO:");
        break;
    case T_YEAR:
        qualifier = _("YE:");
        break;
    default:
        g_return_if_reached ();
        break;
    }

    /* FIXME: delete all playlists named '[<qualifier> .*]' and
     * remember which playlist was selected if it gets deleted */

    master_pl = itdb_playlist_mpl(itdb);
    g_return_if_fail (master_pl);

    for (gl = master_pl->members; gl; gl = gl->next) {
        Track *track = gl->data;
        Playlist *cat_pl = NULL;
        gchar *category = NULL;
        const gchar *track_cat;

        track_cat = track_get_item(track, cat);

        if (track_cat) {
            /* some tracks have empty strings in the genre field */
            if (track_cat[0] == '\0') {
                category = g_strdup_printf("[%s %s]", qualifier, _("Unknown"));
            }
            else {
                category = g_strdup_printf("[%s %s]", qualifier, track_cat);
            }

            /* look for category playlist */
            cat_pl = itdb_playlist_by_name(itdb, category);
            /* or, create category playlist */
            if (!cat_pl) {
                cat_pl = gp_playlist_add_new(itdb, category, FALSE, -1);
            }
            gp_playlist_add_track(cat_pl, track, TRUE);
            g_free(category);
        }
    }
    gtkpod_tracks_statusbar_update();
}

/* Generate a new playlist containing all the tracks currently
 displayed */
Playlist *generate_displayed_playlist(void) {
    GList *tracks = gtkpod_get_displayed_tracks();
    Playlist *result = NULL;

    if (tracks) {
        result = generate_new_playlist(gtkpod_get_current_itdb(), tracks);
        g_list_free(tracks);
    }
    return result;
}

/* Generate a new playlist containing all the tracks currently
 selected */
Playlist *generate_selected_playlist(void) {
    GList *tracks = gtkpod_get_selected_tracks();
    Playlist *result = NULL;

    if (tracks) {
        result = generate_new_playlist(gp_get_selected_itdb(), tracks);
        g_list_free(tracks);
    }
    return result;
}

/* Generates a playlist containing a random selection of
 prefs_get_int("misc_track_nr") tracks in random order from the currently
 displayed tracks */
Playlist *generate_random_playlist(iTunesDB *itdb) {
    GRand *grand = g_rand_new();
    Playlist *new_pl = NULL;
    gchar *pl_name, *pl_name1;
    GList *rtracks = NULL;
    GList *tracks = gtkpod_get_displayed_tracks();
    gint tracks_max = prefs_get_int("misc_track_nr");
    gint tracks_nr = 0;

    while (tracks && (tracks_nr < tracks_max)) {
        /* get random number between 0 and g_list_length()-1 */
        gint rn = g_rand_int_range(grand, 0, g_list_length(tracks));
        GList *random = g_list_nth(tracks, rn);
        rtracks = g_list_append(rtracks, random->data);
        tracks = g_list_delete_link(tracks, random);
        ++tracks_nr;
    }
    pl_name1 = g_strdup_printf(_("Random (%d)"), tracks_max);
    pl_name = g_strdup_printf("[%s]", pl_name1);
    new_pl = generate_playlist_with_name(itdb, rtracks, pl_name, TRUE);
    g_free(pl_name1);
    g_free(pl_name);
    g_list_free(tracks);
    g_list_free(rtracks);
    g_rand_free(grand);
    return new_pl;
}

static void not_listed_make_track_list(gpointer key, gpointer track, gpointer tracks) {
    *(GList **) tracks = g_list_append(*(GList **) tracks, (Track *) track);
}

/* Generate a playlist containing all tracks that are not part of any
 playlist.
 For this, playlists starting with a "[" (generated playlists) are
 being ignored. */
Playlist *generate_not_listed_playlist(iTunesDB *itdb) {
    GHashTable *hash;
    GList *gl, *tracks = NULL;
    guint32 i;
    gchar *pl_name;
    Playlist *new_pl, *pl;

    g_return_val_if_fail (itdb, NULL);

    /* Create hash with all track/track pairs */
    pl = itdb_playlist_mpl(itdb);
    g_return_val_if_fail (pl, NULL);
    hash = g_hash_table_new(NULL, NULL);
    for (gl = pl->members; gl != NULL; gl = gl->next) {
        g_hash_table_insert(hash, gl->data, gl->data);
    }
    /* remove all tracks that are members of other playlists */
    i = 1;
    do {
        pl = itdb_playlist_by_nr(itdb, i);
        ++i;
        /* skip playlists starting with a '[' */
        if (pl && pl->name && (pl->name[0] != '[')) {
            for (gl = pl->members; gl != NULL; gl = gl->next) {
                g_hash_table_remove(hash, gl->data);
            }
        }
    }
    while (pl);

    g_hash_table_foreach(hash, not_listed_make_track_list, &tracks);
    g_hash_table_destroy(hash);
    hash = NULL;

    pl_name = g_strdup_printf("[%s]", _("Not Listed"));

    new_pl = generate_playlist_with_name(itdb, tracks, pl_name, TRUE);
    g_free(pl_name);
    g_list_free(tracks);
    return new_pl;
}

/* Generate a playlist consisting of the tracks in @tracks
 * with @name name. If @del_old ist TRUE, delete any old playlist with
 * the same name. */
Playlist *generate_playlist_with_name(iTunesDB *itdb, GList *tracks, gchar *pl_name, gboolean del_old) {
    Playlist *new_pl = NULL;
    gint n = g_list_length(tracks);
    g_return_val_if_fail (itdb, new_pl);

    if (n > 0) {
        gboolean select = FALSE;
        GList *l;
        if (del_old) {
            /* currently selected playlist */
            Playlist *sel_pl = gtkpod_get_current_playlist();
            if (sel_pl->itdb != itdb) { /* different itdb */
                sel_pl = NULL;
            }
            /* remove all playlists with named @plname */
            gp_playlist_remove_by_name(itdb, pl_name);
            /* check if we deleted the selected playlist */
            if (sel_pl) {
                if (g_list_find(itdb->playlists, sel_pl) == NULL)
                    select = TRUE;
            }
        }
        new_pl = gp_playlist_add_new(itdb, pl_name, FALSE, -1);
        g_return_val_if_fail (new_pl, new_pl);
        for (l = tracks; l; l = l->next) {
            Track *track = l->data;
            g_return_val_if_fail (track, new_pl);
            gp_playlist_add_track(new_pl, track, TRUE);
        }
        gtkpod_statusbar_message(ngettext ("Created playlist '%s' with %d track.",
                "Created playlist '%s' with %d tracks.",
                n), pl_name, n);
        if (new_pl && select) { /* need to select newly created playlist because the old
         * selection was deleted */
            gtkpod_set_current_playlist(new_pl);
        }
    }
    else { /* n==0 */
        gtkpod_statusbar_message(_("No tracks available, playlist not created"));
    }
    gtkpod_tracks_statusbar_update();
    return new_pl;
}

/* Generate a playlist named "New Playlist" consisting of the tracks
 * in @tracks. */
Playlist *generate_new_playlist(iTunesDB *itdb, GList *tracks) {
    gchar
            *name =
                    get_user_string(_("New Playlist"), _("Please enter a name for the new playlist"), _("New Playlist"), NULL, NULL, GTK_STOCK_ADD);
    if (name)
        return generate_playlist_with_name(itdb, tracks, name, FALSE);
    return NULL;
}

/* look at the add_ranked_playlist help:
 * BEWARE this function shouldn't be used by other functions */
static GList *create_ranked_glist(iTunesDB *itdb, gint tracks_nr, PL_InsertFunc insertfunc, GCompareFunc comparefunc, gpointer userdata) {
    GList *tracks = NULL;
    gint f = 0;
    GList *gl;

    g_return_val_if_fail (itdb, tracks);

    for (gl = itdb->tracks; gl; gl = gl->next) {
        Track *track = gl->data;
        g_return_val_if_fail (track, tracks);
        if (track && (!insertfunc || insertfunc(track, userdata))) {
            tracks = g_list_insert_sorted(tracks, track, comparefunc);
            ++f;
            if (tracks_nr && (f > tracks_nr)) { /*cut the tail*/
                tracks = g_list_remove(tracks, g_list_nth_data(tracks, tracks_nr));
                --f;
            }
        }
    }
    return tracks;
}
/* Generate or update a playlist named @pl_name, containing
 * @tracks_nr tracks.
 *
 * @str is the playlist's name (no [ or ])
 * @insertfunc: determines which tracks to enter into the new playlist.
 *              If @insertfunc is NULL, all tracks are added.
 * @comparefunc: determines order of tracks
 * @tracks_nr: max. number of tracks in playlist or 0 for no limit.
 *
 * Return value: the newly created playlist
 */
static Playlist *update_ranked_playlist(iTunesDB *itdb, gchar *str, gint tracks_nr, PL_InsertFunc insertfunc, GCompareFunc comparefunc, gpointer userdata) {
    Playlist *result = NULL;
    gchar *pl_name = g_strdup_printf("[%s]", str);
    GList *tracks;

    g_return_val_if_fail (itdb, result);

    tracks = create_ranked_glist(itdb, tracks_nr, insertfunc, comparefunc, userdata);

    if (tracks)
    /* else generate_playlist_with_name prints something*/
    {
        result = generate_playlist_with_name(itdb, tracks, pl_name, TRUE);
    }
    g_list_free(tracks);
    g_free(pl_name);
    return result;
}

/* ------------------------------------------------------------ */
/* Generate a new playlist containing the most listened (playcount
 * reverse order) tracks. to enter this playlist a track must have been
 * played */

/* Sort Function: determines the order of the generated playlist */

/* NOTE: THE USE OF 'COMP' ARE NECESSARY FOR THE TIME_PLAYED COMPARES
 WHERE A SIGN OVERFLOW MAY OCCUR BECAUSE OF THE 32 BIT UNSIGNED MAC
 TIMESTAMPS. */
static gint Most_Listened_CF(gconstpointer aa, gconstpointer bb) {
    gint result = 0;
    const Track *a = aa;
    const Track *b = bb;

    if (a && b) {
        result = COMP (b->playcount, a->playcount);
        if (result == 0)
            result = COMP (b->rating, a->rating);
        if (result == 0)
            result = COMP (b->time_played, a->time_played);
    }
    return result;
}

/* Insert function: determines whether a track is entered into the playlist */
static gboolean Most_Listened_IF(Track *track, gpointer userdata) {
    if (track)
        return (track->playcount != 0);
    return FALSE;
}

void most_listened_pl(iTunesDB *itdb) {
    gint tracks_nr = prefs_get_int("misc_track_nr");
    gchar *str;

    g_return_if_fail (itdb);
    str = g_strdup_printf(_("Most Listened (%d)"), tracks_nr);
    update_ranked_playlist(itdb, str, tracks_nr, Most_Listened_IF, Most_Listened_CF, (gpointer) 0);
    g_free(str);
}

/* ------------------------------------------------------------ */
/* Generate a new playlist containing all songs never listened to. */

/* Sort Function: determines the order of the generated playlist */

/* NOTE: THE USE OF 'COMP' ARE NECESSARY FOR THE TIME_PLAYED COMPARES
 WHERE A SIGN OVERFLOW MAY OCCUR BECAUSE OF THE 32 BIT UNSIGNED MAC
 TIMESTAMPS. */
static gint Never_Listened_CF(gconstpointer aa, gconstpointer bb) {
    gint result = 0;
    const Track *a = aa;
    const Track *b = bb;

    if (a && b) {
        result = COMP (b->rating, a->rating);
    }
    return result;
}

/* Insert function: determines whether a track is entered into the playlist */
static gboolean Never_Listened_IF(Track *track, gpointer userdata) {
    if (track)
        return (track->playcount == 0);
    return FALSE;
}

void never_listened_pl(iTunesDB *itdb) {
    gint tracks_nr = 0; /* no limit */
    gchar *str;

    g_return_if_fail (itdb);
    str = g_strdup_printf(_("Never Listened"));
    update_ranked_playlist(itdb, str, tracks_nr, Never_Listened_IF, Never_Listened_CF, (gpointer) 0);
    g_free(str);
}

/* ------------------------------------------------------------ */
/* Generate a new playlist containing the most rated (rate
 * reverse order) tracks. */

/* Sort Function: determines the order of the generated playlist */
static gint Most_Rated_CF(gconstpointer aa, gconstpointer bb) {
    gint result = 0;
    const Track *a = aa;
    const Track *b = bb;

    if (a && b) {
        result = COMP (b->rating, a->rating);
        if (result == 0)
            result = COMP (b->playcount, a->playcount);
        if (result == 0)
            result = COMP (b->time_played, a->time_played);
    }
    return result;
}

/* Insert function: determines whether a track is entered into the playlist */
static gboolean Most_Rated_IF(Track *track, gpointer userdata) {
    if (track)
        return ((track->playcount != 0) || prefs_get_int("not_played_track"));
    return FALSE;
}

void most_rated_pl(iTunesDB *itdb) {
    gint tracks_nr = prefs_get_int("misc_track_nr");
    gchar *str;

    g_return_if_fail (itdb);
    str = g_strdup_printf(_("Best Rated (%d)"), tracks_nr);
    update_ranked_playlist(itdb, str, tracks_nr, Most_Rated_IF, Most_Rated_CF, (gpointer) 0);
    g_free(str);
}

/* ------------------------------------------------------------ */
/* Generate 6 playlists,one for each rating 1..5 and unrated    */

/* Sort Function: determines the order of the generated playlist */
static gint All_Ratings_CF(gconstpointer aa, gconstpointer bb) {
    gint result = 0;
    const Track *a = aa;
    const Track *b = bb;

    if (a && b) {
        result = COMP (b->playcount, a->playcount);
        if (result == 0)
            result = COMP (b->time_played, a->time_played);
    }
    return result;
}

/* Insert function: determines whether a track is entered into the playlist */
static gboolean All_Ratings_IF(Track *track, gpointer user_data) {
    guint playlist_nr = GPOINTER_TO_UINT(user_data);
    if (track)
        return (track->rating == playlist_nr * 20);
    return FALSE;
}

void each_rating_pl(iTunesDB *itdb) {
    gchar *str;
    guint playlist_nr;

    g_return_if_fail (itdb);
    str = _("Unrated tracks");
    for (playlist_nr = 0; playlist_nr < 6; playlist_nr++) {
        if (playlist_nr > 0) {
            str = g_strdup_printf(_("Rated %d"), playlist_nr);
        }
        update_ranked_playlist(itdb, str, 0, All_Ratings_IF, All_Ratings_CF, GUINT_TO_POINTER(playlist_nr));
    }
    g_free(str);
}

/* ------------------------------------------------------------ */
/* Generate a new playlist containing the last listened (last time play
 * reverse order) tracks. */

/* Sort Function: determines the order of the generated playlist */
static gint Last_Listened_CF(gconstpointer aa, gconstpointer bb) {
    gint result = 0;
    const Track *a = aa;
    const Track *b = bb;

    if (a && b) {
        result = COMP (b->time_played, a->time_played);
        if (result == 0)
            result = COMP (b->rating, a->rating);
        if (result == 0)
            result = COMP (b->playcount, a->playcount);
    }
    return result;
}

/* Insert function: determines whether a track is entered into the playlist */
static gboolean Last_Listened_IF(Track *track, gpointer userdata) {
    if (track)
        return (track->playcount != 0);
    return FALSE;
}

void last_listened_pl(iTunesDB *itdb) {
    gint tracks_nr = prefs_get_int("misc_track_nr");
    gchar *str;

    g_return_if_fail (itdb);
    str = g_strdup_printf(_("Recent (%d)"), tracks_nr);
    update_ranked_playlist(itdb, str, tracks_nr, Last_Listened_IF, Last_Listened_CF, (gpointer) 0);
    g_free(str);
}

/* ------------------------------------------------------------ */
/* Generate a new playlist containing the tracks listened to since the
 * last time the iPod was connected to a computer (and the playcount
 * file got wiped) */

/* Sort Function: determines the order of the generated playlist */
static gint since_last_CF(gconstpointer aa, gconstpointer bb) {
    gint result = 0;
    const Track *a = aa;
    const Track *b = bb;

    if (a && b) {
        result = COMP (b->recent_playcount, a->recent_playcount);
        if (result == 0)
            result = COMP (b->time_played, a->time_played);
        if (result == 0)
            result = COMP (b->playcount, a->playcount);
        if (result == 0)
            result = COMP (b->rating, a->rating);
    }
    return result;
}

/* Insert function: determines whether a track is entered into the playlist */
static gboolean since_last_IF(Track *track, gpointer userdata) {
    if (track && (track->recent_playcount != 0))
        return TRUE;
    else
        return FALSE;
}

void since_last_pl(iTunesDB *itdb) {
    g_return_if_fail (itdb);
    update_ranked_playlist(itdb, _("Last Time"), 0, since_last_IF, since_last_CF, (gpointer) 0);
}

/*------------------------------------------------------------------*\
 *                                                                  *
 *                Find Orphans                                      *
 *                                                                  *
 \*------------------------------------------------------------------*/

/******************************************************************************
 * Attempt to do everything at once:
 *  - find dangling links in iTunesDB
 *  - find orphaned files in mounted directory
 * Will be done by creating first a hash of all known in iTunesDB filenames,
 * and then checking every file on HDD in the hash table. If it is present -
 * remove from hashtable, if not present - it is orphaned. If at the end
 * hashtable still has some elements - they're dangling...
 *
 * TODO: instead of using case-sensitive comparison function it might be better
 *  just to convert all filenames to lowercase before doing any comparisons
 * FIX:
 *  offline... when you import db offline and then switch to online mode you still
 *  have offline db loaded and if it is different from IPOD's - then a lot of crap
 *  can happen... didn't check yet
 ******************************************************************************/

/* compare @str1 and @str2 case-sensitively only */
gint str_cmp(gconstpointer str1, gconstpointer str2, gpointer data) {
    return compare_string_case_insensitive((gchar *) str1, (gchar *) str2);
    /*return strcmp((gchar *)str1, (gchar *)str2);*/
}

static void treeKeyDestroy(gpointer key) {
    g_free(key);
}
static void treeValueDestroy(gpointer value) {
}

/* call back function for traversing what is left from the tree -
 * dangling files - files present in DB but not present physically on iPod.
 * It adds found tracks to the dandling list so user can see what is missing
 * and then decide on what to do with them */
gboolean remove_dangling(gpointer key, gpointer value, gpointer pl_dangling) {
    /*     printf("Found dangling item pointing file %s\n", ((Track*)value)->ipod_path); */
    Track *track = (Track*) value;
    GList **l_dangling = ((GList **) pl_dangling);
    gchar *filehash = NULL;
    gint lind;
    ExtraTrackData *etr;

    g_return_val_if_fail (l_dangling, FALSE);
    g_return_val_if_fail (track, FALSE);
    etr = track->userdata;
    g_return_val_if_fail (etr, FALSE);

    /* 1 - Original file is present on PC */
    /* 0 - Doesn't exist */
    lind = 0;
    if (etr->pc_path_locale && *etr->pc_path_locale && g_file_test(etr->pc_path_locale, G_FILE_TEST_EXISTS)) {
        lind = 1;
    }
    l_dangling[lind] = g_list_append(l_dangling[lind], track);

    g_free(filehash);
    return FALSE; /* do not stop traversal */
}

guint ntokens(gchar** tokens) {
    guint n = 0;
    while (tokens[n])
        n++;
    return n;
}

void process_gtk_events_blocked() {
    while (widgets_blocked && gtk_events_pending())
        gtk_main_iteration();
}

/* Frees memory busy by the lists containing tracks stored in
 @user_data1 */
static void check_db_danglingcancel0(gpointer user_data1, gpointer user_data2) {
    g_list_free((GList *) user_data1);
    gtkpod_statusbar_message(_("Removal of dangling tracks with no files on PC was canceled."));
}

/* Frees memory busy by the lists containing tracks stored in
 @user_data1 */
static void check_db_danglingcancel1(gpointer user_data1, gpointer user_data2) {
    g_list_free((GList *) user_data1);
    gtkpod_statusbar_message(_("Handling of dangling tracks with files on PC was canceled."));
}

/* "dangling": tracks that are in database but not on disk */
/* To be called for ok to remove dangling Tracks with with no files
 * linked.  Frees @user_data1 and @user_data2*/
static void check_db_danglingok0(gpointer user_data1, gpointer user_data2) {
    GList *tlist = ((GList *) user_data1);
    GList *l_dangling = tlist;
    iTunesDB *itdb = user_data2;

    g_return_if_fail (itdb);
    /* traverse the list and append to the str */
    for (tlist = g_list_first(tlist); tlist != NULL; tlist = g_list_next(tlist)) {
        Track *track = tlist->data;
        g_return_if_fail (track);

        /* printf("Removing track %d\n", track->ipod_id); */
        /* remove track from database */
        gp_playlist_remove_track(NULL, track, DELETE_ACTION_DATABASE);
    }
    g_list_free(l_dangling);
    data_changed(itdb);
    gtkpod_statusbar_message(_("Dangling tracks with no files on PC were removed."));
}

/* To be called for ok to remove dangling Tracks with with no files linked.
 * Frees @user_data1 and @user_data2*/
static void check_db_danglingok1(gpointer user_data1, gpointer user_data2) {
    GList *tlist = ((GList *) user_data1);
    GList *l_dangling = tlist;
    iTunesDB *itdb = user_data2;

    g_return_if_fail (itdb);

    block_widgets();

    /* traverse the list and append to the str */
    for (tlist = g_list_first(tlist); tlist != NULL; tlist = g_list_next(tlist)) {
        Track *oldtrack;
        Track *track = tlist->data;
        ExtraTrackData *etr;
        gchar *buf;

        g_return_if_fail (track);
        etr = track->userdata;
        g_return_if_fail (etr);
        /* printf("Handling track %d\n", track->ipod_id); */

        buf = get_track_info(track, TRUE);
        gtkpod_statusbar_message (_("Processing '%s'..."), buf);
        g_free(buf);

        while (widgets_blocked && gtk_events_pending())
            gtk_main_iteration();

        /* Indicate that file needs to be transfered */
        track->transferred = FALSE;
        /* Update SHA1 information */
        /* remove track from sha1 hash and reinsert it
         (hash value may have changed!) */
        sha1_track_remove(track);
        /* need to remove the old value manually! */
        g_free(etr->sha1_hash);
        etr->sha1_hash = NULL;
        oldtrack = sha1_track_exists_insert(itdb, track);
        if (oldtrack) { /* track exists, remove old track
         and register the new version */
            sha1_track_remove(oldtrack);
            gp_duplicate_remove(track, oldtrack);
            sha1_track_exists_insert(itdb, track);
        }
        /* mark for conversion / transfer */
        file_convert_add_track(track);
    }
    g_list_free(l_dangling);
    data_changed(itdb);
    gtkpod_statusbar_message(_("Dangling tracks with files on PC were handled."));
    /* I don't think it's too interesting to pop up the list of
     duplicates -- but we should reset the list. */
    gp_duplicate_remove(NULL, (void *) -1);

    release_widgets();
}

static void glist_list_tracks(GList * tlist, GString * str) {
    if (str == NULL) {
        fprintf(stderr, "Report the bug please: shouldn't be NULL at %s:%d\n", __FILE__, __LINE__);
        return;
    }
    /* traverse the list and append to the str */
    for (tlist = g_list_first(tlist); tlist != NULL; tlist = g_list_next(tlist)) {
        ExtraTrackData *etr;
        Track *track = tlist->data;
        g_return_if_fail (track);
        etr = track->userdata;
        g_return_if_fail (etr);
        g_string_append_printf(str, "%s(%d) %s-%s -> %s\n", _("Track"), track->id, track->artist, track->title, etr->pc_path_utf8);
    }
} /* end of glist_list_tracks */

/* checks iTunesDB for presence of dangling links and checks IPODs
 * Music directory on subject of orphaned files */
void check_db(iTunesDB *itdb) {

    GTree *files_known = NULL;
    GDir *dir_des = NULL;

    gchar *pathtrack = NULL;
    gchar *ipod_filename = NULL;
#   define localdebug  0      /* may be later becomes more general verbose param */
    Playlist* pl_orphaned = NULL;
    GList * l_dangling[2] =
        { NULL, NULL }; /* 2 kinds of dangling tracks: with approp
     * files and without */
    /* 1 - Original file is present on PC and has the same sha1*/
    /* 0 - Doesn't exist */

    gpointer foundtrack;
    gint h, i;
    gint norphaned = 0;
    gint ndangling = 0;
    gchar ** tokens;
    const gchar *mountpoint = itdb_get_mountpoint(itdb);
    ExtraiTunesDBData *eitdb;
    GList *gl;
    gchar *music_dir = NULL;

    g_return_if_fail (itdb);
    eitdb = itdb->userdata;
    g_return_if_fail (eitdb);

    /* If an iTunesDB exists on the iPod, the user probably is making
     a mistake and we should tell him about it */
    if (!eitdb->itdb_imported) {
        gchar *itunesdb_filename = itdb_get_itunesdb_path(mountpoint);

        if (itunesdb_filename) {
            const gchar
                    *str =
                            _("You did not import the existing iTunesDB. This is most likely incorrect and will result in the loss of the existing database.\n\nIf you abort the operation, you can import the existing database before calling this function again.\n");

            gint
                    result =
                            gtkpod_confirmation_hig(GTK_MESSAGE_WARNING, _("Existing iTunes database not imported"), str, _("Proceed anyway"), _("Abort operation"), NULL, NULL);

            if (result == GTK_RESPONSE_CANCEL) {
                return;
            }
        }
    }

    block_widgets();

    gtkpod_statusbar_message(_("Creating a tree of known files"));
    gtkpod_tracks_statusbar_update();

    /* put all files in the hash table */
    files_known = g_tree_new_full(str_cmp, NULL, treeKeyDestroy, treeValueDestroy);
    for (gl = itdb->tracks; gl; gl = gl->next) {
        Track *track = gl->data;
        gint ntok = 0;
        g_return_if_fail (track);
        /* we don't want to report non-transferred files as dangling */
        if (!track->transferred)
            continue;
        tokens = g_strsplit(track->ipod_path, ":", (track->ipod_path[0] == ':' ? 4 : 3));
        ntok = ntokens(tokens);
        if (ntok >= 3) {
            pathtrack = g_strdup(tokens[ntok - 1]);
        }
        else {
            /* illegal ipod_path */
            /* the track has NO ipod_path, so we want the item to
             ultimately be deleted from DB, * however, we need to
             add it to tree it such a way that:
             a) it will be unique
             b) it won't match to any existing file on the ipod

             so use something invented using the pointer to the
             track structure as a way to generate uniqueness
             */
            pathtrack = g_strdup_printf("NOFILE-%p", track);
        }

        if (localdebug) {
            fprintf(stdout, "File %s\n", pathtrack);
            fflush(stdout);
        }

        g_tree_insert(files_known, pathtrack, track);
        g_strfreev(tokens);
    }

    gtkpod_statusbar_message(_("Checking iPod files against known files in DB"));
    gtkpod_tracks_statusbar_update();
    process_gtk_events_blocked();

    music_dir = itdb_get_music_dir(mountpoint);

    for (h = 0; h < itdb_musicdirs_number(itdb); h++) {
        /* directory name */
        gchar *ipod_dir = g_strdup_printf("F%02d", h); /* just directory name */
        gchar *ipod_fulldir;
        /* full path */
        ipod_fulldir = itdb_get_path(music_dir, ipod_dir);
        if (ipod_fulldir && (dir_des = g_dir_open(ipod_fulldir, 0, NULL))) {
            while ((ipod_filename = g_strdup(g_dir_read_name(dir_des))))
            /* we have a file in the directory*/
            {
                pathtrack = g_strdup_printf("%s%c%s", ipod_dir, ':', ipod_filename);

                if (localdebug) {
                    fprintf(stdout, "Considering %s ", pathtrack);
                    fflush(stdout);
                }

                if (g_tree_lookup_extended(files_known, pathtrack, &foundtrack, &foundtrack)) { /* file is not orphaned */
                    g_tree_remove(files_known, pathtrack); /* we don't need this any more */
                    if (localdebug)
                        fprintf(stdout, " good ");
                }
                else { /* Now deal with orphaned... */
                    gchar *fn_orphaned;
                    gchar *num_str = g_strdup_printf("F%02d", h);
                    Track *dupl_track;

                    const gchar *p_dcomps[] =
                        { num_str, ipod_filename, NULL };

                    fn_orphaned = itdb_resolve_path(music_dir, p_dcomps);

                    if (!pl_orphaned) {
                        gchar *str = g_strdup_printf("[%s]", _("Orphaned"));
                        pl_orphaned = gp_playlist_by_name_or_add(itdb, str, FALSE);
                        g_free(str);
                    }

                    norphaned++;

                    if (localdebug)
                        fprintf(stdout, "to orphaned ");
                    if ((dupl_track = sha1_file_exists(itdb, fn_orphaned, TRUE))) { /* This orphan has already been added again.
                     It will be removed with the next sync */
                        Track *track = gp_track_new();
                        gchar *fn_utf8 = charset_to_utf8(fn_orphaned);
                        const gchar *dir_rel = music_dir + strlen(mountpoint);
                        if (*dir_rel == G_DIR_SEPARATOR)
                            ++dir_rel;
                        track->ipod_path
                                = g_strdup_printf("%c%s%c%s%c%s", G_DIR_SEPARATOR, dir_rel, G_DIR_SEPARATOR, num_str, G_DIR_SEPARATOR, ipod_filename);
                        itdb_filename_fs2ipod(track->ipod_path);

                        gp_track_validate_entries(track);
                        mark_track_for_deletion(itdb, track);
                        gtkpod_warning(_(
                                "The following orphaned file had already "
                                "been added to the iPod again. It will be "
                                "removed with the next sync:\n%s\n\n"), fn_utf8);
                        g_free(fn_utf8);
                    }
                    else {
                        add_track_by_filename(itdb, fn_orphaned, pl_orphaned, FALSE, NULL, NULL);
                    }
                    g_free(fn_orphaned);
                    g_free(num_str);
                }
                if (localdebug)
                    fprintf(stdout, " done\n");

                g_free(ipod_filename);
                g_free(pathtrack);
            }
            g_dir_close(dir_des);
        }
        g_free(ipod_dir);
        g_free(ipod_fulldir);
        process_gtk_events_blocked();
    }

    ndangling = g_tree_nnodes(files_known);
    gtkpod_statusbar_message(_("Found %d orphaned and %d dangling files. Processing..."), norphaned, ndangling);
    gtkpod_tracks_statusbar_update();

    g_free(music_dir);
    music_dir = NULL;

    /* Now lets deal with dangling tracks */
    /* Traverse the tree - leftovers are dangling - put them in two lists */
    g_tree_foreach(files_known, remove_dangling, l_dangling);

    for (i = 0; i < 2; i++) {
        GString *str_dangs = g_string_sized_new(2000);
        gint ndang = 0;
        gchar *buf;

        glist_list_tracks(l_dangling[i], str_dangs); /* compose String list of the tracks */
        ndang = g_list_length(l_dangling[i]);
        if (ndang) {
            if (i == 1)
                buf
                        = g_strdup_printf(ngettext ("The following dangling track has a file on PC.\nPress OK to have them transfered from the file on next Sync, CANCEL to leave it as is.",
                                "The following %d dangling tracks have files on PC.\nPress OK to have them transfered from the files on next Sync, CANCEL to leave them as is.",
                                ndang), ndang);
            else
                buf
                        = g_strdup_printf(ngettext ("The following dangling track doesn't have file on PC. \nPress OK to remove it, CANCEL to leave it as is.",
                                "The following %d dangling tracks do not have files on PC. \nPress OK to remove them, CANCEL to leave them. as is",
                                ndang), ndang);

            if (gtkpod_confirmation((i == 1 ? CONF_ID_DANGLING1 : CONF_ID_DANGLING0), /* we want unique window for each */
            FALSE, /* gboolean modal, */
            _("Dangling Tracks"), /* title */
            buf, /* label */
            str_dangs->str, /* scrolled text */
            NULL, 0, NULL, /* option 1 */
            NULL, 0, NULL, /* option 2 */
            TRUE, /* gboolean confirm_again, */
            NULL, /* ConfHandlerOpt confirm_again_handler,*/
            i == 1 ? check_db_danglingok1 : check_db_danglingok0, /* ConfHandler ok_handler,*/
            NULL, /* don't show "Apply" button */
            i == 1 ? check_db_danglingcancel1 : check_db_danglingcancel0, /* cancel_handler,*/
            l_dangling[i], /* gpointer user_data1,*/
            itdb) /* gpointer user_data2,*/
            == GTK_RESPONSE_REJECT) { /* free memory */
                g_list_free(l_dangling[i]);
            }
            g_free(buf);
            g_string_free(str_dangs, TRUE);
        }
    }

    if (pl_orphaned)
        data_changed(itdb);
    g_tree_destroy(files_known);
    gtkpod_statusbar_message(_("Found %d orphaned and %d dangling files. Done."), norphaned, ndangling);
    //    gtkpod_statusbar_timeout(0);
    release_widgets();
}

/*------------------------------------------------------------------*\
 *                                                                  *
 *             Delete Playlist                                      *
 *                                                                  *
 \*------------------------------------------------------------------*/

/* clean up delete playlist */
static void delete_playlist_cleanup(struct DeleteData *dd) {
    g_return_if_fail (dd);

    g_list_free(dd->tracks);
    g_free(dd);
}

static void delete_playlist_ok(struct DeleteData *dd) {
    gint n;
    gchar *msg = NULL;

    g_return_if_fail (dd);
    g_return_if_fail (dd->pl);
    g_return_if_fail (dd->itdb);

    n = g_list_length(dd->pl->members);

    if (dd->itdb->usertype & GP_ITDB_TYPE_IPOD) {
        switch (dd->deleteaction) {
        case DELETE_ACTION_IPOD:
            while (dd->pl->members) {
                /* remove tracks from iPod */
                gp_playlist_remove_track(dd->pl, dd->pl->members->data, dd->deleteaction);
            }
            if (itdb_playlist_is_mpl(dd->pl)) {
                msg = g_strdup_printf(_("Removed all %d tracks from the iPod"), n);
            }
            else if (itdb_playlist_is_podcasts(dd->pl)) {
                msg = g_strdup_printf(_("Removed all podcasts from the iPod"));
            }
            else {
                /* first use playlist name */
                msg = g_strdup_printf(ngettext ("Deleted playlist '%s' including %d member track",
                        "Deleted playlist '%s' including %d member tracks",
                        n), dd->pl->name, n);
                /* then remove playlist */
                gp_playlist_remove(dd->pl);
            }
            break;
        case DELETE_ACTION_PLAYLIST:
            if (itdb_playlist_is_mpl(dd->pl)) { /* not allowed -- programming error */
                g_return_if_reached ();
            }
            else {
                /* first use playlist name */
                msg = g_strdup_printf(_("Deleted playlist '%s'"), dd->pl->name);
                /* then remove playlist */
                gp_playlist_remove(dd->pl);
            }
            break;
        case DELETE_ACTION_LOCAL:
        case DELETE_ACTION_DATABASE:
            /* not allowed -- programming error */
            g_return_if_reached ();
            break;
        }
    }
    if (dd->itdb->usertype & GP_ITDB_TYPE_LOCAL) {
        switch (dd->deleteaction) {
        case DELETE_ACTION_LOCAL:
            if (itdb_playlist_is_mpl(dd->pl)) { /* for safety reasons this is not implemented (would
             remove all tracks from your local harddisk) */
                g_return_if_reached ();
            }
            else {
                while (dd->pl->members) {
                    /* remove tracks from playlist */
                    gp_playlist_remove_track(dd->pl, dd->pl->members->data, dd->deleteaction);
                }
                /* first use playlist name */
                msg = g_strdup_printf(ngettext ("Deleted playlist '%s' including %d member track on harddisk",
                        "Deleted playlist '%s' including %d member tracks on harddisk",
                        n), dd->pl->name, n);
                /* then remove playlist */
                gp_playlist_remove(dd->pl);
            }
            break;
        case DELETE_ACTION_DATABASE:
            while (dd->pl->members) {
                /* remove tracks from database */
                gp_playlist_remove_track(dd->pl, dd->pl->members->data, dd->deleteaction);
            }
            if (itdb_playlist_is_mpl(dd->pl)) {
                msg = g_strdup_printf(_("Removed all %d tracks from the database"), n);
            }
            else {
                /* first use playlist name */
                msg = g_strdup_printf(ngettext ("Deleted playlist '%s' including %d member track",
                        "Deleted playlist '%s' including %d member tracks",
                        n), dd->pl->name, n);
                /* then remove playlist */
                gp_playlist_remove(dd->pl);
            }
            break;
        case DELETE_ACTION_PLAYLIST:
            if (itdb_playlist_is_mpl(dd->pl)) { /* not allowed -- programming error */
                g_return_if_reached ();
            }
            else {
                /* first use playlist name */
                msg = g_strdup_printf(_("Deleted playlist '%s'"), dd->pl->name);
                /* then remove playlist */
                gp_playlist_remove(dd->pl);
            }
            break;
        case DELETE_ACTION_IPOD:
            /* not allowed -- programming error */
            g_return_if_reached ();
            break;
        }
    }
    delete_playlist_cleanup(dd);

    gtkpod_statusbar_message(msg);
    g_free(msg);
}

void delete_playlist_head(DeleteAction deleteaction) {
    struct DeleteData *dd;
    iTunesDB *itdb;
    GtkResponseType response = GTK_RESPONSE_NONE;
    gchar *label = NULL, *title = NULL;
    gboolean confirm_again;
    gchar *confirm_again_key;
    guint32 n = 0;
    GString *str;

    Playlist *playlist = gtkpod_get_current_playlist();
    if (!playlist) { /* no playlist selected */
        gtkpod_statusbar_message(_("No playlist selected"));
        return;
    }

    itdb = playlist->itdb;
    g_return_if_fail (itdb);

    dd = g_malloc0(sizeof(struct DeleteData));
    dd->deleteaction = deleteaction;
    dd->pl = playlist;
    dd->itdb = itdb;

    if (itdb->usertype & GP_ITDB_TYPE_IPOD) {
        switch (deleteaction) {
        case DELETE_ACTION_IPOD:
            if (itdb_playlist_is_mpl(playlist)) {
                label = g_strdup_printf(_("Are you sure you want to remove all tracks from your iPod?"));
            }
            else if (itdb_playlist_is_podcasts(playlist)) { /* podcasts playlist */
                dd->tracks = g_list_copy(playlist->members);
                label = g_strdup_printf(_("Are you sure you want to remove all podcasts from your iPod?"));
            }
            else { /* normal playlist */
                /* we set selected_tracks to get a list printed by
                 * delete_populate_settings() further down */
                dd->tracks = g_list_copy(playlist->members);
                label
                        = g_strdup_printf(ngettext ("Are you sure you want to delete playlist '%s' and the following track completely from your iPod? The number of playlists this track is a member of is indicated in parentheses.",
                                "Are you sure you want to delete playlist '%s' and the following tracks completely from your iPod? The number of playlists the tracks are member of is indicated in parentheses.", n), playlist->name);
            }
            break;
        case DELETE_ACTION_PLAYLIST:
            if (itdb_playlist_is_mpl(playlist)) { /* not allowed -- programming error */
                g_return_if_reached ();
            }
            else {
                label = g_strdup_printf(_("Are you sure you want to delete the playlist '%s'?"), playlist->name);
            }
            break;
        case DELETE_ACTION_LOCAL:
        case DELETE_ACTION_DATABASE:
            /* not allowed -- programming error */
            g_return_if_reached ();
            break;
        }
    }
    if (itdb->usertype & GP_ITDB_TYPE_LOCAL) {
        switch (deleteaction) {
        case DELETE_ACTION_LOCAL:
            if (itdb_playlist_is_mpl(playlist)) { /* for safety reasons this is not implemented (would
             remove all tracks from your local harddisk */
                g_return_if_reached ();
            }
            else {
                /* we set selected_tracks to get a list printed by
                 * delete_populate_settings() further down */
                dd->tracks = g_list_copy(playlist->members);
                label
                        = g_strdup_printf(ngettext ("Are you sure you want to delete playlist '%s' and remove the following track from your harddisk? The number of playlists this track is a member of is indicated in parentheses.",
                                "Are you sure you want to delete playlist '%s' and remove the following tracks from your harddisk? The number of playlists the tracks are member of is indicated in parentheses.",
                                n), playlist->name);
            }
            break;
        case DELETE_ACTION_DATABASE:
            if (itdb_playlist_is_mpl(playlist)) {
                label = g_strdup_printf(_("Are you sure you want to remove all tracks from the database?"));

            }
            else {
                /* we set selected_tracks to get a list printed by
                 * delete_populate_settings() further down */
                dd->tracks = g_list_copy(playlist->members);
                label
                        = g_strdup_printf(ngettext ("Are you sure you want to delete playlist '%s' and remove the following track from the database? The number of playlists this track is a member of is indicated in parentheses.",
                                "Are you sure you want to delete playlist '%s' and remove the following tracks from the database? The number of playlists the tracks are member of is indicated in parentheses.",
                                n), playlist->name);
            }
            break;
        case DELETE_ACTION_PLAYLIST:
            if (itdb_playlist_is_mpl(playlist)) { /* not allowed -- programming error */
                g_return_if_reached ();
            }
            else {
                label = g_strdup_printf(_("Are you sure you want to delete the playlist '%s'?"), playlist->name);
            }
            break;
        case DELETE_ACTION_IPOD:
            /* not allowed -- programming error */
            g_return_if_reached ();
            break;
        }
    }

    delete_populate_settings(dd, NULL, &title, &confirm_again, &confirm_again_key, &str);

    response = gtkpod_confirmation(-1, /* gint id, */
    TRUE, /* gboolean modal, */
    title, /* title */
    label, /* label */
    str->str, /* scrolled text */
    NULL, 0, NULL, /* option 1 */
    NULL, 0, NULL, /* option 2 */
    confirm_again, /* gboolean confirm_again, */
    confirm_again_key, /* ConfHandlerOpt confirm_again_key,*/
    CONF_NULL_HANDLER, /* ConfHandler ok_handler,*/
    NULL, /* don't show "Apply" button */
    CONF_NULL_HANDLER, /* cancel_handler,*/
    NULL, /* gpointer user_data1,*/
    NULL); /* gpointer user_data2,*/

    g_free(label);
    g_free(title);
    g_free(confirm_again_key);
    g_string_free(str, TRUE);

    switch (response) {
    case GTK_RESPONSE_OK:
        delete_playlist_ok(dd);
        break;
    default:
        delete_playlist_cleanup(dd);
        break;
    }
}

void copy_playlist_to_target_playlist(Playlist *pl, Playlist *t_pl) {
    GList *addtracks = NULL;
    Playlist *t_mpl;
    Exporter *exporter;

    g_return_if_fail (pl);
    g_return_if_fail (t_pl);

    t_mpl = itdb_playlist_mpl(t_pl->itdb);
    g_return_if_fail(t_mpl);

    exporter = gtkpod_get_exporter();
    g_return_if_fail(exporter);

    addtracks = exporter_transfer_track_glist_between_itdbs(exporter, pl->itdb, t_pl->itdb, pl->members);
    if (addtracks || !pl->members) {
        add_trackglist_to_playlist(t_pl, addtracks);
        gtkpod_statusbar_message(_("Copied '%s' playlist to '%s' in '%s'"), pl->name, t_pl->name, t_mpl->name);
        g_list_free(addtracks);
        addtracks = NULL;
    }
}

/*
 * Copy the selected playlist to a specified itdb.
 */
void copy_playlist_to_target_itdb(Playlist *pl, iTunesDB *t_itdb) {
    Playlist *pl_n;
    GList *addtracks = NULL;
    Exporter *exporter;

    g_return_if_fail (pl);
    g_return_if_fail (t_itdb);

    exporter = gtkpod_get_exporter();
    g_return_if_fail(exporter);

    if (pl->itdb != t_itdb) {
        addtracks = exporter_transfer_track_glist_between_itdbs(exporter, pl->itdb, t_itdb, pl->members);
        if (addtracks || !pl->members) {
            pl_n = gp_playlist_add_new(t_itdb, pl->name, FALSE, -1);
            add_trackglist_to_playlist(pl_n, addtracks);
            gtkpod_statusbar_message(_("Copied \"%s\" playlist to %s"), pl->name, (itdb_playlist_mpl(t_itdb))->name);
        }
        g_list_free(addtracks);
        addtracks = NULL;
    }
    else {
        pl_n = itdb_playlist_duplicate(pl);
        g_return_if_fail (pl_n);
        gp_playlist_add(t_itdb, pl_n, -1);
    }
}

const gchar* return_playlist_stock_image(Playlist *playlist) {
    Itdb_iTunesDB *itdb;
    ExtraiTunesDBData *eitdb;

    const gchar *stock_id = NULL;

    g_return_val_if_fail (playlist, NULL);
    g_return_val_if_fail (playlist->itdb, NULL);

    itdb = playlist->itdb;
    g_return_val_if_fail (itdb->userdata, NULL);
    eitdb = itdb->userdata;

    if (playlist->is_spl) {
        stock_id = GTK_STOCK_PROPERTIES;
    }
    else if (!itdb_playlist_is_mpl(playlist)) {
        stock_id = PLAYLIST_DISPLAY_PLAYLIST_ICON_STOCK_ID;
    }
    else {
        if (itdb->usertype & GP_ITDB_TYPE_LOCAL) {
            stock_id = GTK_STOCK_HARDDISK;
        }
        else {
            if (eitdb->itdb_imported) {
                stock_id = GTK_STOCK_CONNECT;
            }
            else {
                stock_id = GTK_STOCK_DISCONNECT;
            }
        }
    }

    return stock_id;
}

/*------------------------------------------------------------------*\
 *                                                                  *
 *              Frequently used error messages *
 *                                                                  *
 \*------------------------------------------------------------------*/

void message_sb_no_itdb_selected() {
    gtkpod_statusbar_message(_("No database or playlist selected"));
}

void message_sb_no_playlist_selected() {
    gtkpod_statusbar_message(_("No playlist selected"));
}

void message_sb_no_ipod_itdb_selected() {
    gtkpod_statusbar_message(_("No iPod or iPod playlist selected"));
}
