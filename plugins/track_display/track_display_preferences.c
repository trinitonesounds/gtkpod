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
 */

#include <gtk/gtk.h>
#include <glade/glade.h>
#include "libgtkpod/misc.h"
#include "libgtkpod/misc_conversion.h"
#include "libgtkpod/prefs.h"
#include "libgtkpod/directories.h"
#include "plugin.h"
#include "display_tracks.h"
#include "sort_window.h"

static GtkWidget *notebook = NULL;
static GtkWidget *displayed_columns_view = NULL;

static GtkWindow *notebook_get_parent_window() {
    if (! notebook) {
        return NULL;
    }

    return GTK_WINDOW(gtk_widget_get_parent(notebook));
}

static gint column_tree_sort (GtkTreeModel *model,
                              GtkTreeIter *a,
                              GtkTreeIter *b,
                              gpointer user_data)
{
    gchar *str1, *str2;
    gint result;

    gtk_tree_model_get (model, a, 0, &str1, -1);
    gtk_tree_model_get (model, b, 0, &str2, -1);
    result = g_utf8_collate (str1, str2);

    g_free (str1);
    g_free (str2);
    return result;
}

static void setup_column_tree (GtkTreeView *treeview, gboolean list_visible)
{
    GtkListStore *store;
    GtkTreeIter iter;
    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;
    gint i;

    /* Delete any existing columns first */
    while (TRUE)
    {
        column = gtk_tree_view_get_column (treeview, 0);

        if (!column)
            break;

        gtk_tree_view_remove_column (treeview, column);
    }

    store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);
    column = gtk_tree_view_column_new ();
    renderer = gtk_cell_renderer_text_new ();

    gtk_tree_view_column_pack_start (column, renderer, TRUE);
    gtk_tree_view_column_set_attributes (column, renderer, "text", 0, NULL);

    gtk_tree_view_append_column (treeview, column);
    gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));

    g_object_unref (G_OBJECT (store));

    for (i = 0; i < TM_NUM_COLUMNS; i++)
    {
        gint visible = prefs_get_int_index("col_visible", i);

        if ((!list_visible && visible) || (list_visible && !visible))
            continue;

        gtk_list_store_append (store, &iter);
        gtk_list_store_set(store, &iter,
                   0, gettext (get_tm_string (i)),
                   1, i, -1);
    }

    if(!list_visible)
    {
        /* Sort invisible columns */
        gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (store),
                             column_tree_sort,
                             NULL,
                             NULL);

        gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
                          GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
                          GTK_SORT_ASCENDING);
    }
}

static gboolean tree_get_current_iter (GtkTreeView *view, GtkTreeIter *iter)
{
    GtkTreeModel *model = gtk_tree_view_get_model (view);
    GtkTreePath *path;

    gtk_tree_view_get_cursor (view, &path, NULL);

    if (!path)
        return FALSE;

    gtk_tree_model_get_iter (model, iter, path);
    gtk_tree_path_free (path);

    return TRUE;
}

/*
    glade callback
*/
G_MODULE_EXPORT void on_sorting_button_clicked (GtkButton *sender, gpointer e)
{
    sort_window_create ();
}

/*
    glade callback
*/
G_MODULE_EXPORT void on_column_add_clicked (GtkButton *sender, gpointer e)
{
    g_return_if_fail(displayed_columns_view);

    gint i;
    GtkTreeModel *model;
    GtkTreeIter iter;

    gchar *glade_path = g_build_filename(get_glade_dir(), "track_display.glade", NULL);
    GladeXML *xml = gtkpod_xml_new (glade_path, "prefs_columns_dialog");
    GtkWidget *dlg = gtkpod_xml_get_widget (xml, "prefs_columns_dialog");
    GtkTreeView *view = GTK_TREE_VIEW (gtkpod_xml_get_widget (xml, "available_columns"));

    g_free(glade_path);

    gtk_window_set_transient_for (GTK_WINDOW (dlg), notebook_get_parent_window());
    setup_column_tree (view, FALSE);

    /* User pressed Cancel */
    if(!gtk_dialog_run (GTK_DIALOG (dlg)))
    {
        gtk_widget_destroy (dlg);
        g_object_unref (xml);
        return;
    }

    /* User pressed Add */
    model = gtk_tree_view_get_model (view);
    tree_get_current_iter (view, &iter);
    gtk_tree_model_get (model, &iter, 1, &i, -1);

    gtk_widget_destroy (dlg);
    g_object_unref (xml);

    view = GTK_TREE_VIEW (displayed_columns_view);
    model = gtk_tree_view_get_model (view);

    gtk_list_store_append (GTK_LIST_STORE (model), &iter);
    gtk_list_store_set(GTK_LIST_STORE (model), &iter,
               0, gettext (get_tm_string (i)),
               1, i, -1);

    prefs_set_int_index ("col_visible", i, TRUE);
    tm_store_col_order ();
    tm_show_preferred_columns ();
}

/*
    glade callback
*/
G_MODULE_EXPORT void on_column_remove_clicked (GtkButton *sender, gpointer e)
{
    g_return_if_fail(displayed_columns_view);

    gint i;
    GtkTreeView *view = GTK_TREE_VIEW (displayed_columns_view);
    GtkTreeModel *model = gtk_tree_view_get_model (view);
    GtkTreeIter iter;

    if(!tree_get_current_iter (view, &iter) || !gtk_list_store_iter_is_valid (GTK_LIST_STORE (model), &iter))
        return;

    gtk_tree_model_get (model, &iter, 1, &i, -1);
    gtk_list_store_remove (GTK_LIST_STORE (model), &iter);

    prefs_set_int_index ("col_visible", i, FALSE);
    tm_store_col_order ();
    tm_show_preferred_columns ();
}

/*
    glade callback
*/
G_MODULE_EXPORT void on_horizontal_scrollbar_toggled (GtkToggleButton *sender, gpointer e)
{
    gboolean active = gtk_toggle_button_get_active (sender);

    prefs_set_int ("horizontal_scrollbar", active);
    tm_show_preferred_columns ();
}

static void trkcmd_combobox_changed(GtkComboBox *combo) {
    gint activeindex = gtk_combo_box_get_active(combo);

    if (activeindex > -1) {
        GList *cmds = g_object_get_data(G_OBJECT(combo), "cmds");
        TrackCommandInterface *cmd = g_list_nth_data(cmds, activeindex);
        prefs_set_string(DEFAULT_TRACK_COMMAND_PREF_KEY, cmd->id);
    }
}

static void populate_track_cmd_combo(GtkComboBox *combo) {
    GtkListStore *store;
    GtkCellRenderer *cell;
    GList *trkcmds = gtkpod_get_registered_track_commands();
    gint i = 0, activeindex = -1;

    g_object_set_data(G_OBJECT(combo), "cmds", trkcmds);

    store = gtk_list_store_new(1, G_TYPE_STRING);
    gtk_combo_box_set_model(combo, GTK_TREE_MODEL (store));
    g_object_unref(store);

    cell = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT (combo), cell, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT (combo), cell, "text", 0, NULL);

    gchar *cmdpref = NULL;
    prefs_get_string_value(DEFAULT_TRACK_COMMAND_PREF_KEY, &cmdpref);

    for (i = 0; i < g_list_length(trkcmds); ++i) {
        TrackCommandInterface *cmd = g_list_nth_data(trkcmds, i);
        gtk_combo_box_append_text(combo, _(cmd->text));
        if (cmdpref && g_str_equal(cmdpref, cmd->id))
            activeindex = i;
    }

    if (activeindex > -1)
        gtk_combo_box_set_active(combo, activeindex);

    g_signal_connect (combo, "changed",
                    G_CALLBACK (trkcmd_combobox_changed),
                    NULL);
}

GtkWidget *init_track_display_preferences() {
    GladeXML *pref_xml;
    GtkComboBox *cmd_combo;

    gchar *glade_path = g_build_filename(get_glade_dir(), "track_display.glade", NULL);
    pref_xml = gtkpod_xml_new(glade_path, "track_settings_notebook");
    notebook = gtkpod_xml_get_widget(pref_xml, "track_settings_notebook");
    cmd_combo = GTK_COMBO_BOX(gtkpod_xml_get_widget(pref_xml, "track_exec_cmd_combo"));
    displayed_columns_view = gtkpod_xml_get_widget(pref_xml, "displayed_columns");
    g_object_ref(notebook);
    g_free(glade_path);

    setup_column_tree (GTK_TREE_VIEW(displayed_columns_view), TRUE);

    glade_xml_signal_autoconnect(pref_xml);

    populate_track_cmd_combo(cmd_combo);

    return notebook;
}
