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
 |  $Id$
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/interfaces/ianjuta-preferences.h>
#include "libgtkpod/gtkpod_app_iface.h"
#include "libgtkpod/prefs.h"
#include "libgtkpod/stock_icons.h"
#include "libgtkpod/directories.h"
#include "plugin.h"
#include "clarity_widget.h"
#include "clarity_preferences.h"

#define TAB_NAME "Clarity"

/* Parent class. Part of standard class definition */
static gpointer parent_class;

static GtkActionEntry cover_actions[] =
    { };

static void set_default_preferences() {
    if (!prefs_get_string_value("clarity_bg_color", NULL))
        prefs_set_string("clarity_bg_color", "#000000");

    if (!prefs_get_string_value("clarity_fg_color", NULL))
        prefs_set_string("clarity_fg_color", "#FFFFFF");
}

static gboolean activate_plugin(AnjutaPlugin *plugin) {
    AnjutaUI *ui;
    ClarityPlugin *clarity_plugin;
    GtkActionGroup* action_group;

    clarity_plugin = (ClarityPlugin*) plugin;

    register_icon_path(get_plugin_dir(), "clarity");
    register_stock_icon(DEFAULT_COVER_ICON, DEFAULT_COVER_ICON_STOCK_ID);

    ui = anjuta_shell_get_ui(plugin->shell, NULL);

    /* Add our cover_actions */
    action_group
            = anjuta_ui_add_action_group_entries(ui, "ActionGroupClarity", _("Cover Display"), cover_actions, G_N_ELEMENTS (cover_actions), GETTEXT_PACKAGE, TRUE, plugin);
    clarity_plugin->action_group = action_group;

    /* Merge UI */
    gchar *uipath = g_build_filename(get_ui_dir(), "clarity.ui", NULL);
    clarity_plugin->uiid = anjuta_ui_merge(ui, uipath);
    g_free(uipath);

    /* Set preferences */
    set_default_preferences();

    /* Add widget in Shell. Any number of widgets can be added */
    clarity_plugin->cover_window = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new(NULL, NULL));
    g_object_ref(clarity_plugin->cover_window);
    gtk_scrolled_window_set_policy(clarity_plugin->cover_window, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(clarity_plugin->cover_window, GTK_SHADOW_IN);

    clarity_plugin->clarity_widget = CLARITY_WIDGET(clarity_widget_new ());
    gtk_scrolled_window_add_with_viewport(clarity_plugin->cover_window, GTK_WIDGET(clarity_plugin->clarity_widget));
    gtk_widget_show_all(GTK_WIDGET(clarity_plugin->cover_window));
    anjuta_shell_add_widget(plugin->shell, GTK_WIDGET(clarity_plugin->cover_window), "ClarityPlugin", _("  Clarity Cover Display"), NULL, ANJUTA_SHELL_PLACEMENT_CENTER, NULL);

    g_signal_connect (gtkpod_app, SIGNAL_PREFERENCE_CHANGE, G_CALLBACK (clarity_widget_preference_changed_cb), clarity_plugin->clarity_widget);
    g_signal_connect (gtkpod_app, SIGNAL_PLAYLIST_SELECTED, G_CALLBACK (clarity_widget_playlist_selected_cb), clarity_plugin->clarity_widget);
    g_signal_connect (gtkpod_app, SIGNAL_PLAYLIST_REMOVED, G_CALLBACK (clarity_widget_playlist_removed_cb), clarity_plugin->clarity_widget);
    g_signal_connect (gtkpod_app, SIGNAL_TRACK_REMOVED, G_CALLBACK (clarity_widget_track_removed_cb), clarity_plugin->clarity_widget);
    g_signal_connect (gtkpod_app, SIGNAL_TRACKS_DISPLAYED, G_CALLBACK (clarity_widget_tracks_selected_cb), clarity_plugin->clarity_widget);
    g_signal_connect (gtkpod_app, SIGNAL_TRACKS_SELECTED, G_CALLBACK (clarity_widget_tracks_selected_cb), clarity_plugin->clarity_widget);
    g_signal_connect (gtkpod_app, SIGNAL_TRACK_UPDATED, G_CALLBACK (clarity_widget_track_updated_cb), clarity_plugin->clarity_widget);
    g_signal_connect (gtkpod_app, SIGNAL_TRACK_ADDED, G_CALLBACK (clarity_widget_track_added_cb), clarity_plugin->clarity_widget);

    return TRUE; /* FALSE if activation failed */
}

static gboolean deactivate_plugin(AnjutaPlugin *plugin) {
    AnjutaUI *ui;
    ClarityPlugin *clarity_plugin;

    clarity_plugin = (ClarityPlugin*) plugin;

    g_signal_handlers_disconnect_by_func(plugin->shell, G_CALLBACK (clarity_widget_preference_changed_cb), clarity_plugin->clarity_widget);
    g_signal_handlers_disconnect_by_func(plugin->shell, G_CALLBACK (clarity_widget_playlist_selected_cb), clarity_plugin->clarity_widget);
    g_signal_handlers_disconnect_by_func(plugin->shell, G_CALLBACK (clarity_widget_track_removed_cb), clarity_plugin->clarity_widget);
    g_signal_handlers_disconnect_by_func(plugin->shell, G_CALLBACK (clarity_widget_tracks_selected_cb), clarity_plugin->clarity_widget);
    g_signal_handlers_disconnect_by_func(plugin->shell, G_CALLBACK (clarity_widget_track_updated_cb), clarity_plugin->clarity_widget);
    g_signal_handlers_disconnect_by_func(plugin->shell, G_CALLBACK (clarity_widget_track_added_cb), clarity_plugin->clarity_widget);

    ui = anjuta_shell_get_ui(plugin->shell, NULL);

    /* Remove widgets from Shell */
    anjuta_shell_remove_widget(plugin->shell, GTK_WIDGET(clarity_plugin->cover_window), NULL);

    /* Unmerge UI */
    anjuta_ui_unmerge(ui, clarity_plugin->uiid);

    /* Destroy the widget */
    gtk_widget_destroy(GTK_WIDGET(clarity_plugin->clarity_widget));
    clarity_plugin->clarity_widget = NULL;

    /* Remove Action groups */
    anjuta_ui_remove_action_group(ui, clarity_plugin->action_group);

    g_free(clarity_plugin->gladepath);

    /* FALSE if plugin doesn't want to deactivate */
    return TRUE;
}

static void clarity_plugin_instance_init(GObject *obj) {
    ClarityPlugin *plugin = (ClarityPlugin*) obj;
    plugin->uiid = 0;
    plugin->cover_window = NULL;
    plugin->action_group = NULL;
    plugin->gladepath = g_build_filename(get_glade_dir(), "clarity.xml", NULL);
}

static void clarity_plugin_class_init(GObjectClass *klass) {
    AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

    parent_class = g_type_class_peek_parent(klass);

    plugin_class->activate = activate_plugin;
    plugin_class->deactivate = deactivate_plugin;
}

static void ipreferences_merge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e) {
    GdkPixbuf *pixbuf;
    GError *error = NULL;

    ClarityPlugin* plugin = CLARITY_PLUGIN(ipref);
    plugin->prefs = init_clarity_preferences(plugin->gladepath, plugin->clarity_widget);
    if (plugin->prefs == NULL)
        return;

    pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), DEFAULT_COVER_ICON, 48, 0, &error);

    if (!pixbuf) {
        g_warning ("Couldn't load icon: %s", error->message);
        g_error_free(error);
    }
    anjuta_preferences_dialog_add_page(ANJUTA_PREFERENCES_DIALOG (anjuta_preferences_get_dialog (prefs)), "gtkpod-clarity-settings", _(TAB_NAME), pixbuf, plugin->prefs);
    g_object_unref(pixbuf);
}

static void ipreferences_unmerge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e) {
    anjuta_preferences_remove_page(prefs, _(TAB_NAME));
    ClarityPlugin* plugin = CLARITY_PLUGIN(ipref);
    gtk_widget_destroy(plugin->prefs);
}

static void ipreferences_iface_init(IAnjutaPreferencesIface* iface) {
    iface->merge = ipreferences_merge;
    iface->unmerge = ipreferences_unmerge;
}

ANJUTA_PLUGIN_BEGIN (ClarityPlugin, clarity_plugin);
        ANJUTA_PLUGIN_ADD_INTERFACE(ipreferences, IANJUTA_TYPE_PREFERENCES);ANJUTA_PLUGIN_END
;

ANJUTA_SIMPLE_PLUGIN (ClarityPlugin, clarity_plugin)
;
