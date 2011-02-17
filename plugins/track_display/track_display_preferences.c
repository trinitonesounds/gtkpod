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
#include "libgtkpod/misc.h"
#include "libgtkpod/misc_conversion.h"
#include "libgtkpod/prefs.h"
#include "libgtkpod/directories.h"
#include "plugin.h"
#include "display_tracks.h"

static GtkBuilder *prefbuilder;

static GtkWidget *notebook = NULL;
static GtkWidget *displayed_columns_view = NULL;
static GtkWidget *ign_words_view = NULL;

static const gint sort_ign_fields[] =
    { T_TITLE, T_ARTIST, T_ALBUM, T_COMPOSER, -1 };

static GtkWindow *notebook_get_parent_window() {
    if (! notebook) {
        return NULL;
    }

    return GTK_WINDOW(gtk_widget_get_toplevel(notebook));
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

static void setup_ign_word_tree (GtkTreeView *treeview, gboolean list_visible)
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

    store = gtk_list_store_new (1, G_TYPE_STRING);
    column = gtk_tree_view_column_new ();
    renderer = gtk_cell_renderer_text_new ();

    gtk_tree_view_column_pack_start (column, renderer, TRUE);
    gtk_tree_view_column_set_attributes (column, renderer, "text", 0, NULL);

    gtk_tree_view_append_column (treeview, column);
    gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));

    g_object_unref (G_OBJECT (store));

    GList *sort_ign_pref_values = prefs_get_list("sort_ign_string_");

    for (i = 0; i < g_list_length(sort_ign_pref_values); i++) {
        gchar* sort_ign_value = g_list_nth_data(sort_ign_pref_values, i);
        if ((!list_visible && sort_ign_value) || (list_visible && !sort_ign_value))
            continue;

        gtk_list_store_append (store, &iter);
        gtk_list_store_set(store, &iter, 0, sort_ign_value, -1);
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

static void apply_ign_strings() {
    gint i;
    gchar *buf;
    GtkTreeModel *model;
    GtkTreeIter iter;
    GList *values = NULL;
    gchar *value;
    gboolean valid;

    /* read sort field states */
    for (i = 0; sort_ign_fields[i] != -1; ++i) {
        buf = g_strdup_printf("sort_ign_field_%d", sort_ign_fields[i]);
        GtkWidget *w = gtkpod_builder_xml_get_widget(prefbuilder, buf);
        g_return_if_fail (w);
        prefs_set_int(buf, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (w)));
        g_free(buf);
    }

    model = gtk_tree_view_get_model (GTK_TREE_VIEW(ign_words_view));
    valid = gtk_tree_model_get_iter_first (model, &iter);
    while(valid) {
        gtk_tree_model_get (model, &iter,
                                  0, &value,
                                  -1);
        values = g_list_append(values, value);
        valid = gtk_tree_model_iter_next(model, &iter);
    }

    prefs_apply_list("sort_ign_string_", values);

    compare_string_fuzzy_generate_keys();
}

void on_ign_field_toggled (GtkToggleButton *togglebutton, gpointer data) {
    apply_ign_strings();
}

/*
    glade callback
*/

G_MODULE_EXPORT void on_ign_word_add_clicked (GtkButton *sender, gpointer e) {
    g_return_if_fail(ign_words_view);
    GtkTreeModel *model;
    GtkTreeIter iter;
    gboolean valid_iter;

    gchar *word = get_user_string_with_parent(notebook_get_parent_window(), _("New Word to Ignore"), _("Please enter a word for sorting functions to ignore"), NULL, NULL, NULL, GTK_STOCK_ADD);
    if (! word)
        return;

    if (strlen(word) == 0)
        return;

    model = gtk_tree_view_get_model (GTK_TREE_VIEW(ign_words_view));

    /* Check if the value is already in the list store */
    valid_iter = gtk_tree_model_get_iter_first (model, &iter);
    while (valid_iter) {
        gchar *curr_ign;
        gint comparison;
        gtk_tree_model_get (model, &iter, 0, &curr_ign, -1);

        comparison = compare_string_case_insensitive(word, curr_ign);
        g_free (curr_ign);
        if (comparison == 0) {
            gtkpod_statusbar_message("The word %s is already in the \"Ignored Frequent Word\" list", word);
            return;
        }

        valid_iter = gtk_tree_model_iter_next (model, &iter);
    }


    gtk_list_store_append (GTK_LIST_STORE (model), &iter);
    gtk_list_store_set(GTK_LIST_STORE (model), &iter, 0, word, -1);
    apply_ign_strings();
}

/*
    glade callback
*/
G_MODULE_EXPORT void on_ign_word_remove_clicked (GtkButton *sender, gpointer e)
{
    g_return_if_fail(ign_words_view);

    GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (ign_words_view));
    GtkTreeIter iter;
    gchar *word;

    if(!tree_get_current_iter (GTK_TREE_VIEW (ign_words_view), &iter) || !gtk_list_store_iter_is_valid (GTK_LIST_STORE (model), &iter))
        return;

    gtk_tree_model_get (model, &iter, 0, &word, -1);
    gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
    apply_ign_strings();
}

/*
    glade callback
*/
G_MODULE_EXPORT void on_column_add_clicked (GtkButton *sender, gpointer e) {
    g_return_if_fail(displayed_columns_view);

    gint i;
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkBuilder *builder;

    gchar *glade_path = g_build_filename(get_glade_dir(), "track_display.xml", NULL);
    builder = gtkpod_builder_xml_new(glade_path);
    GtkWidget *dlg = gtkpod_builder_xml_get_widget (builder, "prefs_columns_dialog");
    GtkTreeView *view = GTK_TREE_VIEW (gtkpod_builder_xml_get_widget (builder, "available_columns"));
    g_free(glade_path);

    gtk_window_set_transient_for (GTK_WINDOW (dlg), notebook_get_parent_window());
    setup_column_tree (view, FALSE);

    /* User pressed Cancel */
    if(!gtk_dialog_run (GTK_DIALOG (dlg)))
    {
        gtk_widget_destroy (dlg);
        g_object_unref (builder);
        return;
    }

    /* User pressed Add */
    model = gtk_tree_view_get_model (view);
    tree_get_current_iter (view, &iter);
    gtk_tree_model_get (model, &iter, 1, &i, -1);

    gtk_widget_destroy (dlg);
    g_object_unref (builder);

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
        TrackCommand *cmd = g_list_nth_data(cmds, activeindex);
        prefs_set_string(DEFAULT_TRACK_COMMAND_PREF_KEY, track_command_get_id(cmd));
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
        TrackCommand *cmd = g_list_nth_data(trkcmds, i);
        gtk_combo_box_append_text(combo, _(track_command_get_text(cmd)));
        if (cmdpref && g_str_equal(cmdpref, track_command_get_id(cmd)))
            activeindex = i;
    }

    if (activeindex > -1)
        gtk_combo_box_set_active(combo, activeindex);

    g_signal_connect (combo, "changed",
                    G_CALLBACK (trkcmd_combobox_changed),
                    NULL);
}

G_MODULE_EXPORT void on_tm_sort_case_sensitive_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
    gboolean val = gtk_toggle_button_get_active(togglebutton);
    prefs_set_int("tm_case_sensitive", val);
    gtkpod_broadcast_preference_change("tm_case_sensitive", val);
}

GtkWidget *init_track_display_preferences() {
    GtkComboBox *cmd_combo;
    gint i = 0;
    GtkWidget *w, *win;

    gchar *glade_path = g_build_filename(get_glade_dir(), "track_display.xml", NULL);
    prefbuilder = gtkpod_builder_xml_new(glade_path);
    win = gtkpod_builder_xml_get_widget(prefbuilder, "prefs_window");
    notebook = gtkpod_builder_xml_get_widget(prefbuilder, "track_settings_notebook");
    cmd_combo = GTK_COMBO_BOX(gtkpod_builder_xml_get_widget(prefbuilder, "track_exec_cmd_combo"));
    displayed_columns_view = gtkpod_builder_xml_get_widget(prefbuilder, "displayed_columns");
    ign_words_view = gtkpod_builder_xml_get_widget(prefbuilder, "ign_words_view");
    g_object_ref(notebook);
    gtk_container_remove(GTK_CONTAINER(win), notebook);
    gtk_widget_destroy(win);

    g_free(glade_path);

    setup_column_tree (GTK_TREE_VIEW(displayed_columns_view), TRUE);
    setup_ign_word_tree(GTK_TREE_VIEW(ign_words_view), TRUE);

    /* label the ignore-field checkbox-labels */
    for (i = 0; sort_ign_fields[i] != -1; ++i) {
        gchar *buf = g_strdup_printf("sort_ign_field_%d", sort_ign_fields[i]);
        w = gtkpod_builder_xml_get_widget(prefbuilder, buf);
        g_return_val_if_fail (w, NULL);
        gtk_button_set_label(GTK_BUTTON (w), gettext (get_t_string (sort_ign_fields[i])));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (w), prefs_get_int(buf));
        g_signal_connect(w, "toggled", G_CALLBACK(on_ign_field_toggled), NULL);
        g_free(buf);
    }

    populate_track_cmd_combo(cmd_combo);

    if ((w = gtkpod_builder_xml_get_widget(prefbuilder, "tm_cfg_case_sensitive"))) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), prefs_get_int("tm_case_sensitive"));
    }

    gtk_builder_connect_signals(prefbuilder, NULL);

    return notebook;
}
