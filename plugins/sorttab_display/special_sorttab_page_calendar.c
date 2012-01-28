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
/* enum to access cat_strings */

#include <gtk/gtk.h>
#include <string.h>
#include "libgtkpod/misc.h"
#include "libgtkpod/misc_conversion.h"
#include "libgtkpod/prefs.h"
#include "date_parser.h"
#include "sorttab_widget.h"
#include "special_sorttab_page.h"

#define CAL_XML "cal_xml"
#define SPECIAL_PAGE "special_sort_tab_page"

enum {
    CAT_STRING_PLAYED = 0,
    CAT_STRING_MODIFIED = 1,
    CAT_STRING_ADDED = 2
};

/* typedef to specify lower or upper margin */
typedef enum {
    LOWER_MARGIN, UPPER_MARGIN
} MarginType;


GtkBuilder *_get_calendar_xml(GtkWidget *cal) {
    g_return_val_if_fail(GTK_IS_WIDGET(cal), NULL);
    GtkBuilder *xml;

    xml = g_object_get_data(G_OBJECT(cal), CAL_XML);
    g_return_val_if_fail(GTK_IS_BUILDER(xml), NULL);

    return xml;
}

SpecialSortTabPage *_get_parent_page(GtkWidget *cal) {
    g_return_val_if_fail(GTK_IS_WIDGET(cal), NULL);
    SpecialSortTabPage *page;

    page = g_object_get_data(G_OBJECT(cal), SPECIAL_PAGE);
    g_return_val_if_fail(SPECIAL_SORT_TAB_IS_PAGE(page), NULL);

    return page;
}

/* Set the calendar @calendar, as well as spin buttons @hour and @min
 * according to @mactime. If @mactime is 0, check @no_margin
 * togglebutton, otherwise uncheck it. */
static void cal_set_time_widgets(GtkCalendar *cal, GtkSpinButton *hour, GtkSpinButton *min, GtkToggleButton *no_margin, time_t timet) {
    struct tm *tm;
    time_t tt = time(NULL);

    /* 0, -1 are treated in a special way (no lower/upper margin
     * -> set calendar to current time */
    if ((timet != 0) && (timet != -1)) {
        tt = timet;
        if (no_margin)
            gtk_toggle_button_set_active(no_margin, FALSE);
    }
    else if (no_margin)
        gtk_toggle_button_set_active(no_margin, TRUE);

    tm = localtime(&tt);

    if (cal) {
        gtk_calendar_select_month(cal, tm->tm_mon, 1900 + tm->tm_year);
        gtk_calendar_select_day(cal, tm->tm_mday);
    }

    if (hour)
        gtk_spin_button_set_value(hour, tm->tm_hour);
    if (min)
        gtk_spin_button_set_value(min, tm->tm_min);
}

static void cal_set_time(GtkWidget *cal, MarginType type, time_t timet) {
    GtkBuilder *cal_xml;
    GtkCalendar *calendar = NULL;
    GtkSpinButton *hour = NULL;
    GtkSpinButton *min = NULL;
    GtkToggleButton *no_margin = NULL;

    cal_xml = _get_calendar_xml(cal);

    switch (type) {
    case LOWER_MARGIN:
        calendar = GTK_CALENDAR (gtkpod_builder_xml_get_widget (cal_xml, "lower_cal"));
        hour = GTK_SPIN_BUTTON (gtkpod_builder_xml_get_widget (cal_xml, "lower_hours"));
        min = GTK_SPIN_BUTTON (gtkpod_builder_xml_get_widget (cal_xml, "lower_minutes"));
        no_margin = GTK_TOGGLE_BUTTON (gtkpod_builder_xml_get_widget (cal_xml, "no_lower_margin"));
        break;
    case UPPER_MARGIN:
        calendar = GTK_CALENDAR (gtkpod_builder_xml_get_widget (cal_xml, "upper_cal"));
        hour = GTK_SPIN_BUTTON (gtkpod_builder_xml_get_widget (cal_xml, "upper_hours"));
        min = GTK_SPIN_BUTTON (gtkpod_builder_xml_get_widget (cal_xml, "upper_minutes"));
        no_margin = GTK_TOGGLE_BUTTON (gtkpod_builder_xml_get_widget (cal_xml, "no_upper_margin"));
        break;
    }
    cal_set_time_widgets(calendar, hour, min, no_margin, timet);
}

/* Extract data from calendar/time.
 *
 * Return value:
 *
 * pointer to 'struct tm' filled with the relevant data or NULL, if
 * the button no_margin was selected.
 *
 * If @tm is != NULL, modify that instead.
 *
 * You must g_free() the retuned value.
 */
static struct tm *cal_get_time(GtkWidget *cal, MarginType type, struct tm *tm) {
    GtkCalendar *calendar = NULL;
    GtkSpinButton *hour = NULL;
    GtkSpinButton *min = NULL;
    GtkSpinButton *sec = NULL;
    GtkToggleButton *no_margin = NULL;
    GtkToggleButton *no_time = NULL;
    GtkBuilder *cal_xml;

    cal_xml = _get_calendar_xml(cal);

    switch (type) {
    case LOWER_MARGIN:
        calendar = GTK_CALENDAR (gtkpod_builder_xml_get_widget (cal_xml, "lower_cal"));
        hour = GTK_SPIN_BUTTON (gtkpod_builder_xml_get_widget (cal_xml, "lower_hours"));
        min = GTK_SPIN_BUTTON (gtkpod_builder_xml_get_widget (cal_xml, "lower_minutes"));
        no_margin = GTK_TOGGLE_BUTTON (gtkpod_builder_xml_get_widget (cal_xml, "no_lower_margin"));
        no_time = GTK_TOGGLE_BUTTON (gtkpod_builder_xml_get_widget (cal_xml, "lower_time"));
        break;
    case UPPER_MARGIN:
        calendar = GTK_CALENDAR (gtkpod_builder_xml_get_widget (cal_xml, "upper_cal"));
        hour = GTK_SPIN_BUTTON (gtkpod_builder_xml_get_widget (cal_xml, "upper_hours"));
        min = GTK_SPIN_BUTTON (gtkpod_builder_xml_get_widget (cal_xml, "upper_minutes"));
        no_margin = GTK_TOGGLE_BUTTON (gtkpod_builder_xml_get_widget (cal_xml, "no_upper_margin"));
        no_time = GTK_TOGGLE_BUTTON (gtkpod_builder_xml_get_widget (cal_xml, "upper_time"));
        break;
    }

    if (!gtk_toggle_button_get_active(no_margin)) {
        /* Initialize tm with current time and copy the result of
         * localtime() to persistent memory that can be g_free()'d */
        time_t tt = time(NULL);
        if (!tm) {
            tm = g_malloc(sizeof(struct tm));
            memcpy(tm, localtime(&tt), sizeof(struct tm));
        }

        if (calendar) {
            guint year, month, day;
            gtk_calendar_get_date(calendar, &year, &month, &day);
            tm->tm_year = year - 1900;
            tm->tm_mon = month;
            tm->tm_mday = day;
        }
        if (gtk_toggle_button_get_active(no_time)) {
            if (hour)
                tm->tm_hour = gtk_spin_button_get_value_as_int(hour);
            if (min)
                tm->tm_min = gtk_spin_button_get_value_as_int(min);
            if (sec)
                tm->tm_min = gtk_spin_button_get_value_as_int(sec);
        }
        else { /* use 0:00 for lower and 23:59 for upper margin */
            switch (type) {
            case LOWER_MARGIN:
                if (hour)
                    tm->tm_hour = 0;
                if (min)
                    tm->tm_min = 0;
                if (sec)
                    tm->tm_sec = 0;
                break;
            case UPPER_MARGIN:
                if (hour)
                    tm->tm_hour = 23;
                if (min)
                    tm->tm_min = 59;
                if (sec)
                    tm->tm_sec = 59;
                break;
            }
        }
    }
    return tm;
}

/* get the category (T_TIME_PLAYED or T_TIME_MODIFIED) selected in the
 * combo */
static T_item cal_get_category(GtkWidget *cal) {
    GtkWidget *w;
    T_item item;
    gint i = -1;
    GtkBuilder *cal_xml;

    cal_xml = _get_calendar_xml(cal);

    w = gtkpod_builder_xml_get_widget(cal_xml, "cat_combo");
    i = gtk_combo_box_get_active(GTK_COMBO_BOX (w));

    switch (i) {
    case CAT_STRING_PLAYED:
        item = T_TIME_PLAYED;
        break;
    case CAT_STRING_MODIFIED:
        item = T_TIME_MODIFIED;
        break;
    case CAT_STRING_ADDED:
        item = T_TIME_ADDED;
        break;
    default:
        fprintf(stderr, "Programming error: cal_get_category () -- item not found.\n");
        /* set to something reasonable at least */
        item = T_TIME_PLAYED;
    }

    return item;
}

/* Returns a string "DD/MM/YYYY HH:MM". Data is taken from
 * @tm. Returns NULL if tm==NULL. You must g_free() the returned
 * string */
static gchar *cal_get_time_string(struct tm *tm) {
    gchar *str = NULL;

    if (tm)
        str
                = g_strdup_printf("%02d/%02d/%04d %d:%02d", tm->tm_mday, tm->tm_mon + 1, 1900 + tm->tm_year, tm->tm_hour, tm->tm_min);
    return str;
}

/* Extract data from calendar/time and write it to the corresponding
 entry in the specified sort tab */
static void cal_apply_data(GtkWidget *cal) {
    struct tm *lower, *upper;
    TimeInfo *ti;
    T_item item;
    SpecialSortTabPage *page;

    page = _get_parent_page(cal);
    lower = cal_get_time(cal, LOWER_MARGIN, NULL);
    upper = cal_get_time(cal, UPPER_MARGIN, NULL);

    /* Get selected category (played, modified or added) */
    item = cal_get_category(cal);
    /* Get pointer to corresponding TimeInfo struct */
    ti = special_sort_tab_page_get_timeinfo(page, item);

    if (ti) {
        GtkToggleButton *act = GTK_TOGGLE_BUTTON (ti->active);
        /* is criteria currently checked (active)? */
        gboolean active = gtk_toggle_button_get_active(act);
        gchar *str = NULL;
        gchar *str1 = cal_get_time_string(lower);
        gchar *str2 = cal_get_time_string(upper);

        if (!lower && !upper)
            if (!active) /* deactivate this criteria */
                gtk_toggle_button_set_active(act, FALSE);
        if (lower && !upper)
            str = g_strdup_printf("> %s", str1);
        if (!lower && upper)
            str = g_strdup_printf("< %s", str2);
        if (lower && upper)
            str = g_strdup_printf("%s < < %s", str1, str2);
        C_FREE (str1);
        C_FREE (str2);

        if (str) { /* set the new string if it's different */
            if (strcmp(str, gtk_entry_get_text(GTK_ENTRY (ti->entry))) != 0) {
                gtk_entry_set_text(GTK_ENTRY (ti->entry), str);
                /* notification that contents have changed */
                g_signal_emit_by_name(ti->entry, "activate");
            }
            g_free(str);
        }
        if (!active) { /* activate the criteria */
            gtk_toggle_button_set_active(act, TRUE);
        }
    }
    g_free(lower);
    g_free(upper);
}

/* Callback for 'Lower/Upper time ' buttons */
static void cal_time_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
    GtkWidget *cal = GTK_WIDGET(user_data);
    GtkBuilder *cal_xml = _get_calendar_xml(cal);
    gboolean sens = gtk_toggle_button_get_active(togglebutton);

    if ((GtkWidget *) togglebutton == gtkpod_builder_xml_get_widget(cal_xml, "lower_time")) {
        gtk_widget_set_sensitive(gtkpod_builder_xml_get_widget(cal_xml, "lower_time_box"), sens);
    }
    if ((GtkWidget *) togglebutton == gtkpod_builder_xml_get_widget(cal_xml, "upper_time")) {
        gtk_widget_set_sensitive(gtkpod_builder_xml_get_widget(cal_xml, "upper_time_box"), sens);
    }
}

/* Callback for 'No Lower/Upper Margin' buttons */
static void cal_no_margin_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
    GtkWidget *cal = GTK_WIDGET(user_data);
    GtkBuilder *cal_xml = _get_calendar_xml(cal);
    gboolean sens = !gtk_toggle_button_get_active(togglebutton);

    if ((GtkWidget *) togglebutton == gtkpod_builder_xml_get_widget(cal_xml, "no_lower_margin")) {
        gtk_widget_set_sensitive(gtkpod_builder_xml_get_widget(cal_xml, "lower_cal_box"), sens);
    }
    if ((GtkWidget *) togglebutton == gtkpod_builder_xml_get_widget(cal_xml, "no_upper_margin")) {
        gtk_widget_set_sensitive(gtkpod_builder_xml_get_widget(cal_xml, "upper_cal_box"), sens);
    }
}

/* Save the default geometry of the window */
static void cal_save_default_geometry(GtkWindow *cal) {
    gint x, y;

    gtk_window_get_size(cal, &x, &y);
    prefs_set_int("size_cal.x", x);
    prefs_set_int("size_cal.y", y);

}

/* Callback for 'delete' event */
static gboolean cal_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
    cal_save_default_geometry(GTK_WINDOW (user_data));
    return FALSE;
}

/* Callback for 'Cancel' button */
static void cal_cancel(GtkButton *button, gpointer user_data) {
    cal_save_default_geometry(GTK_WINDOW (user_data));
    gtk_widget_destroy(user_data);
}

/* Callback for 'Apply' button */
static void cal_apply(GtkButton *button, gpointer user_data) {
    cal_save_default_geometry(GTK_WINDOW (user_data));
    cal_apply_data(GTK_WIDGET (user_data));
}

/* Callback for 'OK' button */
static void cal_ok(GtkButton *button, gpointer user_data) {
    cal_apply(button, user_data);
    gtk_widget_destroy(user_data);
}

/**
 * Open a calendar window. Preset the values for instance @inst,
 * category @item (time played, time modified or time added)
 */
void cal_open_calendar(SpecialSortTabPage *page, T_item item) {
    GtkWidget *w;
    GtkWidget *cal;
    int index = -1;
    gint defx, defy;
    TimeInfo *ti;
    GtkBuilder *cal_xml;
    SortTabWidget *parent;

    /* Sanity */
    if (!SPECIAL_SORT_TAB_IS_PAGE(page))
        return;

    parent = special_sort_tab_page_get_parent(page);
    cal_xml = gtkpod_builder_xml_new(special_sort_tab_page_get_glade_file(page));

    gtk_builder_connect_signals (cal_xml, NULL);

    cal = gtkpod_builder_xml_get_widget(cal_xml, "calendar_window");
    g_object_set_data(G_OBJECT(cal), CAL_XML, cal_xml);
    g_object_set_data(G_OBJECT(cal), SPECIAL_PAGE, page);

    /* Set to saved size */
    defx = prefs_get_int("size_cal.x");
    defy = prefs_get_int("size_cal.y");
    gtk_window_set_default_size(GTK_WINDOW (cal), defx, defy);

    /* Set sorttab number */
    w = gtkpod_builder_xml_get_widget(cal_xml, "sorttab_num_spin");
    gtk_spin_button_set_range(GTK_SPIN_BUTTON (w), 1, sort_tab_widget_get_max_index());
    gtk_spin_button_set_value(GTK_SPIN_BUTTON (w), sort_tab_widget_get_instance(parent));

    /* Set Category-Combo */
    w = gtkpod_builder_xml_get_widget(cal_xml, "cat_combo");

    switch (item) {
    case T_TIME_PLAYED:
        index = CAT_STRING_PLAYED;
        break;
    case T_TIME_MODIFIED:
        index = CAT_STRING_MODIFIED;
        break;
    case T_TIME_ADDED:
        index = CAT_STRING_ADDED;
        break;
    default:
        fprintf(stderr, "Programming error: cal_open_calendar() -- item not found\n");
        break;
    }

    gtk_combo_box_set_active(GTK_COMBO_BOX (w), index);

    /* Make sure we use the current contents of the entry */
    special_sort_tab_page_store_state(page);

    /* set calendar */
    ti = special_sort_tab_page_update_date_interval(page, item, TRUE);

    /* set the calendar if we have a valid TimeInfo */
    if (ti) {
        if (!ti->valid) { /* set to reasonable default */
            ti->lower = 0;
            ti->upper = 0;
        }

        /* Lower Margin */
        w = gtkpod_builder_xml_get_widget(cal_xml, "no_lower_margin");
        g_signal_connect (w,
                "toggled",
                G_CALLBACK (cal_no_margin_toggled),
                cal);
        w = gtkpod_builder_xml_get_widget(cal_xml, "lower_time");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (w), TRUE);
        g_signal_connect (w,
                "toggled",
                G_CALLBACK (cal_time_toggled),
                cal);

        cal_set_time(cal, LOWER_MARGIN, ti->lower);

        /* Upper Margin */
        w = gtkpod_builder_xml_get_widget(cal_xml, "no_upper_margin");
        g_signal_connect (w,
                "toggled",
                G_CALLBACK (cal_no_margin_toggled),
                cal);
        w = gtkpod_builder_xml_get_widget(cal_xml, "upper_time");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (w), TRUE);
        g_signal_connect (w,
                "toggled",
                G_CALLBACK (cal_time_toggled),
                cal);
        cal_set_time(cal, UPPER_MARGIN, ti->upper);
    }

    /* Connect delete-event */
    g_signal_connect (cal, "delete_event",
            G_CALLBACK (cal_delete_event), cal);
    /* Connect cancel-button */
    g_signal_connect (gtkpod_builder_xml_get_widget (cal_xml, "cal_cancel"), "clicked",
            G_CALLBACK (cal_cancel), cal);
    /* Connect apply-button */
    g_signal_connect (gtkpod_builder_xml_get_widget (cal_xml, "cal_apply"), "clicked",
            G_CALLBACK (cal_apply), cal);
    /* Connect ok-button */
    g_signal_connect (gtkpod_builder_xml_get_widget (cal_xml, "cal_ok"), "clicked",
            G_CALLBACK (cal_ok), cal);

    gtk_window_set_transient_for(GTK_WINDOW (cal), GTK_WINDOW (gtkpod_app));
    gtk_widget_show(cal);
}

