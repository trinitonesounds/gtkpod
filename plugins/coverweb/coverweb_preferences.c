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
#include "libgtkpod/prefs.h"
#include "libgtkpod/gtkpod_app_iface.h"
#include "plugin.h"
#include "coverweb.h"
#include "coverweb_preferences.h"

static GtkWidget *bookmarks_view = NULL;

static void setup_bookmarks_tree (GtkTreeView *treeview, gboolean list_visible)
{
    GtkListStore *store;
    GtkTreeIter iter;
    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;
    gint i;

    /* Delete any existing columns first */
    GList *columns = gtk_tree_view_get_columns(treeview);
    for (i = 0; i < g_list_length(columns); ++i) {
        column = gtk_tree_view_get_column (treeview, 0);
        gtk_tree_view_remove_column (treeview, column);
    }
    g_list_free(columns);

    store = gtk_list_store_new (1, G_TYPE_STRING);
    column = gtk_tree_view_column_new ();
    renderer = gtk_cell_renderer_text_new ();

    gtk_tree_view_column_pack_start (column, renderer, TRUE);
    gtk_tree_view_column_set_attributes (column, renderer, "text", 0, NULL);
    gtk_tree_view_append_column (treeview, column);
    gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));
    g_object_unref (G_OBJECT (store));

    GList *bookmarks = prefs_get_list("coverweb_bookmark_");
    for (i = 0; i < g_list_length(bookmarks); i++) {
        gchar *bmark = g_list_nth_data(bookmarks, i);
        gtk_list_store_append (store, &iter);
        gtk_list_store_set(store, &iter,
                   0, bmark, -1);
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

static void save_bookmarks_preferences() {
    g_return_if_fail(bookmarks_view);
    GtkTreeModel *model;
    GtkListStore *store;
    GtkTreeIter iter;
    gint row = 0;
    gboolean valid;

    model = gtk_tree_view_get_model (GTK_TREE_VIEW(bookmarks_view));
    store = GTK_LIST_STORE(model);

    valid = gtk_tree_model_get_iter_first (model, &iter);
    while (valid) {
        /* Walk through the list, reading each row */
        gchar *bmark;

        gtk_tree_model_get (model, &iter,
                              0, &bmark,
                              -1);

        prefs_set_string_index("coverweb_bookmark_", row, bmark);
        g_free (bmark);
        row++;
        valid = gtk_tree_model_iter_next (model, &iter);
    }
    prefs_set_string_index("coverweb_bookmark_", row, LIST_END_MARKER);

    update_bookmark_menu();
}

static GtkWindow *bookmarks_view_get_parent_window() {
    if (! bookmarks_view) {
        return NULL;
    }

    return GTK_WINDOW(gtk_widget_get_toplevel(bookmarks_view));
}

/*
    glade callback
*/
G_MODULE_EXPORT void on_bookmark_add_clicked (GtkButton *sender, gpointer e)
{
    g_return_if_fail(bookmarks_view);

    gchar *bookmark;
    GtkTreeView *view = GTK_TREE_VIEW (bookmarks_view);
    GtkTreeModel *model;
    GtkTreeIter iter;

    bookmark
                = get_user_string_with_parent(bookmarks_view_get_parent_window(), _("Bookmark Url"), _("Please enter the full url of the bookmark"), NULL, NULL, NULL, GTK_STOCK_ADD);

    if (!bookmark)
        return;

    model = gtk_tree_view_get_model (view);
    gtk_list_store_append (GTK_LIST_STORE (model), &iter);
    gtk_list_store_set(GTK_LIST_STORE (model), &iter,
               0, bookmark, -1);

    save_bookmarks_preferences();
}

/*
    glade callback
*/
G_MODULE_EXPORT void on_bookmark_remove_clicked (GtkButton *sender, gpointer e)
{
    g_return_if_fail(bookmarks_view);

    gchar *bmark;
    GtkTreeView *view = GTK_TREE_VIEW (bookmarks_view);
    GtkTreeModel *model = gtk_tree_view_get_model (view);
    GtkTreeIter iter;

    if(!tree_get_current_iter (view, &iter) || !gtk_list_store_iter_is_valid (GTK_LIST_STORE (model), &iter))
        return;

    gtk_tree_model_get (model, &iter, 0, &bmark, -1);
    gtk_list_store_remove (GTK_LIST_STORE (model), &iter);

    save_bookmarks_preferences();
}

GtkWidget *init_coverweb_preferences(gchar *glade_path) {
    GtkWidget *win, *notebook;
    GtkBuilder *pref_xml;

    pref_xml = gtkpod_builder_xml_new(glade_path);
    win = gtkpod_builder_xml_get_widget(pref_xml, "prefs_window");
    notebook = gtkpod_builder_xml_get_widget(pref_xml, "coverweb_settings_notebook");
    bookmarks_view = gtkpod_builder_xml_get_widget(pref_xml, "bookmarks_view");
    g_object_ref(notebook);
    gtk_container_remove(GTK_CONTAINER (win), notebook);

    setup_bookmarks_tree (GTK_TREE_VIEW(bookmarks_view), TRUE);

    gtk_builder_connect_signals(pref_xml, NULL);
    return notebook;
}
