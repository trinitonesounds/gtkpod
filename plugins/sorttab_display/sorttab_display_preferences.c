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
#include "libgtkpod/prefs.h"
#include "libgtkpod/directories.h"
#include "libgtkpod/gp_private.h"
#include "plugin.h"
#include "display_sorttabs.h"

G_MODULE_EXPORT void on_filter_tabs_count_value_changed (GtkSpinButton *sender, gpointer e)
{
    gint num = gtk_spin_button_get_value_as_int (sender);

    /* Update the number of filter tabs */
    prefs_set_int ("sort_tab_num", num);
    st_show_visible();
}

/*
    glade callback
*/
G_MODULE_EXPORT void on_group_compilations_toggled (GtkToggleButton *sender, gpointer e)
{
    gboolean active = gtk_toggle_button_get_active (sender);

    prefs_set_int ("group_compilations", active);
    st_show_visible();
}

G_MODULE_EXPORT void on_st_ascend_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
    if (gtk_toggle_button_get_active(togglebutton))
        st_sort(SORT_ASCENDING);
}

G_MODULE_EXPORT void on_st_descend_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
    if (gtk_toggle_button_get_active(togglebutton))
        st_sort(SORT_DESCENDING);
}

G_MODULE_EXPORT void on_st_none_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
    if (gtk_toggle_button_get_active(togglebutton))
        st_sort(SORT_NONE);
}

G_MODULE_EXPORT void on_st_sort_case_sensitive_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
    gboolean val = gtk_toggle_button_get_active(togglebutton);
    prefs_set_int("st_case_sensitive", val);
    gtkpod_broadcast_preference_change("st_case_sensitive", val);
}

GtkWidget *init_sorttab_preferences() {
    GtkWidget *notebook;
    GtkBuilder *pref_xml;
    GtkWidget *w, *win;

    gchar *glade_path = g_build_filename(get_glade_dir(), "sorttab_display.xml", NULL);
    pref_xml = gtkpod_builder_xml_new(glade_path);
    win = gtkpod_builder_xml_get_widget(pref_xml, "preference_window");
    notebook = gtkpod_builder_xml_get_widget(pref_xml, "sorttab_settings_notebook");
    g_object_ref(notebook);
    gtk_container_remove(GTK_CONTAINER (win), notebook);
    g_free(glade_path);

    switch (prefs_get_int("st_sort")) {
    case SORT_ASCENDING:
        w = gtkpod_builder_xml_get_widget(pref_xml, "st_ascend");
        break;
    case SORT_DESCENDING:
        w = gtkpod_builder_xml_get_widget(pref_xml, "st_descend");
        break;
    default:
        w = gtkpod_builder_xml_get_widget(pref_xml, "st_none");
        break;
    }

    if (w)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);

    if ((w = gtkpod_builder_xml_get_widget(pref_xml, "st_cfg_case_sensitive"))) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), prefs_get_int("st_case_sensitive"));
    }

    if ((w = gtkpod_builder_xml_get_widget(pref_xml, "filter_tabs_count"))) {
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(w), prefs_get_int ("sort_tab_num"));
    }

    gtk_builder_connect_signals(pref_xml, NULL);

    return notebook;
}
