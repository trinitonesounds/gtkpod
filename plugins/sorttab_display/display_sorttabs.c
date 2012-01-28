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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "libgtkpod/gp_private.h"
#include "libgtkpod/prefs.h"
#include "libgtkpod/misc.h"
#include "plugin.h"
#include "sorttab_widget.h"
#include "display_sorttabs.h"

/* array with pointers to the sort tabs */
static SortTabWidget *first_sort_tab_widget;

/**
 * Created paned elements for sorttabs
 */
static GtkPaned *_st_create_paned(GtkPaned *sorttab_parent) {
    g_return_val_if_fail(sorttab_parent, NULL);

    GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_show(paned);

    gtk_paned_pack2(sorttab_parent, paned, TRUE, TRUE);
    return GTK_PANED(paned);
}

/**
 * Create sort tabs
 */
void sorttab_display_new(GtkPaned *sort_tab_parent, gchar *glade_path) {
    g_return_if_fail(sort_tab_parent);
    g_return_if_fail(glade_path);

    gint sort_tab_total = sort_tab_widget_get_max_index();

    gint inst;
    GtkPaned *parent;
    GList *paneds = NULL;

    for (gint i = 0; i < sort_tab_total; ++i) {
        if (i == 0)
            parent = sort_tab_parent;
        else
            parent = _st_create_paned(parent);

        paneds = g_list_append(paneds, parent);
    }

    /*
     * Count downward here because the smaller sort tabs might try to
     * initialize the higher one's -> create the higher ones first
     *
     * paneds(0) is the parent paned passed in to this function
     * sort tab 0 is added to the left of paneds(0)
     * paneds(1) is added to the right of paneds(0)
     * sort tab 1 is added to the left of paneds(0)
     * ...
     * sort tab n is added to the left of paneds(n)
     *
     * Single Exception:
     * sort tab SORT_TAB_MAX is added to the right of paneds(n)
     *
     */
    SortTabWidget *next = NULL;
    for (inst = sort_tab_total; inst >= 0; --inst) {
        if (inst == sort_tab_total)
            parent = g_list_nth_data(paneds, inst - 1);
        else
            parent = g_list_nth_data(paneds, inst);

        first_sort_tab_widget = sort_tab_widget_new(inst, GTK_WIDGET(parent), glade_path);

        sort_tab_widget_set_next(first_sort_tab_widget, next);
        if (next)
            sort_tab_widget_set_previous(next, first_sort_tab_widget);

        next = first_sort_tab_widget;

        /* Add this sort tab widget to the parent */
        if (inst == sort_tab_total) {
            gtk_paned_pack2(parent, GTK_WIDGET(first_sort_tab_widget), TRUE, TRUE);
        }
        else {
            gtk_paned_pack1(parent, GTK_WIDGET(first_sort_tab_widget), FALSE, TRUE);
        }
    }
    next = NULL;
}

void sorttab_display_append_widget() {
    SortTabWidget *last_sort_tab_widget = NULL;
    SortTabWidget *next =  NULL;
    SortTabWidget *prev = NULL;
    gchar *glade_path;
    gint inst;
    GtkWidget *prev_parent;
    GtkPaned *parent;

    prev = first_sort_tab_widget;
    next = prev;

    while (next) {
        prev = next;
        next = sort_tab_widget_get_next(prev);
    }

    glade_path = sort_tab_widget_get_glade_path(prev);
    inst = sort_tab_widget_get_instance(prev) + 1;
    prev_parent = sort_tab_widget_get_parent(prev);

    g_object_ref(prev);

    gtk_container_remove(GTK_CONTAINER(prev_parent), GTK_WIDGET(prev));

    parent = _st_create_paned(GTK_PANED(prev_parent));
    last_sort_tab_widget = sort_tab_widget_new(inst, GTK_WIDGET(parent), glade_path);

    gtk_paned_pack1(parent, GTK_WIDGET(prev), FALSE, TRUE);
    gtk_paned_pack2(parent, GTK_WIDGET(last_sort_tab_widget), TRUE, TRUE);

    sort_tab_widget_set_next(prev, last_sort_tab_widget);
    sort_tab_widget_set_parent(prev, GTK_WIDGET(parent));

    sort_tab_widget_set_previous(last_sort_tab_widget, prev);

    g_object_unref(prev);
}

void sorttab_display_remove_widget() {
    SortTabWidget *last_sort_tab_widget = NULL;
    SortTabWidget *next =  NULL;
    SortTabWidget *prev = NULL;
    SortTabWidget *previous_of_prev = NULL;
    GtkWidget *parent_of_parent, *parent;

    next = first_sort_tab_widget;
    while (next) {
        last_sort_tab_widget = next;
        next = sort_tab_widget_get_next(next);
    }

    parent = sort_tab_widget_get_parent(last_sort_tab_widget);
    prev = sort_tab_widget_get_previous(last_sort_tab_widget);
    previous_of_prev = sort_tab_widget_get_previous(prev);

    if (previous_of_prev == NULL) {
        /*
         * At the stage when there are only 2 filter tabs left
         * so they both share the same parent.
         */
        gtk_container_remove(GTK_CONTAINER(parent), GTK_WIDGET(last_sort_tab_widget));
    } else {
        /*
         * More than 2 tab filters so need to find the parent of the
         * final parent paned. Can do this by finding the parent of
         * the prev<-prev<-last_sort_tab_widget, removing the
         * final paned and adding the prev widget back.
         */
        parent_of_parent = sort_tab_widget_get_parent(previous_of_prev);

        g_object_ref(prev);
        gtk_container_remove(GTK_CONTAINER(parent), GTK_WIDGET(prev));
        gtk_container_remove(GTK_CONTAINER(parent_of_parent), GTK_WIDGET(parent));

        gtk_paned_pack2(GTK_PANED(parent_of_parent), GTK_WIDGET(prev), TRUE, TRUE);

        sort_tab_widget_set_parent(prev, parent_of_parent);

        g_object_unref(prev);
    }

    sort_tab_widget_set_next(prev, NULL);
}

/**
 * Let the user select a sort tab number
 *
 * @text: text to be displayed
 *
 * return
 *      value: -1: user selected cancel
 *      0...prefs_get_sort_tab_number() - 1: selected tab
 */
SortTabWidget *sorttab_display_get_sort_tab_widget(gchar *text) {
    GtkWidget *mdialog;
    GtkDialog *dialog;
    GtkWidget *combo;
    GtkCellRenderer *cell;
    gint result;
    gint i, nr, stn;
    gchar *bufp;
    GtkListStore *store;
    GtkTreeIter iter;
    SortTabWidget *st_widget = NULL;

    mdialog = gtk_message_dialog_new(GTK_WINDOW (gtkpod_app), GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK_CANCEL, "%s", text);

    dialog = GTK_DIALOG (mdialog);

    store = gtk_list_store_new(1, G_TYPE_STRING);
    stn = prefs_get_int("sort_tab_num");
    /* Create list */
    for (i = 1; i <= stn; ++i) {
        bufp = g_strdup_printf("%d", i);
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 0, bufp, -1);
        g_free(bufp);
    }

    combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));

    /* Create cell renderer. */
    cell = gtk_cell_renderer_text_new();

    /* Pack it to the combo box. */
    gtk_cell_layout_pack_start( GTK_CELL_LAYOUT(combo), cell, TRUE );

    /* Connect renderer to data source */
    gtk_cell_layout_set_attributes( GTK_CELL_LAYOUT(combo), cell, "text", 0, NULL );

    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);

    gtk_widget_show_all(combo);
    gtk_container_add(GTK_CONTAINER (gtk_dialog_get_content_area(GTK_DIALOG(dialog))), combo);

    result = gtk_dialog_run(GTK_DIALOG (mdialog));

    /* free the list */
    if (result == GTK_RESPONSE_CANCEL) {
        nr = -1; /* no selection */
    }
    else {
        gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combo), &iter);
        gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &bufp, -1);
        if (bufp) {
            nr = atoi(bufp) - 1;
            g_free(bufp);
        } else {
            nr = -1; /* selection failed */
        }
    }

    gtk_widget_destroy(mdialog);
    g_object_unref(store);

    if (nr >= 0) {
        st_widget = first_sort_tab_widget;
        while (st_widget) {
            if (nr == sort_tab_widget_get_instance(st_widget))
                break;

            st_widget = sort_tab_widget_get_next(st_widget);
        }
    }

    return st_widget;
}

/**
 * Callback for the select playlist signal
 */
void sorttab_display_select_playlist_cb(GtkPodApp *app, gpointer pl, gpointer data) {
    Playlist *new_playlist = pl;

    /* Remove all data from tab */
    sort_tab_widget_build(first_sort_tab_widget, -1);

    /* Add the tracks from the selected playlist to the sorttabs */
    if (new_playlist && new_playlist->members) {
        GList *gl;

        sort_tab_widget_set_sort_enablement(first_sort_tab_widget, FALSE);

        for (gl = new_playlist->members; gl; gl = gl->next) {
            /* add all tracks to sort tab 0 */
            Track *track = gl->data;
            sort_tab_widget_add_track(first_sort_tab_widget, track, FALSE, TRUE);
        }

        sort_tab_widget_set_sort_enablement(first_sort_tab_widget, TRUE);

        sort_tab_widget_add_track(first_sort_tab_widget, NULL, TRUE, TRUE);
    }
}

/**
 * Callback for the track removed signal
 */
void sorttab_display_track_removed_cb(GtkPodApp *app, gpointer tk, gint32 pos, gpointer data) {
    Track *old_track = tk;
    sort_tab_widget_remove_track(first_sort_tab_widget, old_track);
}

/**
 * Callback for the track updated signal
 */
void sorttab_display_track_updated_cb(GtkPodApp *app, gpointer tk, gpointer data) {
    Track *track = tk;
    sort_tab_widget_track_changed(first_sort_tab_widget, track, FALSE);
}

/**
 * Callback for the preference changed signal
 */
void sorttab_display_preference_changed_cb(GtkPodApp *app, gpointer pfname, gpointer value, gpointer data) {
    gchar *pref_name = pfname;
    if (g_str_equal(pref_name, "sort_tab_num")) {
        gint sort_tab_count = 0;
        gint pref_count = GPOINTER_TO_INT(value);

        SortTabWidget *st_widget = first_sort_tab_widget;
        while (st_widget) {
            sort_tab_count++;
            st_widget = sort_tab_widget_get_next(st_widget);
        }

        if (sort_tab_count == pref_count) {
            /* Nothing to do */
            return;
        }

        /* user increased the number of filters */
        while (sort_tab_count < pref_count) {
            sorttab_display_append_widget();
            sort_tab_count++;
        }

        while (sort_tab_count > pref_count) {
            sorttab_display_remove_widget();
            sort_tab_count--;
        }
    }
    else if (g_str_equal(pref_name, "group_compilations")) {
        sorttab_display_select_playlist_cb(gtkpod_app, gtkpod_get_current_playlist(), NULL);
    }
    else if (g_str_equal(pref_name, "st_sort")) {
        enum GtkPodSortTypes order = GPOINTER_TO_INT(value);
        sort_tab_widget_sort(first_sort_tab_widget, order);
    }
}
