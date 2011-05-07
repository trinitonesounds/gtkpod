/* Time-stamp: <2008-10-01 23:36:57 jcs>
 |
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

/* This file provides functions for the edit repositories/playlist
 * option window */

#include "repository.h"
#include <string.h>
#include <gtk/gtk.h>
#include "libgtkpod/misc.h"
#include "libgtkpod/misc_playlist.h"
#include "libgtkpod/directories.h"

/* print local debug message */
#define LOCAL_DEBUG 0

/**
 * pm_set_playlist_renderer_pix
 *
 * Set the appropriate playlist icon.
 *
 * @renderer: renderer to be set
 * @playlist: playlist to consider.
 */
static void set_playlist_renderer_pix(GtkCellRenderer *renderer, Playlist *playlist) {
    const gchar *stock_id = NULL;

    g_return_if_fail (renderer);
    g_return_if_fail (playlist);

    stock_id = return_playlist_stock_image(playlist);
    if (!stock_id)
        return;

    g_object_set(G_OBJECT (renderer), "stock-id", stock_id, NULL);
    g_object_set(G_OBJECT (renderer), "stock-size", GTK_ICON_SIZE_LARGE_TOOLBAR, NULL);
}

/* Provide graphic indicator in playlist combobox */
void playlist_cb_cell_data_func_pix(GtkCellLayout *cell_layout, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, gpointer data) {
    Playlist *playlist;

    g_return_if_fail (cell);
    g_return_if_fail (model);
    g_return_if_fail (iter);

    gtk_tree_model_get(model, iter, 0, &playlist, -1);

    set_playlist_renderer_pix(cell, playlist);
}

/**
 * pm_set_renderer_text
 *
 * Set the playlist name in appropriate style.
 *
 * @renderer: renderer to be set
 * @playlist: playlist to consider.
 */
static void set_playlist_renderer_text(GtkCellRenderer *renderer, Playlist *playlist) {
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

/* Provide playlist name in combobox */
void playlist_cb_cell_data_func_text(GtkCellLayout *cell_layout, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, gpointer data) {
    Playlist *playlist;

    g_return_if_fail (cell);
    g_return_if_fail (model);
    g_return_if_fail (iter);

    gtk_tree_model_get(model, iter, 0, &playlist, -1);

    set_playlist_renderer_text(cell, playlist);
}

/**
 * gp_init_model_number_combo:
 *
 * Set up the the model for a model_number combo with all iPod models
 * known to libgpod.
 *
 * @cb: the combobox that should be set up with a model.
 */
void repository_init_model_number_combo(GtkComboBox *cb) {
    const IpodInfo *table;
    Itdb_IpodGeneration generation;
    GtkCellRenderer *renderer;
    GtkTreeStore *store;
    gboolean info_found;
    gchar buf[PATH_MAX];

    table = itdb_info_get_ipod_info_table();
    g_return_if_fail (table);

    /* We need the G_TYPE_STRING column because GtkComboBoxEntry
     requires it */
    store = gtk_tree_store_new(2, G_TYPE_POINTER, G_TYPE_STRING);

    /* Create a tree model with the model numbers listed as a branch
     under each generation */
    generation = ITDB_IPOD_GENERATION_FIRST;
    do {
        GtkTreeIter iter;
        const IpodInfo *info = table;
        info_found = FALSE;

        while (info->model_number) {
            if (info->ipod_generation == generation) {
                GtkTreeIter iter_child;
                if (!info_found) {
                    gtk_tree_store_append(store, &iter, NULL);
                    gtk_tree_store_set(store, &iter, COL_POINTER, info, COL_STRING, "", -1);
                    info_found = TRUE;
                }
                gtk_tree_store_append(store, &iter_child, &iter);
                /* gtk_tree_store_set() is intelligent enough to copy
                 strings we pass to it */
                g_snprintf(buf, PATH_MAX, "x%s", info->model_number);
                gtk_tree_store_set(store, &iter_child, COL_POINTER, info, COL_STRING, buf, -1);
            }
            ++info;
        }
        ++generation;
    }
    while (info_found);

    /* set the model, specify the text column, and clear the cell
     layout (glade seems to automatically add a text column which
     messes up the entire layout) */
    gtk_combo_box_set_model(cb, GTK_TREE_MODEL (store));
    g_object_unref(store);

    /* Avoid deprecated function */
#if GTK_CHECK_VERSION(2,24,0)
    gtk_combo_box_set_entry_text_column (GTK_COMBO_BOX (cb), COL_STRING);
#else
    gtk_combo_box_entry_set_text_column(GTK_COMBO_BOX_ENTRY (cb), COL_STRING);
#endif

    gtk_cell_layout_clear(GTK_CELL_LAYOUT (cb));

    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT (cb), renderer, FALSE);
    gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT (cb), renderer, set_cell, NULL, NULL);
}

/****** common between init_repository_combo() and create_repository() */
void repository_combo_populate(GtkComboBox *combo_box) {
    struct itdbs_head *itdbs_head;
    GtkCellRenderer *cell;
    GtkListStore *store;
    GList *gl;

    itdbs_head = gp_get_itdbs_head();
    g_return_if_fail (itdbs_head);

    if (g_object_get_data(G_OBJECT (combo_box), "combo_set") == NULL) { /* the combo has not yet been initialized */

        /* Cell for graphic indicator */
        cell = gtk_cell_renderer_pixbuf_new();
        gtk_cell_layout_pack_start(GTK_CELL_LAYOUT (combo_box), cell, FALSE);
        gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT (combo_box), cell, playlist_cb_cell_data_func_pix, NULL, NULL);
        /* Cell for playlist name */
        cell = gtk_cell_renderer_text_new();
        gtk_cell_layout_pack_start(GTK_CELL_LAYOUT (combo_box), cell, FALSE);
        gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT (combo_box), cell, playlist_cb_cell_data_func_text, NULL, NULL);

        g_object_set(G_OBJECT (cell), "editable", FALSE, NULL);
    }

    store = gtk_list_store_new(1, G_TYPE_POINTER);

    for (gl = itdbs_head->itdbs; gl; gl = gl->next) {
        GtkTreeIter iter;
        Playlist *mpl;
        iTunesDB *itdb = gl->data;
        g_return_if_fail (itdb);

        mpl = itdb_playlist_mpl(itdb);
        g_return_if_fail (mpl);

        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 0, mpl, -1);
    }

    gtk_combo_box_set_model(combo_box, GTK_TREE_MODEL (store));
    g_object_unref(store);
}

/* ------------------------------------------------------------
 *
 *        Helper functions to retrieve widgets.
 *
 * ------------------------------------------------------------ */

GtkBuilder *init_repository_builder() {
    GtkBuilder *builder;

    gchar *glade_path = g_build_filename(get_glade_dir(), "repository_editor.xml", NULL);
    builder = gtkpod_builder_xml_new(glade_path);
    g_free(glade_path);

    return builder;
}

/* This is quite dirty: MODEL_ENTRY is not a real widget
 name. Instead it's the entry of a ComboBoxEntry -- hide this from
 the application */
GtkWidget *repository_builder_xml_get_widget(GtkBuilder *builder, const gchar *name) {
    if (strcmp(name, IPOD_MODEL_ENTRY) == 0) {
        GtkWidget *cb = gtkpod_builder_xml_get_widget(builder, IPOD_MODEL_COMBO);
        return gtk_bin_get_child(GTK_BIN (cb));
    } else if (strcmp(name, CRW_IPOD_MODEL_ENTRY) == 0) {
        GtkWidget *cb = gtkpod_builder_xml_get_widget(builder, CRW_IPOD_MODEL_COMBO);
        return gtk_bin_get_child(GTK_BIN (cb));
    }
    else {
        return gtkpod_builder_xml_get_widget(builder, name);
    }
}


