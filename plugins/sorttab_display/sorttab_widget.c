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
#ifndef SORT_TAB_WIDGET_C_
#define SORT_TAB_WIDGET_C_

#include <string.h>
#include "libgtkpod/gp_private.h"
#include "libgtkpod/prefs.h"
#include "libgtkpod/gtkpod_app_iface.h"
#include "libgtkpod/misc_playlist.h"
#include "libgtkpod/misc.h"
#include "libgtkpod/misc_track.h"
#include "plugin.h"
#include "sorttab_conversion.h"
#include "normal_sorttab_page.h"
#include "special_sorttab_page.h"
#include "sorttab_widget.h"

G_DEFINE_TYPE( SortTabWidget, sort_tab_widget, GTK_TYPE_NOTEBOOK);

#define SORT_TAB_WIDGET_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), SORT_TAB_TYPE_WIDGET, SortTabWidgetPrivate))

struct _SortTabWidgetPrivate {

    /* Parent widget of the sorttabs */
    GtkWidget *parent;

    /* Glade xml file path */
    gchar *glade_xml_path;

    /* The next sort tab widget in the sequence */
    SortTabWidget *next;

    /* The previous sort tab widget in the sequence */
    SortTabWidget *prev;

    /* Instance index of this sort tab */
    guint instance;

    /* current page (category) selected) */
    guint current_category;

    /* model for use by normal pages */
    GtkTreeModel *model;

    /* have all tracks been added? */
    gboolean final;

    /* pointer to normal pages */
    NormalSortTabPage *normal_pages[ST_CAT_SPECIAL];

    /* pointer to special page */
    SpecialSortTabPage *special_page;

    /* Track number of times disable/enable function is called */
    gint disable_sort_count;

};

static void _sort_tab_widget_dispose(GObject *gobject) {
    SortTabWidget *st = SORT_TAB_WIDGET(gobject);

    SortTabWidgetPrivate *priv = st->priv;
    if (priv) {
        priv->next = NULL;
        priv->prev = NULL;
        priv->model = NULL;
        priv->parent = NULL;
        g_free(priv->glade_xml_path);
        priv->glade_xml_path = NULL;
    }

    /* call the parent class' dispose() method */
    G_OBJECT_CLASS(sort_tab_widget_parent_class)->dispose(gobject);
}

static gboolean _sort_tab_widget_page_selected_cb(gpointer data) {
    SortTabWidget *self = SORT_TAB_WIDGET(data);

    guint page;
    guint oldpage;
    gboolean is_go;
    GList *copy = NULL;

#if DEBUG_TIMING
    GTimeVal time;
    g_get_current_time (&time);
    printf ("st_page_selected_cb enter (inst: %d, page: %d): %ld.%06ld sec\n",
            inst, page,
            time.tv_sec % 3600, time.tv_usec);
#endif

    if (! SORT_TAB_IS_WIDGET(self))
        return FALSE; /* invalid notebook */

    SortTabWidgetPrivate *priv = SORT_TAB_WIDGET_GET_PRIVATE(self);
    SortTabWidget *prev = sort_tab_widget_get_previous(self);

    page = gtk_notebook_get_current_page(GTK_NOTEBOOK(self));

    /* remember old is_go state and current page */
    is_go = special_sort_tab_page_get_is_go(priv->special_page);
    oldpage = priv->current_category;

    /* re-initialize current instance */
    sort_tab_widget_build(self, page);

    /* write back old is_go state if page hasn't changed (redisplay) */
    if (page == oldpage)
        special_sort_tab_page_set_is_go(priv->special_page, is_go);

    /* Get list of tracks to re-insert */
    copy = sort_tab_widget_get_selected_tracks(prev);
    if (copy) {
        GList *gl;
        gboolean final;
        /* add all tracks previously present to sort tab */
        sort_tab_widget_set_sort_enablement(self, FALSE);
        for (gl = copy; gl; gl = gl->next) {
            Track *track = gl->data;
            sort_tab_widget_add_track(self, track, FALSE, TRUE);
        }
        sort_tab_widget_set_sort_enablement(self, TRUE);
        final = TRUE; /* playlist is always complete */

        /*
         * if playlist is not source, get final flag from
         * corresponding sorttab
         */
        if (priv->instance > 0 && prev)
            final = sort_tab_widget_is_all_tracks_added(prev);

        sort_tab_widget_add_track(self, NULL, final, TRUE);
    }
#if DEBUG_TIMING
    g_get_current_time (&time);
    printf ("st_page_selected_cb exit (inst: %d, page: %d):  %ld.%06ld sec\n",
            inst, page,
            time.tv_sec % 3600, time.tv_usec);
#endif

    return FALSE;
}

/* Called when page in sort tab is selected */
static void _sort_tab_widget_page_selected(SortTabWidget *self, guint page) {

#if DEBUG_CB_INIT
    printf ("st_page_selected: inst: %d, page: %d\n", priv->instance, page);
#endif

    g_idle_add(_sort_tab_widget_page_selected_cb, self);
}

/* callback */
static void _sort_tab_widget_on_st_switch_page(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer user_data) {
    SortTabWidget *self = SORT_TAB_WIDGET( notebook );
    _sort_tab_widget_page_selected(self, page_num);
}

/* Start sorting */
static void _sort_tab_widget_sort_internal(SortTabWidget *self, enum GtkPodSortTypes order) {
    g_return_if_fail(SORT_TAB_IS_WIDGET(self));
    SortTabWidgetPrivate *priv = SORT_TAB_WIDGET_GET_PRIVATE(self);
    NormalSortTabPage *page = priv->normal_pages[priv->current_category];

    switch (priv->current_category) {
    case ST_CAT_ARTIST:
    case ST_CAT_ALBUM:
    case ST_CAT_GENRE:
    case ST_CAT_COMPOSER:
    case ST_CAT_TITLE:
    case ST_CAT_YEAR:
        normal_sort_tab_page_sort(page, order);
        break;
    case ST_CAT_SPECIAL:
        break;
    default:
        g_return_if_reached();
    }
}

/**
 * Create the treeview for category @st_cat of instance @inst
 */
static void _sort_tab_widget_init_page(SortTabWidget *self, ST_CAT_item st_cat) {
    GtkWidget *st0_window0;
    GtkWidget *st0_label0 = NULL;

    SortTabWidgetPrivate *priv = SORT_TAB_WIDGET_GET_PRIVATE(self);

    GtkWidget *page =  NULL;
    if (st_cat != ST_CAT_SPECIAL) {
        page = normal_sort_tab_page_new(self, st_cat);
        priv->normal_pages[st_cat] = NORMAL_SORT_TAB_PAGE(page);

        /* Add scrolled window around sort tab page */
        st0_window0 = gtk_scrolled_window_new(NULL, NULL);
        gtk_container_add(GTK_CONTAINER (st0_window0), page);
    }
    else {
        /* special sort tab is a scrolled window so can add it directly */
        page = special_sort_tab_page_new(self, priv->glade_xml_path);
        priv->special_page = SPECIAL_SORT_TAB_PAGE(page);
        st0_window0 = GTK_WIDGET(page);
    }

    gtk_widget_show(st0_window0);
    gtk_container_add(GTK_CONTAINER (self), st0_window0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (st0_window0), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    switch (st_cat) {
    case ST_CAT_ARTIST:
        st0_label0 = gtk_label_new(_("Artist"));
        break;
    case ST_CAT_ALBUM:
        st0_label0 = gtk_label_new(_("Album"));
        break;
    case ST_CAT_GENRE:
        st0_label0 = gtk_label_new(_("Genre"));
        break;
    case ST_CAT_COMPOSER:
        st0_label0 = gtk_label_new(_("Comp."));
        break;
    case ST_CAT_TITLE:
        st0_label0 = gtk_label_new(_("Title"));
        break;
    case ST_CAT_YEAR:
        st0_label0 = gtk_label_new(_("Year"));
        break;
    case ST_CAT_SPECIAL:
        st0_label0 = gtk_label_new(_("Special"));
        break;
    default: /* should not happen... */
        g_return_if_reached ();
    }
    gtk_widget_show(st0_label0);
    gtk_notebook_set_tab_label(GTK_NOTEBOOK(self), gtk_notebook_get_nth_page(GTK_NOTEBOOK(self), st_cat), st0_label0);
    gtk_label_set_justify(GTK_LABEL (st0_label0), GTK_JUSTIFY_LEFT);
}

static void sort_tab_widget_class_init(SortTabWidgetClass *klass) {
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->dispose = _sort_tab_widget_dispose;

    g_type_class_add_private(klass, sizeof(SortTabWidgetPrivate));
}

static void sort_tab_widget_init(SortTabWidget *self) {
    SortTabWidgetPrivate *priv;

    priv = SORT_TAB_WIDGET_GET_PRIVATE (self);

    priv->parent = NULL;
    priv->next = NULL;
    priv->instance = -1;
    priv->current_category = -1;
    priv->final = FALSE;
    priv->disable_sort_count = 0;

    /* create model */
    priv->model = GTK_TREE_MODEL (gtk_list_store_new(ST_NUM_COLUMNS, G_TYPE_POINTER));

    /*
     * Set this notebook scrollable so that shrinking it
     * will compressing rather than hide it.
     */
    gtk_notebook_set_scrollable(GTK_NOTEBOOK (self), TRUE);

    gtk_widget_show(GTK_WIDGET(self));

    /* Initialise the notebook switch page callback */
    g_signal_connect((gpointer) self, "switch_page",
            G_CALLBACK (_sort_tab_widget_on_st_switch_page), NULL);
}

gint sort_tab_widget_get_max_index() {
    gint sort_tab_total = 0;

    if (!prefs_get_int_value("sort_tab_num", &sort_tab_total))
        sort_tab_total = SORT_TAB_MAX;
    else {
        /*
         * since tab filters index down to 0, we want
         * one less than the preference
         */
        sort_tab_total--;
    }

    return sort_tab_total;
}

SortTabWidget *sort_tab_widget_new(gint inst, GtkWidget *parent, gchar *glade_xml_path) {
    g_return_val_if_fail(parent, NULL);

    SortTabWidget *st = g_object_new(SORT_TAB_TYPE_WIDGET, NULL);
    SortTabWidgetPrivate *priv = SORT_TAB_WIDGET_GET_PRIVATE (st);
    gint page;

    priv->parent = parent;
    priv->instance = inst;
    priv->glade_xml_path = g_strdup(glade_xml_path);

    /*
     * Initialise the notebook pages. This has to be done here rather than
     * in init() since the glade_xml file path is required to be set
     */
    for (gint i = 0; i < ST_CAT_NUM; ++i) {
        ST_CAT_item category = (ST_CAT_item) i;
        _sort_tab_widget_init_page(st, category);
    }

    /* Set the page of the notebook to the current category */
    page = prefs_get_int_index("st_category", priv->instance);
    priv->current_category = page;
    gtk_notebook_set_current_page(GTK_NOTEBOOK(st), page);

    /* Sort the tree views according to the preferences */
    if (prefs_get_int("st_sort") != SORT_NONE)
        _sort_tab_widget_sort_internal(st, prefs_get_int("st_sort"));

    return st;
}

GtkWidget *sort_tab_widget_get_parent(SortTabWidget *self) {
    g_return_val_if_fail(self, NULL);

    SortTabWidgetPrivate *priv = SORT_TAB_WIDGET_GET_PRIVATE(self);

    return priv->parent;
}

void sort_tab_widget_set_parent(SortTabWidget *self, GtkWidget *parent) {
    g_return_if_fail(self);

    SortTabWidgetPrivate *priv = SORT_TAB_WIDGET_GET_PRIVATE(self);
    priv->parent = parent;
}

gchar *sort_tab_widget_get_glade_path(SortTabWidget *self) {
    g_return_val_if_fail(self, NULL);

    SortTabWidgetPrivate *priv = SORT_TAB_WIDGET_GET_PRIVATE(self);

    return priv->glade_xml_path;
}

guint sort_tab_widget_get_instance(SortTabWidget *self) {
    g_return_val_if_fail(self, -1);

    SortTabWidgetPrivate *priv = SORT_TAB_WIDGET_GET_PRIVATE(self);

    return priv->instance;
}

guint sort_tab_widget_get_category(SortTabWidget *self) {
    g_return_val_if_fail(self, -1);

    SortTabWidgetPrivate *priv = SORT_TAB_WIDGET_GET_PRIVATE(self);

    return priv->current_category;
}

void sort_tab_widget_set_category(SortTabWidget *self, guint new_category) {
    g_return_if_fail(self);

    SortTabWidgetPrivate *priv = SORT_TAB_WIDGET_GET_PRIVATE(self);
    priv->current_category = new_category;

    prefs_set_int_index("st_category", priv->instance, new_category);
}

gboolean sort_tab_widget_is_all_tracks_added(SortTabWidget *self) {
    g_return_val_if_fail(self, FALSE);

    SortTabWidgetPrivate *priv = SORT_TAB_WIDGET_GET_PRIVATE(self);

    return priv->final;
}

void sort_tab_widget_set_all_tracks_added(SortTabWidget *self, gboolean status) {
    g_return_if_fail(self);

    SortTabWidgetPrivate *priv = SORT_TAB_WIDGET_GET_PRIVATE(self);
    priv->final = status;
}

GList *sort_tab_widget_get_selected_tracks(SortTabWidget *self) {
    GList *tracks = NULL;
    if (!SORT_TAB_IS_WIDGET(self)) {
        Playlist *pl = gtkpod_get_current_playlist();
        if (pl)
            tracks = pl->members;
    }
    else {
        SortTabWidgetPrivate *priv = SORT_TAB_WIDGET_GET_PRIVATE(self);
        NormalSortTabPage *current_page;

        switch (sort_tab_widget_get_category(self)) {
        case ST_CAT_ARTIST:
        case ST_CAT_ALBUM:
        case ST_CAT_GENRE:
        case ST_CAT_COMPOSER:
        case ST_CAT_TITLE:
        case ST_CAT_YEAR:
            current_page = priv->normal_pages[priv->current_category];
            tracks = normal_sort_tab_page_get_selected_tracks(current_page);
            break;
        case ST_CAT_SPECIAL:
            tracks = special_sort_tab_page_get_selected_tracks(priv->special_page);
            break;
        }
    }

    return tracks;
}

GtkTreeModel *sort_tab_widget_get_normal_model(SortTabWidget *self) {
    g_return_val_if_fail(self, NULL);

    SortTabWidgetPrivate *priv = SORT_TAB_WIDGET_GET_PRIVATE(self);
    return priv->model;
}

SortTabWidget *sort_tab_widget_get_next(SortTabWidget *self) {
    g_return_val_if_fail(self, NULL);

    SortTabWidgetPrivate *priv = SORT_TAB_WIDGET_GET_PRIVATE(self);

    return priv->next;
}

void sort_tab_widget_set_next(SortTabWidget *self, SortTabWidget *next) {
    g_return_if_fail(self);

    SortTabWidgetPrivate *priv = SORT_TAB_WIDGET_GET_PRIVATE(self);
    priv->next = next;
}

SortTabWidget *sort_tab_widget_get_previous(SortTabWidget *self) {
    g_return_val_if_fail(self, NULL);

    SortTabWidgetPrivate *priv = SORT_TAB_WIDGET_GET_PRIVATE(self);

    return priv->prev;
}

void sort_tab_widget_set_previous(SortTabWidget *self, SortTabWidget *prev) {
    g_return_if_fail(self);

    SortTabWidgetPrivate *priv = SORT_TAB_WIDGET_GET_PRIVATE(self);
    priv->prev = prev;
}

/**
 * Disable sorting of the view during lengthy updates.
 *
 * @enable: TRUE: enable, FALSE: disable
 */
void sort_tab_widget_set_sort_enablement(SortTabWidget *self, gboolean enable) {
    if (! SORT_TAB_IS_WIDGET(self)) {
        gtkpod_set_sort_enablement(enable);
        return;
    }

    SortTabWidgetPrivate *priv = SORT_TAB_WIDGET_GET_PRIVATE(self);
    GtkTreeModel *model = priv->model;
    SortTabWidget *next = sort_tab_widget_get_next(self);

    if (enable) {
        priv->disable_sort_count--;
        if (priv->disable_sort_count < 0)
            fprintf(stderr, "Programming error: disable_count < 0\n");
        if (priv->disable_sort_count == 0) {
            /* Re-enable sorting */
            if (prefs_get_int("st_sort") != SORT_NONE) {
                if (sort_tab_widget_get_category(self) != ST_CAT_SPECIAL && model) {
                    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE (model), ST_COLUMN_ENTRY, prefs_get_int("st_sort"));
                }
            }
            sort_tab_widget_set_sort_enablement(next, enable);
        }
    }
    else {
        if (priv->disable_sort_count == 0) {
            /* Disable sorting */
            if (prefs_get_int("st_sort") != SORT_NONE) {
                if (sort_tab_widget_get_category(self) != ST_CAT_SPECIAL && model) {
                    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE (model), GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, prefs_get_int("st_sort"));
                }
            }
            sort_tab_widget_set_sort_enablement(next, enable);
        }
        priv->disable_sort_count++;
    }
}

void sort_tab_widget_sort(SortTabWidget *self, enum GtkPodSortTypes order) {
    if (! SORT_TAB_IS_WIDGET(self)) {
        return;
    }

    SortTabWidgetPrivate *priv = SORT_TAB_WIDGET_GET_PRIVATE(self);
    _sort_tab_widget_sort_internal(self, order);

    sort_tab_widget_sort(priv->next, order);
}

/**
 * Add track to sort tab. If the track matches the currently
 * selected sort criteria, it will be passed on to the next
 * sort tab. The last sort tab will pass the track on to the
 * track model (currently two sort tabs).
 * When the first track is added, the "All" entry is created.
 * If prefs_get_int_index("st_autoselect", inst) is true, the "All" entry is
 * automatically selected, if there was no former selection
 *
 * @display: TRUE: add to track model (i.e. display it)
 */
void sort_tab_widget_add_track(SortTabWidget *self, Track *track, gboolean final, gboolean display) {
#if DEBUG_ADD_TRACK
    printf ("st_add_track: inst: %p, final: %d, display: %d, track: %p\n",
            self, final, display, track);
#endif

    if (! SORT_TAB_IS_WIDGET(self)) {
        /* just add to track model */
        if (final)
            gtkpod_tracks_statusbar_update();

        return;
    }

    SortTabWidgetPrivate *priv = SORT_TAB_WIDGET_GET_PRIVATE(self);
    NormalSortTabPage *normal_page;

    switch (sort_tab_widget_get_category(self)) {
    case ST_CAT_ARTIST:
    case ST_CAT_ALBUM:
    case ST_CAT_GENRE:
    case ST_CAT_COMPOSER:
    case ST_CAT_TITLE:
    case ST_CAT_YEAR:
        normal_page = priv->normal_pages[priv->current_category];
        normal_sort_tab_page_add_track(normal_page, track, final, display);
        break;
    case ST_CAT_SPECIAL:
        special_sort_tab_page_add_track(priv->special_page, track, final, display);
        break;
    default:
        g_return_if_reached();
    }
}

void sort_tab_widget_remove_track(SortTabWidget *self, Track *track) {
    if (!SORT_TAB_IS_WIDGET(self))
        return;

    SortTabWidgetPrivate *priv = SORT_TAB_WIDGET_GET_PRIVATE(self);
    NormalSortTabPage *normal_page;

    switch (priv->current_category) {
    case ST_CAT_ARTIST:
    case ST_CAT_ALBUM:
    case ST_CAT_GENRE:
    case ST_CAT_COMPOSER:
    case ST_CAT_TITLE:
    case ST_CAT_YEAR:
        normal_page = priv->normal_pages[priv->current_category];
        normal_sort_tab_page_remove_track(normal_page, track);
        break;
    case ST_CAT_SPECIAL:
        special_sort_tab_page_remove_track(priv->special_page, track);
        break;
    default:
        g_return_if_reached();
    }
}

void sort_tab_widget_track_changed(SortTabWidget *self, Track *track, gboolean removed) {
    if (!SORT_TAB_IS_WIDGET(self))
        return;

    SortTabWidgetPrivate *priv = SORT_TAB_WIDGET_GET_PRIVATE(self);
    NormalSortTabPage *normal_page;

    switch (priv->current_category) {
    case ST_CAT_ARTIST:
    case ST_CAT_ALBUM:
    case ST_CAT_GENRE:
    case ST_CAT_COMPOSER:
    case ST_CAT_TITLE:
    case ST_CAT_YEAR:
        normal_page = priv->normal_pages[priv->current_category];
        normal_sort_tab_page_track_changed(normal_page, track, removed);
        break;
    case ST_CAT_SPECIAL:
        special_sort_tab_page_track_changed(priv->special_page, track, removed);
        break;
    default:
        g_return_if_reached();
    }
}

/**
 * Build a sort tab: all current entries are removed. The next sort tab
 * is initialized as well (st_init (-1, inst+1)).
 *
 * Set new_category to -1 if the current category is to be
 * left unchanged.
 *
 * Normally we do not specifically remember the "All" entry and will
 * select "All" in accordance to the prefs settings.
 */
void sort_tab_widget_build(SortTabWidget *self, ST_CAT_item new_category) {
    if (! SORT_TAB_IS_WIDGET(self)) {
        gtkpod_tracks_statusbar_update();
        return;
    }

    gint current_category = sort_tab_widget_get_category(self);
    SortTabWidgetPrivate *priv = SORT_TAB_WIDGET_GET_PRIVATE(self);
    NormalSortTabPage *current_page;

    priv->final = TRUE; /* all tracks are added */

    switch (current_category) {
    case ST_CAT_ARTIST:
    case ST_CAT_ALBUM:
    case ST_CAT_GENRE:
    case ST_CAT_COMPOSER:
    case ST_CAT_TITLE:
    case ST_CAT_YEAR:
        current_page = priv->normal_pages[current_category];
        /* nothing was unselected so far */
        normal_sort_tab_page_set_unselected(current_page, FALSE);
        normal_sort_tab_page_clear(current_page);
        break;
    case ST_CAT_SPECIAL:
        /* store sp entries (if applicable) */
        special_sort_tab_page_store_state(priv->special_page);
        /* did not press "Display" yet (special) */
        special_sort_tab_page_set_is_go(priv->special_page, FALSE);
        special_sort_tab_page_clear(priv->special_page);
        break;
    default:
        g_return_if_reached();
    }

    if (new_category != -1) {
        sort_tab_widget_set_category(self, new_category);
    }

    sort_tab_widget_build(priv->next, -1);
}

void sort_tab_widget_refresh(SortTabWidget *self) {
    g_return_if_fail(self);

    _sort_tab_widget_page_selected(self, sort_tab_widget_get_instance(self));
}

void sort_tab_widget_delete_entry_head(SortTabWidget *self, DeleteAction deleteaction) {
    struct DeleteData *dd;
    Playlist *pl;
    GString *str;
    gchar *label = NULL, *title = NULL;
    gboolean confirm_again;
    gchar *confirm_again_key;
    GtkResponseType response;
    iTunesDB *itdb;
    GList *tracks = NULL;
    GList *selected_tracks = NULL;

    g_return_if_fail(SORT_TAB_IS_WIDGET(self));

    pl = gtkpod_get_current_playlist();
    if (!pl) {
        /* no playlist??? Cannot happen, but... */
        message_sb_no_playlist_selected();
        return;
    }

    itdb = pl->itdb;
    g_return_if_fail (itdb);

    tracks = sort_tab_widget_get_selected_tracks(self);
    if (!tracks) {
        /* no tracks selected */
        gtkpod_statusbar_message(_("No tracks selected."));
        return;
    }

    selected_tracks = g_list_copy(tracks);

    dd = g_malloc0(sizeof(struct DeleteData));
    dd->deleteaction = deleteaction;
    dd->tracks = selected_tracks;
    dd->pl = pl;
    dd->itdb = itdb;

    delete_populate_settings(dd, &label, &title, &confirm_again, &confirm_again_key, &str);

    /* open window */
    response = gtkpod_confirmation(-1, /* gint id, */
            TRUE, /* gboolean modal, */
            title, /* title */
            label, /* label */
            str->str, /* scrolled text */
            NULL, 0, NULL, /* option 1 */
            NULL, 0, NULL, /* option 2 */
            confirm_again, /* gboolean confirm_again, */
            confirm_again_key,/* ConfHandlerOpt confirm_again_key,*/
            CONF_NULL_HANDLER, /* ConfHandler ok_handler,*/
            NULL, /* don't show "Apply" button */
            CONF_NULL_HANDLER, /* cancel_handler,*/
            NULL, /* gpointer user_data1,*/
            NULL); /* gpointer user_data2,*/

    switch (response) {
    case GTK_RESPONSE_OK:
        /* Delete the tracks */
        delete_track_ok(dd);
        break;
    default:
        delete_track_cancel(dd);
        break;
    }

    g_free(label);
    g_free(title);
    g_free(confirm_again_key);
    g_string_free(str, TRUE);
}

/**
 * Stop editing. If @cancel is TRUE, the edited value will be
 * discarded
 * (I have the feeling that the "discarding" part does not
 * work quite the way intended).
 */
void sort_tab_widget_stop_editing(SortTabWidget *self, gboolean cancel) {
    g_return_if_fail(SORT_TAB_IS_WIDGET(self));

    SortTabWidgetPrivate *priv = SORT_TAB_WIDGET_GET_PRIVATE(self);
    NormalSortTabPage *normal_page;

    switch (sort_tab_widget_get_category(self)) {
    case ST_CAT_ARTIST:
    case ST_CAT_ALBUM:
    case ST_CAT_GENRE:
    case ST_CAT_COMPOSER:
    case ST_CAT_TITLE:
    case ST_CAT_YEAR:
        normal_page = priv->normal_pages[priv->current_category];
        normal_sort_tab_page_stop_editing(normal_page, cancel);
        break;
    default:
        break;
    }
}

#endif /* SORT_TAB_WIDGET_C_ */

