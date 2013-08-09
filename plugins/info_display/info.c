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

/* This file provides functions for the info window as well as for the
 * statusbar handling */

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include "plugin.h"
#include "info.h"
#include "infoview.h"
#include "libgtkpod/gtkpod_app_iface.h"
#include "libgtkpod/misc.h"
#include "libgtkpod/misc_track.h"
#include "libgtkpod/prefs.h"

#define SPACE_TIMEOUT 1000

/* pointer to info window */
static GtkWidget *info_window = NULL;

/* lock for size related variables (used by child and parent) */
static GMutex mutex; /* shared lock */

static gboolean space_uptodate = FALSE;
static gchar *space_mp = NULL; /* thread save access through mutex */
static iTunesDB *space_itdb = NULL; /* semi thread save access
 * (space_itdb will not be changed
 * while locked) */
static gdouble space_ipod_free = 0; /* thread save access through mutex */
static gdouble space_ipod_used = 0; /* thread save access through mutex */

static GList *callbacks_info_update = NULL;
static GList *callbacks_info_update_track_view = NULL;
static GList *callbacks_info_update_playlist_view = NULL;
static GList *callbacks_info_update_totals_view = NULL;

#if 0
static gdouble get_ipod_used_space(void);
#endif

static void _lock_mutex() {
    g_mutex_lock (&mutex);
}

static void _unlock_mutex() {
    g_mutex_unlock (&mutex);
}

/* callback management */
static void register_callback(GList **list, info_update_callback cb) {
    if (*list && g_list_index(*list, cb) != -1)
        return;

    *list = g_list_append(*list, cb);
}

static void unregister_callback(GList **list, info_update_callback cb) {
    if (*list)
        *list = g_list_remove(*list, cb);
}

static void callback_call_all(GList *list) {
    for (; list; list = list->next) {
        ((info_update_callback) list->data)();
    }
}

void register_info_update(info_update_callback cb) {
    register_callback(&callbacks_info_update, cb);
}

void register_info_update_track_view(info_update_callback cb) {
    register_callback(&callbacks_info_update_track_view, cb);
}

void register_info_update_playlist_view(info_update_callback cb) {
    register_callback(&callbacks_info_update_playlist_view, cb);
}

void register_info_update_totals_view(info_update_callback cb) {
    register_callback(&callbacks_info_update_totals_view, cb);
}

void unregister_info_update(info_update_callback cb) {
    unregister_callback(&callbacks_info_update, cb);
}

void unregister_info_update_track_view(info_update_callback cb) {
    unregister_callback(&callbacks_info_update_track_view, cb);
}

void unregister_info_update_playlist_view(info_update_callback cb) {
    unregister_callback(&callbacks_info_update_playlist_view, cb);
}

void unregister_info_update_totals_view(info_update_callback cb) {
    unregister_callback(&callbacks_info_update_totals_view, cb);
}

/* Is the iPod connected? If space_ipod_used and space_ipod_free are
 both zero, we assume the iPod is not connected */
gboolean ipod_connected(void) {
    gboolean result;
    _lock_mutex();
    if ((space_ipod_used == 0) && (space_ipod_free == 0))
        result = FALSE;
    else
        result = TRUE;
    _unlock_mutex();
    return result;
}

/* iPod space has to be reread */
static void space_data_update(void) {
    space_uptodate = FALSE;
}

/* fill in tracks, playtime and filesize from track list @tl */
void fill_in_info(GList *tl, guint32 *tracks, guint32 *playtime, gdouble *filesize) {
    GList *gl;

    g_return_if_fail (tracks);
    g_return_if_fail (playtime);
    g_return_if_fail (filesize);

    *tracks = 0;
    *playtime = 0;
    *filesize = 0;

    for (gl = tl; gl; gl = gl->next) {
        Track *s = gl->data;
        *tracks += 1;
        *playtime += s->tracklen / 1000;
        *filesize += s->size;
    }
}

/* update all sections of info window */
void info_update(void) {
    callback_call_all(callbacks_info_update);

    info_update_track_view();
    info_update_playlist_view();
    info_update_totals_view();
}

static void info_update_track_view_displayed(void) {
    guint32 tracks, playtime; /* playtime in secs */
    gdouble filesize; /* in bytes */
    GList *displayed;

    if (!info_window)
        return; /* not open */
    displayed = gtkpod_get_displayed_tracks();
    fill_in_info(displayed, &tracks, &playtime, &filesize);
}

static void info_update_track_view_selected(void) {
    guint32 tracks, playtime; /* playtime in secs */
    gdouble filesize; /* in bytes */
    GList *selected;

    if (!info_window)
        return; /* not open */
    selected = gtkpod_get_selected_tracks();
    fill_in_info(selected, &tracks, &playtime, &filesize);
    g_list_free(selected);
}

/* update track view section */
void info_update_track_view(void) {
    callback_call_all(callbacks_info_update_track_view);

    if (!info_window)
        return; /* not open */
    info_update_track_view_displayed();
    info_update_track_view_selected();
}

/* update playlist view section */
void info_update_playlist_view(void) {
    callback_call_all(callbacks_info_update_playlist_view);

    guint32 tracks, playtime; /* playtime in secs */
    gdouble filesize; /* in bytes */
    GList *tl;

    if (!info_window)
        return; /* not open */
    Playlist *pl = gtkpod_get_current_playlist();
    if (!pl)
        return;
    tl = pl->members;
    fill_in_info(tl, &tracks, &playtime, &filesize);
}

/* Get the local itdb */
iTunesDB *get_itdb_local(void) {
    struct itdbs_head *itdbs_head;
    GList *gl;

    g_return_val_if_fail (gtkpod_app, NULL);
    itdbs_head = g_object_get_data(G_OBJECT (gtkpod_app), "itdbs_head");
    if (!itdbs_head)
        return NULL;
    for (gl = itdbs_head->itdbs; gl; gl = gl->next) {
        iTunesDB *itdb = gl->data;
        g_return_val_if_fail (itdb, NULL);
        if (itdb->usertype & GP_ITDB_TYPE_LOCAL)
            return itdb;
    }
    return NULL;
}

/* Get the iPod itdb */
/* FIXME: This function must be expanded if support for several iPods
 is implemented */
iTunesDB *get_itdb_ipod(void) {
    struct itdbs_head *itdbs_head;
    GList *gl;

    g_return_val_if_fail (gtkpod_app, NULL);
    itdbs_head = g_object_get_data(G_OBJECT (gtkpod_app), "itdbs_head");
    if (!itdbs_head)
        return NULL;
    for (gl = itdbs_head->itdbs; gl; gl = gl->next) {
        iTunesDB *itdb = gl->data;
        g_return_val_if_fail (itdb, NULL);
        if (itdb->usertype & GP_ITDB_TYPE_IPOD)
            return itdb;
    }
    return NULL;
}

/* update "free space" section of totals view */
static void info_update_totals_view_space(void) {
    gdouble nt_filesize, del_filesize;
    guint32 nt_tracks, del_tracks;
    iTunesDB *itdb;

    if (!info_window)
        return;
    itdb = get_itdb_ipod();
    if (itdb) {
        gp_info_nontransferred_tracks(itdb, &nt_filesize, &nt_tracks);
        gp_info_deleted_tracks(itdb, &del_filesize, &del_tracks);
    }
}

/* update "totals" view section */
void info_update_totals_view(void) {
    guint32 tracks = 0, playtime = 0; /* playtime in secs */
    gdouble filesize = 0; /* in bytes */
    Playlist *pl;
    iTunesDB *itdb;

    callback_call_all(callbacks_info_update_totals_view);

    if (!info_window)
        return; /* not open */

    itdb = get_itdb_ipod();
    if (itdb) {
        pl = itdb_playlist_mpl(itdb);
        g_return_if_fail (pl);
        fill_in_info(pl->members, &tracks, &playtime, &filesize);
    }
    itdb = get_itdb_local();
    if (itdb) {
        pl = itdb_playlist_mpl(itdb);
        g_return_if_fail (pl);
        fill_in_info(pl->members, &tracks, &playtime, &filesize);
    }
    info_update_totals_view_space();
}

/*------------------------------------------------------------------*\
 *                                                                  *
 *                       free space stuff                           *
 *                                                                  *
 \*------------------------------------------------------------------*/

/* Since the mount point is used by two separate threads, it can only
 be accessed securely by using a locking mechanism. Therefore we
 keep a copy of the mount point here. Access must only be done
 after locking. */
void space_set_ipod_itdb(iTunesDB *itdb) {
    const gchar *mp = NULL;

    if (itdb) {
        ExtraiTunesDBData *eitdb = itdb->userdata;
        g_return_if_fail (eitdb);

        if (!eitdb->ipod_ejected) {
            mp = itdb_get_mountpoint(itdb);
        }
    }

    _lock_mutex ();

    space_itdb = itdb;

    /* update the free space data if mount point changed */
    if (!space_mp || !mp || (strcmp(space_mp, mp) != 0)) {
        g_free(space_mp);
        space_mp = g_strdup(mp);

        space_data_update();
    }

    _unlock_mutex ();
}

/* retrieve the currently set ipod itdb -- needed in case the itdb is
 deleted */
iTunesDB *space_get_ipod_itdb(void) {
    return space_itdb;
}

/* in Bytes */
gdouble get_ipod_free_space(void) {
    gdouble result;
    _lock_mutex ();
    result = space_ipod_free;
    _unlock_mutex ();
    return result;
}

#if 0
/* in Bytes */
static gdouble get_ipod_used_space(void)
{
    gdouble result;
    _lock_mutex ();
    result = space_ipod_used;
    _unlock_mutex ();
    return result;
}
#endif

/* @size: size in B */
gchar*
get_filesize_as_string(gdouble size) {
    guint i = 0;
    gchar *result = NULL;
    gchar *sizes[] =
        { _("B"), _("kB"), _("MB"), _("GB"), _("TB"), NULL };

    while ((fabs(size) > 1024) && (i < 4)) {
        size /= 1024;
        ++i;
    }
    if (i > 0) {
        if (fabs(size) < 10)
            result = g_strdup_printf("%0.2f %s", size, sizes[i]);
        else if (fabs(size) < 100)
            result = g_strdup_printf("%0.1f %s", size, sizes[i]);
        else
            result = g_strdup_printf("%0.0f %s", size, sizes[i]);
    }
    else { /* Bytes do not have decimal places */
        result = g_strdup_printf("%0.0f %s", size, sizes[i]);
    }
    return result;
}

/* Action Callbacks */
void on_info_view_open(GtkAction *action, InfoDisplayPlugin* plugin) {
    open_info_view();
}

/* Selection Callbacks */
void info_display_playlist_selected_cb(GtkPodApp *app, gpointer pl, gpointer data) {
    Playlist *playlist = pl;
    if (playlist && playlist->itdb) {
        space_set_ipod_itdb(playlist->itdb);
    }
    else {
        space_set_ipod_itdb(NULL);
    }

    info_update();
}

void info_display_playlist_added_cb(GtkPodApp *app, gpointer pl, gint32 pos, gpointer data) {
    info_update();
}

void info_display_playlist_removed_cb(GtkPodApp *app, gpointer pl, gpointer data) {
    info_update();
}

void info_display_tracks_selected_cb(GtkPodApp *app, gpointer tk, gpointer data) {
    info_update();
}

void info_display_tracks_displayed_cb(GtkPodApp *app, gpointer tk, gpointer data) {
    info_update();
}

void info_display_track_updated_cb(GtkPodApp *app, gpointer tk, gpointer data) {
    info_update();
}

void info_display_itdb_changed_cb(GtkPodApp *app, gpointer itdb, gpointer data) {
    info_update();
}

void info_display_track_removed_cb(GtkPodApp *app, gpointer tk, gint32 pos, gpointer data) {
    info_update();
}
