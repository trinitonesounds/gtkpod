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
#include "libgtkpod/prefs.h"
#include "libgtkpod/misc.h"
#include "libgtkpod/directories.h"
#include "libgtkpod/gp_private.h"
#include "libgtkpod/gtkpod_app_iface.h"
#include "plugin.h"

static GtkWidget *notebook = NULL;

static void set_pm_sort(gint val) {
    prefs_set_int("pm_sort", val);
    gtkpod_broadcast_preference_change("pm_sort", &val);
}

G_MODULE_EXPORT void on_pm_ascend_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
    if (gtk_toggle_button_get_active(togglebutton))
        set_pm_sort(SORT_ASCENDING);
}

G_MODULE_EXPORT void on_pm_descend_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
    if (gtk_toggle_button_get_active(togglebutton))
        set_pm_sort(SORT_DESCENDING);
}

G_MODULE_EXPORT void on_pm_none_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
    if (gtk_toggle_button_get_active(togglebutton))
        set_pm_sort(SORT_NONE);
}

G_MODULE_EXPORT void on_pm_sort_case_sensitive_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
    gboolean val = gtk_toggle_button_get_active(togglebutton);
    prefs_set_int("pm_case_sensitive", val);
    gtkpod_broadcast_preference_change("pm_case_sensitive", &val);
}

GtkWidget *init_playlist_display_preferences() {
    GtkBuilder *prefbuilder;
    GtkWidget *w = NULL;

    gchar *glade_path = g_build_filename(get_glade_dir(), "playlist_display.xml", NULL);
    prefbuilder = gtkpod_builder_xml_new(glade_path);
    w = gtkpod_builder_xml_get_widget(prefbuilder, "prefs_window");
    notebook = gtkpod_builder_xml_get_widget(prefbuilder, "playlist_settings_notebook");
    g_object_ref(notebook);
    gtk_container_remove(GTK_CONTAINER(w), notebook);
    gtk_widget_destroy(w);
    g_free(glade_path);

    switch (prefs_get_int("pm_sort")) {
    case SORT_ASCENDING:
        w = gtkpod_builder_xml_get_widget(prefbuilder, "pm_ascend");
        break;
    case SORT_DESCENDING:
        w = gtkpod_builder_xml_get_widget(prefbuilder, "pm_descend");
        break;
    case SORT_NONE:
        w = gtkpod_builder_xml_get_widget(prefbuilder, "pm_none");
        break;
    }

    if (w)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);

    if ((w = gtkpod_builder_xml_get_widget(prefbuilder, "pm_cfg_case_sensitive"))) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), prefs_get_int("pm_case_sensitive"));
    }

    gtk_builder_connect_signals(prefbuilder, NULL);
    g_object_unref(prefbuilder);

    return notebook;
}
