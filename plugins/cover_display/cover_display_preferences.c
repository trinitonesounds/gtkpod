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

#include "libgtkpod/misc.h"
#include "libgtkpod/prefs.h"
#include "libgtkpod/gp_private.h"
#include "plugin.h"
#include "display_coverart.h"
#include "cover_display_preferences.h"

/*
    glade callback
*/
G_MODULE_EXPORT void on_coverart_dialog_bg_color_set (GtkColorButton *widget, gpointer user_data)
{
    GdkRGBA color;
    gtk_color_button_get_rgba (widget, &color);
    gchar *color_string = gdk_rgba_to_string(&color);

    prefs_set_string ("coverart_display_bg_color", color_string);
    g_free (color_string);
    coverart_display_update (FALSE);
}

/*
    glade callback
*/
G_MODULE_EXPORT void on_coverart_dialog_fg_color_set (GtkColorButton *widget, gpointer user_data)
{
    GdkRGBA color;
    gtk_color_button_get_rgba (widget, &color);
    gchar *color_string = gdk_rgba_to_string(&color);

    prefs_set_string ("coverart_display_fg_color", color_string);
    g_free (color_string);
    coverart_display_update (FALSE);
}

G_MODULE_EXPORT void on_cad_ascend_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
    if (gtk_toggle_button_get_active(togglebutton))
        coverart_display_sort(SORT_ASCENDING);
}

G_MODULE_EXPORT void on_cad_descend_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
    if (gtk_toggle_button_get_active(togglebutton))
        coverart_display_sort(SORT_DESCENDING);
}

G_MODULE_EXPORT void on_cad_none_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
    if (gtk_toggle_button_get_active(togglebutton))
        coverart_display_sort(SORT_NONE);
}

G_MODULE_EXPORT void on_cad_sort_case_sensitive_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
    gboolean val = gtk_toggle_button_get_active(togglebutton);
    prefs_set_int("cad_case_sensitive", val);
    gtkpod_broadcast_preference_change("cad_case_sensitive", &val);
}

GtkWidget *init_cover_preferences(gchar *gladepath) {
    GtkWidget *notebook;
    GtkBuilder *pref_xml;
    GtkWidget *coverart_bgcolorselect_button;
    GtkWidget *coverart_fgcolorselect_button;
    GtkWidget *w, *win;
    GdkRGBA *color;

    pref_xml = gtkpod_builder_xml_new(gladepath);
    win = gtkpod_builder_xml_get_widget(pref_xml, "preference_window");
    notebook = gtkpod_builder_xml_get_widget(pref_xml, "cover_settings_notebook");
    coverart_bgcolorselect_button = gtkpod_builder_xml_get_widget (pref_xml, "coverart_display_bg_button");
    coverart_fgcolorselect_button = gtkpod_builder_xml_get_widget (pref_xml, "coverart_display_fg_button");
    g_object_ref(notebook);
    gtk_container_remove(GTK_CONTAINER (win), notebook);

    color = coverart_get_background_display_color();
    gtk_color_button_set_rgba (GTK_COLOR_BUTTON(coverart_bgcolorselect_button), color);
    gdk_rgba_free(color);

    color = coverart_get_foreground_display_color();
    gtk_color_button_set_rgba (GTK_COLOR_BUTTON(coverart_fgcolorselect_button), color);
    gdk_rgba_free(color);

    switch (prefs_get_int("cad_sort")) {
    case SORT_ASCENDING:
        w = gtkpod_builder_xml_get_widget(pref_xml, "cad_ascend");
        break;
    case SORT_DESCENDING:
        w = gtkpod_builder_xml_get_widget(pref_xml, "cad_descend");
        break;
    default:
        w = gtkpod_builder_xml_get_widget(pref_xml, "cad_none");
        break;
    }

    if (w)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);

    if ((w = gtkpod_builder_xml_get_widget(pref_xml, "cad_cfg_case_sensitive"))) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), prefs_get_int("cad_case_sensitive"));
    }

    gtk_builder_connect_signals(pref_xml, NULL);
    return notebook;
}
