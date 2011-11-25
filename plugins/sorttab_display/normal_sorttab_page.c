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
#ifndef NORMAL_SORT_TAB_PAGE_C_
#define NORMAL_SORT_TAB_PAGE_C_

#include <string.h>
#include <stdlib.h>
#include "libgtkpod/gp_itdb.h"
#include "libgtkpod/prefs.h"
#include "libgtkpod/misc.h"
#include "libgtkpod/misc_track.h"
#include "libgtkpod/gp_private.h"
#include "plugin.h"
#include "sorttab_conversion.h"
#include "sorttab_widget.h"
#include "normal_sorttab_page.h"
#include "sorttab_display_context_menu.h"

G_DEFINE_TYPE( NormalSortTabPage, normal_sort_tab_page, GTK_TYPE_TREE_VIEW);

#define NORMAL_SORT_TAB_PAGE_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NORMAL_SORT_TAB_TYPE_PAGE, NormalSortTabPagePrivate))

struct _NormalSortTabPagePrivate {

    /* Parent Sort Tab Widget */
    SortTabWidget *st_widget_parent;

    /* list with entries */
    GList *entries;

    /* pointer to currently selected TabEntries */
    GList *selected_entries;

    /* Handler id of the selection changed callback */
    gulong selection_changed_id;

    /* name of entry last selected */
    GList *last_selection;

    /* table for quick find of tab entries */
    GHashTable *entry_hash;

    /* unselected item since last st_init? */
    gboolean unselected;

    /* function used for string comparisons, set in on_st_switch_page   */
    gint (*entry_compare_func)(const TabEntry *a, const TabEntry *b);
};

/* Drag and drop definitions */
static GtkTargetEntry st_drag_types[] =
    {
        { DND_GTKPOD_TRACKLIST_TYPE, 0, DND_GTKPOD_TRACKLIST },
        { "text/uri-list", 0, DND_TEXT_URI_LIST },
        { "text/plain", 0, DND_TEXT_PLAIN },
        { "STRING", 0, DND_TEXT_PLAIN } };

gint _compare_entry(const TabEntry *a, const TabEntry *b) {
    return strcmp(a->name_sortkey, b->name_sortkey);
}

gint _compare_entry_fuzzy(const TabEntry *a, const TabEntry *b) {
    const gchar *ka, *kb;
    ka = a->name_fuzzy_sortkey ? a->name_fuzzy_sortkey : a->name_sortkey;
    kb = b->name_fuzzy_sortkey ? b->name_fuzzy_sortkey : b->name_sortkey;
    return strcmp(ka, kb);
}

/*
 * utility function for appending ipod track for st treeview callback
 */
static void _on_st_dnd_get_track_foreach(GtkTreeModel *tm, GtkTreePath *tp, GtkTreeIter *i, gpointer data) {
    GList *gl;
    TabEntry *entry = NULL;
    GString *tracklist = (GString *) data;

    g_return_if_fail (tracklist);

    gtk_tree_model_get(tm, i, ST_COLUMN_ENTRY, &entry, -1);
    g_return_if_fail (entry);

    /* add all member tracks of entry to tracklist */
    for (gl = entry->members; gl; gl = gl->next) {
        Track *tr = gl->data;
        g_return_if_fail (tr);
        g_string_append_printf(tracklist, "%p\n", tr);
    }
}

/*
 * utility function for appending filenames for st treeview callback
 */
static void _on_st_dnd_get_file_foreach(GtkTreeModel *tm, GtkTreePath *tp, GtkTreeIter *iter, gpointer data) {
    GList *gl;
    TabEntry *entry = NULL;
    GString *filelist = data;

    g_return_if_fail (tm);
    g_return_if_fail (iter);
    g_return_if_fail (data);

    gtk_tree_model_get(tm, iter, ST_COLUMN_ENTRY, &entry, -1);
    g_return_if_fail (entry);

    /* add all member tracks of entry to tracklist */
    for (gl = entry->members; gl; gl = gl->next) {
        gchar *name;
        Track *tr = gl->data;

        g_return_if_fail (tr);
        name = get_file_name_from_source(tr, SOURCE_PREFER_LOCAL);
        if (name) {
            g_string_append_printf(filelist, "file:%s\n", name);
            g_free(name);
        }
    }
}

static void _st_drag_end(GtkWidget *widget, GdkDragContext *dc, gpointer user_data) {
    /*     puts ("st_drag_end"); */
    gtkpod_tracks_statusbar_update();
}

/*
 * utility function for appending uris for st treeview callback
 */
static void _on_st_dnd_get_uri_foreach(GtkTreeModel *tm, GtkTreePath *tp, GtkTreeIter *iter, gpointer data) {
    GList *gl;
    TabEntry *entry = NULL;
    GString *filelist = data;

    g_return_if_fail (tm);
    g_return_if_fail (iter);
    g_return_if_fail (data);

    gtk_tree_model_get(tm, iter, ST_COLUMN_ENTRY, &entry, -1);
    g_return_if_fail (entry);

    /* add all member tracks of entry to tracklist */
    for (gl = entry->members; gl; gl = gl->next) {
        gchar *name;
        Track *tr = gl->data;

        g_return_if_fail (tr);
        name = get_file_name_from_source(tr, SOURCE_PREFER_LOCAL);
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

static void _st_drag_data_get(GtkWidget *widget, GdkDragContext *context, GtkSelectionData *data, guint info, guint time, gpointer user_data) {
    GtkTreeSelection *ts = NULL;
    GString *reply = g_string_sized_new(2000);

    if (!data)
        return;

    ts = gtk_tree_view_get_selection(GTK_TREE_VIEW (widget));
    if (ts) {
        switch (info) {
        case DND_GTKPOD_TRACKLIST:
            gtk_tree_selection_selected_foreach(ts, _on_st_dnd_get_track_foreach, reply);
            break;
        case DND_TEXT_URI_LIST:
            gtk_tree_selection_selected_foreach(ts, _on_st_dnd_get_uri_foreach, reply);
            break;
        case DND_TEXT_PLAIN:
            gtk_tree_selection_selected_foreach(ts, _on_st_dnd_get_file_foreach, reply);
            break;
        default:
            g_warning ("Programming error: st_drag_data_get received unknown info type (%d)\n", info);
            break;
        }
    }
    gtk_selection_data_set(data, gtk_selection_data_get_target(data), 8, reply->str, reply->len);
    g_string_free(reply, TRUE);
}

/* Function used to compare two cells during sorting (sorttab view) */
gint _st_data_compare_func(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data) {
    TabEntry *entry1;
    TabEntry *entry2;
    GtkSortType order;
    gint corr, colid;

    g_return_val_if_fail(NORMAL_SORT_TAB_IS_PAGE(user_data), -1);

    NormalSortTabPage *self = NORMAL_SORT_TAB_PAGE(user_data);
    NormalSortTabPagePrivate *priv = NORMAL_SORT_TAB_PAGE_GET_PRIVATE(self);

    gtk_tree_model_get(model, a, ST_COLUMN_ENTRY, &entry1, -1);
    gtk_tree_model_get(model, b, ST_COLUMN_ENTRY, &entry2, -1);
    if (gtk_tree_sortable_get_sort_column_id(GTK_TREE_SORTABLE (model), &colid, &order) == FALSE)
        return 0;

    /* We make sure that the "all" entry always stays on top, closely followed
     by the compilation entry */
    if (order == GTK_SORT_ASCENDING)
        corr = +1;
    else
        corr = -1;
    if (entry1->master)
        return (-corr);
    if (entry2->master)
        return (corr);
    if (entry1->compilation)
        return (-corr);
    if (entry2->compilation)
        return (corr);

    /*
     * compare the two entries
     * string_compare_func is set to either compare_string_fuzzy
     * or compare_string in on_st_switch_page() which is called
     * once before the comparing begins.
     */
    return priv->entry_compare_func(entry1, entry2);
}

/**
 * The sort tab entries are stored in a separate list (sorttab->entries)
 * and only pointers to the corresponding TabEntry structure are placed
 * into the model. This function reads the data for the given cell from
 * the list and passes it to the renderer.
 */
static void _st_cell_data_func(GtkTreeViewColumn *tree_column, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer data) {
    TabEntry *entry;
    gint column;

    column = (gint) GPOINTER_TO_INT(g_object_get_data (G_OBJECT (renderer), "column"));
    gtk_tree_model_get(model, iter, ST_COLUMN_ENTRY, &entry, -1);

    switch (column) { /* We only have one column, so this code is overkill... */
    case ST_COLUMN_ENTRY:
        if (entry->master || entry->compilation) { /* mark the "All" entry */
            g_object_set(G_OBJECT (renderer), "text", entry->name, "editable", FALSE, "weight", PANGO_WEIGHT_BOLD, NULL);
        }
        else {
            g_object_set(G_OBJECT (renderer), "text", entry->name, "editable", TRUE, "weight", PANGO_WEIGHT_NORMAL, NULL);
        }
        break;
    }
}

static void _st_build_sortkeys(TabEntry *entry) {
    if (entry->name_sortkey)
        C_FREE (entry->name_sortkey);

    if (entry->name_fuzzy_sortkey)
        C_FREE (entry->name_fuzzy_sortkey);

    gint case_sensitive = prefs_get_int("st_case_sensitive");
    entry->name_sortkey = make_sortkey(entry->name, case_sensitive);
    if (entry->name != fuzzy_skip_prefix(entry->name)) {
        entry->name_fuzzy_sortkey = make_sortkey(fuzzy_skip_prefix(entry->name), case_sensitive);
    }
}

/**
 * Called when editable cell is being edited. Stores new data to
 * the entry list and changes all members.
 */
static void _st_cell_edited(GtkCellRendererText *renderer, const gchar *path_string, const gchar *new_text, gpointer data) {
    GtkTreePath *path;
    GtkTreeIter iter;
    TabEntry *entry;
    ST_item column;
    gint i, n;
    GList *members;

    g_return_if_fail(NORMAL_SORT_TAB_IS_PAGE(data));

    NormalSortTabPage *self = NORMAL_SORT_TAB_PAGE(data);
    NormalSortTabPagePrivate *priv = NORMAL_SORT_TAB_PAGE_GET_PRIVATE(self);

    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(self));

    path = gtk_tree_path_new_from_string(path_string);
    column = (ST_item) g_object_get_data(G_OBJECT (renderer), "column");
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_model_get(model, &iter, column, &entry, -1);

    /*printf("Inst %d: st_cell_edited: column: %d  :%lx\n", inst, column, entry);*/

    switch (column) {
    case ST_COLUMN_ENTRY:
        /* We only do something, if the name actually got changed */
        if (g_utf8_collate(entry->name, new_text) != 0) {
            iTunesDB *itdb = NULL;

            /* remove old hash entry if available */
            TabEntry *hash_entry = g_hash_table_lookup(priv->entry_hash, entry->name);
            if (hash_entry == entry)
                g_hash_table_remove(priv->entry_hash, entry->name);

            /* replace entry name */
            g_free(entry->name);

            SortTabWidget *parent = priv->st_widget_parent;
            guint category = sort_tab_widget_get_category(parent);
            if (category == ST_CAT_YEAR) {
                /* make sure the entry->name is identical to atoi(new_text) */
                entry->name = g_strdup_printf("%d", atoi(new_text));
                g_object_set(G_OBJECT (renderer), "text", entry->name, NULL);
            }
            else {
                entry->name = g_strdup(new_text);
            }
            _st_build_sortkeys(entry);

            /* re-insert into hash table if the same name doesn't
             already exist */
            if (g_hash_table_lookup(priv->entry_hash, entry->name) == NULL)
                g_hash_table_insert(priv->entry_hash, entry->name, entry);

            /*
             * Now we look up all the tracks and change the ID3 Tag as well
             * We make a copy of the current members list, as it may change
             * during the process
             */
            members = g_list_copy(entry->members);
            n = g_list_length(members);

            /* block user input if we write tags (might take a while) */
            if (prefs_get_int("id3_write"))
                block_widgets();

            for (i = 0; i < n; ++i) {
                ExtraTrackData *etr;
                Track *track = (Track *) g_list_nth_data(members, i);
                T_item t_item;

                g_return_if_fail (track);
                etr = track->userdata;
                g_return_if_fail (etr);
                g_return_if_fail (track->itdb);
                if (!itdb)
                    itdb = track->itdb;

                SortTabWidget *parent = priv->st_widget_parent;
                guint category = sort_tab_widget_get_category(parent);
                t_item = ST_to_T(category);

                if (t_item == T_YEAR) {
                    gint nr = atoi(new_text);
                    if (nr < 0)
                        nr = 0;
                    track->year = nr;
                    g_free(etr->year_str);
                    etr->year_str = g_strdup_printf("%d", nr);
                }
                else {
                    gchar **itemp_utf8 = track_get_item_pointer(track, t_item);
                    g_return_if_fail (itemp_utf8);
                    g_free(*itemp_utf8);
                    *itemp_utf8 = g_strdup(new_text);
                }
                track->time_modified = time(NULL);
                gtkpod_track_updated(track);
                /* If prefs say to write changes to file, do so */
                if (prefs_get_int("id3_write")) {
                    /* T_item tag_id;*/
                    /* should we update all ID3 tags or just the one
                     changed? -- obsoleted in 0.71 */
                    /*        if (prefs_get_id3_writeall ()) tag_id = T_ALL;
                     else                        tag_id = t_item;*/
                    write_tags_to_file(track);
                    while (widgets_blocked && gtk_events_pending())
                        gtk_main_iteration();
                }
            }
            g_list_free(members);
            /* allow user input again */
            if (prefs_get_int("id3_write"))
                release_widgets();
            /* display possible duplicates that have been removed */
            gp_duplicate_remove(NULL, NULL);
            /* indicate that data has changed */
            if (itdb)
                data_changed(itdb);
        }
        break;
    default:
        break;
    }
    gtk_tree_path_free(path);
}

static void _st_clear_last_selection(NormalSortTabPagePrivate *priv) {
    if (priv->last_selection)
        g_list_free(priv->last_selection);

    priv->last_selection = NULL;
}

static void _st_record_last_selection(NormalSortTabPagePrivate *priv) {
    g_return_if_fail(priv);

    if (! priv->selected_entries)
        return;

    _st_clear_last_selection(priv);

    GList *old_selection = priv->selected_entries;
    while (old_selection) {
        TabEntry *old_entry = old_selection->data;
        if (!old_entry->master) {
            priv->last_selection = g_list_append(priv->last_selection, g_strdup(old_entry->name));
        }
        old_selection = old_selection->next;
    }
}

static gboolean _st_selection_changed_cb(gpointer data) {

    if(!NORMAL_SORT_TAB_IS_PAGE(data))
        return FALSE;

    NormalSortTabPage *self = NORMAL_SORT_TAB_PAGE(data);
    NormalSortTabPagePrivate *priv = NORMAL_SORT_TAB_PAGE_GET_PRIVATE(self);

    SortTabWidget *next = sort_tab_widget_get_next(priv->st_widget_parent);

    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(self));
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(self));
    gboolean final = sort_tab_widget_is_all_tracks_added(priv->st_widget_parent);

#if DEBUG_TIMING || DEBUG_CB_INIT
    GTimeVal time;
    g_get_current_time (&time);
    printf ("st_selection_changed_cb enter (inst: %d): %ld.%06ld sec\n",
            inst, time.tv_sec % 3600, time.tv_usec);
#endif

    printf("st_s_c_cb instance %d: entered\n", sort_tab_widget_get_instance(priv->st_widget_parent));
    if (gtk_tree_selection_count_selected_rows(selection) == 0) {
        /*
         * no selection -- unselect current selection (unless
         * priv->current_entry == NULL
         * -- that means we're in the middle of a  st_init()
         * (removing all entries). In that case we don't want
         * to forget our last selection!
         */
        if (priv->selected_entries) {
            priv->selected_entries = NULL;

            _st_clear_last_selection(priv);
            priv->unselected = TRUE;
        }

        sort_tab_widget_build(next, -1);
    }
    else { /* handle new selection */
        GList *paths = gtk_tree_selection_get_selected_rows(selection, &model);

        _st_record_last_selection(priv);
        priv->selected_entries = NULL;

        while (paths) {
            GtkTreePath *path = paths->data;
            GtkTreeIter iter;

            if (gtk_tree_model_get_iter(model, &iter, path)) {
                TabEntry *entry;
                gtk_tree_model_get(model, &iter, ST_COLUMN_ENTRY, &entry, -1);
                if (entry) {
                    priv->selected_entries = g_list_append(priv->selected_entries, entry);
                }
            }

            paths = paths->next;
        }
        g_list_free(paths);

        /* initialize next instance */
        sort_tab_widget_build(next, -1);

        /* remember new selection */
        priv->unselected = FALSE;

        GList *entries = priv->selected_entries;

        while(entries && next) {
            TabEntry *entry = entries->data;
            if (entry->members) {
                GList *gl;
                sort_tab_widget_set_sort_enablement(next, FALSE);

                for (gl = entry->members; gl; gl = gl->next) {
                    /* add all member tracks to next instance */
                    Track *track = gl->data;
                    sort_tab_widget_add_track(next, track, FALSE, TRUE);
                }
                sort_tab_widget_set_sort_enablement(next, TRUE);

                sort_tab_widget_add_track(next, NULL, TRUE, final);
            }

            entries = entries->next;
        }

        /* Advertise that a new set of tracks has been selected */
        if (! SORT_TAB_IS_WIDGET(next)) {
            GList *tracks = NULL;

            tracks = normal_sort_tab_page_get_selected_tracks(self);
            gtkpod_set_displayed_tracks(tracks);
        }

        gtkpod_tracks_statusbar_update();
    }

#if DEBUG_TIMING
    g_get_current_time (&time);
    printf ("st_selection_changed_cb exit:  %ld.%06ld sec\n",
            time.tv_sec % 3600, time.tv_usec);
#endif

    return FALSE;
}

/**
 * Callback function called when the selection of the sort tab
 * view has changed Instead of handling the selection directly,
 * we add a "callback". Currently running display updates will
 * be stopped before the st_selection_changed_cb is actually
 * called.
 */
static void _st_selection_changed(GtkTreeSelection *selection, gpointer user_data) {
#if DEBUG_CB_INIT
    printf("st_s_c enter (inst: %p)\n", (gint)user_data);
#endif
    g_idle_add_full(G_PRIORITY_DEFAULT_IDLE, _st_selection_changed_cb, user_data, NULL);
#if DEBUG_CB_INIT
    printf("st_s_c exit (inst: %p)\n", (gint)user_data);
#endif
}

static gboolean _st_is_track_selected(NormalSortTabPage *self, Track *track) {
    NormalSortTabPagePrivate *priv = NORMAL_SORT_TAB_PAGE_GET_PRIVATE(self);

    if (!priv->selected_entries)
        return FALSE;

    GList *entries = priv->selected_entries;

    while (entries) {
        TabEntry *entry = entries->data;

        if (g_list_index(entry->members, track) > 0)
            return TRUE;

        entries = entries->next;
    }

    return FALSE;
}

static gboolean _st_is_entry_selected(NormalSortTabPage *self, TabEntry *entry) {
    NormalSortTabPagePrivate *priv = NORMAL_SORT_TAB_PAGE_GET_PRIVATE(self);

    if (!priv->selected_entries)
        return FALSE;

    if (g_list_index(priv->selected_entries, entry) == -1)
        return FALSE;

    g_message("Entry %s selected", entry->name);

    return TRUE;
}

static gboolean _st_was_entry_selected(NormalSortTabPage *self, TabEntry *entry) {
    NormalSortTabPagePrivate *priv = NORMAL_SORT_TAB_PAGE_GET_PRIVATE(self);

    if (!priv->last_selection)
        return FALSE;

    if (g_list_index(priv->last_selection, entry) == -1)
        return FALSE;

    return TRUE;
}

/**
 * Is the "All" entry included in the selected entries
 *
 */
static gboolean _st_is_all_entry_selected(NormalSortTabPage *self) {
    NormalSortTabPagePrivate *priv = NORMAL_SORT_TAB_PAGE_GET_PRIVATE(self);

    if (!priv->selected_entries)
        return FALSE;

    GList *se = priv->selected_entries;
    while(se) {
        TabEntry *entry = se->data;
        if (entry->master) {
            g_message("Entry is master so all entry selected");
            return TRUE;
        }

        se = se->next;
    }

    return FALSE;
}

/**
 * Was the "All" entry included in the selected entries
 *
 */
static gboolean _st_was_all_entry_selected(NormalSortTabPage *self) {
    NormalSortTabPagePrivate *priv = NORMAL_SORT_TAB_PAGE_GET_PRIVATE(self);

    if (!priv->last_selection)
        return FALSE;

    GList *se = priv->last_selection;
    while(se) {
        TabEntry *entry = se->data;
        if (entry->master)
            return TRUE;

        se = se->next;
    }

    return FALSE;
}
/**
 * Function used to compare rows with user's search string
 */
gboolean _st_search_equal_func(GtkTreeModel *model, gint column, const gchar *key, GtkTreeIter *iter, gpointer search_data) {
    TabEntry *entry1;
    gboolean cmp;
    gtk_tree_model_get(model, iter, ST_COLUMN_ENTRY, &entry1, -1);

    cmp = (compare_string_start_case_insensitive(entry1->name, key) != 0);
    return cmp;
}

/* Find TabEntry with compilation set true. Return NULL if no entry was found. */
static TabEntry *_st_get_compilation_entry(NormalSortTabPage *self) {
    TabEntry *entry;
    guint i;

    NormalSortTabPagePrivate *priv = NORMAL_SORT_TAB_PAGE_GET_PRIVATE(self);
    i = 1; /* skip master entry, which is supposed to be at first position */
    while ((entry = (TabEntry *) g_list_nth_data(priv->entries, i)) != NULL) {
        if (entry->compilation)
            break; /* found! */
        ++i;
    }
    return entry;
}

/**
 * Get the correct name for the entry according to currently
 * selected category (page). Do _not_ g_free() the return value!
 */
static const gchar *_st_get_entry_name(NormalSortTabPage *self, Track *track) {
    NormalSortTabPagePrivate *priv = NORMAL_SORT_TAB_PAGE_GET_PRIVATE(self);
    SortTabWidget *st_widget_parent = priv->st_widget_parent;

    T_item t_item = ST_to_T(sort_tab_widget_get_category(st_widget_parent));
    return track_get_item(track, t_item);
}

/**
 * Find TabEntry with name "name".
 *
 * Return NULL if no entry was found.
 *
 * If "name" is {-1, 0x00}, it returns the master entry. Otherwise
 * it skips the master entry.
 */
static TabEntry *_st_get_entry_by_name(NormalSortTabPage *self, const gchar *name) {
    TabEntry *entry = NULL;

    NormalSortTabPagePrivate *priv = NORMAL_SORT_TAB_PAGE_GET_PRIVATE(self);

    if (!name)
        return NULL;

    /* check if we need to return the master entry */
    if ((strlen(name) == 1) && (*name == -1)) {
        entry = (TabEntry *) g_list_nth_data(priv->entries, 0);
    }
    else {
        if (priv->entry_hash)
            entry = g_hash_table_lookup(priv->entry_hash, name);
    }
    return entry;
}

/* Returns the entry "track" is stored in or NULL. The master entry
 "All" is skipped */
static TabEntry *_st_get_entry_by_track(NormalSortTabPage *self, Track *track) {
    GList *entries;
    TabEntry *entry = NULL;
    NormalSortTabPagePrivate *priv = NORMAL_SORT_TAB_PAGE_GET_PRIVATE(self);

    if (!track)
        return NULL;

    /* skip master entry, which is supposed to be at first position */
    entries = g_list_nth(priv->entries, 1);
    while (entries) {
        entry = (TabEntry *) entries->data;
        g_warning("name %s", entry->name);
        g_warning("length %d" , g_list_length(entry->members));

        if (entry && entry->members && g_list_find(entry->members, track))
            break; /* found! */

        entries = entries->next;
    }

    return entry;
}

/* Append playlist to the playlist model. */
static void _st_add_entry(NormalSortTabPage *self, TabEntry *entry) {
    GtkTreeIter iter;
    GtkTreeModel *model;

    NormalSortTabPagePrivate *priv = NORMAL_SORT_TAB_PAGE_GET_PRIVATE(self);

    model = gtk_tree_view_get_model(GTK_TREE_VIEW(self));
    g_return_if_fail (model != NULL);
    /* Insert the compilation entry between All and the first entry
     so it remains at the top even when the list is not sorted */
    if (entry->compilation) {
        gtk_list_store_insert(GTK_LIST_STORE (model), &iter, 1);
    }
    else {
        gtk_list_store_append(GTK_LIST_STORE (model), &iter);
    }
    gtk_list_store_set(GTK_LIST_STORE (model), &iter, ST_COLUMN_ENTRY, entry, -1);
    /* Prepend entry to the list, but always add after the master. */
    priv->entries = g_list_insert(priv->entries, entry, 1);

    if (!entry->master && !entry->compilation) {
        if (!priv->entry_hash) {
            priv->entry_hash = g_hash_table_new(g_str_hash, g_str_equal);
        }
        g_hash_table_insert(priv->entry_hash, entry->name, entry);
    }
}

static void _st_free_entry_cb(gpointer data, gpointer user_data) {
    TabEntry *entry = (TabEntry *) data;
    if (!entry)
        return;

    g_list_free(entry->members);
}

static void _cell_renderer_stop_editing(GtkCellRenderer *renderer, gpointer user_data) {
    gtk_cell_renderer_stop_editing (renderer, (gboolean) GPOINTER_TO_INT(user_data));
}

static gboolean _normal_sort_tab_page_button_press_event(GtkWidget *w, GdkEventButton *e, gpointer data) {
    if (w && e) {
        NormalSortTabPage *self = NORMAL_SORT_TAB_PAGE(w);
        NormalSortTabPagePrivate *priv = NORMAL_SORT_TAB_PAGE_GET_PRIVATE(self);
        SortTabWidget *parent = priv->st_widget_parent;

        switch (e->button) {
        case 3:
            st_context_menu_init(parent);
            return TRUE;
        default:
            break;
        }

    }
    return FALSE;
}

static void normal_sort_tab_page_dispose(GObject *gobject) {

    /* call the parent class' dispose() method */
    G_OBJECT_CLASS(normal_sort_tab_page_parent_class)->dispose(gobject);
}

static void normal_sort_tab_page_class_init(NormalSortTabPageClass *klass) {
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->dispose = normal_sort_tab_page_dispose;

    g_type_class_add_private(klass, sizeof(NormalSortTabPagePrivate));
}

static void normal_sort_tab_page_init(NormalSortTabPage *self) {
    GtkTreeSelection *selection;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    gtk_widget_show(GTK_WIDGET (self));

    gtk_drag_source_set(GTK_WIDGET (self), GDK_BUTTON1_MASK, st_drag_types, TGNR (st_drag_types), GDK_ACTION_COPY
                | GDK_ACTION_MOVE);
    g_signal_connect ((gpointer) self, "drag_data_get",
                G_CALLBACK (_st_drag_data_get),
                NULL);
    g_signal_connect ((gpointer) self, "drag-end",
                G_CALLBACK (_st_drag_end),
                NULL);
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW (self), TRUE);
    gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW (self), TRUE);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW (self), FALSE);
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW (self), TRUE);
    gtk_tree_view_set_search_column(GTK_TREE_VIEW (self), 0);
    gtk_tree_view_set_search_equal_func(GTK_TREE_VIEW (self), _st_search_equal_func, NULL, NULL);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(self));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);

    /* Add column */
    renderer = gtk_cell_renderer_text_new();
    g_signal_connect (G_OBJECT (renderer), "edited",
            G_CALLBACK (_st_cell_edited),
            self);
    g_object_set_data(G_OBJECT (renderer), "column", (gint *) ST_COLUMN_ENTRY);
//    column = gtk_tree_view_column_new();
//    gtk_tree_view_column_pack_start(column, renderer, TRUE);
    column = gtk_tree_view_column_new_with_attributes("", renderer, NULL);

    gtk_tree_view_column_set_cell_data_func(column, renderer, _st_cell_data_func, NULL, NULL);
    gtk_tree_view_column_set_sort_column_id(column, ST_COLUMN_ENTRY);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_sort_order(column, GTK_SORT_ASCENDING);
    gtk_tree_view_append_column(GTK_TREE_VIEW(self), column);

    g_signal_connect (G_OBJECT (self), "button-press-event",
                G_CALLBACK (_normal_sort_tab_page_button_press_event), NULL);
}

GtkWidget *normal_sort_tab_page_new(SortTabWidget *st_widget_parent, ST_CAT_item category) {
    NormalSortTabPage *nst = g_object_new(NORMAL_SORT_TAB_TYPE_PAGE, NULL);
    NormalSortTabPagePrivate *priv = NORMAL_SORT_TAB_PAGE_GET_PRIVATE(nst);
    gchar *buf;
    GtkTreeSelection *selection;

    priv->st_widget_parent = st_widget_parent;

    GtkTreeModel *model = sort_tab_widget_get_normal_model(priv->st_widget_parent);
    gtk_tree_view_set_model(GTK_TREE_VIEW(nst), model);

    gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE (model), ST_COLUMN_ENTRY, _st_data_compare_func, nst, NULL);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(nst));

    priv->selection_changed_id = g_signal_connect (G_OBJECT (selection), "changed",
                G_CALLBACK (_st_selection_changed),
                nst);
    /*
     * set string compare function according to
     * whether the ignore field is set or not
     */
    buf = g_strdup_printf("sort_ign_field_%d", ST_to_T(category));
    if (prefs_get_int(buf))
        priv->entry_compare_func = _compare_entry_fuzzy;
    else
        priv->entry_compare_func = _compare_entry;
    g_free(buf);

    return GTK_WIDGET(nst);
}

void normal_sort_tab_page_set_unselected(NormalSortTabPage *self, gboolean state) {
    g_return_if_fail(NORMAL_SORT_TAB_IS_PAGE(self));

    NormalSortTabPagePrivate *priv = NORMAL_SORT_TAB_PAGE_GET_PRIVATE(self);

    priv->unselected = state;
}

/* called by st_add_track() */
void normal_sort_tab_page_add_track(NormalSortTabPage *self, Track *track, gboolean final, gboolean display) {
    TabEntry *entry, *master_entry;
    const gchar *entryname = NULL;
    TabEntry *select_entry = NULL;
    gboolean first = FALSE;
    gboolean group_track = FALSE;
    GtkTreeIter iter;
    GtkTreeModel *model;
    GtkTreeSelection *selection;

    NormalSortTabPagePrivate *priv = NORMAL_SORT_TAB_PAGE_GET_PRIVATE(self);
    SortTabWidget *st_parent_widget = priv->st_widget_parent;
    SortTabWidget *st_next = sort_tab_widget_get_next(st_parent_widget);

    model = gtk_tree_view_get_model(GTK_TREE_VIEW(self));

    sort_tab_widget_set_all_tracks_added(st_parent_widget, final);


    if (track) {
        /* add track to "All" (master) entry */
        master_entry = g_list_nth_data(priv->entries, 0);
        if (!master_entry) {
            /* doesn't exist yet -- let's create it */
            master_entry = g_malloc0(sizeof(TabEntry));
            master_entry->name = g_strdup(_("All"));
            _st_build_sortkeys(master_entry);
            master_entry->master = TRUE;
            master_entry->compilation = FALSE;
            _st_add_entry(self, master_entry);
            first = TRUE; /* this is the first track */
        }

        master_entry->members = g_list_prepend(master_entry->members, track);
        /* Check if this track should go in the compilation artist group */
        guint current_category = sort_tab_widget_get_category(st_parent_widget);
        group_track = (prefs_get_int("group_compilations") && (track->compilation == TRUE) && (current_category
                == ST_CAT_ARTIST));

        /* Check whether entry of same name already exists */
        if (group_track) {
            entry = _st_get_compilation_entry(self);
        }
        else {
            entryname = _st_get_entry_name(self, track);
            entry = _st_get_entry_by_name(self, entryname);
        }

        if (!entry) {
            /* not found, create new one */
            entry = g_malloc0(sizeof(TabEntry));
            if (group_track)
                entry->name = g_strdup(_("Compilations"));
            else
                entry->name = g_strdup(entryname);

            _st_build_sortkeys(entry);
            entry->compilation = group_track;
            entry->master = FALSE;
            _st_add_entry(self, entry);
        }

        /* add track to entry members list */
        entry->members = g_list_prepend(entry->members, track);

        /* add track to next tab if "entry" is selected */
        if (_st_is_all_entry_selected(self) || _st_is_entry_selected(self, entry)) {
            g_warning("Called from add track normal (1)");
            sort_tab_widget_add_track(st_next, track, final, display);
        }

        /* check if we should select some entry */
        if (! priv->selected_entries && ! priv->last_selection) {
            /*
             * no last selection -- check if we should select "All"
             * only select "All" when currently adding the first track
             */
            if (first)
                select_entry = master_entry;
        }
        else if (! priv->selected_entries && priv->last_selection) {
            /*
             * select current entry if it corresponds to the last
             * selection, or last_entry if that's the master entry
             */
            if (_st_was_entry_selected(self, entry) || _st_was_all_entry_selected(self))
                select_entry = entry;
        }
    }

    /*
     * select "All" if it's the last track added, no entry currently
     * selected (including "select_entry", which is to be selected
     * allows us to select "All"
     */
    if (final && !priv->selected_entries && !select_entry && !priv->unselected) {
        /* auto-select entry "All" */
        select_entry = g_list_nth_data(priv->entries, 0);
    }

    if (select_entry) {
        /* select current select_entry */
        if (!gtk_tree_model_get_iter_first(model, &iter)) {
            g_warning ("Programming error: st_add_track: iter invalid\n");
            return;
        }

        priv->selected_entries = NULL;
        do {
            gtk_tree_model_get(model, &iter, ST_COLUMN_ENTRY, &entry, -1);
            if (entry == select_entry) {
                selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(self));

                priv->selected_entries = g_list_append(priv->selected_entries, select_entry);
                gtk_tree_selection_select_iter(selection, &iter);
                break;
            }
        }
        while (gtk_tree_model_iter_next(model, &iter));
    }
    else if (!track && final) {
        sort_tab_widget_add_track(st_next, NULL, final, display);
    }
}

void normal_sort_tab_page_remove_track(NormalSortTabPage *self, Track *track) {
    TabEntry *master, *entry;
    NormalSortTabPagePrivate *priv = NORMAL_SORT_TAB_PAGE_GET_PRIVATE(self);
    SortTabWidget *next = sort_tab_widget_get_next(priv->st_widget_parent);

    master = g_list_nth_data(priv->entries, 0);
    if (!master)
        return; /* should not happen! */

    /* remove "track" from master entry "All" */
    master->members = g_list_remove(master->members, track);

    /* find entry which other entry contains the track... */
    entry = _st_get_entry_by_track(self, track);

    /* ...and remove it */
    if (entry) {
        entry->members = g_list_remove(entry->members, track);

        if (! g_list_length(entry->members)) {
            GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(self));
            GtkTreeIter iter;
            gboolean valid;

            valid = gtk_tree_model_get_iter_first(model, &iter);
            while(valid) {
               TabEntry *rowentry;
                gtk_tree_model_get(model, &iter, ST_COLUMN_ENTRY, &rowentry, -1);
                if (entry == rowentry)
                    break;

                gtk_tree_model_iter_next(model, &iter);
            }

            gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
        }
    }

    sort_tab_widget_remove_track(next, track);
}

void normal_sort_tab_page_track_changed(NormalSortTabPage *self, Track *track, gboolean removed) {
    TabEntry *master, *entry;
    NormalSortTabPagePrivate *priv = NORMAL_SORT_TAB_PAGE_GET_PRIVATE(self);
    SortTabWidget *next = sort_tab_widget_get_next(priv->st_widget_parent);

    master = g_list_nth_data(priv->entries, 0);
    if (!master)
        return; /* should not happen */

    /* if track is not in tab, don't proceed (should not happen) */
    if (g_list_find(master->members, track) == NULL)
        return;

    /*
     * Note this has been heavily reduced in scope since
     * the original recategorize method was commented
     * out in 2003 (see version history). This may need
     * to be corrected or not.
     */
    if (removed) {
        /* remove "track" from master entry "All" */
        master->members = g_list_remove(master->members, track);

        /* find entry which other entry contains the track... */
        entry = _st_get_entry_by_track(self, track);

        /* ...and remove it */
        if (entry)
            entry->members = g_list_remove(entry->members, track);

        if ((_st_is_entry_selected(self, entry)) || (_st_is_all_entry_selected(self)))
            sort_tab_widget_track_changed(next, track, TRUE);
    }
    else {
        if (_st_is_track_selected(self, track)) {
            /* "track" is in currently selected entry */
            sort_tab_widget_track_changed(next, track, FALSE);
        }
    }
}

/**
 * Remove all entries from the display model
 */
void normal_sort_tab_page_clear(NormalSortTabPage *self) {
    gint column;
    GtkSortType sortorder;

    NormalSortTabPagePrivate *priv = NORMAL_SORT_TAB_PAGE_GET_PRIVATE(self);
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(self));
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(self));

    g_signal_handler_block (selection, priv->selection_changed_id);

    if (priv->selected_entries) {
        priv->selected_entries = NULL;

        /* We may have to unselect the previous selection */
        gtk_tree_selection_unselect_all(selection);
    }

    if (model) {
        gtk_list_store_clear(GTK_LIST_STORE(model));
    }

    g_list_foreach(priv->entries, _st_free_entry_cb, NULL);
    g_list_free(priv->entries);
    priv->entries = NULL;

    if (priv->entry_hash)
        g_hash_table_destroy(priv->entry_hash);

    priv->entry_hash = NULL;

    if ((prefs_get_int("st_sort") == SORT_NONE)
            && gtk_tree_sortable_get_sort_column_id(GTK_TREE_SORTABLE (model), &column, &sortorder)) {

        /* recreate track treeview to unset sorted column */
        if (column >= 0) {
            /* Use the special value to set the sorted column to unsorted */
            gtk_tree_sortable_set_sort_column_id(
                    GTK_TREE_SORTABLE(model),
                    GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID,
                    GTK_SORT_ASCENDING);
        }
    }

    g_signal_handler_unblock (selection, priv->selection_changed_id);
}

GList *normal_sort_tab_page_get_selected_tracks(NormalSortTabPage *self) {
    NormalSortTabPagePrivate *priv = NORMAL_SORT_TAB_PAGE_GET_PRIVATE(self);
    GList *tracks = NULL;
    GList *entries = priv->selected_entries;

    while(entries) {
        TabEntry *entry = entries->data;
        GList *gl;

        for (gl = entry->members; gl; gl = gl->next) {
            /* add all member tracks to next instance */
            Track *track = gl->data;
            tracks = g_list_prepend(tracks, track);
        }

        entries = entries->next;
    }

    return tracks;
}

void normal_sort_tab_page_sort(NormalSortTabPage *self, enum GtkPodSortTypes order) {
    g_return_if_fail(NORMAL_SORT_TAB_IS_PAGE(self));

    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(self));
    if (order != SORT_NONE)
        gtk_tree_sortable_set_sort_column_id(
                GTK_TREE_SORTABLE (model),
                ST_COLUMN_ENTRY,
                order);
    else
        gtk_tree_sortable_set_sort_column_id(
                GTK_TREE_SORTABLE(model),
                GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID,
                GTK_SORT_ASCENDING);
}

void normal_sort_tab_page_stop_editing(NormalSortTabPage *self, gboolean cancel) {
    g_return_if_fail(NORMAL_SORT_TAB_IS_PAGE(self));
    GList *cells;
    GtkTreeViewColumn *col;
    gtk_tree_view_get_cursor(GTK_TREE_VIEW(self), NULL, &col);

    if (col) {
        cells = gtk_cell_layout_get_cells(GTK_CELL_LAYOUT (col));
        g_list_foreach(cells, (GFunc) _cell_renderer_stop_editing, GINT_TO_POINTER((gint) cancel));
        g_list_free(cells);
    }
}
#endif /* NORMAL_SORT_TAB_PAGE_C_*/

