/*
 |  Copyright (C) 2002 Corey Donohoe <atmos at atmos.org>
 |  Copyright (C) 2002-2005 Jorg Schuler <jcsjcs at users sourceforge net>
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

#include <stdio.h>
#include <string.h>
#include <glade/glade-xml.h>
#include "libgtkpod/gtkpod_app_iface.h"
#include "libgtkpod/misc_conversion.h"
#include "libgtkpod/misc.h"
#include "libgtkpod/gp_private.h"
#include "libgtkpod/directories.h"
#include "plugin.h"
#include "display_tracks.h"
#include "sort_window.h"

GladeXML *prefs_window_xml;
GladeXML *sort_window_xml;

static GtkWidget *sort_window = NULL;

/* New prefs temp handling */
static TempPrefs *sort_temp_prefs;
static TempLists *sort_temp_lists;

static const GtkFileChooserAction path_type[] =
    {
        GTK_FILE_CHOOSER_ACTION_OPEN, /* select file */
        GTK_FILE_CHOOSER_ACTION_OPEN, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_FILE_CHOOSER_ACTION_OPEN,
        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, /* select folder */
        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_FILE_CHOOSER_ACTION_OPEN,
        GTK_FILE_CHOOSER_ACTION_OPEN, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_FILE_CHOOSER_ACTION_OPEN, -1 };

enum {
    TRACK_COLUMNS_TEXT, TRACK_COLUMNS_INT, TRACK_N_COLUMNS
};

typedef enum {
    HIDE, SHOW
} TrackColumnsType;

/* ------------------------------------------------------------ *\
 *                                                              *
 * Sort-Prefs Window                                            *
 *                                                              *
 \* ------------------------------------------------------------ */

/* the following checkboxes exist */
static const gint sort_ign_fields[] =
    { T_TITLE, T_ARTIST, T_ALBUM, T_COMPOSER, -1 };

/* Copy the current ignore fields and ignore strings into scfg */
static void sort_window_read_sort_ign() {
    gint i;
    GtkTextView *tv;
    GtkTextBuffer *tb;
    GList *sort_ign_strings;
    GList *current;
    gchar *buf;

    /* read sort field states */
    for (i = 0; sort_ign_fields[i] != -1; ++i) {
        buf = g_strdup_printf("sort_ign_field_%d", sort_ign_fields[i]);
        GtkWidget *w = gtkpod_xml_get_widget(sort_window_xml, buf);
        g_return_if_fail (w);
        prefs_set_int(buf, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (w)));
        g_free(buf);
    }

    /* Read sort ignore strings */
    tv = GTK_TEXT_VIEW (gtkpod_xml_get_widget (sort_window_xml,
                    "sort_ign_strings"));
    g_return_if_fail (tv);
    tb = gtk_text_view_get_buffer(tv);
    g_return_if_fail (tb);

    sort_ign_strings = get_list_from_buffer(tb);
    current = sort_ign_strings;

    /* Add a trailing whitespace to strings */
    while (current) {
        g_strstrip(current->data);

        if (strlen(current->data) != 0) {
            buf = g_strdup_printf("%s ", (gchar *) current->data);
            g_free(current->data);
            current->data = buf;
        }

        current = g_list_next(current);
    }

    temp_list_add(sort_temp_lists, "sort_ign_string_", sort_ign_strings);
}

/**
 * sort_window_create
 * Create, Initialize, and Show the sorting preferences window
 * allocate a static sort struct for temporary variables
 */
void sort_window_create(void) {
    if (sort_window) {
        /* sort options already open --> simply raise to the top */
        gdk_window_raise(sort_window->window);
    }
    else {
        GList *sort_ign_strings;
        GList *current; /* current sort ignore item */
        GtkWidget *w;
        GtkTextView *tv;
        GtkTextBuffer *tb;
        gint i;
        GtkTextIter ti;
        gchar *str;
        GtkTooltips *tooltips;
        gint *tm_listed_order, tm_list_pos;

        sort_temp_prefs = temp_prefs_create();
        sort_temp_lists = temp_lists_create();

        gchar *glade_path = g_build_filename(get_glade_dir(), "track_display.glade", NULL);
        sort_window_xml = gtkpod_xml_new(glade_path, "sort_window");
        glade_xml_signal_autoconnect(sort_window_xml);
        g_free(glade_path);

        sort_window = gtkpod_xml_get_widget(sort_window_xml, "sort_window");

        /* label the ignore-field checkbox-labels */
        for (i = 0; sort_ign_fields[i] != -1; ++i) {
            gchar *buf = g_strdup_printf("sort_ign_field_%d", sort_ign_fields[i]);
            GtkWidget *w = gtkpod_xml_get_widget(sort_window_xml, buf);
            g_return_if_fail (w);
            gtk_button_set_label(GTK_BUTTON (w), gettext (get_t_string (sort_ign_fields[i])));
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (w), prefs_get_int(buf));
            g_free(buf);
        }
        /* set the ignore strings */
        tv = GTK_TEXT_VIEW (gtkpod_xml_get_widget (sort_window_xml,
                        "sort_ign_strings"));
        tb = gtk_text_view_get_buffer(tv);
        if (!tb) { /* text buffer doesn't exist yet */
            tb = gtk_text_buffer_new(NULL);
            gtk_text_view_set_buffer(tv, tb);
            gtk_text_view_set_editable(tv, FALSE);
            gtk_text_view_set_cursor_visible(tv, FALSE);
        }

        sort_ign_strings = prefs_get_list("sort_ign_string_");
        current = sort_ign_strings;
        while (current) {
            str = (gchar *) current->data;
            current = g_list_next(current);

            /* append new text to the end */
            gtk_text_buffer_get_end_iter(tb, &ti);
            gtk_text_buffer_insert(tb, &ti, str, -1);
            /* append newline */
            gtk_text_buffer_get_end_iter(tb, &ti);
            gtk_text_buffer_insert(tb, &ti, "\n", -1);
        }

        prefs_free_list(sort_ign_strings);

        sort_window_read_sort_ign();

        /* Set Sort-Column-Combo */
        /* create the list in the order of the columns displayed */
        tm_store_col_order();

        /* Here we store the order of TM_Items in the
         * GtkComboBox */
        tm_listed_order = g_new (gint, TM_NUM_COLUMNS);
        tm_list_pos = 1;

        w = gtkpod_xml_get_widget(sort_window_xml, "sort_combo");
        gtk_combo_box_remove_text(GTK_COMBO_BOX (w), 0);

        gtk_combo_box_append_text(GTK_COMBO_BOX (w), _("No sorting"));

        for (i = 0; i < TM_NUM_COLUMNS; ++i) { /* first the visible columns */
            TM_item col = prefs_get_int_index("col_order", i);
            if (col != -1) {
                if (prefs_get_int_index("col_visible", col)) {
                    gtk_combo_box_append_text(GTK_COMBO_BOX (w), gettext (get_tm_string (col)));
                    tm_listed_order[col] = tm_list_pos;
                    ++tm_list_pos;
                }
            }
        }

        for (i = 0; i < TM_NUM_COLUMNS; ++i) { /* now the hidden colums */
            TM_item col = prefs_get_int_index("col_order", i);
            if (col != -1) {
                if (!prefs_get_int_index("col_visible", col)) {
                    gtk_combo_box_append_text(GTK_COMBO_BOX (w), gettext (get_tm_string (col)));
                    tm_listed_order[col] = tm_list_pos;
                    ++tm_list_pos;
                }
            }
        }

        /* associate tm_listed_order with sort_window */
        g_object_set_data(G_OBJECT (sort_window), "tm_listed_order", tm_listed_order);

        tooltips = gtk_tooltips_new();
        gtk_tooltips_set_tip(tooltips, w, _("You can also use the table headers, but this allows you to sort according to a column that is not displayed."), NULL);

        sort_window_update();

        sort_window_show_hide_tooltips();
        gtk_widget_show(sort_window);
    }
}

/* Update sort_window's settings (except for ignore list and ignore
 * fields) */
void sort_window_update(void) {
    if (sort_window) {
        /*	gchar *str; */
        GtkWidget *w = NULL;
        gint *tm_listed_order;
        gint sortorder;
        TM_item sortcol;

        switch (prefs_get_int("pm_sort")) {
        case SORT_ASCENDING:
            w = gtkpod_xml_get_widget(sort_window_xml, "pm_ascend");
            break;
        case SORT_DESCENDING:
            w = gtkpod_xml_get_widget(sort_window_xml, "pm_descend");
            break;
        case SORT_NONE:
            w = gtkpod_xml_get_widget(sort_window_xml, "pm_none");
            break;
        }
        if (w)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);

        w = NULL;
        switch (prefs_get_int("st_sort")) {
        case SORT_ASCENDING:
            w = gtkpod_xml_get_widget(sort_window_xml, "st_ascend");
            break;
        case SORT_DESCENDING:
            w = gtkpod_xml_get_widget(sort_window_xml, "st_descend");
            break;
        case SORT_NONE:
            w = gtkpod_xml_get_widget(sort_window_xml, "st_none");
            break;
        }
        if (w)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);

        switch (prefs_get_int("tm_sort")) {
        case SORT_ASCENDING:
            w = gtkpod_xml_get_widget(sort_window_xml, "tm_ascend");
            break;
        case SORT_DESCENDING:
            w = gtkpod_xml_get_widget(sort_window_xml, "tm_descend");
            break;
        case SORT_NONE:
            w = gtkpod_xml_get_widget(sort_window_xml, "tm_none");
            break;
        }
        if (w)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);

        w = gtkpod_xml_get_widget(sort_window_xml, "tm_autostore");
        if (w)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), prefs_get_int("tm_autostore"));

        if ((w = gtkpod_xml_get_widget(sort_window_xml, "cfg_case_sensitive"))) {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), prefs_get_int("case_sensitive"));
        }
        /* set standard entry in combo */
        /*	str = gettext (get_tm_string (prefs_get_int("tm_sortcol")));
         w = gtkpod_xml_get_widget (sort_window_xml, "sort_combo");
         gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (w)->entry), str); */
        w = gtkpod_xml_get_widget(sort_window_xml, "sort_combo");
        tm_listed_order = g_object_get_data(G_OBJECT (sort_window), "tm_listed_order");
        g_return_if_fail (tm_listed_order);
        sortcol = prefs_get_int("tm_sortcol");
        sortorder = prefs_get_int("tm_sort");
        if ((sortcol >= 0) && (sortcol < TM_NUM_COLUMNS) && (sortorder != SORT_NONE)) {
            gtk_combo_box_set_active(GTK_COMBO_BOX(w), tm_listed_order[sortcol]);
        }
        else {
            gtk_combo_box_set_active(GTK_COMBO_BOX(w), 0);
        }
    }
}

/* turn the sort window insensitive (if it's open) */
void sort_window_block(void) {
    if (sort_window)
        gtk_widget_set_sensitive(sort_window, FALSE);
}

/* turn the sort window sensitive (if it's open) */
void sort_window_release(void) {
    if (sort_window)
        gtk_widget_set_sensitive(sort_window, TRUE);
}

/* make the tooltips visible or hide it depending on the value set in
 * the prefs (tooltips_prefs) */
void sort_window_show_hide_tooltips(void) {
    if (sort_window) {
        GtkTooltips *tt;
        GtkTooltipsData *tooltips_data;

        tooltips_data = gtk_tooltips_data_get(gtkpod_xml_get_widget(sort_window_xml, "sort_combo"));
        tt = tooltips_data->tooltips;

        if (tt) {
            if (prefs_get_int("display_tooltips_prefs"))
                gtk_tooltips_enable(tt);
            else
                gtk_tooltips_disable(tt);
        }
        else {
            g_warning ("***tt is NULL***");
        }
    }
}

/* get the sort_column selected in the combo */
static TM_item sort_window_get_sort_col(void) {
    GtkWidget *w;
    TM_item sortcol;
    gint item;
    gint *tm_listed_order;

    g_return_val_if_fail (sort_window, -1);
    tm_listed_order = g_object_get_data(G_OBJECT (sort_window), "tm_listed_order");

    w = gtkpod_xml_get_widget(sort_window_xml, "sort_combo");
    item = gtk_combo_box_get_active(GTK_COMBO_BOX (w));

    if ((item <= 0) || (item > TM_NUM_COLUMNS)) { /* either an error or no entry is active or "No sorting" is
     selected (0). In all these cases we return -1 ("No
     sorting") */
        sortcol = -1;
    }
    else {
        gint i;
        sortcol = -1;
        for (i = 0; i < TM_NUM_COLUMNS; ++i) {
            if (tm_listed_order[i] == item) {
                sortcol = i;
                break;
            }
        }
    }

    return sortcol;
}

/* Prepare keys to be copied to prefs table */
static void sort_window_set() {
    gint val; /* A value from temp prefs */
    TM_item sortcol_new;
    TM_item sortcol_old;

    sortcol_old = prefs_get_int("tm_sortcol");
    sortcol_new = sort_window_get_sort_col();
    if (sortcol_new != -1) {
        prefs_set_int("tm_sortcol", sortcol_new);
        if (prefs_get_int("tm_sort") == SORT_NONE) {
            if (temp_prefs_get_int_value(sort_temp_prefs, "tm_sort", &val)) {
                prefs_set_int("tm_sort", val);
            }
            else {
                prefs_set_int("tm_sort", GTK_SORT_ASCENDING);
            }
        }
    }
    else {
        if (prefs_get_int("tm_sort") == SORT_NONE) { /* no change */
            sortcol_new = sortcol_old;
        }
        else {
            prefs_set_int("tm_sort", SORT_NONE);
        }
    }

    /* update compare string keys */
    compare_string_fuzzy_generate_keys();

    /* if case sensitive has changed, rebuild sortkeys */
    if (temp_prefs_get_int_value(sort_temp_prefs, "case_sensitive", &val)) {
        gtkpod_broadcast_preference_change("case_sensitive", val);
        temp_prefs_remove_key(sort_temp_prefs, "case_sensitive");
    }
    /* if sort type has changed, initialize display */
    if (temp_prefs_get_int_value(sort_temp_prefs, "pm_sort", &val)) {
        gtkpod_broadcast_preference_change("pm_sort", val);
        temp_prefs_remove_key(sort_temp_prefs, "pm_sort");
    }
    if (temp_prefs_get_int_value(sort_temp_prefs, "st_sort", &val)) {
        gtkpod_broadcast_preference_change("st_sort", val);
        temp_prefs_remove_key(sort_temp_prefs, "st_sort");
    }
    if (temp_prefs_get_int_value(sort_temp_prefs, "tm_sort", NULL) || (sortcol_old != sortcol_new)) {
        gtkpod_broadcast_preference_change("tm_sort", val);
        temp_prefs_remove_key(sort_temp_prefs, "tm_sort");
    }
    /* if auto sort was changed to TRUE, store order */
    if (!temp_prefs_get_int(sort_temp_prefs, "tm_autostore")) {
        tm_rows_reordered();
        temp_prefs_remove_key(sort_temp_prefs, "tm_autostore");
    }

    sort_window_update();
}

/* -----------------------------------------------------------------

 Callbacks

 ----------------------------------------------------------------- */

G_MODULE_EXPORT void on_st_ascend_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
    if (gtk_toggle_button_get_active(togglebutton))
        sort_window_set_st_sort(SORT_ASCENDING);
}

G_MODULE_EXPORT void on_st_descend_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
    if (gtk_toggle_button_get_active(togglebutton))
        sort_window_set_st_sort(SORT_DESCENDING);
}

G_MODULE_EXPORT void on_st_none_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
    if (gtk_toggle_button_get_active(togglebutton))
        sort_window_set_st_sort(SORT_NONE);
}

G_MODULE_EXPORT void on_pm_ascend_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
    if (gtk_toggle_button_get_active(togglebutton))
        sort_window_set_pm_sort(SORT_ASCENDING);
}

G_MODULE_EXPORT void on_pm_descend_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
    if (gtk_toggle_button_get_active(togglebutton))
        sort_window_set_pm_sort(SORT_DESCENDING);
}

G_MODULE_EXPORT void on_pm_none_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
    if (gtk_toggle_button_get_active(togglebutton))
        sort_window_set_pm_sort(SORT_NONE);
}

G_MODULE_EXPORT void on_tm_ascend_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
    if (gtk_toggle_button_get_active(togglebutton))
        sort_window_set_tm_sort(SORT_ASCENDING);
}

G_MODULE_EXPORT void on_tm_descend_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
    if (gtk_toggle_button_get_active(togglebutton))
        sort_window_set_tm_sort(SORT_DESCENDING);
}

G_MODULE_EXPORT void on_tm_none_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
    if (gtk_toggle_button_get_active(togglebutton))
        sort_window_set_tm_sort(SORT_NONE);
}

G_MODULE_EXPORT void on_tm_autostore_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
    sort_window_set_tm_autostore(gtk_toggle_button_get_active(togglebutton));
}

G_MODULE_EXPORT void on_sort_case_sensitive_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
    sort_window_set_case_sensitive(gtk_toggle_button_get_active(togglebutton));
}

G_MODULE_EXPORT void on_sort_apply_clicked(GtkButton *button, gpointer user_data) {
    sort_window_apply();
}

G_MODULE_EXPORT void on_sort_cancel_clicked(GtkButton *button, gpointer user_data) {
    sort_window_cancel();
}

G_MODULE_EXPORT void on_sort_ok_clicked(GtkButton *button, gpointer user_data) {
    sort_window_ok();
}

G_MODULE_EXPORT gboolean on_sort_window_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
    sort_window_delete();
    return FALSE;
}

/**
 * sort_window_cancel
 * UI has requested sort prefs changes be ignored -- write back the
 * original values
 */
void sort_window_cancel(void) {
    /* close the window */
    if (sort_window)
        gtk_widget_destroy(sort_window);
    sort_window = NULL;
}

/* when window is deleted, we keep the currently applied prefs */
void sort_window_delete(void) {
    temp_prefs_destroy(sort_temp_prefs);
    sort_temp_prefs = NULL;
    temp_lists_destroy(sort_temp_lists);
    sort_temp_lists = NULL;

    /* close the window */
    if (sort_window) {
        gint *tm_listed_order;
        tm_listed_order = g_object_get_data(G_OBJECT (sort_window), "tm_listed_order");
        g_warn_if_fail (tm_listed_order);
        g_free(tm_listed_order);
        gtk_widget_destroy(sort_window);
    }
    sort_window = NULL;
}

/* apply the current settings and close the window */
void sort_window_ok(void) {
    /* update the sort ignore strings */
    sort_window_read_sort_ign();

    temp_prefs_apply(sort_temp_prefs);
    temp_lists_apply(sort_temp_lists);

    /* save current settings */
    sort_window_set();

    /* close the window */
    if (sort_window)
        gtk_widget_destroy(sort_window);
    sort_window = NULL;
}

/* apply the current settings, don't close the window */
void sort_window_apply(void) {
    /* update the sort ignore strings */
    sort_window_read_sort_ign();

    temp_prefs_apply(sort_temp_prefs);
    temp_lists_apply(sort_temp_lists);

    /* save current settings */
    sort_window_set();
}

void sort_window_set_tm_autostore(gboolean val) {
    temp_prefs_set_int(sort_temp_prefs, "tm_autostore", val);
}

void sort_window_set_pm_sort(gint val) {
    temp_prefs_set_int(sort_temp_prefs, "pm_sort", val);
}

void sort_window_set_st_sort(gint val) {
    temp_prefs_set_int(sort_temp_prefs, "st_sort", val);
}

void sort_window_set_tm_sort(gint val) {
    temp_prefs_set_int(sort_temp_prefs, "tm_sort", val);
}

void sort_window_set_case_sensitive(gboolean val) {
    temp_prefs_set_int(sort_temp_prefs, "case_sensitive", val);
}
