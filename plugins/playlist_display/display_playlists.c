/*
 |  Copyright (C) 2002-2010 Jorg Schuler <jcsjcs at users sourceforge net>
 |                                          Paul Richardson <phantom_sf at users.sourceforge.net>
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
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include "libgtkpod/gp_itdb.h"
#include "plugin.h"
#include "display_playlists.h"
#include "playlist_display_actions.h"
#include "playlist_display_context_menu.h"
#include "libgtkpod/gp_private.h"
#include "libgtkpod/directories.h"
#include "libgtkpod/file.h"
#include "libgtkpod/misc.h"
#include "libgtkpod/misc_track.h"
#include "libgtkpod/misc_playlist.h"
#include "libgtkpod/gtkpod_app_iface.h"
#include "libgtkpod/prefs.h"

/* pointer to the playlist display's toolbar */
static GtkToolbar *playlist_toolbar = NULL;
/* pointer to the treeview for the playlist display */
static GtkTreeView *playlist_treeview = NULL;
/* flag set if selection changes to be ignored temporarily */
static gboolean pm_selection_blocked = FALSE;

/* Drag and drop definitions */
static GtkTargetEntry pm_drag_types[] =
    {
        { DND_GTKPOD_PLAYLISTLIST_TYPE, 0, DND_GTKPOD_PLAYLISTLIST },
        { "text/uri-list", 0, DND_TEXT_URI_LIST },
        { "text/plain", 0, DND_TEXT_PLAIN },
        { "STRING", 0, DND_TEXT_PLAIN } };
static GtkTargetEntry pm_drop_types[] =
    {
        { DND_GTKPOD_PLAYLISTLIST_TYPE, 0, DND_GTKPOD_PLAYLISTLIST },
        { DND_GTKPOD_TRACKLIST_TYPE, 0, DND_GTKPOD_TRACKLIST },
        { "text/uri-list", 0, DND_TEXT_URI_LIST },
        { "text/plain", 0, DND_TEXT_PLAIN },
        { "STRING", 0, DND_TEXT_PLAIN } };

static void pm_create_treeview(void);
static void pm_rows_reordered(void);
static GtkTreePath *pm_get_path_for_itdb(Itdb_iTunesDB *itdb);
static GtkTreePath *pm_get_path_for_playlist(Playlist *pl);
static gint pm_get_position_for_playlist(Playlist *pl);
static gboolean pm_get_iter_for_itdb(Itdb_iTunesDB *itdb, GtkTreeIter *iter);
static gboolean pm_get_iter_for_playlist(Playlist *pl, GtkTreeIter *iter);

/* ---------------------------------------------------------------- */
/* Section for playlist display                                     */
/* drag and drop                                                    */
/* ---------------------------------------------------------------- */

/* ----------------------------------------------------------------
 *
 * For drag and drop within the playlist view the following rules apply:
 *
 * 1) Drags between different itdbs: playlists are copied (moved with
 *    CONTROL pressed)
 *
 * 2) Drags within the same itdb: playlist is moved (copied with CONTROL
 *    pressed)
 *
 * ---------------------------------------------------------------- */
static void pm_drag_begin(GtkWidget *widget, GdkDragContext *drag_context, gpointer user_data) {
    /* nothing to do */
}

static void pm_drag_data_delete_remove_playlist(GtkTreeModel *tm, GtkTreePath *tp, GtkTreeIter *iter, gpointer data) {
    Playlist *pl;
    g_return_if_fail (tm);
    g_return_if_fail (iter);
    gtk_tree_model_get(tm, iter, PM_COLUMN_PLAYLIST, &pl, -1);
    g_return_if_fail (pl);
    gp_playlist_remove(pl);
}

/* remove dragged playlist after successful MOVE */
static void pm_drag_data_delete(GtkWidget *widget, GdkDragContext *drag_context, gpointer user_data) {
    g_return_if_fail (widget);
    g_return_if_fail (drag_context);

    if (gdk_drag_context_get_selected_action(drag_context) == GDK_ACTION_MOVE) {
        GtkTreeSelection *ts = gtk_tree_view_get_selection(GTK_TREE_VIEW (widget));
        gtk_tree_selection_selected_foreach(ts, pm_drag_data_delete_remove_playlist, NULL);
    }
}

static gboolean pm_drag_drop(GtkWidget *widget, GdkDragContext *drag_context, gint x, gint y, guint time, gpointer user_data) {
    GdkAtom target;

    gp_remove_autoscroll_row_timeout(widget);

    target = gtk_drag_dest_find_target(widget, drag_context, NULL);

    if (target != GDK_NONE) {
        gtk_drag_get_data(widget, drag_context, target, time);
        return TRUE;
    }
    return FALSE;
}

static void pm_drag_end(GtkWidget *widget, GdkDragContext *drag_context, gpointer user_data) {
    gp_remove_autoscroll_row_timeout(widget);
    gtkpod_tracks_statusbar_update();
}

static void pm_drag_leave(GtkWidget *widget, GdkDragContext *drag_context, guint time, gpointer user_data) {
    gp_remove_autoscroll_row_timeout(widget);
}

static gboolean pm_drag_motion(GtkWidget *widget, GdkDragContext *dc, gint x, gint y, guint time, gpointer user_data) {
    GtkTreeModel *model;
    GtkTreeIter iter_d;
    GtkTreePath *path;
    GtkTreeViewDropPosition pos;
    GdkAtom target;
    guint info;
    PM_column_type type;
    Playlist *pl_d;
    Itdb_iTunesDB *itdb;
    PhotoDB *photodb;
    ExtraiTunesDBData *eitdb;

    g_return_val_if_fail (widget, FALSE);
    g_return_val_if_fail (GTK_IS_TREE_VIEW (widget), FALSE);

    gp_install_autoscroll_row_timeout(widget);

    /* no drop possible if position is not valid */
    if (!gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW (widget), x, y, &path, &pos))
        return FALSE;

    g_return_val_if_fail (path, FALSE);

    gtk_tree_view_set_drag_dest_row(GTK_TREE_VIEW (widget), path, pos);

    /* Get destination playlist in case it's needed */
    model = gtk_tree_view_get_model(GTK_TREE_VIEW (widget));
    g_return_val_if_fail (model, FALSE);

    if (gtk_tree_model_get_iter(model, &iter_d, path)) {
        gtk_tree_model_get(model, &iter_d, PM_COLUMN_TYPE, &type, PM_COLUMN_ITDB, &itdb, PM_COLUMN_PLAYLIST, &pl_d, PM_COLUMN_PHOTOS, &photodb, -1);
    }
    g_return_val_if_fail (itdb, FALSE);
    eitdb = itdb->userdata;
    g_return_val_if_fail (eitdb, FALSE);

    target = gtk_drag_dest_find_target(widget, dc, NULL);

    /* no drop possible if repository is not loaded */
    if (!eitdb->itdb_imported) {
        gtk_tree_path_free(path);
        gdk_drag_status(dc, 0, time);
        return FALSE;
    }

    /* no drop possible if no valid target can be found */
    if (target == GDK_NONE) {
        gtk_tree_path_free(path);
        gdk_drag_status(dc, 0, time);
        return FALSE;
    }

    /* no drop possible _before_ MPL */
    if (gtk_tree_path_get_depth(path) == 1) { /* MPL */
        if (pos == GTK_TREE_VIEW_DROP_BEFORE) {
            gtk_tree_path_free(path);
            gdk_drag_status(dc, 0, time);
            return FALSE;
        }
    }

    /* get 'info'-id from target-atom */
    if (!gtk_target_list_find(gtk_drag_dest_get_target_list(widget), target, &info)) {
        gtk_tree_path_free(path);
        gdk_drag_status(dc, 0, time);
        return FALSE;
    }

    switch (type) {
    case PM_COLUMN_PLAYLIST:
        switch (info) {
        case DND_GTKPOD_PLAYLISTLIST:
            /* need to consult drag data to decide */
            g_object_set_data(G_OBJECT (widget), "drag_data_by_motion_path", path);
            g_object_set_data(G_OBJECT (widget), "drag_data_by_motion_pos", (gpointer) pos);
            gtk_drag_get_data(widget, dc, target, time);
            return TRUE;
        case DND_GTKPOD_TRACKLIST:
            /* do not allow drop into currently selected playlist */
            if (pl_d == pm_get_first_selected_playlist()) {
                if ((pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE) || (pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)) {
                    gtk_tree_path_free(path);
                    gdk_drag_status(dc, 0, time);
                    return FALSE;
                }
            }
            /* need to consult drag data to decide */
            g_object_set_data(G_OBJECT (widget), "drag_data_by_motion_path", path);
            g_object_set_data(G_OBJECT (widget), "drag_data_by_motion_pos", (gpointer) pos);
            gtk_drag_get_data(widget, dc, target, time);
            return TRUE;
        case DND_TEXT_PLAIN:
        case DND_TEXT_URI_LIST:
            gdk_drag_status(dc, gdk_drag_context_get_suggested_action(dc), time);
            gtk_tree_path_free(path);
            return TRUE;
        default:
            g_warning ("Programming error: pm_drag_motion received unknown info type (%d)\n", info);
            gtk_tree_path_free(path);
            return FALSE;
        }
        g_return_val_if_reached (FALSE);

    case PM_COLUMN_PHOTOS:
        /* We don't handle drops into the photo playlist yet */
        return FALSE;
    case PM_NUM_COLUMNS:
    case PM_COLUMN_ITDB:
    case PM_COLUMN_TYPE:
        g_return_val_if_reached (FALSE);
    }
    g_return_val_if_reached (FALSE);
}

/*
 * utility function for appending file for track view (DND)
 */
static void on_pm_dnd_get_file_foreach(GtkTreeModel *tm, GtkTreePath *tp, GtkTreeIter *iter, gpointer data) {
    Playlist *pl = NULL;
    GList *gl;
    GString *filelist = data;

    g_return_if_fail (tm);
    g_return_if_fail (iter);
    g_return_if_fail (data);

    /* get current playlist */
    gtk_tree_model_get(tm, iter, PM_COLUMN_PLAYLIST, &pl, -1);
    g_return_if_fail (pl);

    for (gl = pl->members; gl; gl = gl->next) {
        gchar *name;
        Track *track = gl->data;

        g_return_if_fail (track);
        name = get_file_name_from_source(track, SOURCE_PREFER_LOCAL);
        if (name) {
            g_string_append_printf(filelist, "file:%s\n", name);
            g_free(name);
        }
    }
}

/*
 * utility function for appending uris for track view (DND)
 */
static void on_pm_dnd_get_uri_foreach(GtkTreeModel *tm, GtkTreePath *tp, GtkTreeIter *iter, gpointer data) {
    Playlist *pl = NULL;
    GList *gl;
    GString *filelist = data;

    g_return_if_fail (tm);
    g_return_if_fail (iter);
    g_return_if_fail (data);

    /* get current playlist */
    gtk_tree_model_get(tm, iter, PM_COLUMN_PLAYLIST, &pl, -1);
    g_return_if_fail (pl);

    for (gl = pl->members; gl; gl = gl->next) {
        gchar *name;
        Track *track = gl->data;

        g_return_if_fail (track);
        name = get_file_name_from_source(track, SOURCE_PREFER_LOCAL);
        if (name) {
            gchar *uri = g_filename_to_uri(name, NULL, NULL);
            if (uri) {
                g_string_append_printf(filelist, "file:%s\n", name);
                g_free(uri);
            }
            g_free(name);
        }
    }
}

/*
 * utility function for appending pointers to a playlist (DND)
 */
static void on_pm_dnd_get_playlist_foreach(GtkTreeModel *tm, GtkTreePath *tp, GtkTreeIter *iter, gpointer data) {
    Playlist *pl = NULL;
    GString *playlistlist = (GString *) data;

    g_return_if_fail (tm);
    g_return_if_fail (iter);
    g_return_if_fail (playlistlist);

    gtk_tree_model_get(tm, iter, PM_COLUMN_PLAYLIST, &pl, -1);
    g_return_if_fail (pl);
    g_string_append_printf(playlistlist, "%p\n", pl);
}

static void pm_drag_data_get(GtkWidget *widget, GdkDragContext *dc, GtkSelectionData *data, guint info, guint time, gpointer user_data) {
    GtkTreeSelection *ts;
    GString *reply = g_string_sized_new(2000);

    if (!data)
        return;

    ts = gtk_tree_view_get_selection(GTK_TREE_VIEW (widget));
    if (ts) {
        switch (info) {
        case DND_GTKPOD_PLAYLISTLIST:
            gtk_tree_selection_selected_foreach(ts, on_pm_dnd_get_playlist_foreach, reply);
            break;
        case DND_TEXT_PLAIN:
            gtk_tree_selection_selected_foreach(ts, on_pm_dnd_get_file_foreach, reply);
            break;
        case DND_TEXT_URI_LIST:
            gtk_tree_selection_selected_foreach(ts, on_pm_dnd_get_uri_foreach, reply);
            break;
        default:
            g_warning ("Programming error: pm_drag_data_get received unknown info type (%d)\n", info);
            break;
        }
    }
    gtk_selection_data_set(data, gtk_selection_data_get_target(data), 8, reply->str, reply->len);
    g_string_free(reply, TRUE);
}

/* get the action to be used for a drag and drop within the playlist
 * view */
static GdkDragAction pm_pm_get_action(Playlist *src, Playlist *dest, GtkWidget *widget, GtkTreeViewDropPosition pos, GdkDragContext *dc) {
    GdkModifierType mask;

    g_return_val_if_fail (src, 0);
    g_return_val_if_fail (dest, 0);
    g_return_val_if_fail (widget, 0);
    g_return_val_if_fail (dc, 0);

    /* get modifier mask */
    gdk_window_get_pointer(gtk_tree_view_get_bin_window(GTK_TREE_VIEW (widget)), NULL, NULL, &mask);

    /* don't allow copy/move before the MPL */
    if ((itdb_playlist_is_mpl(dest)) && (pos == GTK_TREE_VIEW_DROP_BEFORE))
        return 0;

    /* don't allow moving of MPL */
    if (itdb_playlist_is_mpl(src))
        return GDK_ACTION_COPY;

    /* don't allow drop onto itself */
    if ((src == dest) && ((pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER) || (pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE)))
        return 0;

    if (src->itdb == dest->itdb) { /* DND within the same itdb */
        /* don't allow copy/move onto MPL */
        if ((itdb_playlist_is_mpl(dest)) && (pos != GTK_TREE_VIEW_DROP_AFTER))
            return 0;

        /* DND within the same itdb -> default is moving, shift means
         copying */
        if (mask & GDK_SHIFT_MASK) {
            return GDK_ACTION_COPY;
        }
        else { /* don't allow move if view is sorted */
            gint column;
            GtkSortType order;
            GtkTreeModel *model;
            GtkWidget *src_widget = gtk_drag_get_source_widget(dc);
            g_return_val_if_fail (src_widget, 0);
            model = gtk_tree_view_get_model(GTK_TREE_VIEW(src_widget));
            g_return_val_if_fail (model, 0);
            if (gtk_tree_sortable_get_sort_column_id(GTK_TREE_SORTABLE (model), &column, &order)) {
                return 0;
            }
            else {
                return GDK_ACTION_MOVE;
            }
        }
    }
    else { /* DND between different itdbs */
        /* Do not allow drags from the iPod in offline mode */
        if (get_offline(src->itdb) && (src->itdb->usertype & GP_ITDB_TYPE_IPOD)) { /* give a notice on the statusbar -- otherwise the user
         * will never know why the drag is not possible */
            gtkpod_statusbar_message(_("Error: drag from iPod not possible in offline mode."));
            return 0;
        }
        /* default is copying, shift means moving */
        if (mask & GDK_SHIFT_MASK)
            return GDK_ACTION_MOVE;
        else
            return GDK_ACTION_COPY;
    }
}

/* get the action to be used for a drag and drop from the track view
 * or filter tab view to the playlist view */
static GdkDragAction pm_tm_get_action(Track *src, Playlist *dest, GtkTreeViewDropPosition pos, GdkDragContext *dc) {
    g_return_val_if_fail (src, 0);
    g_return_val_if_fail (dest, 0);
    g_return_val_if_fail (dc, 0);

    /* don't allow copy/move before the MPL */
    if ((itdb_playlist_is_mpl(dest)) && (pos == GTK_TREE_VIEW_DROP_BEFORE))
        return 0;

    if (src->itdb == dest->itdb) { /* DND within the same itdb */
        /* don't allow copy/move onto MPL */
        if ((itdb_playlist_is_mpl(dest)) && (pos != GTK_TREE_VIEW_DROP_AFTER))
            return 0;
    }
    else { /* drag between different itdbs */
        /* Do not allow drags from the iPod in offline mode */
        if (get_offline(src->itdb) && (src->itdb->usertype & GP_ITDB_TYPE_IPOD)) { /* give a notice on the statusbar -- otherwise the user
         * will never know why the drag is not possible */
            gtkpod_statusbar_message(_("Error: drag from iPod not possible in offline mode."));
            return 0;
        }
    }
    /* otherwise: do as suggested */
    return gdk_drag_context_get_suggested_action(dc);
}

/* Print a message about the number of tracks copied (the number of
 tracks moved is printed in tm_drag_data_delete()) */
static void pm_tm_tracks_moved_or_copied(gchar *tracks, gboolean moved) {
    g_return_if_fail (tracks);
    if (!moved) {
        gchar *ptr = tracks;
        gint n = 0;

        /* count the number of tracks */
        while ((ptr = strchr(ptr, '\n'))) {
            ++n;
            ++ptr;
        }
        /* display message in statusbar */
        gtkpod_statusbar_message(ngettext ("Copied one track",
                "Copied %d tracks", n), n);
    }
}

static gint pm_adjust_for_drop_pos(gint position, GtkTreeViewDropPosition pos)
{
    switch (pos) {
    case GTK_TREE_VIEW_DROP_BEFORE:
    case GTK_TREE_VIEW_DROP_INTO_OR_BEFORE:
        return position;
    case GTK_TREE_VIEW_DROP_AFTER:
    case GTK_TREE_VIEW_DROP_INTO_OR_AFTER:
        return position + 1;
    default:
        g_warn_if_reached();
        return position;
    }
}

static void pm_drag_data_received(GtkWidget *widget, GdkDragContext *dc, gint x, gint y, GtkSelectionData *data, guint info, guint time, gpointer user_data) {
    GtkTreeIter iter_d, iter_s;
    GtkTreePath *path_d = NULL;
    GtkTreePath *path_m;
    GtkTreeModel *model;
    GtkTreeViewDropPosition pos = 0;
    gint position = -1;
    Playlist *pl, *pl_s, *pl_d;
    Track *tr_s = NULL;
    gboolean path_ok;
    gboolean del_src;

    Exporter *exporter = gtkpod_get_exporter();
    g_return_if_fail(exporter);

    g_return_if_fail (widget);
    g_return_if_fail (dc);
    g_return_if_fail (data);
    g_return_if_fail (gtk_selection_data_get_length(data) > 0);
    g_return_if_fail (gtk_selection_data_get_data(data));
    g_return_if_fail (gtk_selection_data_get_format(data) == 8);

    model = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
    g_return_if_fail (model);

    path_m = g_object_get_data(G_OBJECT (widget), "drag_data_by_motion_path");

    if (path_m) {
        /* this callback was caused by pm_drag_motion -- we are
         * supposed to call gdk_drag_status () */

        pos = (GtkTreeViewDropPosition) g_object_get_data(G_OBJECT (widget), "drag_data_by_motion_pos");
        /* unset flag that */
        g_object_set_data(G_OBJECT (widget), "drag_data_by_motion_path", NULL);
        g_object_set_data(G_OBJECT (widget), "drag_data_by_motion_pos", NULL);
        if (gtk_tree_model_get_iter(model, &iter_d, path_m)) {
            gtk_tree_model_get(model, &iter_d, PM_COLUMN_PLAYLIST, &pl, -1);
        }
        gtk_tree_path_free(path_m);

        g_return_if_fail (pl);

        switch (info) {
        case DND_GTKPOD_TRACKLIST:
            /* get first track and check itdb */
            sscanf(gtk_selection_data_get_data(data), "%p", &tr_s);
            if (!tr_s) {
                gdk_drag_status(dc, 0, time);
                g_return_if_reached ();
            }
            gdk_drag_status(dc, pm_tm_get_action(tr_s, pl, pos, dc), time);
            return;
        case DND_GTKPOD_PLAYLISTLIST:
            /* get first playlist and check itdb */
            sscanf(gtk_selection_data_get_data(data), "%p", &pl_s);
            if (!pl_s) {
                gdk_drag_status(dc, 0, time);
                g_return_if_reached ();
            }
            gdk_drag_status(dc, pm_pm_get_action(pl_s, pl, widget, pos, dc), time);
            return;
        }
        g_return_if_reached ();
        return;
    }

    gp_remove_autoscroll_row_timeout(widget);

    path_ok = gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(widget), x, y, &path_d, &pos);

    /* return if drop path is invalid */
    if (!path_ok) {
        gtk_drag_finish(dc, FALSE, FALSE, time);
        return;
    }
    g_return_if_fail (path_d);

    if (gtk_tree_model_get_iter(model, &iter_d, path_d)) {
        gtk_tree_model_get(model, &iter_d, PM_COLUMN_PLAYLIST, &pl, -1);
    }
    gtk_tree_path_free(path_d);
    path_d = NULL;

    g_return_if_fail (pl);

    position = pm_get_position_for_playlist(pl);

    gchar *data_copy = g_strdup(gtk_selection_data_get_data(data));

    switch (info) {
    case DND_GTKPOD_TRACKLIST:
        /* get first track */
        sscanf(gtk_selection_data_get_data(data), "%p", &tr_s);
        if (!tr_s) {
            gtk_drag_finish(dc, FALSE, FALSE, time);
            g_free(data_copy);
            g_return_if_reached ();
        }

        /* Find out action */
        gdk_drag_status(dc, pm_tm_get_action(tr_s, pl, pos, dc), time);

        if (gdk_drag_context_get_selected_action(dc) & GDK_ACTION_MOVE)
            del_src = TRUE;
        else
            del_src = FALSE;

        if ((pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE) || (pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)) { /* drop into existing playlist */
            /* copy files from iPod if necessary */
            GList *trackglist = exporter_transfer_track_names_between_itdbs(exporter, tr_s->itdb, pl->itdb, data_copy);
            if (trackglist) {
                add_trackglist_to_playlist(pl, trackglist);
                g_list_free(trackglist);
                trackglist = NULL;
                pm_tm_tracks_moved_or_copied(data_copy, del_src);
                gtk_drag_finish(dc, TRUE, del_src, time);
            }
            else {
                gtk_drag_finish(dc, FALSE, FALSE, time);
            }
        }
        else { /* drop between playlists */
            Playlist *plitem;
            /* adjust position */
            plitem = add_new_pl_user_name(pl->itdb, NULL, pm_adjust_for_drop_pos(position, pos));

            if (plitem) {
                /* copy files from iPod if necessary */
                GList *trackglist = exporter_transfer_track_names_between_itdbs(exporter, tr_s->itdb, pl->itdb, data_copy);
                if (trackglist) {
                    add_trackglist_to_playlist(plitem, trackglist);
                    g_list_free(trackglist);
                    trackglist = NULL;
                    pm_tm_tracks_moved_or_copied(data_copy, del_src);
                    gtk_drag_finish(dc, TRUE, del_src, time);
                }
                else {
                    gp_playlist_remove(plitem);
                    plitem = NULL;
                    gtk_drag_finish(dc, FALSE, FALSE, time);
                }
            }
            else {
                gtk_drag_finish(dc, FALSE, FALSE, time);
            }
        }
        break;
    case DND_TEXT_URI_LIST:
    case DND_TEXT_PLAIN:
        if ((pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE) || (pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)) { /* drop into existing playlist */
            add_text_plain_to_playlist(pl->itdb, pl, data_copy, 0, NULL, NULL);
            gdk_drag_status(dc, GDK_ACTION_COPY, time);
            gtk_drag_finish(dc, TRUE, FALSE, time);
        }
        else { /* drop between playlists */
            Playlist *plitem;
            plitem = add_text_plain_to_playlist(pl->itdb, NULL, data_copy,
                        pm_adjust_for_drop_pos(position, pos), NULL, NULL);

            if (plitem) {
                gdk_drag_status(dc, GDK_ACTION_COPY, time);
                gtk_drag_finish(dc, TRUE, FALSE, time);
            }
            else {
                gdk_drag_status(dc, 0, time);
                gtk_drag_finish(dc, FALSE, FALSE, time);
            }
        }
        break;
    case DND_GTKPOD_PLAYLISTLIST:
        /* get first playlist and check action */
        sscanf(gtk_selection_data_get_data(data), "%p", &pl_s);
        if (!pl_s) {
            gtk_drag_finish(dc, FALSE, FALSE, time);
            g_free(data_copy);
            g_return_if_reached ();
        }

        gdk_drag_status(dc, pm_pm_get_action(pl_s, pl, widget, pos, dc), time);

        if (gdk_drag_context_get_selected_action(dc) == 0) {
            gtk_drag_finish(dc, FALSE, FALSE, time);
            g_free(data_copy);
            return;
        }

        if (pl->itdb == pl_s->itdb) { /* handle DND within the same itdb */
            switch (gdk_drag_context_get_selected_action(dc)) {
            case GDK_ACTION_COPY:
                /* if copy-drop is between two playlists, create new
                 * playlist */
                switch (pos) {
                case GTK_TREE_VIEW_DROP_BEFORE:
                case GTK_TREE_VIEW_DROP_AFTER:
                    pl_d = itdb_playlist_duplicate(pl_s);
                    gp_playlist_add(pl->itdb, pl_d, pm_adjust_for_drop_pos(position, pos));
                    break;
                default:
                    pl_d = pl;
                    if (pl_d != pl_s)
                        add_trackglist_to_playlist(pl_d, pl_s->members);
                    break;
                }
                gtk_drag_finish(dc, TRUE, FALSE, time);
                break;
            case GDK_ACTION_MOVE:
                pm_get_iter_for_playlist(pl_s, &iter_s);
                switch (pos) {
                case GTK_TREE_VIEW_DROP_BEFORE:
                case GTK_TREE_VIEW_DROP_AFTER:
                    if (prefs_get_int("pm_sort") != SORT_NONE) {
                        gtkpod_statusbar_message(_("Can't reorder sorted treeview."));
                        gtk_drag_finish(dc, FALSE, FALSE, time);
                        g_free(data_copy);
                        return;
                    }
                    if (pos == GTK_TREE_VIEW_DROP_BEFORE)
                        gtk_tree_store_move_before(GTK_TREE_STORE (model), &iter_s, &iter_d);
                    else
                        gtk_tree_store_move_after(GTK_TREE_STORE (model), &iter_s, &iter_d);

                    pm_rows_reordered();
                    gtk_drag_finish(dc, TRUE, FALSE, time);
                    break;
                default:
                    pl_d = pl;
                    if (pl_d != pl_s)
                        add_trackglist_to_playlist(pl_d, pl_s->members);
                    gtk_drag_finish(dc, TRUE, FALSE, time);
                    break;
                }
                break;
            default:
                gtk_drag_finish(dc, FALSE, FALSE, time);
                g_free(data_copy);
                g_return_if_reached ();
            }
        }
        else { /*handle DND between two itdbs */
            GList *trackglist = NULL;

            /* create new playlist */
            pl_d = gp_playlist_add_new(pl->itdb, pl_s->name, FALSE,
                                       pm_adjust_for_drop_pos(position, pos));
            g_free(data_copy);
            data_copy = NULL;
            g_return_if_fail (pl_d);

            /* copy files from iPod if necessary */
            trackglist = exporter_transfer_track_glist_between_itdbs(exporter, pl_s->itdb, pl_d->itdb, pl_s->members);

            /* check if copying went fine (trackglist is empty if
             pl_s->members is empty, so this must not be counted as
             an error */
            if (trackglist || !pl_s->members) {
                add_trackglist_to_playlist(pl_d, trackglist);
                g_list_free(trackglist);
                trackglist = NULL;
                switch (gdk_drag_context_get_selected_action(dc)) {
                case GDK_ACTION_MOVE:
                    gtk_drag_finish(dc, TRUE, TRUE, time);
                    break;
                case GDK_ACTION_COPY:
                    gtk_drag_finish(dc, TRUE, FALSE, time);
                    break;
                default:
                    gtk_drag_finish(dc, FALSE, FALSE, time);
                    break;
                }
            }
            else {
                if (pl_d != pl) { /* remove newly created playlist */
                    gp_playlist_remove(pl_d);
                    pl_d = NULL;
                }
                gtk_drag_finish(dc, FALSE, FALSE, time);
            }

        }
        pm_rows_reordered();
        break;
    default:
        gtkpod_warning(_("This DND type (%d) is not (yet) supported. If you feel implementing this would be useful, please contact the author.\n\n"), info);
        gtk_drag_finish(dc, FALSE, FALSE, time);
        break;
    }

    g_free(data_copy);

    /* display if any duplicates were skipped */
    gp_duplicate_remove(NULL, NULL);
}

/* ---------------------------------------------------------------- */
/* Section for playlist display                                     */
/* other callbacks                                                  */
/* ---------------------------------------------------------------- */

static gboolean on_playlist_treeview_key_release_event(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    guint mods;
    mods = event->state;

    if (!widgets_blocked && (mods & GDK_CONTROL_MASK)) {
        Itdb_iTunesDB *itdb = gp_get_selected_itdb();

        switch (event->keyval) {
        /* 	    case GDK_u: */
        /* 		gp_do_selected_playlist (update_tracks); */
        /* 		break; */
        case GDK_KEY_N:
            if (itdb) {
                add_new_pl_or_spl_user_name(itdb, NULL, -1);
            }
            else {
                message_sb_no_itdb_selected();
            }
            break;
        default:
            break;
        }

    }
    return FALSE;
}

/* ---------------------------------------------------------------- */
/* Section for playlist display helper functions                    */
/* ---------------------------------------------------------------- */

/* Find the iter that represents the repository @itdb
 *
 * Return TRUE if the repository could be found. In that case @itdb_iter
 * will be set to the corresponding iter. The value of @itdb_iter is
 * undefined when the repository couldn't be found, in which case FALSE
 * is returned. */
static gboolean pm_get_iter_for_itdb(Itdb_iTunesDB *itdb, GtkTreeIter *itdb_iter) {
    GtkTreeModel *model;

    g_return_val_if_fail (playlist_treeview, FALSE);
    g_return_val_if_fail (itdb, FALSE);
    g_return_val_if_fail (itdb_iter, FALSE);

    model = GTK_TREE_MODEL (gtk_tree_view_get_model (playlist_treeview));

    if (gtk_tree_model_get_iter_first(model, itdb_iter)) {
        do {
            Itdb_iTunesDB *itdb_model;
            gtk_tree_model_get(model, itdb_iter, PM_COLUMN_ITDB, &itdb_model, -1);
            g_return_val_if_fail (itdb_model, FALSE);
            if (itdb == itdb_model) {
                return TRUE;
            }
        }
        while (gtk_tree_model_iter_next(model, itdb_iter));
    }
    return FALSE;
}

/* Find the iter that contains Playlist @playlist
 *
 * Return TRUE if the playlist could be found. In that case @pl_iter
 * will be set to the corresponding iter. The value of @pl_iter is
 * undefined when the playlist couldn't be found, in which case FALSE
 * is returned. */
static gboolean pm_get_iter_for_playlist(Playlist *playlist, GtkTreeIter *pl_iter) {
    GtkTreeIter itdb_iter;

    g_return_val_if_fail (playlist_treeview, FALSE);
    g_return_val_if_fail (playlist, FALSE);
    g_return_val_if_fail (pl_iter, FALSE);

    /* First get the iter with the itdb in it */

    if (pm_get_iter_for_itdb(playlist->itdb, &itdb_iter)) {
        GtkTreeModel *model;
        Playlist *pl;

        model = GTK_TREE_MODEL (gtk_tree_view_get_model (playlist_treeview));

        /* Check if this is already the right iter */
        gtk_tree_model_get(model, &itdb_iter, PM_COLUMN_PLAYLIST, &pl, -1);

        if (pl == playlist) {
            *pl_iter = itdb_iter;
            return TRUE;
        }

        /* no -- go down one hierarchy and try all other iters */
        if (!gtk_tree_model_iter_children(model, pl_iter, &itdb_iter)) { /* This indicates screwed up programming so we better cry
         out */
            g_return_val_if_reached (FALSE);
        }

        do {
            gtk_tree_model_get(model, pl_iter, PM_COLUMN_PLAYLIST, &pl, -1);

            if (pl == playlist) {
                return TRUE;
            }
        }
        while (gtk_tree_model_iter_next(model, pl_iter));
    }
    return FALSE;
}

/* ---------------------------------------------------------------- */
/* Section for playlist display                                     */
/* ---------------------------------------------------------------- */

/* One of the playlist names has changed (this happens when the
 Itdb_iTunesDB is read */
void pm_itdb_name_changed(Itdb_iTunesDB *itdb) {
    GtkTreeIter iter;

    g_return_if_fail (itdb);

    if (pm_get_iter_for_itdb(itdb, &iter)) {
        GtkTreeModel *model;
        GtkTreePath *path;
        model = GTK_TREE_MODEL (gtk_tree_view_get_model (playlist_treeview));
        path = gtk_tree_model_get_path(model, &iter);
        gtk_tree_model_row_changed(model, path, &iter);
        gtk_tree_path_free(path);
    }
}

/* Add playlist to the playlist model */
/* If @position = -1: append to end */
/* If @position >=0: insert at that position (count starts with MPL as
 * 0) */
void pm_add_child(Itdb_iTunesDB *itdb, PM_column_type type, gpointer item, gint pos) {
    GtkTreeIter mpl_iter;
    GtkTreeIter *mpli = NULL;
    GtkTreeIter iter;
    GtkTreeModel *model;
    /*  GtkTreeSelection *selection;*/

    g_return_if_fail (playlist_treeview);
    g_return_if_fail (item);
    g_return_if_fail (itdb);

    model = GTK_TREE_MODEL (gtk_tree_view_get_model (playlist_treeview));
    g_return_if_fail (model);

    /* Find the iter with the mpl in it */
    if (pm_get_iter_for_itdb(itdb, &mpl_iter)) {
        mpli = &mpl_iter;
    }

    switch (type) {
    case PM_COLUMN_PLAYLIST:
        if (itdb_playlist_is_mpl((Playlist *) item)) { /* MPLs are always added top-level */
            mpli = NULL;
        }
        else { /* Handle normal playlist */
            /* MPL must be set before calling this function */
            g_return_if_fail (mpli);
            if (pos == -1) { /* just adding at the end will add behind the photo
             * item. Find out how many playlists there are and add
             * at the end. */
                GtkTreeIter pl_iter;
                Playlist *pl;
                pos = 0;
                /* go down one hierarchy and try all other iters */
                if (gtk_tree_model_iter_children(model, &pl_iter, &mpl_iter)) {
                    do {
                        gtk_tree_model_get(model, &pl_iter, PM_COLUMN_PLAYLIST, &pl, -1);
                        if (pl != NULL) {
                            ++pos;
                        }
                    }
                    while (pl && gtk_tree_model_iter_next(model, &pl_iter));
                }
            }
            else { /* reduce position by one because the MPL is not included in the
             tree model's count */
                --pos;
            }
        }
        break;
    case PM_COLUMN_PHOTOS:
        /* MPL must be set before calling this function */
        g_return_if_fail (mpli);
        /* always add at the end */
        pos = -1;
        break;
    case PM_COLUMN_ITDB:
    case PM_COLUMN_TYPE:
    case PM_NUM_COLUMNS:
        g_return_if_reached ();
    }
    gtk_tree_store_insert(GTK_TREE_STORE (model), &iter, mpli, pos);

    gtk_tree_store_set(GTK_TREE_STORE (model), &iter, PM_COLUMN_ITDB, itdb, PM_COLUMN_TYPE, type, type, item, -1);
}

/* Remove "playlist" from the display model */
void pm_remove_playlist(Playlist *playlist) {
    GtkTreeModel *model;
    GtkTreeIter iter;

    g_return_if_fail (playlist);
    model = gtk_tree_view_get_model(playlist_treeview);
    g_return_if_fail (model);

    gtkpod_set_current_playlist(NULL);

    if (pm_get_iter_for_playlist(playlist, &iter)) {
        gtk_tree_store_remove(GTK_TREE_STORE (model), &iter);
    }
}

/* Remove all playlists from the display model */
/* ATTENTION: the playlist_treeview and model might be changed by
 calling this function */
/* @clear_sort: TRUE: clear "sortable" setting of treeview */
void pm_remove_all_playlists(gboolean clear_sort) {
    GtkTreeModel *model;
    GtkTreeIter iter;
    gint column;
    GtkSortType order;

    g_return_if_fail (playlist_treeview);
    model = gtk_tree_view_get_model(playlist_treeview);
    g_return_if_fail (model);

    while (gtk_tree_model_get_iter_first(model, &iter)) {
        gtk_tree_store_remove(GTK_TREE_STORE (model), &iter);
    }
    if (clear_sort && gtk_tree_sortable_get_sort_column_id(GTK_TREE_SORTABLE (model), &column, &order)) { /* recreate track treeview to unset sorted column */
        if (column >= 0) {
            pm_create_treeview();
        }
    }
}

/* Select specified playlist */
void pm_select_playlists(GList *playlists) {
    GtkTreeIter iter;
    GtkTreeSelection *ts;

    g_return_if_fail (playlist_treeview);

    if (!playlists) {
        ts = gtk_tree_view_get_selection(playlist_treeview);
        gtk_tree_selection_unselect_all(ts);
        return;
    }

    ts = gtk_tree_view_get_selection(playlist_treeview);

    for (gint i = 0; i < g_list_length(playlists); ++i) {
        Playlist *pl = g_list_nth_data(playlists, i);

        if (pm_get_iter_for_playlist(pl, &iter)) {
            gtk_tree_selection_select_iter(ts, &iter);
        }

        /* Only properly select the first in the list */
        if (i == 0 && gtkpod_get_current_playlist() != pl) {
            gtkpod_set_current_playlist(pl);
        }
    }
}

/* Select specified playlist */
void pm_select_playlist(Playlist *playlist) {
    GtkTreeIter iter;
    GtkTreeSelection *ts;

    g_return_if_fail (playlist_treeview);

    if (!playlist) {
        ts = gtk_tree_view_get_selection(playlist_treeview);
        gtk_tree_selection_unselect_all(ts);
    }
    else if (pm_get_iter_for_playlist(playlist, &iter)) {
        ts = gtk_tree_view_get_selection(playlist_treeview);
        gtk_tree_selection_select_iter(ts, &iter);
    }

    if (gtkpod_get_current_playlist() != playlist) {
        gtkpod_set_current_playlist(playlist);
    }
}

/* Unselect specified playlist */
void pm_unselect_playlist(Playlist *playlist) {
    GtkTreeIter iter;

    g_return_if_fail (playlist_treeview);
    g_return_if_fail (playlist);

    if (pm_get_iter_for_playlist(playlist, &iter)) {
        GtkTreeSelection *ts;
        ts = gtk_tree_view_get_selection(playlist_treeview);
        gtk_tree_selection_unselect_iter(ts, &iter);
    }

    gtkpod_set_current_playlist(NULL);
}

static gboolean pm_selection_changed_cb(gpointer data) {
    GtkTreeIter iter;
    GtkTreeView *tree_view = GTK_TREE_VIEW (data);

    g_return_val_if_fail(tree_view, FALSE);

    GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
    g_return_val_if_fail(model, FALSE);

#if DEBUG_TIMING
    GTimeVal time;
    g_get_current_time (&time);
    printf ("pm_selection_changed_cb enter: %ld.%06ld sec\n",
            time.tv_sec % 3600, time.tv_usec);
#endif

    if (! pm_is_playlist_selected()) {
        /* no selection */
        gtkpod_set_current_playlist(NULL);
    }
    else {
        Playlist *new_playlist = pm_get_first_selected_playlist();
        g_return_val_if_fail(new_playlist, FALSE);

        Itdb_iTunesDB *itdb = NULL;
        Itdb_PhotoDB *photodb = NULL;
        PM_column_type type = 0;

        /* handle new selection */
        pm_get_iter_for_playlist(new_playlist, &iter);
        gtk_tree_model_get(model, &iter, PM_COLUMN_TYPE, &type, PM_COLUMN_ITDB, &itdb, PM_COLUMN_PLAYLIST, &new_playlist, PM_COLUMN_PHOTOS, &photodb, -1);

        gtkpod_set_current_playlist(new_playlist);

        switch (type) {
        case PM_COLUMN_PLAYLIST:
            g_return_val_if_fail (new_playlist, FALSE);
            g_return_val_if_fail (itdb, FALSE);

            if (new_playlist->is_spl && new_playlist->splpref.liveupdate)
                itdb_spl_update(new_playlist);

            gtkpod_tracks_statusbar_update();
            break;
        case PM_COLUMN_PHOTOS:
            g_return_val_if_fail (photodb, FALSE);
            g_return_val_if_fail (itdb, FALSE);
            break;
        case PM_COLUMN_ITDB:
        case PM_COLUMN_TYPE:
        case PM_NUM_COLUMNS:
            g_warn_if_reached ();
        }
    }

#if DEBUG_TIMING
    g_get_current_time (&time);
    printf ("pm_selection_changed_cb exit:  %ld.%06ld sec\n",
            time.tv_sec % 3600, time.tv_usec);
#endif

    return FALSE;
}

/* Callback function called when the selection
 of the playlist view has changed */
static void pm_selection_changed(GtkTreeSelection *selection, gpointer user_data) {
    if (!pm_selection_blocked) {
        g_idle_add(pm_selection_changed_cb, gtk_tree_selection_get_tree_view(selection));
    }
}

static void cell_renderer_stop_editing(GtkCellRenderer *renderer, gpointer user_data) {
    gtk_cell_renderer_stop_editing (renderer, (gboolean) GPOINTER_TO_INT(user_data));
}

/* Stop editing. If @cancel is TRUE, the edited value will be
 discarded (I have the feeling that the "discarding" part does not
 work quite the way intended). */
void pm_stop_editing(gboolean cancel) {
    GtkTreeViewColumn *col;
    GList *cells;

    g_return_if_fail (playlist_treeview);

    gtk_tree_view_get_cursor(playlist_treeview, NULL, &col);

    if (col) {
        cells = gtk_cell_layout_get_cells(GTK_CELL_LAYOUT (col));
        g_list_foreach(cells, (GFunc) cell_renderer_stop_editing, GINT_TO_POINTER((gint) cancel));
        g_list_free(cells);
    }
}

/* set/read the counter used to remember how often the sort column has
 been clicked.
 @inc: negative: reset counter to 0
 @inc: positive or zero : add to counter
 return value: new value of the counter */
static gint pm_sort_counter(gint inc) {
    static gint cnt = 0;
    if (inc < 0)
        cnt = 0;
    else
        cnt += inc;
    return cnt;
}

/* Add all playlists of @itdb at position @pos */
void pm_add_itdb(Itdb_iTunesDB *itdb, gint pos) {
    GtkTreeIter mpl_iter;
    GList *gl_pl;
    ExtraiTunesDBData *eitdb;

    g_return_if_fail (itdb);
    eitdb = itdb->userdata;
    g_return_if_fail (eitdb);

    for (gl_pl = itdb->playlists; gl_pl; gl_pl = gl_pl->next) {
        Playlist *pl = gl_pl->data;
        g_return_if_fail (pl);
        if (itdb_playlist_is_mpl(pl)) {
            pm_add_child(itdb, PM_COLUMN_PLAYLIST, pl, pos);
        }
        else {
            pm_add_child(itdb, PM_COLUMN_PLAYLIST, pl, -1);
        }
    }

    /* expand the itdb */
    if (pm_get_iter_for_itdb(itdb, &mpl_iter)) {
        GtkTreeModel *model;
        GtkTreePath *mpl_path;
        model = GTK_TREE_MODEL (gtk_tree_view_get_model (playlist_treeview));
        g_return_if_fail (model);
        mpl_path = gtk_tree_model_get_path(model, &mpl_iter);
        g_return_if_fail (mpl_path);
        gtk_tree_view_expand_row(playlist_treeview, mpl_path, TRUE);
        gtk_tree_path_free(mpl_path);
    }
}

/* Helper function: add all playlists to playlist model */
void pm_add_all_itdbs(void) {
    GList *gl_itdb;
    struct itdbs_head *itdbs_head;

    g_return_if_fail (gtkpod_app);
    itdbs_head = gp_get_itdbs_head();
    g_return_if_fail (itdbs_head);
    for (gl_itdb = itdbs_head->itdbs; gl_itdb; gl_itdb = gl_itdb->next) {
        Itdb_iTunesDB *itdb = gl_itdb->data;
        g_return_if_fail (itdb);
        pm_add_itdb(itdb, -1);
    }
}

/* Return GtkTreePath for playlist @playlist. The returned path must be
 freed using gtk_tree_path_free() after it is no needed any more */
static GtkTreePath *pm_get_path_for_playlist(Playlist *playlist) {
    GtkTreeIter iter;

    g_return_val_if_fail (playlist_treeview, NULL);
    g_return_val_if_fail (playlist, NULL);

    if (pm_get_iter_for_playlist(playlist, &iter)) {
        GtkTreeModel *model;
        model = gtk_tree_view_get_model(playlist_treeview);
        return gtk_tree_model_get_path(model, &iter);
    }
    return NULL;
}

/* Return GtkTreePath for repository @itdb. The returned path must be
 freed using gtk_tree_path_free() after it is no needed any more */
GtkTreePath *pm_get_path_for_itdb(Itdb_iTunesDB *itdb) {
    GtkTreeIter iter;

    g_return_val_if_fail (playlist_treeview, NULL);
    g_return_val_if_fail (itdb, NULL);

    if (pm_get_iter_for_itdb(itdb, &iter)) {
        GtkTreeModel *model;
        model = gtk_tree_view_get_model(playlist_treeview);
        return gtk_tree_model_get_path(model, &iter);
    }
    return NULL;
}

/* Return position of repository @itdb */
gint pm_get_position_for_itdb(Itdb_iTunesDB *itdb) {
    GtkTreePath *path;
    gint position = -1;

    g_return_val_if_fail (playlist_treeview, -1);
    g_return_val_if_fail (itdb, -1);

    path = pm_get_path_for_itdb(itdb);

    if (path) {
        gint *indices = gtk_tree_path_get_indices(path);
        if (indices) {
            position = indices[0];
        }
        gtk_tree_path_free(path);
    }
    return position;
}

/* Return position of repository @itdb */
static gint pm_get_position_for_playlist(Playlist *playlist) {
    GtkTreePath *path;
    gint position = -1;

    g_return_val_if_fail (playlist_treeview, -1);
    g_return_val_if_fail (playlist, -1);

    path = pm_get_path_for_playlist(playlist);

    if (path) {
        /* get position of current path */
        if (gtk_tree_path_get_depth(path) == 1) { /* MPL */
            position = 0;
        }
        else {
            gint *indices = gtk_tree_path_get_indices(path);
            /* need to add 1 because MPL is one level higher and not
             counted */
            position = indices[1] + 1;
        }
        gtk_tree_path_free(path);
    }
    return position;
}

/* "unsort" the playlist view without causing the sort tabs to be
 touched. */
static void pm_unsort() {
    GList *cur_pls;

    pm_selection_blocked = TRUE;

    /* remember */
    cur_pls = pm_get_selected_playlists();

    pm_remove_all_playlists(TRUE);

    pm_select_playlists(cur_pls);

    pm_selection_blocked = FALSE;
    /* reset sort counter */
    pm_sort_counter(-1);
}

/* Set the sorting accordingly */
void pm_sort(enum GtkPodSortTypes order) {
    GtkTreeModel *model = gtk_tree_view_get_model(playlist_treeview);
    g_return_if_fail (model);
    if (order != SORT_NONE) {
        gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE (model), PM_COLUMN_PLAYLIST, order);
    }
    else { /* only unsort if treeview is sorted */
        gint column;
        GtkSortType order;
        if (gtk_tree_sortable_get_sort_column_id(GTK_TREE_SORTABLE (model), &column, &order))
            pm_unsort();
    }
}

#if 0
FIXME: see comments at pm_data_compare_func()
/**
 * pm_track_column_button_clicked
 * @tvc - the tree view colum that was clicked
 * @data - ignored user data
 * When the sort button is clicked we want to update our internal playlist
 * representation to what's displayed on screen.
 * If the button was clicked three times, the sort order is undone.
 */
static void
pm_track_column_button_clicked(GtkTreeViewColumn *tvc, gpointer data)
{
    gint cnt = pm_sort_counter (1);
    if (cnt >= 3)
    {
        prefs_set_int("pm_sort", SORT_NONE);
        pm_unsort (); /* also resets the sort_counter */
    }
    else
    {
        prefs_set_int("pm_sort", gtk_tree_view_column_get_sort_order (tvc));
        pm_rows_reordered ();
    }
}
#endif

/**
 * Reorder playlists to match order of playlists displayed.
 * data_changed() is called when necessary.
 */
void pm_rows_reordered(void) {
    GtkTreeModel *tm = NULL;
    GtkTreeIter parent;
    gboolean p_valid;

    g_return_if_fail (playlist_treeview);
    tm = gtk_tree_view_get_model(GTK_TREE_VIEW(playlist_treeview));
    g_return_if_fail (tm);

    p_valid = gtk_tree_model_get_iter_first(tm, &parent);
    while (p_valid) {
        guint32 pos;
        Playlist *pl;
        Itdb_iTunesDB *itdb;
        GtkTreeIter child;
        gboolean c_valid;

        /* get master playlist */
        gtk_tree_model_get(tm, &parent, PM_COLUMN_PLAYLIST, &pl, -1);
        g_return_if_fail (pl);
        g_return_if_fail (itdb_playlist_is_mpl (pl));
        itdb = pl->itdb;
        g_return_if_fail (itdb);

        pos = 1;
        /* get all children */
        c_valid = gtk_tree_model_iter_children(tm, &child, &parent);
        while (c_valid) {
            gtk_tree_model_get(tm, &child, PM_COLUMN_PLAYLIST, &pl, -1);
            g_return_if_fail (pl);
            if (itdb_playlist_by_nr(itdb, pos) != pl) {
                /* move the playlist to indicated position */
                g_return_if_fail (!itdb_playlist_is_mpl (pl));
                itdb_playlist_move(pl, pos);
                data_changed(itdb);
            }
            ++pos;
            c_valid = gtk_tree_model_iter_next(tm, &child);
        }
        p_valid = gtk_tree_model_iter_next(tm, &parent);
    }
}

/* Function used to compare two cells during sorting (playlist view) */
gint pm_data_compare_func(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data) {
    Playlist *playlist1 = NULL;
    Playlist *playlist2 = NULL;
    enum GtkPodSortTypes order;
    gint corr, colid;

    g_return_val_if_fail (model, 0);
    g_return_val_if_fail (a, 0);
    g_return_val_if_fail (b, 0);

    GtkSortType sortorder;
    if (gtk_tree_sortable_get_sort_column_id(GTK_TREE_SORTABLE (model), &colid, &sortorder) == FALSE)
        return 0;

    order = (enum GtkPodSortTypes) sortorder;

    if (order == SORT_NONE)
        return 0;

    gtk_tree_model_get(model, a, colid, &playlist1, -1);
    gtk_tree_model_get(model, b, colid, &playlist2, -1);

    g_return_val_if_fail (playlist1 && playlist2, 0);

    /* We make sure that the master playlist always stays on top */
    if (order == SORT_ASCENDING)
        corr = +1;
    else
        corr = -1;

    if (itdb_playlist_is_mpl(playlist1) && itdb_playlist_is_mpl(playlist2))
        return 0; // Don't resort mpl playlists - only sub playlists

    if (itdb_playlist_is_mpl(playlist1))
        return (-corr);

    if (itdb_playlist_is_mpl(playlist2))
        return (corr);

    /* compare the two entries */
    return compare_string(playlist1->name, playlist2->name, prefs_get_int("pm_case_sensitive"));
}

/* Called when editable cell is being edited. Stores new data to
 the playlist list. */
static void pm_cell_edited(GtkCellRendererText *renderer, const gchar *path_string, const gchar *new_text, gpointer data) {
    GtkTreeModel *model = data;
    GtkTreeIter iter;
    Playlist *playlist = NULL;

    g_return_if_fail (model);
    g_return_if_fail (new_text);
    if (!gtk_tree_model_get_iter_from_string(model, &iter, path_string)) {
        g_return_if_reached ();
    }

    gtk_tree_model_get(model, &iter, PM_COLUMN_PLAYLIST, &playlist, -1);
    g_return_if_fail (playlist);

    /* We only do something, if the name actually got changed */
    if (!playlist->name || g_utf8_collate(playlist->name, new_text) != 0) {
        g_free(playlist->name);
        playlist->name = g_strdup(new_text);
        data_changed(playlist->itdb);
        if (itdb_playlist_is_mpl(playlist)) { /* Need to change name in prefs system */
            set_itdb_prefs_string(playlist->itdb, "name", new_text);
        }
    }
}

/**
 * pm_set_renderer_text
 *
 * Set the playlist name in appropriate style.
 *
 * @renderer: renderer to be set
 * @playlist: playlist to consider.
 */
static void pm_set_playlist_renderer_text(GtkCellRenderer *renderer, Playlist *playlist) {
    ExtraiTunesDBData *eitdb;

    g_return_if_fail (playlist);
    g_return_if_fail (playlist->itdb);
    eitdb = playlist->itdb->userdata;
    g_return_if_fail (eitdb);

    if (itdb_playlist_is_mpl(playlist)) { /* mark MPL */
        g_object_set(G_OBJECT (renderer), "text", playlist->name, "weight", PANGO_WEIGHT_BOLD, NULL);
        if (eitdb->data_changed) {
            g_object_set(G_OBJECT (renderer), "style", PANGO_STYLE_ITALIC, NULL);
        }
        else {
            g_object_set(G_OBJECT (renderer), "style", PANGO_STYLE_NORMAL, NULL);
        }
    }
    else {
        if (itdb_playlist_is_podcasts(playlist)) {
            g_object_set(G_OBJECT (renderer), "text", playlist->name, "weight", PANGO_WEIGHT_SEMIBOLD, "style", PANGO_STYLE_ITALIC, NULL);
        }
        else {
            g_object_set(G_OBJECT (renderer), "text", playlist->name, "weight", PANGO_WEIGHT_NORMAL, "style", PANGO_STYLE_NORMAL, NULL);
        }
    }
}

/**
 * pm_set_photodb_renderer_text
 *
 * Set the PhotoDB name in appropriate style.
 *
 * @renderer: renderer to be set
 * @PhotoDB: photodb to consider.
 */
void pm_set_photodb_renderer_text(GtkCellRenderer *renderer, PhotoDB *photodb) {
    g_return_if_fail (photodb);

    /* bold face */
    g_object_set(G_OBJECT (renderer), "text", _("Photos"), "weight", PANGO_WEIGHT_BOLD, NULL);
    /* (example for italic style)
     if (eitdb->data_changed)
     {
     g_object_set (G_OBJECT (renderer),
     "style", PANGO_STYLE_ITALIC,
     NULL);
     }
     else
     {
     g_object_set (G_OBJECT (renderer),
     "style", PANGO_STYLE_NORMAL,
     NULL);
     }
     */
}

/**
 * pm_set_playlist_renderer_pix
 *
 * Set the appropriate playlist icon.
 *
 * @renderer: renderer to be set
 * @playlist: playlist to consider.
 */
static void pm_set_playlist_renderer_pix(GtkCellRenderer *renderer, Playlist *playlist) {
    const gchar *stock_id = NULL;

    g_return_if_fail (renderer);

    stock_id = return_playlist_stock_image(playlist);
    if (!stock_id)
        return;

    g_object_set(G_OBJECT (renderer), "stock-id", stock_id, NULL);
    g_object_set(G_OBJECT (renderer), "stock-size", GTK_ICON_SIZE_LARGE_TOOLBAR, NULL);
}

/**
 * pm_set_photodb_renderer_pix
 *
 * Set the appropriate photodb icon.
 *
 * @renderer: renderer to be set
 * @photodb: photodb to consider.
 */
void pm_set_photodb_renderer_pix(GtkCellRenderer *renderer, Itdb_PhotoDB *photodb) {
    g_return_if_fail (renderer);
    g_return_if_fail (photodb);

    g_object_set(G_OBJECT (renderer), "stock-id", PLAYLIST_DISPLAY_PHOTO_ICON_STOCK_ID, NULL);
    g_object_set(G_OBJECT (renderer), "stock-size", GTK_ICON_SIZE_LARGE_TOOLBAR, NULL);
}

/* The playlist data is stored in a separate list
 and only pointers to the corresponding playlist structure are placed
 into the model.
 This function reads the data for the given cell from the list and
 passes it to the renderer. */
static void pm_cell_data_func(GtkTreeViewColumn *tree_column, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer data) {
    Playlist *playlist = NULL;
    Itdb_PhotoDB *photodb = NULL;
    PM_column_type type;

    g_return_if_fail (renderer);
    g_return_if_fail (model);
    g_return_if_fail (iter);

    gtk_tree_model_get(model, iter, PM_COLUMN_TYPE, &type, PM_COLUMN_PLAYLIST, &playlist, PM_COLUMN_PHOTOS, &photodb, -1);
    switch (type) {
    case PM_COLUMN_PLAYLIST:
        pm_set_playlist_renderer_text(renderer, playlist);
        break;
    case PM_COLUMN_PHOTOS:
        pm_set_photodb_renderer_text(renderer, photodb);
        break;
    case PM_COLUMN_ITDB:
    case PM_COLUMN_TYPE:
    case PM_NUM_COLUMNS:
        g_return_if_reached ();
    }
}

/* set graphic indicator for smart playlists */
static void pm_cell_data_func_pix(GtkTreeViewColumn *tree_column, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer data) {
    Playlist *playlist = NULL;
    Itdb_PhotoDB *photodb = NULL;
    PM_column_type type;

    g_return_if_fail (renderer);
    g_return_if_fail (model);
    g_return_if_fail (iter);

    gtk_tree_model_get(model, iter, PM_COLUMN_TYPE, &type, PM_COLUMN_PLAYLIST, &playlist, PM_COLUMN_PHOTOS, &photodb, -1);
    switch (type) {
    case PM_COLUMN_PLAYLIST:
        pm_set_playlist_renderer_pix(renderer, playlist);
        break;
    case PM_COLUMN_PHOTOS:
        pm_set_photodb_renderer_pix(renderer, photodb);
        break;
    case PM_COLUMN_ITDB:
    case PM_COLUMN_TYPE:
    case PM_NUM_COLUMNS:
        g_return_if_reached ();
    }
}

static void pm_select_current_position(gint x, gint y) {
    GtkTreePath *path;

    g_return_if_fail (playlist_treeview);

    gtk_tree_view_get_path_at_pos(playlist_treeview, x, y, &path, NULL, NULL, NULL);
    if (path) {
        GtkTreeSelection *ts = gtk_tree_view_get_selection(playlist_treeview);
        gtk_tree_selection_select_path(ts, path);
        gtk_tree_path_free(path);
    }
}

/* Return the number (0...) of the renderer the click was in or -1 if
 no renderer was found. @cell (if != NULL) is filled with a pointer
 to the renderer. */
gint tree_view_get_cell_from_pos(GtkTreeView *view, guint x, guint y, GtkCellRenderer **cell) {
    GtkTreeViewColumn *col = NULL;
    GList *node, *cells;
    gint pos = 0;
    GdkRectangle rect;
    GtkTreePath *path = NULL;
    gint cell_x, cell_y;

    g_return_val_if_fail ( view != NULL, -1 );

    if (cell)
        *cell = NULL;

    gtk_tree_view_get_path_at_pos(view, x, y, &path, &col, &cell_x, &cell_y);

    if (col == NULL)
        return -1; /* not found */

    cells = gtk_cell_layout_get_cells(GTK_CELL_LAYOUT(col));

    gtk_tree_view_get_cell_area(view, path, col, &rect);
    gtk_tree_path_free(path);

    /* gtk_tree_view_get_cell_area() should return the rectangle
     _excluding_ the expander arrow(s), but until 2.8.17 it forgets
     about the space occupied by the top level expander arrow. We
     therefore need to add the width of one expander arrow */
    if (!RUNTIME_GTK_CHECK_VERSION(2, 8, 18)) {
        if (col == gtk_tree_view_get_expander_column(view)) {
            GValue *es = g_malloc0(sizeof(GValue));
            g_value_init(es, G_TYPE_INT);
            gtk_widget_style_get_property(GTK_WIDGET (view), "expander_size", es);
            rect.x += g_value_get_int(es);
            rect.width -= g_value_get_int(es);
            g_free(es);
        }
    }

    for (node = cells; node != NULL; node = node->next) {
        GtkCellRenderer *checkcell = (GtkCellRenderer*) node->data;
        gint start_pos, width;

        if (gtk_tree_view_column_cell_get_position(col, checkcell, &start_pos, &width)) {
            if (x >= (rect.x + start_pos) && x < (rect.x + start_pos + width)) {
                if (cell)
                    *cell = checkcell;
                g_list_free(cells);
                return pos;
            }
        }
        ++pos;
    }

    g_list_free(cells);
    return -1; /* not found */
}

static gboolean pm_button_press(GtkWidget *w, GdkEventButton *e, gpointer data) {
    gint cell_nr;
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    Playlist *pl;
    ExtraiTunesDBData *eitdb;

    g_return_val_if_fail (w && e, FALSE);
    switch (e->button) {
    case 1:
        cell_nr = tree_view_get_cell_from_pos(GTK_TREE_VIEW(w), e->x, e->y, NULL);
        if (cell_nr == 0) {
            /* don't accept clicks while widgets are blocked -- this
             might cause a crash (e.g. when we click the 'Eject
             iPod' symbol while we are ejecting it already) */
            if (widgets_blocked)
                return FALSE;
            /* */
            model = gtk_tree_view_get_model(GTK_TREE_VIEW (w));
            gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(w), e->x, e->y, &path, NULL, NULL, NULL);
            gtk_tree_model_get_iter(model, &iter, path);
            gtk_tree_path_free(path);
            gtk_tree_model_get(model, &iter, PM_COLUMN_PLAYLIST, &pl, -1);
            if (pl == NULL)
                break;

            g_return_val_if_fail (pl->itdb, FALSE);

            if (!itdb_playlist_is_mpl(pl))
                break;

            if (pl->itdb->usertype & GP_ITDB_TYPE_IPOD) {

                /* the user clicked on the connect/disconnect icon of
                 * an iPod */
                eitdb = pl->itdb->userdata;
                g_return_val_if_fail (eitdb, FALSE);
                block_widgets();
                if (!eitdb->itdb_imported) {
                    gp_load_ipod(pl->itdb);
                }
                else {
                    gp_eject_ipod(pl->itdb);
                }
                release_widgets();
                return TRUE;
            }
            if (pl->itdb->usertype & GP_ITDB_TYPE_LOCAL) {

                /* the user clicked on the 'harddisk' icon of
                 * a local repository */
            }
        }
        break;
    case 3:
        pm_select_current_position(e->x, e->y);
        pm_context_menu_init();
        return TRUE;
    default:
        break;
    }
    return FALSE;
}

/* Adds the columns to our playlist_treeview */
static void pm_add_columns(void) {
    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;
    GtkTreeModel *model;

    model = gtk_tree_view_get_model(playlist_treeview);
    g_return_if_fail (model);

    /* playlist column */
    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(column, _("Playlists"));

    gtk_tree_view_column_set_sort_column_id (column, PM_COLUMN_PLAYLIST);
    gtk_tree_view_column_set_sort_order (column, GTK_SORT_ASCENDING);
    gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
            PM_COLUMN_PLAYLIST,
            pm_data_compare_func, column, NULL);

    gtk_tree_view_append_column(playlist_treeview, column);

    /* cell for graphic indicator */
    renderer = gtk_cell_renderer_pixbuf_new();

    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_cell_data_func(column, renderer, pm_cell_data_func_pix, NULL, NULL);
    /* cell for playlist name */
    renderer = gtk_cell_renderer_text_new();
    g_signal_connect (G_OBJECT (renderer), "edited",
            G_CALLBACK (pm_cell_edited), model);
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_cell_data_func(column, renderer, pm_cell_data_func, NULL, NULL);
    g_object_set(G_OBJECT (renderer), "editable", TRUE, NULL);
}

/* Free the playlist listview */
void pm_destroy_playlist_view(void) {
    if (GTK_IS_WIDGET(playlist_toolbar))
        gtk_widget_destroy(GTK_WIDGET(playlist_toolbar));

    if (GTK_IS_WIDGET(playlist_treeview))
        gtk_widget_destroy(GTK_WIDGET(playlist_treeview));

    playlist_toolbar = NULL;
    playlist_treeview = NULL;
}

/* Create playlist listview */
static void pm_create_treeview(void) {
    GtkTreeStore *model;
    GtkTreeSelection *selection;

    /* destroy old treeview */
    if (! playlist_treeview) {
        /* create new one */
        playlist_treeview = GTK_TREE_VIEW (gtk_tree_view_new());
        gtk_widget_set_events(GTK_WIDGET(playlist_treeview), GDK_KEY_RELEASE_MASK);
        gtk_tree_view_set_headers_visible(playlist_treeview, FALSE);
    } else {
        model = GTK_TREE_STORE (gtk_tree_view_get_model(playlist_treeview));
        g_return_if_fail (model);
        g_object_unref(model);
        GList *columns = gtk_tree_view_get_columns(playlist_treeview);
        while (columns != NULL) {
            GtkTreeViewColumn *column = columns->data;
            gtk_tree_view_remove_column(playlist_treeview, column);
            columns = columns->next;
        }
        g_list_free(columns);
    }

    /* create model */
    model = gtk_tree_store_new(PM_NUM_COLUMNS, G_TYPE_POINTER, G_TYPE_INT, G_TYPE_POINTER, G_TYPE_POINTER);
    /* set tree model */
    gtk_tree_view_set_model(playlist_treeview, GTK_TREE_MODEL (model));

    /* set selection mode */
    selection = gtk_tree_view_get_selection(playlist_treeview);
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
    g_signal_connect (G_OBJECT (selection), "changed", G_CALLBACK (pm_selection_changed), NULL);

    pm_add_columns();
    pm_add_all_itdbs();

    gtk_drag_source_set(GTK_WIDGET (playlist_treeview), GDK_BUTTON1_MASK, pm_drag_types, TGNR(pm_drag_types), GDK_ACTION_COPY
            | GDK_ACTION_MOVE);
    gtk_drag_dest_set(GTK_WIDGET (playlist_treeview), GTK_DEST_DEFAULT_HIGHLIGHT, pm_drop_types, TGNR(pm_drop_types), GDK_ACTION_COPY
            | GDK_ACTION_MOVE);

    /*   gtk_tree_view_enable_model_drag_dest (playlist_treeview, */
    /* 					pm_drop_types, TGNR (pm_drop_types), */
    /* 					GDK_ACTION_COPY); */
    /* need the gtk_drag_dest_set() with no actions ("0") so that the
     data_received callback gets the correct info value. This is most
     likely a bug... */
    /*   gtk_drag_dest_set_target_list (GTK_WIDGET (playlist_treeview), */
    /* 				 gtk_target_list_new (pm_drop_types, */
    /* 						      TGNR (pm_drop_types))); */

    g_signal_connect ((gpointer) playlist_treeview, "drag-begin",
            G_CALLBACK (pm_drag_begin),
            NULL);

    g_signal_connect ((gpointer) playlist_treeview, "drag-data-delete",
            G_CALLBACK (pm_drag_data_delete),
            NULL);

    g_signal_connect ((gpointer) playlist_treeview, "drag-data-get",
            G_CALLBACK (pm_drag_data_get),
            NULL);

    g_signal_connect ((gpointer) playlist_treeview, "drag-data-received",
            G_CALLBACK (pm_drag_data_received),
            NULL);

    g_signal_connect ((gpointer) playlist_treeview, "drag-drop",
            G_CALLBACK (pm_drag_drop),
            NULL);

    g_signal_connect ((gpointer) playlist_treeview, "drag-end",
            G_CALLBACK (pm_drag_end),
            NULL);

    g_signal_connect ((gpointer) playlist_treeview, "drag-leave",
            G_CALLBACK (pm_drag_leave),
            NULL);

    g_signal_connect ((gpointer) playlist_treeview, "drag-motion",
            G_CALLBACK (pm_drag_motion),
            NULL);

    g_signal_connect_after ((gpointer) playlist_treeview, "key_release_event",
            G_CALLBACK (on_playlist_treeview_key_release_event),
            NULL);
    g_signal_connect (G_OBJECT (playlist_treeview), "button-press-event",
            G_CALLBACK (pm_button_press), model);
}

static void pm_create_toolbar(GtkActionGroup *action_group) {
    GtkUIManager *mgr;

    mgr = gtk_ui_manager_new();

    gtk_ui_manager_insert_action_group(mgr, action_group, 0);

    gchar *toolbar_path = g_build_filename(get_glade_dir(), "playlist_display_toolbar.xml", NULL);
    gtk_ui_manager_add_ui_from_file(mgr, toolbar_path, NULL);

    playlist_toolbar = GTK_TOOLBAR(gtk_ui_manager_get_widget(mgr, "/PlaylistToolbar"));
    gtk_toolbar_set_style(playlist_toolbar, GTK_TOOLBAR_ICONS);
    gtk_toolbar_set_icon_size(playlist_toolbar, GTK_ICON_SIZE_SMALL_TOOLBAR);

}

GtkWidget *pm_create_playlist_view(GtkActionGroup *action_group) {
    GtkBox *vbox;
    GtkScrolledWindow *scrolledwin;

    vbox = GTK_BOX(gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));

    pm_create_toolbar(action_group);
    gtk_box_pack_start(vbox, GTK_WIDGET(playlist_toolbar), FALSE, TRUE, 0);

    pm_create_treeview();
    pm_sort(prefs_get_int("pm_sort"));

    // Add only the tree view to a scrolled window so that toolbar is always visible
    scrolledwin = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new(NULL, NULL));
    gtk_scrolled_window_set_policy(scrolledwin, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(scrolledwin, GTK_SHADOW_IN);
    gtk_widget_set_size_request(GTK_WIDGET(scrolledwin), 250, -1);
    gtk_container_add(GTK_CONTAINER(scrolledwin), GTK_WIDGET(playlist_treeview));
    gtk_box_pack_start(vbox, GTK_WIDGET(scrolledwin), TRUE, TRUE, 0);

    return GTK_WIDGET(vbox);
}

void pm_selected_playlists_foreach(PlaylistSelectionForeachFunc func, gpointer data) {
    GList *playlists = pm_get_selected_playlists();
    while(playlists) {
        Playlist *pl = playlists->data;
        (* func) (pl ,data);
        playlists = playlists->next;
    }
}

GList *pm_get_selected_playlists() {
    g_return_val_if_fail(playlist_treeview, NULL);

    GtkTreeSelection *selection = gtk_tree_view_get_selection(playlist_treeview);
    g_return_val_if_fail(selection, NULL);

    GtkTreeModel *model = gtk_tree_view_get_model(playlist_treeview);
    GList *paths = gtk_tree_selection_get_selected_rows(selection, &model);
    GList *playlists = NULL;

    while (paths) {
        GtkTreePath *path = paths->data;
        GtkTreeIter iter;

        if (gtk_tree_model_get_iter(model, &iter, path)) {
            Playlist *pl;
            gtk_tree_model_get(model, &iter, PM_COLUMN_PLAYLIST, &pl, -1);
            if (pl) {
                playlists = g_list_append(playlists, pl);
            }
        }

        paths = paths->next;
    }

    g_list_free(paths);

    return playlists;
}

Playlist *pm_get_first_selected_playlist(void) {

    GList *playlists = pm_get_selected_playlists();

    if (!playlists) {
        return NULL;
    }

    return playlists->data;
}

gint pm_get_selected_playlist_count() {
    GtkTreeSelection *selection = gtk_tree_view_get_selection(playlist_treeview);
    return gtk_tree_selection_count_selected_rows(selection);
}

gboolean pm_is_playlist_selected() {
    return pm_get_selected_playlist_count() > 0;
}

void pm_show_all_playlists() {
    gtk_tree_view_expand_all(playlist_treeview);
}

/**
 * playlist_display_update_itdb_cb:
 *
 * Callback for the itdb_updated signal emitted when an itdb is replaced in the gtkpod library.
 * Designed to remove the old itdb and add the new itdb in its place.
 *
 * @app: instance of the gtkpod app currently loaded.
 * @olditdb: pointer to the old itdb that should be removed from the display.
 * @newitdb: pointer to the new itdb that should be added in place of the old itdb.
 *
 */
void playlist_display_update_itdb_cb(GtkPodApp *app, gpointer olditdb, gpointer newitdb, gpointer data) {
    gint pos = -1; /* default: add to the end */

    g_return_if_fail (olditdb);
    g_return_if_fail (newitdb);

    iTunesDB *old_itdb = olditdb;
    iTunesDB *new_itdb = newitdb;

    /* get position of @old_itdb */
    pos = pm_get_position_for_itdb(old_itdb);

    /* remove @old_itdb (all playlists are removed if the MPL is
     removed and add @new_itdb at its place */

    pm_remove_playlist(itdb_playlist_mpl(old_itdb));

    /* display replacement */
    pm_add_itdb(new_itdb, pos);
}

void playlist_display_itdb_added_cb(GtkPodApp *app, gpointer itdb, gint32 pos, gpointer data) {
    iTunesDB *new_itdb = itdb;
    if (new_itdb == NULL) {
        return;
    }

    pm_add_itdb(new_itdb, pos);
}

void playlist_display_itdb_removed_cb(GtkPodApp *app, gpointer itdb, gpointer data) {
    iTunesDB *old_itdb = itdb;
    if (old_itdb == NULL) {
        return;
    }

    pm_remove_playlist(itdb_playlist_mpl(old_itdb));
}

void playlist_display_playlist_added_cb(GtkPodApp *app, gpointer pl, gint32 pos, gpointer data) {
    Playlist *new_playlist = pl;

    pm_add_child(new_playlist->itdb, PM_COLUMN_PLAYLIST, new_playlist, pos);
}

void playlist_display_playlist_removed_cb(GtkPodApp *app, gpointer pl, gpointer data) {
    Playlist *old_playlist = pl;

    pm_remove_playlist(old_playlist);
}

void playlist_display_preference_changed_cb(GtkPodApp *app, gpointer pfname, gpointer value, gpointer data) {
    gchar *pref_name = pfname;
    if (g_str_equal(pref_name, "pm_sort")) {
        GtkSortType *sort_type = value;
        pm_sort(*sort_type);
    }
    else if (g_str_equal(pref_name, "pm_case_sensitive")) {
        gint val = prefs_get_int("pm_sort");
        pm_sort((GtkSortType) val);
    }
}

void playlist_display_itdb_data_changed_cb(GtkPodApp *app, gpointer itdb, gpointer data) {
    iTunesDB *changed_itdb = itdb;
    pm_itdb_name_changed(changed_itdb);
}
