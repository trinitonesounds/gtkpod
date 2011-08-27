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

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <pango/pango.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "libgtkpod/gp_private.h"
#include "libgtkpod/misc_conversion.h"
#include "libgtkpod/misc_track.h"
#include "libgtkpod/misc.h"
#include "libgtkpod/prefs.h"
#include "libgtkpod/directories.h"
#include "libgtkpod/gtkpod_app_iface.h"
#include "display_tracks.h"
#include "rb_cell_renderer_rating.h"
#include "track_display_context_menu.h"

/* reference to gtkbuilder for use with track plugin */
static GtkBuilder *trackbuilder = NULL;
/* pointer to the container for the track display */
static GtkWidget *track_container;
/* pointer to the current playlist label */
static GtkWidget *current_playlist_label;
/* pointer to the search text entry */
static GtkWidget *search_entry;
/* pointer to the window containing the track view */
static GtkWidget *track_window;
/* pointer to the treeview for the track display */
static GtkTreeView *track_treeview = NULL;
/* array with pointers to the columns used in the track display */
static GtkTreeViewColumn *tm_columns[TM_NUM_COLUMNS];
/* column in which track pointer is stored */
static const gint READOUT_COL = 0;

/* compare function to be used for string comparisons */
static gint (*string_compare_func)(const gchar *str1, const gchar *str2, const gint case_sensitive) = compare_string;

static GtkTreeViewColumn *tm_add_column(TM_item tm_item, gint position);
static TM_item tm_lookup_col_id(GtkTreeViewColumn *column);

/* Drag and drop definitions */
static GtkTargetEntry tm_drag_types[] =
    {
        { DND_GTKPOD_TM_PATHLIST_TYPE, 0, DND_GTKPOD_TM_PATHLIST },
        { DND_GTKPOD_TRACKLIST_TYPE, 0, DND_GTKPOD_TRACKLIST },
        { "text/uri-list", 0, DND_TEXT_URI_LIST },
        { "text/plain", 0, DND_TEXT_PLAIN },
        { "STRING", 0, DND_TEXT_PLAIN } };
static GtkTargetEntry tm_drop_types[] =
    {
        { DND_GTKPOD_TM_PATHLIST_TYPE, 0, DND_GTKPOD_TM_PATHLIST },
        { "text/uri-list", 0, DND_TEXT_URI_LIST },
        { "text/plain", 0, DND_TEXT_PLAIN },
        { "STRING", 0, DND_TEXT_PLAIN } };

/* prefs strings */
const gchar *TM_PREFS_SEARCH_COLUMN = "tm_prefs_search_column";
const gchar *KEY_DISPLAY_SEARCH_ENTRY = "display_search_entry";

static GtkBuilder *get_track_builder() {
    if (!trackbuilder) {
        gchar *glade_path = g_build_filename(get_glade_dir(), "track_display.xml", NULL);
        trackbuilder = gtkpod_builder_xml_new(glade_path);
        g_free(glade_path);
    }
    return trackbuilder;
}

/* Convenience functions */
static gboolean filter_tracks(GtkTreeModel *model, GtkTreeIter *iter, gpointer entry) {
    Track *tr;
    gboolean result = FALSE;
    const gchar *text = gtk_entry_get_text(GTK_ENTRY (entry));
    int i;

    gtk_tree_model_get(model, iter, READOUT_COL, &tr, -1);

    if (tr) {
        if (text[0] == 0x0)
            return TRUE;
        for (i = 0; i < TM_NUM_COLUMNS; i++) {
            gint visible = prefs_get_int_index("col_visible", i);
            gchar *data;

            if (!visible)
                continue;

            data = track_get_text(tr, TM_to_T(i));
            if (data && utf8_strcasestr(data, text)) {
                g_free(data);
                result = TRUE;
                break;
            }

            g_free(data);
        }
    }

    return result;
}

static GtkListStore *get_model_as_store(GtkTreeModel *model) {
    if (!GTK_IS_TREE_MODEL_FILTER (model))
        return GTK_LIST_STORE (model);
    else
        return GTK_LIST_STORE (gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (model)));
}

static void convert_iter(GtkTreeModel *model, GtkTreeIter *from, GtkTreeIter *to) {
    if (!GTK_IS_TREE_MODEL_FILTER (model))
        *to = *from;
    else
        gtk_tree_model_filter_convert_iter_to_child_iter(GTK_TREE_MODEL_FILTER (model), to, from);
}

static GtkTreeModelFilter *get_filter(GtkTreeView *tree) {
    GtkTreeModel *model = gtk_tree_view_get_model(tree);

    if (GTK_IS_TREE_MODEL_FILTER (model))
        return GTK_TREE_MODEL_FILTER (model);
    else {
        GtkTreeModelFilter *filter = GTK_TREE_MODEL_FILTER (gtk_tree_model_filter_new (model, NULL));
        gtk_tree_model_filter_set_visible_func(filter, filter_tracks, search_entry, NULL);
        gtk_tree_model_filter_refilter(filter);
        gtk_tree_view_set_model(tree, GTK_TREE_MODEL (filter));

        return filter;
    }
}

void on_search_entry_changed(GtkEditable *editable, gpointer user_data) {
    gtk_tree_model_filter_refilter(get_filter(track_treeview));
}

/* ---------------------------------------------------------------- */
/* Section for track display                                        */
/* DND -- Drag And Drop                                             */
/* ---------------------------------------------------------------- */

/* Move the paths listed in @data before or after (according to @pos)
 @path. Used for DND */
static gboolean tm_move_pathlist(gchar *data, GtkTreePath *path, GtkTreeViewDropPosition pos) {
    GtkTreeIter temp;
    GtkTreeIter to_iter;
    GtkTreeIter *from_iter;
    GtkTreeModel *model;
    GtkListStore *store;
    GList *iterlist = NULL;
    GList *link;
    gchar **paths, **pathp;

    g_return_val_if_fail (data, FALSE);
    g_return_val_if_fail (*data, FALSE);

    model = gtk_tree_view_get_model(track_treeview);
    g_return_val_if_fail (model, FALSE);
    store = get_model_as_store(model);
    g_return_val_if_fail (store, FALSE);

    g_return_val_if_fail (gtk_tree_model_get_iter (model, &temp, path), FALSE);
    convert_iter(model, &temp, &to_iter);

    /* split the path list into individual strings */
    paths = g_strsplit(data, "\n", -1);
    pathp = paths;

    /* Convert the list of paths into a list of iters */
    while (*pathp) {
        if ((strlen(*pathp) > 0) && gtk_tree_model_get_iter_from_string(model, &temp, *pathp)) {
            from_iter = g_new (GtkTreeIter, 1);
            convert_iter(model, &temp, from_iter);
            iterlist = g_list_append(iterlist, from_iter);
        }

        ++pathp;
    }

    g_strfreev(paths);

    /* Move the iters in iterlist before or after @to_iter */
    switch (pos) {
    case GTK_TREE_VIEW_DROP_INTO_OR_AFTER:
    case GTK_TREE_VIEW_DROP_AFTER:
        for (link = g_list_last(iterlist); link; link = link->prev) {
            from_iter = (GtkTreeIter *) link->data;
            gtk_list_store_move_after(store, from_iter, &to_iter);
        }
        break;
    case GTK_TREE_VIEW_DROP_INTO_OR_BEFORE:
    case GTK_TREE_VIEW_DROP_BEFORE:
        for (link = g_list_first(iterlist); link; link = link->next) {
            from_iter = (GtkTreeIter *) link->data;
            gtk_list_store_move_before(store, from_iter, &to_iter);
        }
        break;
    }

    /* free iterlist */
    for (link = iterlist; link; link = link->next)
        g_free(link->data);
    g_list_free(iterlist);

    /*    update_model_view (model); -- not needed */
    tm_rows_reordered();
    return TRUE;
}

/*
 * utility function for appending ipod track ids for track view (DND)
 */
static void on_tm_dnd_get_track_foreach(GtkTreeModel *tm, GtkTreePath *tp, GtkTreeIter *i, gpointer data) {
    Track *tr;
    GString *tracklist = (GString *) data;

    g_return_if_fail (tracklist);

    gtk_tree_model_get(tm, i, READOUT_COL, &tr, -1);
    g_return_if_fail (tr);

    g_string_append_printf(tracklist, "%p\n", tr);
}

/*
 * utility function for appending path for track view (DND)
 */
static void on_tm_dnd_get_path_foreach(GtkTreeModel *tm, GtkTreePath *tp, GtkTreeIter *iter, gpointer data) {
    GString *filelist = (GString *) data;
    gchar *ps = gtk_tree_path_to_string(tp);
    g_string_append_printf(filelist, "%s\n", ps);
    g_free(ps);
}

/*
 * utility function for appending file for track view (DND)
 */
static void on_tm_dnd_get_file_foreach(GtkTreeModel *tm, GtkTreePath *tp, GtkTreeIter *iter, gpointer data) {
    Track *track;
    GString *filelist = (GString *) data;
    gchar *name;

    gtk_tree_model_get(tm, iter, READOUT_COL, &track, -1);
    name = get_file_name_from_source(track, SOURCE_PREFER_LOCAL);
    if (name) {
        g_string_append_printf(filelist, "file:%s\n", name);
        g_free(name);
    }
}

/*
 * utility function for appending file-uri for track view (DND)
 */
static void on_tm_dnd_get_uri_foreach(GtkTreeModel *tm, GtkTreePath *tp, GtkTreeIter *iter, gpointer data) {
    Track *track;
    GString *filelist = (GString *) data;
    gchar *name;

    gtk_tree_model_get(tm, iter, READOUT_COL, &track, -1);
    name = get_file_name_from_source(track, SOURCE_PREFER_LOCAL);
    if (name) {
        gchar *uri = g_filename_to_uri(name, NULL, NULL);
        if (uri) {
            g_string_append_printf(filelist, "%s\n", uri);
            g_free(uri);
        }
        g_free(name);
    }
}

/* remove dragged playlist after successful MOVE */
static void tm_drag_data_delete(GtkWidget *widget, GdkDragContext *dc, gpointer user_data) {
    GtkTreeSelection *ts;
    Playlist *pl = gtkpod_get_current_playlist();
    gint num;

    /*     puts ("tm_drag_data_delete"); */

    g_return_if_fail (widget);
    ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
    g_return_if_fail (ts);
    /* number of selected tracks */
    num = gtk_tree_selection_count_selected_rows(ts);
    if (num == 0)
        return;

    /* Check if we really have to delete the tracks */
    if (!itdb_playlist_is_mpl(pl)) { /* get list of selected tracks */
        GString *reply = g_string_sized_new(2000);
        gchar *str;
        Track *track;

        gtk_tree_selection_selected_foreach(ts, on_tm_dnd_get_track_foreach, reply);
        str = reply->str;
        while (parse_tracks_from_string(&str, &track)) {
            gp_playlist_remove_track(pl, track, DELETE_ACTION_PLAYLIST);
        }
        g_string_free(reply, TRUE);

        gtkpod_statusbar_message(ngettext ("Moved one track",
                "Moved %d tracks", num), num);
    }
    else {
        gtkpod_statusbar_message(ngettext ("Copied one track",
                "Copied %d tracks", num), num);
    }
}

static void tm_drag_end(GtkWidget *widget, GdkDragContext *dc, gpointer user_data) {
    /*     puts ("tm_drag_end"); */
    gp_remove_autoscroll_row_timeout(widget);
    gtkpod_tracks_statusbar_update();
}

static gboolean tm_drag_drop(GtkWidget *widget, GdkDragContext *dc, gint x, gint y, guint time, gpointer user_data) {
    GdkAtom target;

    /*     puts ("tm_drag_data_drop"); */

    gp_remove_autoscroll_row_timeout(widget);

    target = gtk_drag_dest_find_target(widget, dc, NULL);

    if (target != GDK_NONE) {
        gtk_drag_get_data(widget, dc, target, time);
        return TRUE;
    }
    return FALSE;
}

static void tm_drag_leave(GtkWidget *widget, GdkDragContext *dc, guint time, gpointer user_data) {
    /*     puts ("tm_drag_leave"); */
    gp_remove_autoscroll_row_timeout(widget);
}

static gboolean tm_drag_motion(GtkWidget *widget, GdkDragContext *dc, gint x, gint y, guint time, gpointer user_data) {
    GtkTreeView *treeview;
    GdkAtom target;
    GtkTreePath *path = NULL;
    GtkTreeViewDropPosition pos;
    iTunesDB *itdb;
    ExtraiTunesDBData *eitdb;

    /*     printf ("drag_motion  suggested: %d actions: %d\n", */
    /*  	    dc->suggested_action, dc->actions); */

    /*     printf ("x: %d y: %d\n", x, y); */

    g_return_val_if_fail (GTK_IS_TREE_VIEW (widget), FALSE);

    treeview = GTK_TREE_VIEW (widget);

    gp_install_autoscroll_row_timeout(widget);

    itdb = gp_get_selected_itdb();
    /* no drop is possible if no playlist/repository is selected */
    if (itdb == NULL) {
        gdk_drag_status(dc, 0, time);
        return FALSE;
    }
    eitdb = itdb->userdata;
    g_return_val_if_fail (eitdb, FALSE);
    /* no drop is possible if no repository is loaded */
    if (!eitdb->itdb_imported) {
        gdk_drag_status(dc, 0, time);
        return FALSE;
    }

    /* optically set destination row if available */
    if (gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW (widget), x, y, &path, &pos)) {
        /* drops are only allowed before and after -- not onto
         existing paths */
        switch (pos) {
        case GTK_TREE_VIEW_DROP_INTO_OR_AFTER:
        case GTK_TREE_VIEW_DROP_AFTER:
            gtk_tree_view_set_drag_dest_row(GTK_TREE_VIEW (widget), path, GTK_TREE_VIEW_DROP_AFTER);
            break;
        case GTK_TREE_VIEW_DROP_INTO_OR_BEFORE:
        case GTK_TREE_VIEW_DROP_BEFORE:
            gtk_tree_view_set_drag_dest_row(GTK_TREE_VIEW (widget), path, GTK_TREE_VIEW_DROP_BEFORE);
            break;
        }

        gtk_tree_path_free(path);
        path = NULL;
    }
    else {
        path = gtk_tree_path_new_first();
        gtk_tree_view_set_drag_dest_row(GTK_TREE_VIEW (widget), path, GTK_TREE_VIEW_DROP_BEFORE);
        gtk_tree_path_free(path);
        path = NULL;
    }

    target = gtk_drag_dest_find_target(widget, dc, NULL);

    /* no drop possible if no valid target can be found */
    if (target == GDK_NONE) {
        gdk_drag_status(dc, 0, time);
        return FALSE;
    }

    if (widget == gtk_drag_get_source_widget(dc)) { /* drag is within the same widget */
        gint column;
        GtkSortType order;
        GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW (widget));
        g_return_val_if_fail (model, FALSE);
        if (gtk_tree_sortable_get_sort_column_id(GTK_TREE_SORTABLE (model), &column, &order)) { /* don't allow move because the model is sorted */
            gdk_drag_status(dc, 0, time);
            return FALSE;
        }
        else { /* only allow moves within the same widget */
            gdk_drag_status(dc, GDK_ACTION_MOVE, time);
        }
    }
    else { /* whatever the source suggests */
        gdk_drag_status(dc, gdk_drag_context_get_suggested_action(dc), time);
    }

    return TRUE;
}

static void tm_drag_data_get(GtkWidget *widget, GdkDragContext *context, GtkSelectionData *data, guint info, guint time, gpointer user_data) {
    GtkTreeSelection *ts = NULL;
    GString *reply = g_string_sized_new(2000);

    /*     printf("tm drag get info: %d\n", info); */
    if ((data) && (ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget)))) {
        switch (info) {
        case DND_GTKPOD_TRACKLIST:
            gtk_tree_selection_selected_foreach(ts, on_tm_dnd_get_track_foreach, reply);
            break;
        case DND_GTKPOD_TM_PATHLIST:
            gtk_tree_selection_selected_foreach(ts, on_tm_dnd_get_path_foreach, reply);
            break;
        case DND_TEXT_URI_LIST:
            gtk_tree_selection_selected_foreach(ts, on_tm_dnd_get_uri_foreach, reply);
            break;
        case DND_TEXT_PLAIN:
            gtk_tree_selection_selected_foreach(ts, on_tm_dnd_get_file_foreach, reply);
            break;
        default:
            g_warning ("Programming error: tm_drag_data_get received unknown info type (%d)\n", info);
            break;
        }
    }
    gtk_selection_data_set(data, gtk_selection_data_get_target(data), 8, reply->str, reply->len);
    g_string_free(reply, TRUE);
}

static void tm_drag_data_received(GtkWidget *widget, GdkDragContext *dc, gint x, gint y, GtkSelectionData *data, guint info, guint time, gpointer user_data) {
    GtkTreePath *path = NULL;
    GtkTreeModel *model = NULL;
    GtkTreeViewDropPosition pos = 0;
    gboolean result = FALSE;

    /* printf ("sm drop received info: %d\n", info); */

    /* sometimes we get empty dnd data, ignore */
    if (widgets_blocked || (!dc) || (!data) || (gtk_selection_data_get_length(data) < 0))
        return;
    /* yet another check, i think it's an 8 bit per byte check */
    if (gtk_selection_data_get_format(data) != 8)
        return;

    gp_remove_autoscroll_row_timeout(widget);

    model = gtk_tree_view_get_model(GTK_TREE_VIEW (widget));
    g_return_if_fail (model);
    if (!gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW (widget), x, y, &path, &pos)) {
        gint py;
        gdk_window_get_pointer(gtk_tree_view_get_bin_window(GTK_TREE_VIEW (widget)), NULL, &py, NULL);
        if (py < 5) {
            /* initialize with first displayed and drop before */
            GtkTreeIter iter;
            if (gtk_tree_model_get_iter_first(model, &iter)) {
                path = gtk_tree_model_get_path(model, &iter);
                pos = GTK_TREE_VIEW_DROP_BEFORE;
            }
        }
        else { /* initialize with last path if available and drop after */
            GtkTreeIter iter;
            if (gtk_tree_model_get_iter_first(model, &iter)) {
                GtkTreeIter last_valid_iter;
                do {
                    last_valid_iter = iter;
                }
                while (gtk_tree_model_iter_next(model, &iter));
                path = gtk_tree_model_get_path(model, &last_valid_iter);
                pos = GTK_TREE_VIEW_DROP_AFTER;
            }
        }
    }

    if (path) { /* map position onto BEFORE or AFTER */
        switch (pos) {
        case GTK_TREE_VIEW_DROP_INTO_OR_AFTER:
        case GTK_TREE_VIEW_DROP_AFTER:
            pos = GTK_TREE_VIEW_DROP_AFTER;
            break;
        case GTK_TREE_VIEW_DROP_INTO_OR_BEFORE:
        case GTK_TREE_VIEW_DROP_BEFORE:
            pos = GTK_TREE_VIEW_DROP_BEFORE;
            break;
        }
    }

    gchar *data_copy = g_strdup(gtk_selection_data_get_data(data));
    switch (info) {
    case DND_GTKPOD_TM_PATHLIST:
        g_return_if_fail (path);
        result = tm_move_pathlist(data_copy, path, pos);
        gdk_drag_status(dc, GDK_ACTION_MOVE, time);
        gtk_drag_finish(dc, TRUE, FALSE, time);
        break;
    case DND_TEXT_PLAIN:
        result = tm_add_filelist(data_copy, path, pos);
        gdk_drag_status(dc, gdk_drag_context_get_suggested_action(dc), time);
        if (gdk_drag_context_get_selected_action(dc) == GDK_ACTION_MOVE)
            gtk_drag_finish(dc, TRUE, TRUE, time);
        else
            gtk_drag_finish(dc, TRUE, FALSE, time);
        break;
    case DND_TEXT_URI_LIST:
        result = tm_add_filelist(data_copy, path, pos);
        gdk_drag_status(dc, gdk_drag_context_get_suggested_action(dc), time);
        if (gdk_drag_context_get_selected_action(dc) == GDK_ACTION_MOVE)
            gtk_drag_finish(dc, TRUE, TRUE, time);
        else
            gtk_drag_finish(dc, TRUE, FALSE, time);
        break;
    default:
        gdk_drag_status(dc, 0, time);
        gtk_drag_finish(dc, FALSE, FALSE, time);
        /* 	puts ("tm_drag_data_received(): should not be reached"); */
        break;
    }
    g_free(data_copy);

    if (path)
        gtk_tree_path_free(path);
}

/* ---------------------------------------------------------------- */
/* Section for track display                                        */
/* other callbacks                                                  */
/* ---------------------------------------------------------------- */

static gboolean on_track_treeview_key_release_event(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    guint mods;
    mods = event->state;

    if (!widgets_blocked && (mods & GDK_CONTROL_MASK)) {
        switch (event->keyval) {
        /* 	    case GDK_u: */
        /* 		gp_do_selected_tracks (update_tracks); */
        /* 		break; */
        default:
            break;
        }
    }
    return FALSE;
}

/* ---------------------------------------------------------------- */
/* Section for track display                                        */
/* ---------------------------------------------------------------- */

/* Append track to the track model (or write into @into_iter if != 0) */
void tm_add_track_to_track_model(Track *track, GtkTreeIter *into_iter) {
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model(track_treeview);

    g_return_if_fail (model);

    if (into_iter) {
        convert_iter(model, into_iter, &iter);
    }
    else {
        gtk_list_store_append(get_model_as_store(model), &iter);
    }

    gtk_list_store_set(get_model_as_store(model), &iter, READOUT_COL, track, -1);
}

/* Used by remove_track() to remove track from model by calling
 gtk_tree_model_foreach ().
 Entry is deleted if data == track */
static gboolean tm_delete_track(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data) {
    Track *track;

    gtk_tree_model_get(model, iter, READOUT_COL, &track, -1);

    if (track == (Track *) data) {
        GtkTreeIter temp;

        GtkTreeSelection *selection = gtk_tree_view_get_selection(track_treeview);
        /*       printf("unselect...\n"); */
        gtk_tree_selection_unselect_iter(selection, iter);
        /*       printf("...unselect done\n"); */

        convert_iter(model, iter, &temp);
        gtk_list_store_remove(get_model_as_store(model), &temp);
        /*        update_model_view (model); -- not needed */
        return TRUE;
    }
    return FALSE;
}

/* Remove track from the display model */
void tm_remove_track(Track *track) {
    GtkTreeModel *model = gtk_tree_view_get_model(track_treeview);

    if (model) {
        gtk_tree_model_foreach(model, tm_delete_track, track);
        /*        update_model_view (model); -- not needed */
    }
}

/* Remove all tracks from the display model */
void tm_remove_all_tracks() {
    GtkTreeModel *model = gtk_tree_view_get_model(track_treeview);

    /* remove all tracks, including tracks filtered out */
    gtk_list_store_clear(get_model_as_store(model));

    /* reset filter text -- if many tracks are added with the filter
     * activated, a lot of time is needed */
    gtk_entry_set_text(GTK_ENTRY (search_entry), "");

    tm_store_col_order();
    tm_update_default_sizes();
}

/* find out at which position column @tm_item is displayed */
/* static gint tm_get_col_position (TM_item tm_item) */
/* { */
/*     gint i; */
/*     GtkTreeViewColumn *col; */

/*     if (!track_treeview) return -1; */

/*     for (i=0; i<TM_NUM_COLUMNS_PREFS; ++i) */
/*     { */
/* 	col = gtk_tree_view_get_column (track_treeview, i); */
/* 	if (col->sort_column_id == tm_item) return i; */
/*     } */
/*     return -1; */
/* } */

/* store the order of the track view columns */
void tm_store_col_order(void) {
    gint i;
    GtkTreeViewColumn *col;

    for (i = 0; i < TM_NUM_COLUMNS; ++i) {
        col = gtk_tree_view_get_column(track_treeview, i);
        if (col != NULL) {
            prefs_set_int_index("col_order", i, gtk_tree_view_column_get_sort_column_id(col));
        }
    }
}

/* Used by tm_track_changed() to find the track that
 changed name. If found, emit a "row changed" signal */
static gboolean tm_model_track_changed(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data) {
    Track *track;

    gtk_tree_model_get(model, iter, READOUT_COL, &track, -1);
    if (track == (Track *) data) {
        gtk_tree_model_row_changed(model, path, iter);
        return TRUE;
    }
    return FALSE;
}

/* One of the tracks has changed (this happens when the
 iTunesDB is read and some IDs are renumbered */
void tm_track_changed(Track *track) {
    GtkTreeModel *model = gtk_tree_view_get_model(track_treeview);
    if (model != NULL)
        gtk_tree_model_foreach(model, tm_model_track_changed, track);
}

#if ((GTK_MAJOR_VERSION == 2) && (GTK_MINOR_VERSION < 2))
/* gtk_tree_selection_get_selected_rows() was introduced in 2.2 */
struct gtsgsr
{
    GtkTreeModel **model;
    GList **list;
};

void gtssf (GtkTreeModel *model,
        GtkTreePath *path,
        GtkTreeIter *iter,
        gpointer data)
{
    struct gtsgsr *gts = data;
    *gts->model = model;
    *gts->list = g_list_append (*gts->list, gtk_tree_path_copy (path));
}

GList *gtk_tree_selection_get_selected_rows (GtkTreeSelection *selection,
        GtkTreeModel **model)
{
    struct gtsgsr gts;
    GList *list = NULL;

    gts.model = model;
    gts.list = &list;

    gtk_tree_selection_selected_foreach (selection, gtssf, &gts);
    return list;
}
#endif

static void tm_rating_edited(RBCellRendererRating *renderer, const gchar *path_string, double rating) {
    GtkTreeModel *model = gtk_tree_view_get_model(track_treeview);
    GtkTreeIter iter;
    Track *track;
    GtkTreePath *path = gtk_tree_path_new_from_string(path_string);

    g_return_if_fail (model);
    g_return_if_fail (path);
    g_return_if_fail (gtk_tree_model_get_iter (model, &iter, path));

    gtk_tree_path_free(path);
    gtk_tree_model_get(model, &iter, READOUT_COL, &track, -1);

    if ((int) rating * ITDB_RATING_STEP != track->rating) {
        track->rating = (int) rating * ITDB_RATING_STEP;
        track->time_modified = time(NULL);
        gtkpod_track_updated(track);
        data_changed(track->itdb);

        if (prefs_get_int("id3_write")) {
            write_tags_to_file(track);
            gp_duplicate_remove(NULL, NULL);
        }
    }
}

static void update_text_column_layout(GtkTreeViewColumn *tree_column, GtkCellRenderer *renderer, const gchar* text) {
    GtkWidget* tree_widget;
    PangoLayout* layout;
    guint xpad;
    int col_width;
    int new_width;

    tree_widget = gtk_tree_view_column_get_tree_view(tree_column);
    if (!tree_widget)
        return;

    layout = gtk_widget_create_pango_layout(tree_widget, text);

    /* Expand the width, if necessary. This is done manually
     because the column is set to fixed width for performance
     reasons. */
    col_width = gtk_tree_view_column_get_fixed_width(tree_column);
    g_object_get(G_OBJECT (renderer), "xpad", &xpad, NULL);
    pango_layout_get_pixel_size(layout, &new_width, NULL);
    new_width += xpad;
    if (col_width < new_width) {
        gtk_tree_view_column_set_fixed_width(tree_column, new_width);
    }

    g_object_unref(G_OBJECT (layout));
}

/* The track data is stored in a separate list (static GList *tracks)
 and only pointers to the corresponding Track structure are placed
 into the model.
 This function reads the data for the given cell from the list and
 passes it to the renderer. */
static void tm_cell_data_text_func(GtkTreeViewColumn *tree_column, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer data) {
    Track *track;
    ExtraTrackData *etr;
    iTunesDB *itdb;
    TM_item column;
    gchar *text;

    column = (TM_item) g_object_get_data(G_OBJECT (renderer), "column");

    g_return_if_fail ((column >= 0) && (column < TM_NUM_COLUMNS));

    gtk_tree_model_get(model, iter, READOUT_COL, &track, -1);
    g_return_if_fail (track);
    etr = track->userdata;
    g_return_if_fail (etr);
    itdb = track->itdb;
    g_return_if_fail (itdb);

    text = track_get_text(track, TM_to_T(column));

    g_object_set(G_OBJECT (renderer), "text", text, NULL);

    update_text_column_layout(tree_column, renderer, text);

    g_free(text);
}

/* The track data is stored in a separate list (static GList *tracks)
 and only pointers to the corresponding Track structure are placed
 into the model.
 This function reads the data for the given cell from the list and
 passes it to the renderer. */
static void tm_cell_data_toggle_func(GtkTreeViewColumn *tree_column, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer data) {
    Track *track;
    ExtraTrackData *etr;
    iTunesDB *itdb;
    TM_item column;

    column = (TM_item) g_object_get_data(G_OBJECT (renderer), "column");

    g_return_if_fail ((column >= 0) && (column < TM_NUM_COLUMNS));

    gtk_tree_model_get(model, iter, READOUT_COL, &track, -1);
    g_return_if_fail (track);
    etr = track->userdata;
    g_return_if_fail (etr);
    itdb = track->itdb;
    g_return_if_fail (itdb);

    switch (column) {
    case TM_COLUMN_LYRICS:
        g_object_set(G_OBJECT (renderer), "active", track->lyrics_flag, NULL);
        break;
    case TM_COLUMN_TRANSFERRED:
        g_object_set(G_OBJECT (renderer), "active", track->transferred, NULL);
        break;
    case TM_COLUMN_COMPILATION:
        g_object_set(G_OBJECT (renderer), "active", track->compilation, NULL);
        break;
    default:
        g_return_if_reached();
    }
}

/* The track data is stored in a separate list (static GList *tracks)
 and only pointers to the corresponding Track structure are placed
 into the model.
 This function reads the data for the given cell from the list and
 passes it to the renderer. */
static void tm_cell_data_rating_func(GtkTreeViewColumn *tree_column, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer data) {
    Track *track;
    ExtraTrackData *etr;
    iTunesDB *itdb;
    TM_item column;

    column = (TM_item) g_object_get_data(G_OBJECT (renderer), "column");

    g_return_if_fail ((column >= 0) && (column < TM_NUM_COLUMNS));

    gtk_tree_model_get(model, iter, READOUT_COL, &track, -1);
    g_return_if_fail (track);
    etr = track->userdata;
    g_return_if_fail (etr);
    itdb = track->itdb;
    g_return_if_fail (itdb);

    switch (column) {
    case TM_COLUMN_RATING:
        g_object_set(G_OBJECT (renderer), "rating", (double) (track->rating / ITDB_RATING_STEP), NULL);
        break;
    default:
        g_return_if_reached();
    }
}

/* This function is analogous to tm_cell_data_func(), but is only used
 for the title column to distinguish between the text and the toggle
 button there. The other toggle buttons (e.g. compilation) can
 easily be handled in the original tm_cell_data_func() */
static void tm_cell_data_func_toggle(GtkTreeViewColumn *tree_column, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer data) {
    Track *track;
    TM_item column;

    column = (TM_item) g_object_get_data(G_OBJECT (renderer), "column");
    gtk_tree_model_get(model, iter, READOUT_COL, &track, -1);

    /* printf ("tm_cell_data_func_toggle() entered\n"); */

    switch (column) {
    case TM_COLUMN_TITLE:
        g_object_set(G_OBJECT (renderer), "active", !track->checked, "activatable", TRUE, NULL);
        break;
    default:
        g_warning ("Programming error: unknown column in tm_cell_data_func_toggle: %d\n", column);
        break;
    }
}

/* Called when a toggle cell is being changed. Stores new data to the
 track list. */
static void tm_cell_toggled(GtkCellRendererToggle *renderer, gchar *arg1, gpointer user_data) {
    GtkTreeModel *model;
    GtkTreeSelection *selection;
    TM_item column;
    gboolean multi_edit;
    gint sel_rows_num;
    GList *row_list, *row_node, *first;
    gboolean active;
    GList *selected_tracks = NULL;

    column = (TM_item) g_object_get_data(G_OBJECT(renderer), "column");
    multi_edit = prefs_get_int("multi_edit");
    selection = gtk_tree_view_get_selection(track_treeview);
    row_list = gtk_tree_selection_get_selected_rows(selection, &model);

    /* printf("tm_cell_toggled: column: %d, arg1: %p\n", column, arg1); */

    sel_rows_num = g_list_length(row_list);

    /* block widgets and update display if multi-edit is active */
    if (multi_edit && (sel_rows_num > 1))
        block_widgets();

    first = g_list_first(row_list);

    /* active will show the old state -- before the toggle */
    g_object_get(G_OBJECT (renderer), "active", &active, NULL);

    for (row_node = first; row_node && (multi_edit || (row_node == first)); row_node = g_list_next(row_node)) {
        Track *track;
        gboolean changed;
        GtkTreeIter iter;

        gtk_tree_model_get_iter(model, &iter, (GtkTreePath *) row_node->data);
        gtk_tree_model_get(model, &iter, READOUT_COL, &track, -1);
        changed = FALSE;

        switch (column) {
        case TM_COLUMN_TITLE:
            if ((active && (track->checked == 0)) || (!active && (track->checked == 1)))
                changed = TRUE;
            if (active)
                track->checked = 1;
            else
                track->checked = 0;
            break;
        case TM_COLUMN_COMPILATION:
            if ((!active && (track->compilation == 0)) || (active && (track->compilation == 1)))
                changed = TRUE;
            if (!active)
                track->compilation = 1;
            else
                track->compilation = 0;
            break;
        case TM_COLUMN_LYRICS:
            /* collect all selected tracks -- then call "edit details" */
            selected_tracks = g_list_append(selected_tracks, track);
            break;
        case TM_COLUMN_ARTIST:
        case TM_COLUMN_ALBUM:
        case TM_COLUMN_GENRE:
        case TM_COLUMN_COMPOSER:
        case TM_COLUMN_TRACK_NR:
        case TM_COLUMN_IPOD_ID:
        case TM_COLUMN_PC_PATH:
        case TM_COLUMN_TRANSFERRED:
        case TM_COLUMN_SIZE:
        case TM_COLUMN_TRACKLEN:
        case TM_COLUMN_BITRATE:
        case TM_COLUMN_PLAYCOUNT:
        case TM_COLUMN_RATING:
        case TM_COLUMN_TIME_PLAYED:
        case TM_COLUMN_TIME_MODIFIED:
        case TM_COLUMN_VOLUME:
        case TM_COLUMN_YEAR:
        case TM_COLUMN_CD_NR:
        case TM_COLUMN_TIME_ADDED:
        case TM_COLUMN_IPOD_PATH:
        case TM_COLUMN_SOUNDCHECK:
        case TM_COLUMN_SAMPLERATE:
        case TM_COLUMN_BPM:
        case TM_COLUMN_FILETYPE:
        case TM_COLUMN_GROUPING:
        case TM_COLUMN_COMMENT:
        case TM_COLUMN_CATEGORY:
        case TM_COLUMN_DESCRIPTION:
        case TM_COLUMN_PODCASTURL:
        case TM_COLUMN_PODCASTRSS:
        case TM_COLUMN_SUBTITLE:
        case TM_COLUMN_TIME_RELEASED:
        case TM_COLUMN_THUMB_PATH:
        case TM_COLUMN_MEDIA_TYPE:
        case TM_COLUMN_TV_SHOW:
        case TM_COLUMN_TV_EPISODE:
        case TM_COLUMN_TV_NETWORK:
        case TM_COLUMN_SEASON_NR:
        case TM_COLUMN_EPISODE_NR:
        case TM_COLUMN_ALBUMARTIST:
        case TM_COLUMN_SORT_ARTIST:
        case TM_COLUMN_SORT_TITLE:
        case TM_COLUMN_SORT_ALBUM:
        case TM_COLUMN_SORT_ALBUMARTIST:
        case TM_COLUMN_SORT_COMPOSER:
        case TM_COLUMN_SORT_TVSHOW:
        case TM_NUM_COLUMNS:
            /* these are not toggle buttons or are non-editable */
            break;
        }
        /*      printf ("  changed: %d\n", changed); */
        if (changed) {
            track->time_modified = time(NULL);
            /*        pm_track_changed (track);  notify playlist model... -- not
             *        necessary here because only the track model is affected */
            data_changed(track->itdb); /* indicate that data has changed */

            /* If the changed column is the compilation flag update the file
             if required */
            if (column == TM_COLUMN_COMPILATION)
                if (prefs_get_int("id3_write"))
                    write_tags_to_file(track);

        }
        while (widgets_blocked && gtk_events_pending())
            gtk_main_iteration();
    }

    if ((column == TM_COLUMN_LYRICS) && (selected_tracks != NULL)) {
        gtkpod_edit_lyrics(selected_tracks);
        g_list_free(selected_tracks);
    }

    if (multi_edit && (sel_rows_num > 1))
        release_widgets();

    g_list_foreach(row_list, (GFunc) gtk_tree_path_free, NULL);
    g_list_free(row_list);
}

/**
 * tm_get_nr_of_tracks - get the number of tracks displayed
 * currently in the track model Returns - the number of tracks displayed
 * currently
 */
gint tm_get_nr_of_tracks(void) {
    gint result = 0;
    GtkTreeModel *tm = NULL;

    tm = gtk_tree_view_get_model(track_treeview);
    if (tm) {
        result = gtk_tree_model_iter_n_children(tm, NULL);
    }
    return result;
}

static gint comp_int(gconstpointer a, gconstpointer b) {
    return (GPOINTER_TO_INT(a) - (GPOINTER_TO_INT(b)));
}

/* Redisplays the tracks in the track view according to the order
 * stored in the sort tab view. This only works if the track view is
 * not sorted --> skip if sorted */

static void tm_adopt_order(void) {
    if (prefs_get_int("tm_sort") == SORT_NONE) {
        GList *gl, *tracks = NULL;

        /* retrieve the currently displayed tracks (non ordered) from
         the last sort tab or from the selected playlist if no sort
         tabs are being used */
        tm_remove_all_tracks();
        tracks = gtkpod_get_displayed_tracks();
        for (gl = tracks; gl; gl = gl->next)
            tm_add_track_to_track_model((Track *) gl->data, NULL);
    }
}

/**
 * Reorder tracks in playlist to match order of tracks displayed in track
 * view. Only the subset of tracks currently displayed is reordered.
 * data_changed() is called when necessary.
 */
void tm_rows_reordered(void) {
    Playlist *current_pl;

    g_return_if_fail (track_treeview);
    current_pl = gtkpod_get_current_playlist();

    if (current_pl) {
        GtkTreeModel *tm = NULL;
        GtkTreeIter i;
        GList *new_list = NULL, *old_pos_l = NULL;
        gboolean valid = FALSE;
        GList *nlp, *olp;
        gboolean changed = FALSE;
        iTunesDB *itdb = NULL;

        tm = gtk_tree_view_get_model(track_treeview);
        g_return_if_fail (tm);

        valid = gtk_tree_model_get_iter_first(tm, &i);
        while (valid) {
            Track *new_track;
            gint old_position;

            gtk_tree_model_get(tm, &i, READOUT_COL, &new_track, -1);
            g_return_if_fail (new_track);

            if (!itdb)
                itdb = new_track->itdb;
            new_list = g_list_append(new_list, new_track);
            /* what position was this track in before? */
            old_position = g_list_index(current_pl->members, new_track);
            /* check if we already used this position before (can
             happen if track has been added to playlist more than
             once */
            while ((old_position != -1) && g_list_find(old_pos_l, GINT_TO_POINTER(old_position))) { /* find next occurence */
                GList *link;
                gint next;
                link = g_list_nth(current_pl->members, old_position + 1);
                next = g_list_index(link, new_track);
                if (next == -1)
                    old_position = -1;
                else
                    old_position += (1 + next);
            }
            /* we make a sorted list of the old positions */
            old_pos_l = g_list_insert_sorted(old_pos_l, GINT_TO_POINTER(old_position), comp_int);
            valid = gtk_tree_model_iter_next(tm, &i);
        }
        nlp = new_list;
        olp = old_pos_l;
        while (nlp && olp) {
            GList *old_link;
            guint position = GPOINTER_TO_INT(olp->data);

            /* if position == -1 one of the tracks in the track view
             could not be found in the selected playlist -> stop! */
            if (position == -1) {
                g_warning ("Programming error: tm_rows_reordered_callback: track in track view was not in selected playlist\n");
                g_return_if_reached ();
            }
            old_link = g_list_nth(current_pl->members, position);
            /* replace old track with new track */
            if (old_link->data != nlp->data) {
                old_link->data = nlp->data;
                changed = TRUE;
            }
            /* next */
            nlp = nlp->next;
            olp = olp->next;
        }
        g_list_free(new_list);
        g_list_free(old_pos_l);
        /* if we changed data, mark data as changed and adopt order in
         sort tabs */
        if (changed) {
            data_changed(itdb);
        }
    }
}

static void on_trackids_list_foreach(GtkTreeModel *tm, GtkTreePath *tp, GtkTreeIter *i, gpointer data) {
    Track *tr = NULL;
    GList *l = *((GList**) data);
    gtk_tree_model_get(tm, i, READOUT_COL, &tr, -1);
    g_return_if_fail (tr);
    l = g_list_append(l, GUINT_TO_POINTER(tr->id));
    *((GList**) data) = l;
}

/* return a list containing the track IDs of all tracks currently being
 selected */
GList *
tm_get_selected_trackids(void) {
    GList *result = NULL;
    GtkTreeSelection *ts = NULL;

    if ((ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(track_treeview)))) {
        gtk_tree_selection_selected_foreach(ts, on_trackids_list_foreach, &result);
    }
    return (result);
}

static gboolean on_all_trackids_list_foreach(GtkTreeModel *tm, GtkTreePath *tp, GtkTreeIter *i, gpointer data) {
    on_trackids_list_foreach(tm, tp, i, data);
    return FALSE;
}

/* return a list containing the track IDs of all tracks currently being
 displayed */
GList *
tm_get_all_trackids(void) {
    GList *result = NULL;
    GtkTreeModel *model;

    if ((model = gtk_tree_view_get_model(track_treeview))) {
        gtk_tree_model_foreach(model, on_all_trackids_list_foreach, &result);
    }
    return (result);
}

/* Prepends to list, so the list will be reversed. Make sure to
 reverse the result if you care about the order of the tracks. */
static void on_tracks_list_foreach(GtkTreeModel *tm, GtkTreePath *tp, GtkTreeIter *i, gpointer data) {
    Track *tr = NULL;
    GList *l = *((GList**) data);

    gtk_tree_model_get(tm, i, READOUT_COL, &tr, -1);
    g_return_if_fail (tr);
    l = g_list_prepend(l, tr);
    *((GList**) data) = l;
}

/* return a list containing pointers to all tracks currently being
 selected */
GList *
tm_get_selected_tracks(void) {
    GList *result = NULL;
    GtkTreeSelection *ts = NULL;

    if ((ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(track_treeview)))) {
        gtk_tree_selection_selected_foreach(ts, on_tracks_list_foreach, &result);
        result = g_list_reverse(result);
    }
    return (result);
}

void tm_select_all_tracks(void) {
    GtkTreeSelection *ts = NULL;
    ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(track_treeview));
    gtk_tree_selection_select_all(ts);
}

/* used by tm_get_all_tracks */
static gboolean on_all_tracks_list_foreach(GtkTreeModel *tm, GtkTreePath *tp, GtkTreeIter *i, gpointer data) {
    on_tracks_list_foreach(tm, tp, i, data);
    return FALSE;
}

/* return a list containing pointers to all tracks currently being
 displayed. You must g_list_free() the list after use. */
GList *
tm_get_all_tracks(void) {
    GList *result = NULL;
    GtkTreeModel *model = gtk_tree_view_get_model(track_treeview);

    g_return_val_if_fail (model, NULL);

    gtk_tree_model_foreach(model, on_all_tracks_list_foreach, &result);
    result = g_list_reverse(result);
    return result;
}

/* Function to compare @tm_item of @track1 and @track2. Used by
 tm_data_compare_func() */
static gint tm_data_compare(Track *track1, Track *track2, TM_item tm_item) {
    gint cmp = 0;
    ExtraTrackData *etr1, *etr2;

    /* string_compare_func is set to either compare_string_fuzzy or
     compare_string in tm_sort_column_changed() which is called
     once before the comparing begins. */
    gint case_sensitive = prefs_get_int("tm_case_sensitive");
    switch (tm_item) {
    case TM_COLUMN_TITLE:
        cmp = string_compare_func(track1->title, track2->title, case_sensitive);
        break;
    case TM_COLUMN_ALBUM:
        cmp = string_compare_func(track1->album, track2->album, case_sensitive);
        break;
    case TM_COLUMN_ALBUMARTIST:
        cmp = string_compare_func(track1->albumartist, track2->albumartist, case_sensitive);
        break;
    case TM_COLUMN_GENRE:
        cmp = string_compare_func(track1->genre, track2->genre, case_sensitive);
        break;
    case TM_COLUMN_COMPOSER:
        cmp = string_compare_func(track1->composer, track2->composer, case_sensitive);
        break;
    case TM_COLUMN_COMMENT:
        cmp = string_compare_func(track1->comment, track2->comment, case_sensitive);
        break;
    case TM_COLUMN_FILETYPE:
        cmp = string_compare_func(track1->filetype, track2->filetype, case_sensitive);
        break;
    case TM_COLUMN_GROUPING:
        cmp = string_compare_func(track1->grouping, track2->grouping, case_sensitive);
        break;
    case TM_COLUMN_ARTIST:
        cmp = string_compare_func(track1->artist, track2->artist, case_sensitive);
        break;
    case TM_COLUMN_CATEGORY:
        cmp = string_compare_func(track1->category, track2->category, case_sensitive);
        break;
    case TM_COLUMN_DESCRIPTION:
        cmp = string_compare_func(track1->description, track2->description, case_sensitive);
        break;
    case TM_COLUMN_PODCASTURL:
        cmp = string_compare_func(track1->podcasturl, track2->podcasturl, case_sensitive);
        break;
    case TM_COLUMN_PODCASTRSS:
        cmp = string_compare_func(track1->podcastrss, track2->podcastrss, case_sensitive);
        break;
    case TM_COLUMN_SUBTITLE:
        cmp = string_compare_func(track1->subtitle, track2->subtitle, case_sensitive);
        break;
    case TM_COLUMN_TV_SHOW:
        cmp = string_compare_func(track1->tvshow, track2->tvshow, case_sensitive);
        break;
    case TM_COLUMN_TV_EPISODE:
        cmp = string_compare_func(track1->tvepisode, track2->tvepisode, case_sensitive);
        break;
    case TM_COLUMN_TV_NETWORK:
        cmp = string_compare_func(track1->tvnetwork, track2->tvnetwork, case_sensitive);
        break;
    case TM_COLUMN_SORT_TITLE:
        cmp = string_compare_func(track1->sort_title, track2->sort_title, case_sensitive);
        break;
    case TM_COLUMN_SORT_ALBUM:
        cmp = string_compare_func(track1->sort_album, track2->sort_album, case_sensitive);
        break;
    case TM_COLUMN_SORT_ARTIST:
        cmp = string_compare_func(track1->sort_artist, track2->sort_artist, case_sensitive);
        break;
    case TM_COLUMN_SORT_ALBUMARTIST:
        cmp = string_compare_func(track1->sort_albumartist, track2->sort_albumartist, case_sensitive);
        break;
    case TM_COLUMN_SORT_COMPOSER:
        cmp = string_compare_func(track1->sort_composer, track2->sort_composer, case_sensitive);
        break;
    case TM_COLUMN_SORT_TVSHOW:
        cmp = string_compare_func(track1->sort_tvshow, track2->sort_tvshow, case_sensitive);
        break;
    case TM_COLUMN_TRACK_NR:
        cmp = track1->tracks - track2->tracks;
        if (cmp == 0)
            cmp = track1->track_nr - track2->track_nr;
        break;
    case TM_COLUMN_CD_NR:
        cmp = track1->cds - track2->cds;
        if (cmp == 0)
            cmp = track1->cd_nr - track2->cd_nr;
        break;
    case TM_COLUMN_IPOD_ID:
        cmp = track1->id - track2->id;
        break;
    case TM_COLUMN_PC_PATH:
        etr1 = track1->userdata;
        etr2 = track2->userdata;
        g_return_val_if_fail (etr1 && etr2, 0);
        cmp = g_utf8_collate(etr1->pc_path_utf8, etr2->pc_path_utf8);
        break;
    case TM_COLUMN_IPOD_PATH:
        cmp = g_utf8_collate(track1->ipod_path, track2->ipod_path);
        break;
    case TM_COLUMN_THUMB_PATH:
        etr1 = track1->userdata;
        etr2 = track2->userdata;
        g_return_val_if_fail (etr1 && etr2, 0);
        cmp = g_utf8_collate(etr1->thumb_path_utf8, etr2->thumb_path_utf8);
        break;
    case TM_COLUMN_TRANSFERRED:
        if (track1->transferred == track2->transferred)
            cmp = 0;
        else if (track1->transferred == TRUE)
            cmp = 1;
        else
            cmp = -1;
        break;
    case TM_COLUMN_COMPILATION:
        if (track1->compilation == track2->compilation)
            cmp = 0;
        else if (track1->compilation == TRUE)
            cmp = 1;
        else
            cmp = -1;
        break;
    case TM_COLUMN_SIZE:
        cmp = track1->size - track2->size;
        break;
    case TM_COLUMN_TRACKLEN:
        cmp = track1->tracklen - track2->tracklen;
        break;
    case TM_COLUMN_BITRATE:
        cmp = track1->bitrate - track2->bitrate;
        break;
    case TM_COLUMN_SAMPLERATE:
        cmp = track1->samplerate - track2->samplerate;
        break;
    case TM_COLUMN_BPM:
        cmp = track1->BPM - track2->BPM;
        break;
    case TM_COLUMN_PLAYCOUNT:
        cmp = track1->playcount - track2->playcount;
        break;
    case TM_COLUMN_RATING:
        cmp = track1->rating - track2->rating;
        break;
    case TM_COLUMN_TIME_ADDED:
    case TM_COLUMN_TIME_PLAYED:
    case TM_COLUMN_TIME_MODIFIED:
    case TM_COLUMN_TIME_RELEASED:
        cmp = COMP (time_get_time (track1, TM_to_T (tm_item)),
                time_get_time (track2, TM_to_T (tm_item)));
        break;
    case TM_COLUMN_VOLUME:
        cmp = track1->volume - track2->volume;
        break;
    case TM_COLUMN_SOUNDCHECK:
        /* If soundcheck is unset (0) use 0 dB (1000) */
        cmp = (track1->soundcheck ? track1->soundcheck : 1000) - (track2->soundcheck ? track2->soundcheck : 1000);
        break;
    case TM_COLUMN_YEAR:
        cmp = track1->year - track2->year;
        break;
    case TM_COLUMN_SEASON_NR:
        cmp = track1->season_nr - track2->season_nr;
        break;
    case TM_COLUMN_EPISODE_NR:
        cmp = track1->episode_nr - track2->episode_nr;
        break;
    case TM_COLUMN_MEDIA_TYPE:
        cmp = track1->mediatype - track2->mediatype;
        break;
    case TM_COLUMN_LYRICS:
        cmp = track1->lyrics_flag - track2->lyrics_flag;
        break;
    case TM_NUM_COLUMNS:
        break;
    }
    return cmp;
}

/* Function used to compare rows with user's search string */
gboolean tm_search_equal_func(GtkTreeModel *model, gint column, const gchar *key, GtkTreeIter *iter, gpointer search_data) {
    Track *track1;
    gboolean cmp = 0;
    gtk_tree_model_get(model, iter, READOUT_COL, &track1, -1);
    switch ((TM_item) column) {
    case TM_COLUMN_TITLE:
    case TM_COLUMN_ALBUM:
    case TM_COLUMN_GENRE:
    case TM_COLUMN_COMPOSER:
    case TM_COLUMN_COMMENT:
    case TM_COLUMN_FILETYPE:
    case TM_COLUMN_GROUPING:
    case TM_COLUMN_ARTIST:
    case TM_COLUMN_CATEGORY:
    case TM_COLUMN_DESCRIPTION:
    case TM_COLUMN_PODCASTURL:
    case TM_COLUMN_PODCASTRSS:
    case TM_COLUMN_SUBTITLE:
    case TM_COLUMN_PC_PATH:
    case TM_COLUMN_YEAR:
    case TM_COLUMN_IPOD_PATH:
    case TM_COLUMN_COMPILATION:
    case TM_COLUMN_THUMB_PATH:
    case TM_COLUMN_TV_SHOW:
    case TM_COLUMN_TV_EPISODE:
    case TM_COLUMN_TV_NETWORK:
    case TM_COLUMN_ALBUMARTIST:
    case TM_COLUMN_SORT_ARTIST:
    case TM_COLUMN_SORT_TITLE:
    case TM_COLUMN_SORT_ALBUM:
    case TM_COLUMN_SORT_ALBUMARTIST:
    case TM_COLUMN_SORT_COMPOSER:
    case TM_COLUMN_SORT_TVSHOW:
        cmp = (compare_string_start_case_insensitive(track_get_item(track1, TM_to_T(column)), key) != 0);
        break;
    case TM_COLUMN_TRACK_NR:
    case TM_COLUMN_IPOD_ID:
    case TM_COLUMN_TRANSFERRED:
    case TM_COLUMN_SIZE:
    case TM_COLUMN_TRACKLEN:
    case TM_COLUMN_BITRATE:
    case TM_COLUMN_PLAYCOUNT:
    case TM_COLUMN_RATING:
    case TM_COLUMN_TIME_PLAYED:
    case TM_COLUMN_TIME_MODIFIED:
    case TM_COLUMN_VOLUME:
    case TM_COLUMN_CD_NR:
    case TM_COLUMN_TIME_ADDED:
    case TM_COLUMN_SOUNDCHECK:
    case TM_COLUMN_SAMPLERATE:
    case TM_COLUMN_BPM:
    case TM_COLUMN_TIME_RELEASED:
    case TM_COLUMN_MEDIA_TYPE:
    case TM_COLUMN_SEASON_NR:
    case TM_COLUMN_EPISODE_NR:
    case TM_COLUMN_LYRICS:
    case TM_NUM_COLUMNS:
        break;
    }
    return cmp;
}
;

/* Function used to compare two cells during sorting (track view) */
gint tm_data_compare_func(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data) {
    Track *track1;
    Track *track2;
    gint column;
    GtkSortType order;
    gint result;

    if (gtk_tree_sortable_get_sort_column_id(GTK_TREE_SORTABLE (model), &column, &order) == FALSE)
        return 0;

    gtk_tree_model_get(model, a, READOUT_COL, &track1, -1);
    gtk_tree_model_get(model, b, READOUT_COL, &track2, -1);
    g_return_val_if_fail (track1 && track2, 0);

    result = tm_data_compare(track1, track2, column);
    /* implement stable sorting: if two items are the same, revert to
     the last relative positition */
    if (result == 0) {
        ExtraTrackData *etr1 = track1->userdata;
        ExtraTrackData *etr2 = track2->userdata;
        g_return_val_if_fail (etr1 && etr2, 0);
        result = etr1->sortindex - etr2->sortindex;
    }
    return result;
}

/* set/read the counter used to remember how often the sort column has
 been clicked.
 @inc: negative: reset counter to 0
 @inc: positive or zero : add to counter
 return value: new value of the counter */
gint tm_sort_counter(gint inc) {
    static gint cnt = 0;

    if (inc < 0) {
        cnt = 0;
    }
    else {
        cnt += inc;
    }
    return cnt;
}

/* redisplay the contents of the track view in it's unsorted order */
static void tm_unsort(void) {
    if (track_treeview) {
        GtkTreeModel *model = gtk_tree_view_get_model(track_treeview);

        if (GTK_IS_TREE_MODEL_FILTER (model)) {
            model = gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(model));
        }

        prefs_set_int("tm_sort", SORT_NONE);
        if (!BROKEN_GTK_TREE_SORT) {
            /* no need to comment this out -- searching still works, but for lack
             of a ctrl-g only the first occurence will be found */
            /*	    gtk_tree_view_set_enable_search (GTK_TREE_VIEW
             * (track_treeview), FALSE);*/
            gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE (model), GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, GTK_SORT_ASCENDING);
            tm_adopt_order();
        }
        else {
            gtkpod_warning(_("Cannot unsort track view because of a bug in the GTK lib you are using (%d.%d.%d < 2.5.4). Once you sort the track view, you cannot go back to the unsorted state.\n\n"), gtk_major_version, gtk_minor_version, gtk_micro_version);
        }
        tm_sort_counter(-1);
    }
}

static void tm_set_search_column(TM_item newcol) {
    /*     printf ("track_treeview: %p, col: %d\n", track_treeview, newcol); */
    g_return_if_fail (track_treeview);

    gtk_tree_view_set_search_column(GTK_TREE_VIEW (track_treeview), newcol);
    switch (newcol) {
    case TM_COLUMN_TITLE:
    case TM_COLUMN_ALBUM:
    case TM_COLUMN_GENRE:
    case TM_COLUMN_COMPOSER:
    case TM_COLUMN_COMMENT:
    case TM_COLUMN_FILETYPE:
    case TM_COLUMN_GROUPING:
    case TM_COLUMN_ARTIST:
    case TM_COLUMN_CATEGORY:
    case TM_COLUMN_DESCRIPTION:
    case TM_COLUMN_PODCASTURL:
    case TM_COLUMN_PODCASTRSS:
    case TM_COLUMN_SUBTITLE:
    case TM_COLUMN_PC_PATH:
    case TM_COLUMN_YEAR:
    case TM_COLUMN_IPOD_PATH:
    case TM_COLUMN_COMPILATION:
    case TM_COLUMN_THUMB_PATH:
    case TM_COLUMN_TV_SHOW:
    case TM_COLUMN_TV_EPISODE:
    case TM_COLUMN_TV_NETWORK:
    case TM_COLUMN_ALBUMARTIST:
    case TM_COLUMN_SORT_ARTIST:
    case TM_COLUMN_SORT_TITLE:
    case TM_COLUMN_SORT_ALBUM:
    case TM_COLUMN_SORT_ALBUMARTIST:
    case TM_COLUMN_SORT_COMPOSER:
    case TM_COLUMN_SORT_TVSHOW:
        gtk_tree_view_set_enable_search(GTK_TREE_VIEW (track_treeview), TRUE);
        break;
    case TM_COLUMN_TRACK_NR:
    case TM_COLUMN_IPOD_ID:
    case TM_COLUMN_TRANSFERRED:
    case TM_COLUMN_SIZE:
    case TM_COLUMN_TRACKLEN:
    case TM_COLUMN_BITRATE:
    case TM_COLUMN_PLAYCOUNT:
    case TM_COLUMN_RATING:
    case TM_COLUMN_TIME_PLAYED:
    case TM_COLUMN_TIME_MODIFIED:
    case TM_COLUMN_VOLUME:
    case TM_COLUMN_CD_NR:
    case TM_COLUMN_TIME_ADDED:
    case TM_COLUMN_SOUNDCHECK:
    case TM_COLUMN_SAMPLERATE:
    case TM_COLUMN_BPM:
    case TM_COLUMN_TIME_RELEASED:
    case TM_COLUMN_MEDIA_TYPE:
    case TM_COLUMN_SEASON_NR:
    case TM_COLUMN_EPISODE_NR:
    case TM_COLUMN_LYRICS:
    case TM_NUM_COLUMNS:
        gtk_tree_view_set_enable_search(GTK_TREE_VIEW (track_treeview), FALSE);
        break;
    }
    prefs_set_int(TM_PREFS_SEARCH_COLUMN, newcol);
}

/* This is called before when changing the sort order or the sort
 column, and before doing the sorting */
static void tm_sort_column_changed(GtkTreeSortable *ts, gpointer user_data) {
    static gint lastcol = -1; /* which column was sorted last time? */
    gchar *buf;
    gint newcol;
    GtkSortType order;
    GList *tracks, *gl;
    gint32 i, inc;

    gtk_tree_sortable_get_sort_column_id(ts, &newcol, &order);

    /* set compare function for strings (to speed up sorting) */
    buf = g_strdup_printf("sort_ign_field_%d", TM_to_T(newcol));
    if (prefs_get_int(buf))
        string_compare_func = compare_string_fuzzy;
    else
        string_compare_func = compare_string;
    g_free(buf);

    /* don't do anything if no sort column is set */
    if (newcol == -2) {
        lastcol = newcol;
        return;
    }

    if (newcol != lastcol) {
        tm_sort_counter(-1);
        lastcol = newcol;
    }

    if (tm_sort_counter(1) >= 3) { /* after clicking three times, reset sort order! */
        tm_unsort(); /* also resets sort counter */
    }
    else {
        prefs_set_int("tm_sort", order);
    }
    prefs_set_int("tm_sortcol", newcol);

    tm_set_search_column(newcol);

    if (prefs_get_int("tm_autostore"))
        tm_rows_reordered();

    /* stable sorting: index original order */
    tracks = tm_get_all_tracks();
    /* make numbering ascending or decending depending on sort order
     (then we don't have to worry about the sort order when doing
     the comparison in tm_data_compare_func() */
    if (order == GTK_SORT_ASCENDING) {
        i = 0;
        inc = 1;
    }
    else {
        i = -1;
        inc = -1;
    }
    for (gl = tracks; gl; gl = gl->next) {
        ExtraTrackData *etr;
        Track *tr = gl->data;
        g_return_if_fail (tr);
        etr = tr->userdata;
        g_return_if_fail (etr);
        etr->sortindex = i;
        i += inc;
    }
    g_list_free(tracks);
}

void tm_sort(TM_item col, GtkSortType order) {
    if (track_treeview) {
        GtkTreeModel *model = gtk_tree_view_get_model(track_treeview);

        if (GTK_IS_TREE_MODEL_FILTER (model)) {
            model = gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(model));
        }

        if (order != SORT_NONE) {
            gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE (model), col, order);
        }
        else { /* only unsort if treeview is sorted */
            gint column;
            GtkSortType order;
            if (gtk_tree_sortable_get_sort_column_id(GTK_TREE_SORTABLE (model), &column, &order)) {
                /* column == -2 actually is not defined, but it means
                 that the model is unsorted. The sortable interface
                 is badly implemented in gtk 2.4 */
                if (column != -2)
                    tm_unsort();
            }
        }
    }
}

/* Adds the columns to our track_treeview */
static GtkTreeViewColumn *tm_add_column(TM_item tm_item, gint pos) {
    GtkTreeModel *model = gtk_tree_view_get_model(track_treeview);
    GtkTreeViewColumn *col = NULL;
    const gchar *text;
    GtkCellRenderer *renderer = NULL; /* default */
    GtkTreeCellDataFunc cell_data_func = tm_cell_data_text_func;

    g_return_val_if_fail (gtkpod_app, NULL);

    g_return_val_if_fail (tm_item >= 0, NULL);
    g_return_val_if_fail (tm_item < TM_NUM_COLUMNS, NULL);

    text = gettext (get_tm_string (tm_item));

    g_return_val_if_fail (text, NULL);

    col = gtk_tree_view_column_new();
    gtk_tree_view_column_set_resizable(col, TRUE);

    switch (tm_item) {
    case TM_COLUMN_TITLE:
        /* Add additional toggle box for 'checked' property */
        renderer = gtk_cell_renderer_toggle_new();
        g_object_set_data(G_OBJECT (renderer), "column", (gint *) tm_item);
        g_signal_connect (G_OBJECT (renderer), "toggled",
                G_CALLBACK (tm_cell_toggled), model);
        gtk_tree_view_column_pack_start(col, renderer, FALSE);
        gtk_tree_view_column_set_cell_data_func(col, renderer, tm_cell_data_func_toggle, NULL, NULL);
        renderer = NULL;
        break;
    case TM_COLUMN_ARTIST:
    case TM_COLUMN_ALBUM:
    case TM_COLUMN_GENRE:
    case TM_COLUMN_COMPOSER:
    case TM_COLUMN_COMMENT:
    case TM_COLUMN_FILETYPE:
    case TM_COLUMN_GROUPING:
    case TM_COLUMN_BITRATE:
    case TM_COLUMN_SAMPLERATE:
    case TM_COLUMN_BPM:
    case TM_COLUMN_CATEGORY:
    case TM_COLUMN_DESCRIPTION:
    case TM_COLUMN_PODCASTURL:
    case TM_COLUMN_PODCASTRSS:
    case TM_COLUMN_SUBTITLE:
    case TM_COLUMN_PC_PATH:
    case TM_COLUMN_IPOD_PATH:
    case TM_COLUMN_THUMB_PATH:
    case TM_COLUMN_SIZE:
    case TM_COLUMN_MEDIA_TYPE:
    case TM_COLUMN_TV_SHOW:
    case TM_COLUMN_TV_EPISODE:
    case TM_COLUMN_TV_NETWORK:
    case TM_COLUMN_SEASON_NR:
    case TM_COLUMN_EPISODE_NR:
    case TM_COLUMN_ALBUMARTIST:
    case TM_COLUMN_SORT_ARTIST:
    case TM_COLUMN_SORT_TITLE:
    case TM_COLUMN_SORT_ALBUM:
    case TM_COLUMN_SORT_ALBUMARTIST:
    case TM_COLUMN_SORT_COMPOSER:
    case TM_COLUMN_SORT_TVSHOW:
        break;
        /* for some column names we want to use shorter alternatives to
         get_tm_string() */
    case TM_COLUMN_RATING:
        text = _("Rating");
        break;
    case TM_COLUMN_TRACK_NR:
        text = _("#");
        break;
    case TM_COLUMN_CD_NR:
        text = _("CD");
        break;
    case TM_COLUMN_IPOD_ID:
        text = _("ID");
        break;
    case TM_COLUMN_TRANSFERRED:
        text = _("Transferred");
        renderer = gtk_cell_renderer_toggle_new();
        cell_data_func = tm_cell_data_toggle_func;
        break;
    case TM_COLUMN_LYRICS:
        renderer = gtk_cell_renderer_toggle_new();
        cell_data_func = tm_cell_data_toggle_func;
        g_signal_connect (G_OBJECT (renderer), "toggled",
                G_CALLBACK (tm_cell_toggled), model);
        break;
    case TM_COLUMN_COMPILATION:
        text = _("Cmpl");
        renderer = gtk_cell_renderer_toggle_new();
        cell_data_func = tm_cell_data_toggle_func;
        g_signal_connect (G_OBJECT (renderer), "toggled",
                G_CALLBACK (tm_cell_toggled), model);
        break;
    case TM_COLUMN_TRACKLEN:
        text = _("Time");
        break;
    case TM_COLUMN_PLAYCOUNT:
        text = _("Plycnt");
        break;
    case TM_COLUMN_TIME_PLAYED:
        text = _("Played");
        break;
    case TM_COLUMN_TIME_MODIFIED:
        text = _("Modified");
        break;
    case TM_COLUMN_TIME_ADDED:
        text = _("Added");
        break;
    case TM_COLUMN_TIME_RELEASED:
        text = _("Released");
        break;
    case TM_COLUMN_YEAR:
        text = _("Year");
        break;
    case TM_COLUMN_VOLUME:
        text = _("Vol.");
        break;
    case TM_COLUMN_SOUNDCHECK:
        text = _("Sndchk.");
        break;
    case TM_NUM_COLUMNS:
        g_return_val_if_reached (NULL);
        break;
    }

    if (!renderer) {
        if (tm_item == TM_COLUMN_RATING) {
            renderer = rb_cell_renderer_rating_new();
            cell_data_func = tm_cell_data_rating_func;
            g_signal_connect (G_OBJECT (renderer), "rated",
                    G_CALLBACK (tm_rating_edited), NULL);
        }
        else {
            /* text renderer */
            renderer = gtk_cell_renderer_text_new();
        }
    }

    g_object_set_data(G_OBJECT (renderer), "column", (gint *) tm_item);

    gtk_tree_view_column_set_title(col, text);
    gtk_tree_view_column_pack_start(col, renderer, FALSE);
    gtk_tree_view_column_set_cell_data_func(col, renderer, cell_data_func, NULL, NULL);
    gtk_tree_view_column_set_sort_column_id(col, tm_item);
    gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE (model), tm_item, tm_data_compare_func, NULL, NULL);
    gtk_tree_view_column_set_reorderable(col, TRUE);

    gtk_tree_view_insert_column(track_treeview, col, pos);
    tm_columns[tm_item] = col;

    if (pos != -1) {
        gtk_tree_view_column_set_visible(col, prefs_get_int_index("col_visible", tm_item));
    }

    if (get_tm_tooltip(tm_item)) {
        GtkWidget *label = GTK_WIDGET (gtk_label_new(text));
        gtk_widget_show(label);
        gtk_widget_set_tooltip_text(label, gettext (get_tm_tooltip(tm_item)));
        gtk_tree_view_column_set_widget(col, label);
    }

    return col;
}

/* Adds the columns to our track_treeview */
static void tm_add_columns(void) {
    gint i;

    for (i = 0; i < TM_NUM_COLUMNS; ++i) {
        tm_add_column(prefs_get_int_index("col_order", i), -1);
    }
    tm_show_preferred_columns();
}

static void tm_select_current_position(gint x, gint y) {
    if (track_treeview) {
        GtkTreePath *path;

        gtk_tree_view_get_path_at_pos(track_treeview, x, y, &path, NULL, NULL, NULL);
        if (path) {
            GtkTreeSelection *ts = gtk_tree_view_get_selection(track_treeview);
            gtk_tree_selection_select_path(ts, path);
            gtk_tree_path_free(path);
        }
    }
}

static gboolean tm_button_press_event(GtkWidget *w, GdkEventButton *e, gpointer data) {
    if (w && e) {
        switch (e->button) {
        case 1:
            /*
             printf ("Pressed in cell %d (%f/%f)\n\n",
             tree_view_get_cell_from_pos(GTK_TREE_VIEW(w),
             (guint)e->x, (guint)e->y, NULL),
             e->x, e->y);
             */
            break;
        case 3:
            tm_select_current_position(e->x, e->y);
            tm_context_menu_init();
            return TRUE;
        default:
            break;
        }
    }
    return (FALSE);
}

static void tm_row_activated_event(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data) {
    GList *tracks = tm_get_selected_tracks();
    gtkpod_execute_track_command(tracks);
}

static gboolean tm_selection_changed_cb(gpointer data) {
    GtkTreeView *treeview = GTK_TREE_VIEW (data);
    GtkTreePath *path;
    GtkTreeViewColumn *column;
    TM_item col_id;

    gtk_tree_view_get_cursor(treeview, &path, &column);
    if (path) {
        col_id = tm_lookup_col_id(column);
        if (col_id != -1)
            tm_set_search_column(col_id);
    }

    gtkpod_set_selected_tracks(tm_get_selected_tracks());

    return FALSE;
}

/* called when the track selection changes */
static void tm_selection_changed(GtkTreeSelection *selection, gpointer data) {
    g_idle_add(tm_selection_changed_cb, gtk_tree_selection_get_tree_view(selection));
}

/* Create tracks treeview */
static void tm_create_treeview(void) {
    GtkTreeModel *model = NULL;
    GtkWidget *track_window;
    GtkWidget *track_filter_entry;
    GtkTreeSelection *select;
    gint col;
    GtkWidget *stv = gtk_tree_view_new();

    track_window = gtkpod_builder_xml_get_widget(get_track_builder(), "track_window");
    g_return_if_fail (track_window);

    /* create tree view */
    if (track_treeview) { /* delete old tree view */
        model = gtk_tree_view_get_model(track_treeview);
        g_object_unref(model);
        gtk_widget_destroy(GTK_WIDGET (track_treeview));
    }
    track_treeview = GTK_TREE_VIEW (stv);
    gtk_widget_show(stv);
    gtk_container_add(GTK_CONTAINER (track_window), stv);
    /* create model (we only need one column for the model -- only a
     * pointer to the track has to be stored) */
    model = GTK_TREE_MODEL (
            gtk_list_store_new (1, G_TYPE_POINTER));
    gtk_tree_view_set_model(track_treeview, GTK_TREE_MODEL (model));
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW (track_treeview), TRUE);
    gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW (track_treeview), TRUE);
    gtk_tree_view_set_search_equal_func(GTK_TREE_VIEW (track_treeview), tm_search_equal_func, NULL, NULL);
    select = gtk_tree_view_get_selection(track_treeview);
    gtk_tree_selection_set_mode(select, GTK_SELECTION_MULTIPLE);
    g_signal_connect (G_OBJECT (select) , "changed",
            G_CALLBACK (tm_selection_changed),
            NULL);

    tm_add_columns();

    gtk_drag_source_set(GTK_WIDGET (track_treeview), GDK_BUTTON1_MASK, tm_drag_types, TGNR (tm_drag_types), GDK_ACTION_COPY
            | GDK_ACTION_MOVE);
    gtk_drag_dest_set(GTK_WIDGET (track_treeview), 0, tm_drop_types, TGNR (tm_drop_types), GDK_ACTION_COPY
            | GDK_ACTION_MOVE);

    g_signal_connect ((gpointer) track_treeview, "drag-data-delete",
            G_CALLBACK (tm_drag_data_delete),
            NULL);

    g_signal_connect ((gpointer) track_treeview, "drag-data-get",
            G_CALLBACK (tm_drag_data_get),
            NULL);

    g_signal_connect ((gpointer) track_treeview, "drag-data-received",
            G_CALLBACK (tm_drag_data_received),
            NULL);

    g_signal_connect ((gpointer) track_treeview, "drag-drop",
            G_CALLBACK (tm_drag_drop),
            NULL);

    g_signal_connect ((gpointer) track_treeview, "drag-end",
            G_CALLBACK (tm_drag_end),
            NULL);

    g_signal_connect ((gpointer) track_treeview, "drag-leave",
            G_CALLBACK (tm_drag_leave),
            NULL);

    g_signal_connect ((gpointer) track_treeview, "drag-motion",
            G_CALLBACK (tm_drag_motion),
            NULL);

    g_signal_connect_after ((gpointer) stv, "key_release_event",
            G_CALLBACK (on_track_treeview_key_release_event),
            NULL);
    g_signal_connect ((gpointer) track_treeview, "button-press-event",
            G_CALLBACK (tm_button_press_event),
            NULL);
    g_signal_connect ((gpointer) track_treeview, "row-activated",
                G_CALLBACK (tm_row_activated_event),
                NULL);
    g_signal_connect (G_OBJECT (model), "sort-column-changed",
            G_CALLBACK (tm_sort_column_changed),
            (gpointer)0);

    /* set correct column for typeahead */
    if (prefs_get_int_value(TM_PREFS_SEARCH_COLUMN, &col)) {
        tm_set_search_column(col);
    }
    else { /* reasonable default */
        tm_set_search_column(TM_COLUMN_TITLE);
    }

    track_filter_entry = gtkpod_builder_xml_get_widget(get_track_builder(), "search_entry");
    g_return_if_fail (track_filter_entry);

    g_signal_connect (G_OBJECT (track_filter_entry), "changed",
                G_CALLBACK (on_search_entry_changed),
                (gpointer)0);
}

void tm_create_track_display(GtkWidget *parent) {
    GtkWidget *track_display_window = gtkpod_builder_xml_get_widget(get_track_builder(), "track_display_window");
    track_container = gtkpod_builder_xml_get_widget(get_track_builder(), "track_display_vbox");
    search_entry = gtkpod_builder_xml_get_widget(get_track_builder(), "search_entry");
    current_playlist_label = gtkpod_builder_xml_get_widget(get_track_builder(), "current_playlist_label");
    tm_create_treeview();

    g_object_ref(track_container);
    gtk_container_remove(GTK_CONTAINER(track_display_window), GTK_WIDGET(track_container));
    if (GTK_IS_SCROLLED_WINDOW(parent)) {
        gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(parent), GTK_WIDGET(track_container));
    }
    else {
        gtk_container_add(GTK_CONTAINER (parent), GTK_WIDGET (track_container));
    }

    g_object_unref(track_container);
    gtk_widget_destroy(track_display_window);
}

/* Free the widgets */
void tm_destroy_widgets(void) {
    if (GTK_IS_WIDGET(track_container))
        gtk_widget_destroy(track_container);

    track_treeview = NULL;
    search_entry = NULL;
    current_playlist_label = NULL;
    track_window = NULL;
}

void tm_show_preferred_columns(void) {
    gint i;
    gboolean horizontal_scrollbar = prefs_get_int("horizontal_scrollbar");

    for (i = 0; i < TM_NUM_COLUMNS; ++i) {
        TM_item tm_item = prefs_get_int_index("col_order", i);
        GtkTreeViewColumn *tvc = gtk_tree_view_get_column(track_treeview, i);
        gboolean visible = prefs_get_int_index("col_visible", tm_item);
        gint col_width;

        gtk_tree_view_column_set_visible(tvc, visible);

        col_width = prefs_get_int_index("tm_col_width", tm_item);

        if (col_width == 0)
            col_width = 80;

        if (horizontal_scrollbar) {
            switch(tm_item) {
            case TM_COLUMN_RATING:
                gtk_tree_view_column_set_fixed_width(tvc, 85);
                gtk_tree_view_column_set_min_width(tvc, 85);
                break;
            default:
                gtk_tree_view_column_set_fixed_width(tvc, col_width);
                gtk_tree_view_column_set_min_width(tvc, -1);
            }
            gtk_tree_view_column_set_expand(tvc, FALSE);
        }
        else {
            switch (tm_item) {
            case TM_COLUMN_TITLE:
            case TM_COLUMN_ARTIST:
            case TM_COLUMN_ALBUM:
            case TM_COLUMN_GENRE:
            case TM_COLUMN_COMPOSER:
            case TM_COLUMN_COMMENT:
            case TM_COLUMN_CATEGORY:
            case TM_COLUMN_DESCRIPTION:
            case TM_COLUMN_PODCASTURL:
            case TM_COLUMN_PODCASTRSS:
            case TM_COLUMN_SUBTITLE:
            case TM_COLUMN_PC_PATH:
            case TM_COLUMN_IPOD_PATH:
            case TM_COLUMN_THUMB_PATH:
            case TM_COLUMN_TV_SHOW:
            case TM_COLUMN_TV_EPISODE:
            case TM_COLUMN_TV_NETWORK:
            case TM_COLUMN_ALBUMARTIST:
                gtk_tree_view_column_set_min_width(tvc, 0);
                gtk_tree_view_column_set_expand(tvc, TRUE);
                break;
            default:
                gtk_tree_view_column_set_min_width(tvc, 80);
                gtk_tree_view_column_set_fixed_width(tvc, col_width);
                gtk_tree_view_column_set_expand(tvc, FALSE);
                break;
            }
        }
    }
}

/* update the cfg structure (preferences) with the current sizes /
 positions (called by display_update_default_sizes():
 column widths of track model */
void tm_update_default_sizes(void) {
    TM_item tm_item;
    GtkTreeViewColumn *col;
    gint col_width;

    /* column widths */
    for (tm_item = 0; tm_item < TM_NUM_COLUMNS; ++tm_item) {
        col = tm_columns[tm_item];
        if (col) {
            col_width = gtk_tree_view_column_get_width(col);

            if (col_width > 0) { /* columns not displayed seem to be 0 pixels wide --
             only change the visible columns */
                prefs_set_int_index("tm_col_width", tm_item, col_width);
            }
        }
    }
}

/* get the TM_ITEM column id for @column. Returns -1 if column could
 not be found */
static TM_item tm_lookup_col_id(GtkTreeViewColumn *column) {
    gint i;

    if (column) {
        for (i = 0; i < TM_NUM_COLUMNS; ++i) {
            if (column == tm_columns[i])
                return i;
        }
    }
    return -1;
}

/* Compare function to avoid sorting */
static gint tm_nosort_comp(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data) {
    return 0;
}

/* Disable sorting of the view during lengthy updates. */
/* @enable: TRUE: enable, FALSE: disable */
void tm_enable_disable_view_sort(gboolean enable) {
    static gint disable_count = 0;

    if (enable) {
        disable_count--;
        if (disable_count < 0)
            fprintf(stderr, "Programming error: disable_count < 0\n");
        if (disable_count == 0 && track_treeview) {
            if (prefs_get_int("tm_sort") != SORT_NONE) {
                /* Re-enable sorting */
                GtkTreeModel *model = gtk_tree_view_get_model(track_treeview);

                if (GTK_IS_TREE_MODEL_FILTER (model)) {
                    model = gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(model));
                }

                if (BROKEN_GTK_TREE_SORT) {
                    gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE (model), prefs_get_int("tm_sortcol"), tm_data_compare_func, NULL, NULL);
                }
                else {
                    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE (model), prefs_get_int("tm_sortcol"), prefs_get_int("tm_sort"));
                }
            }
        }
    }
    else {
        if (disable_count == 0 && track_treeview) {
            if (prefs_get_int("tm_sort") != SORT_NONE) {
                /* Disable sorting */
                GtkTreeModel *model = gtk_tree_view_get_model(track_treeview);

                if (GTK_IS_TREE_MODEL_FILTER (model)) {
                    model = gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(model));
                }

                if (BROKEN_GTK_TREE_SORT) {
                    gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE (model), prefs_get_int("tm_sortcol"), tm_nosort_comp, NULL, NULL);
                }
                else {
                    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE (model), GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, prefs_get_int("tm_sort"));
                }
            }
        }
        disable_count++;
    }
}

/* Callback for adding tracks within tm_add_filelist */
void tm_addtrackfunc(Playlist *plitem, Track *track, gpointer data) {
    struct asf_data *asf = (struct asf_data *) data;
    GtkTreeModel *model;
    GtkTreeIter new_iter;

    model = gtk_tree_view_get_model(track_treeview);

    /*    printf("plitem: %p\n", plitem);
     if (plitem) printf("plitem->type: %d\n", plitem->type);*/
    /* add to playlist but not to the display */
    gp_playlist_add_track(plitem, track, FALSE);

    /* create new iter in track view */
    switch (asf->pos) {
    case GTK_TREE_VIEW_DROP_INTO_OR_BEFORE:
    case GTK_TREE_VIEW_DROP_INTO_OR_AFTER:
    case GTK_TREE_VIEW_DROP_AFTER:
        gtk_list_store_insert_after(get_model_as_store(model), &new_iter, asf->to_iter);
        break;
    case GTK_TREE_VIEW_DROP_BEFORE:
        gtk_list_store_insert_before(get_model_as_store(model), &new_iter, asf->to_iter);
        break;
    }
    /* set the iter */
    tm_add_track_to_track_model(track, &new_iter);
}

/* DND: insert a list of files before/after @path
 @data: list of files
 @path: where to drop (NULL to drop at the end)
 @pos:  before/after... (ignored if @path is NULL)
 */
gboolean tm_add_filelist(gchar *data, GtkTreePath *path, GtkTreeViewDropPosition pos) {
    GtkTreeModel *model;
    gchar *buf = NULL, *use_data;
    gchar **files, **filep;
    Playlist *current_playlist = gtkpod_get_current_playlist();

    g_return_val_if_fail (data, FALSE);
    g_return_val_if_fail (*data, FALSE);
    g_return_val_if_fail (current_playlist, FALSE);

    model = gtk_tree_view_get_model(track_treeview);
    g_return_val_if_fail (model, FALSE);
    if (path) {
    }

    if (pos != GTK_TREE_VIEW_DROP_BEFORE) { /* need to reverse the list of files -- otherwise wie add them
     * in reverse order */
        /* split the path list into individual strings */
        gint len = strlen(data) + 1;
        files = g_strsplit(data, "\n", -1);
        filep = files;
        /* find the end of the list */
        while (*filep)
            ++filep;
        /* reserve memory */
        buf = g_malloc0(len);
        /* reverse the list */
        while (filep != files) {
            --filep;
            g_strlcat(buf, *filep, len);
            g_strlcat(buf, "\n", len);
        }
        g_strfreev(files);
        use_data = buf;
    }
    else {
        use_data = data;
    }

    /*     printf("filelist: (%s) -> (%s)\n", data, use_data); */
    /* initialize add-track-struct */
    if (path) { /* add where specified (@path/@pos) */
        GtkTreeIter to_iter;
        GtkTreeIter temp;

        struct asf_data asf;
        if (!gtk_tree_model_get_iter(model, &to_iter, path))
            g_return_val_if_reached (FALSE);

        convert_iter(model, &to_iter, &temp);
        asf.to_iter = &temp;
        asf.pos = pos;
        add_text_plain_to_playlist(current_playlist->itdb, current_playlist, use_data, 0, tm_addtrackfunc, &asf);
    }
    else { /* add to the end */
        add_text_plain_to_playlist(current_playlist->itdb, current_playlist, use_data, 0, NULL, NULL);
    }

    /*    update_model_view (model); -- not needed */
    tm_rows_reordered();
    C_FREE (buf);
    return TRUE;
}

void track_display_set_tracks_cb(GtkPodApp *app, gpointer tks, gpointer data) {
    GList *tracks = tks;

    tm_remove_all_tracks();

    while (tracks != NULL) { /* add all tracks to model */
        Track *track = tracks->data;
        tm_add_track_to_track_model(track, NULL);
        tracks = tracks->next;
    }

}

void track_display_set_playlist_cb(GtkPodApp *app, gpointer pl, gpointer data) {
    Playlist *playlist = pl;
    gchar *label_text;

    if (!current_playlist_label)
        return;

    if (playlist) {
        label_text = g_markup_printf_escaped("<span weight='bold' size='larger'>%s</span>", playlist->name);
    }
    else {
        label_text = g_markup_printf_escaped("<span weight='bold' size='larger'>%s</span>", "No playlist selected");
    }

    gtk_label_set_markup(GTK_LABEL (current_playlist_label), label_text);
    g_free(label_text);
}

void track_display_set_sort_enablement(GtkPodApp *app, gboolean flag, gpointer data) {
    tm_enable_disable_view_sort(flag);
}

void track_display_track_removed_cb(GtkPodApp *app, gpointer tk, gint32 pos, gpointer data) {
    Track *old_track = tk;

    tm_remove_track(old_track);
}

void track_display_track_updated_cb(GtkPodApp *app, gpointer tk, gpointer data) {
    Track *track = tk;

    tm_track_changed(track);
}

void track_display_preference_changed_cb(GtkPodApp *app, gpointer pfname, gpointer value, gpointer data) {
    gchar *pref_name = pfname;
    if (g_str_equal(pref_name, "tm_sort")) {
        tm_sort_counter(-1);
        tm_sort(prefs_get_int("tm_sortcol"), prefs_get_int("tm_sort"));
    }
}

