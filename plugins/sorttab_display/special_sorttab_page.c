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
#ifndef SPECIAL_SORTTAB_C_
#define SPECIAL_SORTTAB_C_

#include <string.h>
#include "libgtkpod/misc.h"
#include "libgtkpod/misc_track.h"
#include "libgtkpod/prefs.h"
#include "plugin.h"
#include "sorttab_conversion.h"
#include "special_sorttab_page.h"
#include "special_sorttab_page_calendar.h"

G_DEFINE_TYPE( SpecialSortTabPage, special_sort_tab_page, GTK_TYPE_SCROLLED_WINDOW);

#define SPECIAL_SORT_TAB_PAGE_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), SPECIAL_SORT_TAB_TYPE_PAGE, SpecialSortTabPagePrivate))

struct _SpecialSortTabPagePrivate {

    /* path to glade xml */
    gchar *glade_xml;

    /* Parent Sort Tab Widget */
    SortTabWidget *st_widget_parent;

    /* list of tracks in sorttab */
    GList *sp_members;

    /* list of tracks selected */
    GList *sp_selected;

    /* pass new members on automatically */
    gboolean pass_on_new_members;

    /* TimeInfo "added" (sp)         */
    TimeInfo ti_added;

    /* TimeInfo "modified" (sp)      */
    TimeInfo ti_modified;

    /* TimeInfo "played" (sp)        */
    TimeInfo ti_played;
};

typedef enum {
    /* track's timestamp is inside the specified interval  */
    IS_INSIDE,
    /* track's timestamp is outside the specified interval */
    IS_OUTSIDE,
    /* error parsing date string (or wrong parameters) */
    IS_ERROR,
} IntervalState;

typedef struct {

    SpecialSortTabPage *page;

    gpointer data;

} PageData;

/**
 * Wrap a special sort tab page with
 * extra data.
 */
static PageData *_create_page_data(SpecialSortTabPage *self) {
    PageData *data = NULL;

    data = g_new0 (PageData, 1);
    data->page = self;

    return data;
}

static guint32 _get_sort_tab_widget_instance(SpecialSortTabPage *self) {
    SpecialSortTabPagePrivate *priv = SPECIAL_SORT_TAB_PAGE_GET_PRIVATE(self);
    return sort_tab_widget_get_instance(priv->st_widget_parent);
}

/**
 * Called when the user changed the sort conditions in the special
 * sort tab
 */
static void _sp_conditions_changed(SpecialSortTabPage *self) {
    g_return_if_fail(SPECIAL_SORT_TAB_IS_PAGE(self));
    SpecialSortTabPagePrivate *priv = SPECIAL_SORT_TAB_PAGE_GET_PRIVATE(self);
    guint32 inst = _get_sort_tab_widget_instance(self);

    /* Only redisplay if data is actually being passed on to the next
     sort tab */
    if (priv->pass_on_new_members || prefs_get_int_index("sp_autodisplay", inst)) {
        sort_tab_widget_refresh(priv->st_widget_parent);
    }
}

/**
 * Set rating  for tab @inst and rating @n
 */
static void _set_sp_rating_n(SpecialSortTabPage *self, gint n, gboolean state) {
    guint32 rating;
    guint32 inst = _get_sort_tab_widget_instance(self);

    if (SPECIAL_SORT_TAB_IS_PAGE(self) && (n <= RATING_MAX)) {
        rating = (guint32) prefs_get_int_index("sp_rating_state", inst);

        if (state)
            rating |= (1 << n);
        else
            rating &= ~(1 << n);

        prefs_set_int_index("sp_rating_state", inst, rating);
    }
}

static gboolean _get_sp_rating_n(SpecialSortTabPage *self, gint n) {
    guint32 rating;
    guint32 inst = _get_sort_tab_widget_instance(self);

    if (SPECIAL_SORT_TAB_IS_PAGE(self) && (n <= RATING_MAX)) {
        rating = (guint32) prefs_get_int_index("sp_rating_state", inst);

        if ((rating & (1 << n)) != 0)
            return TRUE;
        else
            return FALSE;
    }

    return FALSE;
}

/*
 * Check if @track's timestamp is within the interval given for @item.
 *
 * Return value:
 *
 * IS_ERROR:   error parsing date string (or wrong parameters)
 * IS_INSIDE:  track's timestamp is inside the specified interval
 * IS_OUTSIDE: track's timestamp is outside the specified interval
 */
static IntervalState _sp_check_time(SpecialSortTabPage *self, T_item item, Track *track) {
    TimeInfo *ti;
    IntervalState result = IS_ERROR;

    ti = special_sort_tab_page_update_date_interval(self, item, FALSE);
    if (ti && ti->valid) {
        guint32 stamp = track_get_timestamp(track, item);
        if (stamp && (ti->lower <= stamp) && (stamp <= ti->upper))
            result = IS_INSIDE;
        else
            result = IS_OUTSIDE;
    }
    if (result == IS_ERROR) {
        switch (item) {
        case T_TIME_PLAYED:
            gtkpod_statusbar_message(_("'Played' condition ignored because of error."));
            break;
        case T_TIME_MODIFIED:
            gtkpod_statusbar_message(_("'Modified' condition ignored because of error."));
            break;
        case T_TIME_ADDED:
            gtkpod_statusbar_message(_("'Added' condition ignored because of error."));
            break;
        default:
            break;
        }
    }
    return result;
}

/**
 * Decide whether or not @track satisfies the conditions specified in
 * the special sort tab of instance @inst.
 *
 * Return value:  TRUE: satisfies, FALSE: does not satisfy
 */
static gboolean _sp_check_track(SpecialSortTabPage *self, Track *track) {
    guint32 inst = _get_sort_tab_widget_instance(self);
    gboolean sp_or = prefs_get_int_index("sp_or", inst);
    gboolean result, cond, checked = FALSE;

    if (!track)
        return FALSE;

    /* Initial state depends on logical operation */
    if (sp_or)
        result = FALSE; /* OR  */
    else
        result = TRUE; /* AND */

    /* RATING */
    if (prefs_get_int_index("sp_rating_cond", inst)) {
        /* checked = TRUE: at least one condition was checked */
        checked = TRUE;
        cond = _get_sp_rating_n(self, track->rating / ITDB_RATING_STEP);
        /* If one of the two combinations occur, we can take a
         shortcut and stop checking the other conditions */
        if (sp_or && cond)
            return TRUE;
        if ((!sp_or) && (!cond))
            return FALSE;
        /* We don't have to calculate a new 'result' value because for
         the other two combinations it does not change */
    }

    /* PLAYCOUNT */
    if (prefs_get_int_index("sp_playcount_cond", inst)) {
        guint32 low = prefs_get_int_index("sp_playcount_low", inst);
        /* "-1" will translate into about 4 billion because I use
         guint32 instead of gint32. Since 4 billion means "no upper
         limit" the logic works fine */
        guint32 high = prefs_get_int_index("sp_playcount_high", inst);
        checked = TRUE;
        if ((low <= track->playcount) && (track->playcount <= high))
            cond = TRUE;
        else
            cond = FALSE;
        if (sp_or && cond)
            return TRUE;
        if ((!sp_or) && (!cond))
            return FALSE;
    }
    /* time played */
    if (prefs_get_int_index("sp_played_cond", inst)) {
        IntervalState result = _sp_check_time(self, T_TIME_PLAYED, track);
        if (sp_or && (result == IS_INSIDE))
            return TRUE;
        if ((!sp_or) && (result == IS_OUTSIDE))
            return FALSE;
        if (result != IS_ERROR)
            checked = TRUE;
    }
    /* time modified */
    if (prefs_get_int_index("sp_modified_cond", inst)) {
        IntervalState result = _sp_check_time(self, T_TIME_MODIFIED, track);
        if (sp_or && (result == IS_INSIDE))
            return TRUE;
        if ((!sp_or) && (result == IS_OUTSIDE))
            return FALSE;
        if (result != IS_ERROR)
            checked = TRUE;
    }
    /* time added */
    if (prefs_get_int_index("sp_added_cond", inst)) {
        IntervalState result = _sp_check_time(self, T_TIME_ADDED, track);
        g_message("time added result %d for track %s", result, track->title);
        if (sp_or && (result == IS_INSIDE))
            return TRUE;
        if ((!sp_or) && (result == IS_OUTSIDE))
            return FALSE;
        if (result != IS_ERROR)
            checked = TRUE;
    }

    g_message("Returning %d (checked %d) for track %s", result, checked, track->title);
    if (checked)
        return result;
    else
        return FALSE;
}

/**
 * Display the members satisfying the conditions specified in the
 * special sort tab of instance @inst
 */
void _sp_go(SpecialSortTabPage *self) {
    g_return_if_fail(SPECIAL_SORT_TAB_IS_PAGE(self));

    SpecialSortTabPagePrivate *priv = SPECIAL_SORT_TAB_PAGE_GET_PRIVATE(self);
    SortTabWidget *next = sort_tab_widget_get_next(priv->st_widget_parent);

#if DEBUG_CB_INIT
    printf ("st_go: inst: %d\n", inst);
#endif

    /* Make sure the information typed into the entries is actually
     * being used (maybe the user 'forgot' to press enter */
    special_sort_tab_page_store_state(self);

#if DEBUG_TIMING
    GTimeVal time;
    g_get_current_time (&time);
    printf ("sp_go enter: %ld.%06ld sec\n",
            time.tv_sec % 3600, time.tv_usec);
#endif

    /* remember that "Display" was already pressed */
    priv->pass_on_new_members = TRUE;

    /* Clear the sp_selected list */
    g_list_free(priv->sp_selected);
    priv->sp_selected = NULL;

    /* initialize next instance */
    sort_tab_widget_build(next, -1);

    if (priv->sp_members) {
        GList *gl;

        sort_tab_widget_set_sort_enablement(priv->st_widget_parent, FALSE);

        for (gl = priv->sp_members; gl; gl = gl->next) {
            /* add all member tracks to next instance */
            Track *track = (Track *) gl->data;
            if (_sp_check_track(self, track)) {
                priv->sp_selected = g_list_append(priv->sp_selected, track);
                sort_tab_widget_add_track(next, track, FALSE, TRUE);
            }
        }
        gtkpod_set_displayed_tracks(priv->sp_members);

        sort_tab_widget_set_sort_enablement(priv->st_widget_parent, TRUE);
        sort_tab_widget_add_track(next, NULL, TRUE, sort_tab_widget_is_all_tracks_added(priv->st_widget_parent));
    }

    gtkpod_tracks_statusbar_update();
#if DEBUG_TIMING
    g_get_current_time (&time);
    printf ("st_selection_changed_cb exit:  %ld.%06ld sec\n",
            time.tv_sec % 3600, time.tv_usec);
#endif
}

static void _on_sp_or_button_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
    SpecialSortTabPage *self = SPECIAL_SORT_TAB_PAGE(user_data);
    guint32 inst = _get_sort_tab_widget_instance(self);

    prefs_set_int_index("sp_or", inst, gtk_toggle_button_get_active(togglebutton));
    _sp_conditions_changed(self);
}

static void _on_sp_cond_button_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
    PageData *pagedata = (PageData *) user_data;
    guint32 inst = _get_sort_tab_widget_instance(pagedata->page);
    T_item cond = (guint32) GPOINTER_TO_UINT(pagedata->data);

    g_message("value of cond: %d", cond);
    switch (cond) {
    case T_RATING:
        prefs_set_int_index("sp_rating_cond", inst, gtk_toggle_button_get_active(togglebutton));
        break;
    case T_PLAYCOUNT:
        prefs_set_int_index("sp_playcount_cond", inst, gtk_toggle_button_get_active(togglebutton));
        break;
    case T_TIME_PLAYED:
        prefs_set_int_index("sp_played_cond", inst, gtk_toggle_button_get_active(togglebutton));
        break;
    case T_TIME_MODIFIED:
        prefs_set_int_index("sp_modified_cond", inst, gtk_toggle_button_get_active(togglebutton));
        break;
    case T_TIME_ADDED:
        prefs_set_int_index("sp_added_cond", inst, gtk_toggle_button_get_active(togglebutton));
        break;
    default:
        break;
    }
    _sp_conditions_changed(pagedata->page);
}

static void _on_sp_rating_n_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
    PageData *pagedata = (PageData *) user_data;
    guint32 inst = _get_sort_tab_widget_instance(pagedata->page);
    guint32 n = (guint32) GPOINTER_TO_UINT(pagedata->data);

    _set_sp_rating_n(pagedata->page, n, gtk_toggle_button_get_active(togglebutton));
    if (prefs_get_int_index("sp_rating_cond", inst))
        _sp_conditions_changed(pagedata->page);
}

static void _on_sp_entry_activate(GtkEditable *editable, gpointer user_data) {
    PageData *pagedata = (PageData *) user_data;
    guint32 inst = _get_sort_tab_widget_instance(pagedata->page);
    T_item item = (guint32) GPOINTER_TO_UINT(pagedata->data);
    gchar *buf = gtk_editable_get_chars(editable, 0, -1);

    switch (item) {
    case T_TIME_PLAYED:
        prefs_set_string_index("sp_played_state", inst, buf);
        break;
    case T_TIME_MODIFIED:
        prefs_set_string_index("sp_modified_state", inst, buf);
        break;
    case T_TIME_ADDED:
        prefs_set_string_index("sp_added_state", inst, buf);
        break;
    default:
        break;
    }

    g_free(buf);
    special_sort_tab_page_update_date_interval(pagedata->page, item, TRUE);
    _sp_go(pagedata->page);
}

static void _on_sp_cal_button_clicked(GtkButton *button, gpointer user_data) {
    PageData *pagedata = (PageData *) user_data;
    T_item item = (guint32) GPOINTER_TO_UINT(pagedata->data);

    cal_open_calendar(pagedata->page, item);
}

static void _on_sp_go_clicked(GtkButton *button, gpointer user_data) {
    SpecialSortTabPage *self = SPECIAL_SORT_TAB_PAGE(user_data);
    _sp_go(self);
}

static void _on_sp_go_always_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
    SpecialSortTabPage *self = SPECIAL_SORT_TAB_PAGE(user_data);
    guint32 inst = _get_sort_tab_widget_instance(self);
    gboolean state = gtk_toggle_button_get_active(togglebutton);

    /* display data if autodisplay is turned on */
    if (state)
        _on_sp_go_clicked(NULL, self);

    prefs_set_int_index("sp_autodisplay", inst, state);
}

static void _on_sp_playcount_low_value_changed(GtkSpinButton *spinbutton, gpointer user_data) {
    PageData *pagedata = (PageData *) user_data;
    guint32 inst = _get_sort_tab_widget_instance(pagedata->page);

    prefs_set_int_index("sp_playcount_low", inst, gtk_spin_button_get_value(spinbutton));
    if (prefs_get_int_index("sp_playcount_cond", inst))
        _sp_conditions_changed(pagedata->page);
}

static void _on_sp_playcount_high_value_changed(GtkSpinButton *spinbutton, gpointer user_data) {
    PageData *pagedata = (PageData *) user_data;
    guint32 inst = _get_sort_tab_widget_instance(pagedata->page);

    prefs_set_int_index("sp_playcount_high", inst, gtk_spin_button_get_value(spinbutton));
    if (prefs_get_int_index("sp_playcount_cond", inst))
        _sp_conditions_changed(pagedata->page);
}

static void special_sort_tab_page_dispose(GObject *gobject) {
    /* call the parent class' dispose() method */
    G_OBJECT_CLASS(special_sort_tab_page_parent_class)->dispose(gobject);
}

static void special_sort_tab_page_class_init(SpecialSortTabPageClass *klass) {
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->dispose = special_sort_tab_page_dispose;

    g_type_class_add_private(klass, sizeof(SpecialSortTabPagePrivate));
}

static void special_sort_tab_page_init(SpecialSortTabPage *self) {
    SpecialSortTabPagePrivate *priv = SPECIAL_SORT_TAB_PAGE_GET_PRIVATE(self);

    priv->pass_on_new_members = FALSE;
    priv->sp_members = NULL;
    priv->sp_selected = NULL;
    priv->st_widget_parent = NULL;

}

GtkWidget *special_sort_tab_page_new(SortTabWidget *st_widget_parent, gchar *glade_file_path) {
    GtkWidget *special;
    GtkWidget *viewport;
    GtkWidget *w;
    gint i;
    GtkBuilder *special_xml;
    gchar *buf;
    guint32 inst;
    PageData *userdata;

    SpecialSortTabPage *sst = g_object_new(SPECIAL_SORT_TAB_TYPE_PAGE, NULL);
    SpecialSortTabPagePrivate *priv = SPECIAL_SORT_TAB_PAGE_GET_PRIVATE(sst);

    priv->st_widget_parent = st_widget_parent;
    priv->glade_xml = glade_file_path;

    inst = sort_tab_widget_get_instance(priv->st_widget_parent);

    special_xml = gtkpod_builder_xml_new(glade_file_path);
    special = gtkpod_builder_xml_get_widget(special_xml, "special_sorttab");
    viewport = gtkpod_builder_xml_get_widget(special_xml, "special_viewport");

    /* according to GTK FAQ: move a widget to a new parent */
    g_object_ref(viewport);
    gtk_container_remove(GTK_CONTAINER (special), viewport);
    gtk_container_add(GTK_CONTAINER (sst), viewport);
    g_object_unref(viewport);

    /*
     * Connect the signal handlers and set default value.
     * AND/OR button
     */
    w = gtkpod_builder_xml_get_widget(special_xml, "sp_or_button");
    g_signal_connect ((gpointer)w,
            "toggled", G_CALLBACK (_on_sp_or_button_toggled),
            sst);

    if (prefs_get_int_index("sp_or", inst))
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
    else {
        w = gtkpod_builder_xml_get_widget(special_xml, "sp_and_button");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
    }

    /* RATING */
    w = gtkpod_builder_xml_get_widget(special_xml, "sp_rating_button");
    userdata = _create_page_data(sst);
    userdata->data = GUINT_TO_POINTER(T_RATING);
    g_signal_connect ((gpointer)w,
            "toggled", G_CALLBACK (_on_sp_cond_button_toggled),
            userdata);

    gtk_toggle_button_set_active(
            GTK_TOGGLE_BUTTON(w),
            prefs_get_int_index("sp_rating_cond", inst));

    for (i = 0; i <= RATING_MAX; ++i) {
        gchar *buf = g_strdup_printf("sp_rating%d", i);
        w = gtkpod_builder_xml_get_widget(special_xml, buf);
        userdata = _create_page_data(sst);
        userdata->data = GUINT_TO_POINTER(i);

        g_signal_connect ((gpointer)w,
                "toggled", G_CALLBACK (_on_sp_rating_n_toggled),
                userdata);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), _get_sp_rating_n(sst, i));
        g_free(buf);
    }

    /* PLAYCOUNT */
    w = gtkpod_builder_xml_get_widget(special_xml, "sp_playcount_button");
    userdata = _create_page_data(sst);
    userdata->data = GUINT_TO_POINTER(T_PLAYCOUNT);

    g_signal_connect ((gpointer)w,
            "toggled", G_CALLBACK (_on_sp_cond_button_toggled),
            userdata);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
            prefs_get_int_index("sp_playcound_cond",
            inst));

    w = gtkpod_builder_xml_get_widget(special_xml, "sp_playcount_low");
    gtk_spin_button_set_value(GTK_SPIN_BUTTON (w),
                prefs_get_int_index("sp_playcount_low", inst));
    g_signal_connect ((gpointer)w,
            "value_changed",
            G_CALLBACK (_on_sp_playcount_low_value_changed),
            sst);

    w = gtkpod_builder_xml_get_widget(special_xml, "sp_playcount_high");
    gtk_spin_button_set_value(GTK_SPIN_BUTTON (w),
                prefs_get_int_index("sp_playcount_high", inst));
    g_signal_connect ((gpointer)w,
            "value_changed",
            G_CALLBACK (_on_sp_playcount_high_value_changed),
            sst);

    /* PLAYED */
    buf = prefs_get_string_index("sp_played_state", inst);

    w = gtkpod_builder_xml_get_widget(special_xml, "sp_played_button");
    userdata = _create_page_data(sst);
    userdata->data = GUINT_TO_POINTER(T_TIME_PLAYED);

    priv->ti_played.active = w;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
            prefs_get_int_index("sp_played_cond", inst));
    g_signal_connect ((gpointer)w,
            "toggled", G_CALLBACK (_on_sp_cond_button_toggled),
            userdata);

    w = gtkpod_builder_xml_get_widget(special_xml, "sp_played_entry");

    priv->ti_played.entry = w;
    gtk_entry_set_text(GTK_ENTRY (w), buf);
    g_signal_connect ((gpointer)w,
            "activate", G_CALLBACK (_on_sp_entry_activate),
            userdata);


    g_signal_connect ((gpointer)gtkpod_builder_xml_get_widget (special_xml,
            "sp_played_cal_button"),
            "clicked",
            G_CALLBACK (_on_sp_cal_button_clicked),
            userdata);
    g_free(buf);

    /* MODIFIED */
    buf = prefs_get_string_index("sp_modified_state", inst);

    w = gtkpod_builder_xml_get_widget(special_xml, "sp_modified_button");
    priv->ti_modified.active = w;
    userdata = _create_page_data(sst);
    userdata->data = GUINT_TO_POINTER(T_TIME_MODIFIED);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
            prefs_get_int_index("sp_modified_cond", inst));
    g_signal_connect ((gpointer)w,
            "toggled", G_CALLBACK (_on_sp_cond_button_toggled),
            userdata);


    w = gtkpod_builder_xml_get_widget(special_xml, "sp_modified_entry");
    priv->ti_modified.entry = w;
    gtk_entry_set_text(GTK_ENTRY (w), buf);
    g_signal_connect ((gpointer)w,
            "activate", G_CALLBACK (_on_sp_entry_activate),
            userdata);

    g_signal_connect ((gpointer)gtkpod_builder_xml_get_widget (special_xml,
            "sp_modified_cal_button"),
            "clicked",
            G_CALLBACK (_on_sp_cal_button_clicked),
            userdata);
    g_free(buf);

    /* ADDED */
    buf = prefs_get_string_index("sp_added_state", inst);

    w = gtkpod_builder_xml_get_widget(special_xml, "sp_added_button");
    userdata = _create_page_data(sst);
    userdata->data = GUINT_TO_POINTER(T_TIME_ADDED);

    priv->ti_added.active = w;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
            prefs_get_int_index("sp_added_cond", inst));
    g_signal_connect ((gpointer)w,
            "toggled", G_CALLBACK (_on_sp_cond_button_toggled),
            userdata);

    w = gtkpod_builder_xml_get_widget(special_xml, "sp_added_entry");
    priv->ti_added.entry = w;
    gtk_entry_set_text(GTK_ENTRY (w), buf);
    g_signal_connect ((gpointer)w,
            "activate", G_CALLBACK (_on_sp_entry_activate),
            userdata);

    g_signal_connect ((gpointer)gtkpod_builder_xml_get_widget (special_xml,
            "sp_added_cal_button"),
            "clicked",
            G_CALLBACK (_on_sp_cal_button_clicked),
            userdata);

    g_signal_connect ((gpointer)gtkpod_builder_xml_get_widget (special_xml, "sp_go"),
            "clicked", G_CALLBACK (_on_sp_go_clicked),
            sst);

    w = gtkpod_builder_xml_get_widget(special_xml, "sp_go_always");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
                prefs_get_int_index("sp_autodisplay", inst));
    g_signal_connect ((gpointer)w,
            "toggled", G_CALLBACK (_on_sp_go_always_toggled),
            sst);

    g_free(buf);

    /* we don't need this any more */
    gtk_widget_destroy(special);


    return GTK_WIDGET(sst);
}

gchar *special_sort_tab_page_get_glade_file(SpecialSortTabPage *self) {
    g_return_val_if_fail(SPECIAL_SORT_TAB_IS_PAGE(self), NULL);
    SpecialSortTabPagePrivate *priv = SPECIAL_SORT_TAB_PAGE_GET_PRIVATE(self);

    return priv->glade_xml;
}

SortTabWidget *special_sort_tab_page_get_parent(SpecialSortTabPage *self) {
    g_return_val_if_fail(SPECIAL_SORT_TAB_IS_PAGE(self), NULL);
    SpecialSortTabPagePrivate *priv = SPECIAL_SORT_TAB_PAGE_GET_PRIVATE(self);

    return priv->st_widget_parent;
}

/**
 * Save the contents of the entry to prefs
 */
void special_sort_tab_page_store_state(SpecialSortTabPage *self) {
    g_return_if_fail(SPECIAL_SORT_TAB_IS_PAGE(self));
    SpecialSortTabPagePrivate *priv = SPECIAL_SORT_TAB_PAGE_GET_PRIVATE(self);
    guint32 inst = _get_sort_tab_widget_instance(self);

    prefs_set_string_index("sp_played_state", inst, gtk_entry_get_text(GTK_ENTRY(priv->ti_played.entry)));
    prefs_set_string_index("sp_modified_state", inst, gtk_entry_get_text(GTK_ENTRY(priv->ti_modified.entry)));
    prefs_set_string_index("sp_added_state", inst, gtk_entry_get_text(GTK_ENTRY(priv->ti_added.entry)));
}

/**
 * Update the date interval from the string provided by prefs_get_sp_entry()
 *
 * @inst: instance
 * @item: T_TIME_PLAYED, or T_TIME_MODIFIED,
 * @force_update: usually the update is only performed if the string
 * has changed. TRUE will re-evaluate the string (and print an error
 * message again, if necessary
 *
 * Return value: pointer to the corresponding TimeInfo struct (for
 * convenience) or NULL if error occurred
 */
TimeInfo *special_sort_tab_page_update_date_interval(SpecialSortTabPage *self, T_item item, gboolean force_update) {
    TimeInfo *ti;
    guint32 inst;

    if (! SPECIAL_SORT_TAB_IS_PAGE(self))
        return NULL;

    ti = special_sort_tab_page_get_timeinfo(self, item);
    inst = _get_sort_tab_widget_instance(self);

    if (ti) {
        gchar *new_string = NULL;
        switch (item) {
        case T_TIME_PLAYED:
            new_string = prefs_get_string_index("sp_played_state", inst);
            break;
        case T_TIME_MODIFIED:
            new_string = prefs_get_string_index("sp_modified_state", inst);
            break;
        case T_TIME_ADDED:
            new_string = prefs_get_string_index("sp_added_state", inst);
            break;
        default:
            break;
        }

        if (!new_string) {
            new_string = g_strdup("");
        }

        if (force_update || !ti->int_str || (strcmp(new_string, ti->int_str) != 0)) {
            /* Re-evaluate the interval */
            g_free(ti->int_str);
            ti->int_str = g_strdup(new_string);
            dp2_parse(ti);
        }
        g_free(new_string);
    }

    return ti;
}

/**
 * Return a pointer to ti_added, ti_modified or ti_played.
 * Returns NULL if either inst or item are out of range
 */
TimeInfo *special_sort_tab_page_get_timeinfo(SpecialSortTabPage *self, T_item item) {
    if (!SPECIAL_SORT_TAB_IS_PAGE(self)) {
        fprintf(stderr, "Programming error: st_get_timeinfo_ptr: inst out of range: %d\n", _get_sort_tab_widget_instance(self));
    }
    else {
        SpecialSortTabPagePrivate *priv = SPECIAL_SORT_TAB_PAGE_GET_PRIVATE(self);
        switch (item) {
        case T_TIME_ADDED:
            return &priv->ti_added;
        case T_TIME_PLAYED:
            return &priv->ti_played;
        case T_TIME_MODIFIED:
            return &priv->ti_modified;
        default:
            fprintf(stderr, "Programming error: st_get_timeinfo_ptr: item invalid: %d\n", _get_sort_tab_widget_instance(self));
        }
    }
    return NULL;
}

GList *special_sort_tab_page_get_selected_tracks(SpecialSortTabPage *self) {
    g_return_val_if_fail(SPECIAL_SORT_TAB_IS_PAGE(self), NULL);
    SpecialSortTabPagePrivate *priv = SPECIAL_SORT_TAB_PAGE_GET_PRIVATE(self);

    return priv->sp_selected;
}

gboolean special_sort_tab_page_get_is_go(SpecialSortTabPage *self) {
    g_return_val_if_fail(SPECIAL_SORT_TAB_IS_PAGE(self), FALSE);
    SpecialSortTabPagePrivate *priv = SPECIAL_SORT_TAB_PAGE_GET_PRIVATE(self);

    return priv->pass_on_new_members;
}

void special_sort_tab_page_set_is_go(SpecialSortTabPage *self, gboolean pass_on_new_members) {
    g_return_if_fail(SPECIAL_SORT_TAB_IS_PAGE(self));
    SpecialSortTabPagePrivate *priv = SPECIAL_SORT_TAB_PAGE_GET_PRIVATE(self);

    priv->pass_on_new_members = pass_on_new_members;
}

void special_sort_tab_page_add_track(SpecialSortTabPage *self, Track *track, gboolean final, gboolean display) {
    g_return_if_fail(SPECIAL_SORT_TAB_IS_PAGE(self));

    SpecialSortTabPagePrivate *priv = SPECIAL_SORT_TAB_PAGE_GET_PRIVATE(self);
    SortTabWidget *st_parent_widget = priv->st_widget_parent;
    SortTabWidget *st_next = sort_tab_widget_get_next(st_parent_widget);
    gint inst = sort_tab_widget_get_instance(st_parent_widget);

    sort_tab_widget_set_all_tracks_added(st_parent_widget, final);

    if (track) {
        /* Add track to member list */
        priv->sp_members = g_list_append(priv->sp_members, track);
        /* Check if track is to be passed on to next sort tab */
        if (priv->pass_on_new_members || prefs_get_int_index("sp_autodisplay", inst)) {
            /* Check if track matches sort criteria to be displayed */
            if (_sp_check_track(self, track)) {
                priv->sp_selected = g_list_append(priv->sp_selected, track);
                sort_tab_widget_add_track(st_next, track, final, display);
            }
        }
    }

    if (!track && final) {
        if (priv->pass_on_new_members || prefs_get_int_index("sp_autodisplay", inst))
            sort_tab_widget_add_track(st_next, NULL, final, display);
    }
}

void special_sort_tab_page_remove_track(SpecialSortTabPage *self, Track *track) {
    g_return_if_fail(SPECIAL_SORT_TAB_IS_PAGE(self));

    SpecialSortTabPagePrivate *priv = SPECIAL_SORT_TAB_PAGE_GET_PRIVATE(self);
    SortTabWidget *st_parent_widget = priv->st_widget_parent;
    SortTabWidget *st_next = sort_tab_widget_get_next(st_parent_widget);
    GList *link;

    /* Remove track from member list */
    link = g_list_find(priv->sp_members, track);
    if (link) {
        /*
         * Only remove track from next sort tab if it was a member of
         * this sort tab (slight performance improvement when being
         * called with non-existing tracks
         */
        priv->sp_members = g_list_delete_link(priv->sp_members, link);
        sort_tab_widget_remove_track(st_next, track);
    }
}

void special_sort_tab_page_track_changed(SpecialSortTabPage *self, Track *track, gboolean removed) {
    g_return_if_fail(SPECIAL_SORT_TAB_IS_PAGE(self));

    SpecialSortTabPagePrivate *priv = SPECIAL_SORT_TAB_PAGE_GET_PRIVATE(self);
    SortTabWidget *st_parent_widget = priv->st_widget_parent;
    SortTabWidget *st_next = sort_tab_widget_get_next(st_parent_widget);

    if (g_list_find(priv->sp_members, track)) {
        /*
         * Only do anything if @track was a member of this
         * sort tab (slight performance improvement when
         * being called with non-existing tracks
         */
        if (removed) {
            /* Remove track from member list */
            priv->sp_members = g_list_remove(priv->sp_members, track);
            if (g_list_find(priv->sp_selected, track)) {
                /* only remove from next sort tab if it was passed on */
                priv->sp_selected = g_list_remove(priv->sp_selected, track);
                sort_tab_widget_track_changed(st_next, track, TRUE);
            }
        }
        else {
            if (g_list_find(priv->sp_selected, track)) {
                /* track is being passed on to next sort tab */
                if (_sp_check_track(self, track)) {
                    /* only changed */
                    sort_tab_widget_track_changed(st_next, track, FALSE);
                }
                else {
                    /* has to be removed */
                    priv->sp_selected = g_list_remove(priv->sp_selected, track);
                    sort_tab_widget_track_changed(st_next, track, TRUE);
                }
            }
            else {
                /* track is not being passed on to next sort tab */
                if (_sp_check_track(self, track)) {
                    /* add to next sort tab */
                    priv->sp_selected = g_list_append(priv->sp_selected, track);
                    sort_tab_widget_add_track(st_next, track, TRUE, TRUE);
                }
            }
        }
    }
}

void special_sort_tab_page_clear(SpecialSortTabPage *self) {
    g_return_if_fail(SPECIAL_SORT_TAB_IS_PAGE(self));

    SpecialSortTabPagePrivate *priv = SPECIAL_SORT_TAB_PAGE_GET_PRIVATE(self);

    g_list_free(priv->sp_members);
    priv->sp_members = NULL;
    g_list_free(priv->sp_selected);
    priv->sp_selected = NULL;
}

#endif /* SPECIAL_SORTTAB_C_ */

