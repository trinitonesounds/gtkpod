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
#include "display_coverart.h"
#include "cover_display_preferences.h"

#define TAB_NAME _("Coverart Display")

/* Parent class. Part of standard class definition */
static gpointer parent_class;

static GtkActionEntry cover_actions[] =
    { };

static void set_default_preferences() {
    if (!prefs_get_string_value("coverart_display_bg_color", NULL))
        prefs_set_string("coverart_display_bg_color", "#000000");

    if (!prefs_get_string_value("coverart_display_fg_color", NULL))
        prefs_set_string("coverart_display_fg_color", "#FFFFFF");

    if (! prefs_get_int_value("cad_case_sensitive", NULL))
        prefs_set_int("cad_case_sensitive", FALSE);
}

static gboolean activate_plugin(AnjutaPlugin *plugin) {
    AnjutaUI *ui;
    CoverDisplayPlugin *cover_display_plugin;
    GtkActionGroup* action_group;

    cover_display_plugin = (CoverDisplayPlugin*) plugin;

    register_icon_path(get_plugin_dir(), "cover_display");
    register_stock_icon(DEFAULT_COVER_ICON, DEFAULT_COVER_ICON_STOCK_ID);

    ui = anjuta_shell_get_ui(plugin->shell, NULL);

    /* Add our cover_actions */
    action_group
            = anjuta_ui_add_action_group_entries(ui, "ActionGroupCoverDisplay", _("Cover Display"), cover_actions, G_N_ELEMENTS (cover_actions), GETTEXT_PACKAGE, TRUE, plugin);
    cover_display_plugin->action_group = action_group;

    /* Merge UI */
    gchar *uipath = g_build_filename(get_ui_dir(), "cover_display.ui", NULL);
    cover_display_plugin->uiid = anjuta_ui_merge(ui, uipath);
    g_free(uipath);

    /* Set preferences */
    set_default_preferences();

    /* Add widget in Shell. Any number of widgets can be added */
    cover_display_plugin->cover_window = gtk_scrolled_window_new(NULL, NULL);
    g_object_ref(cover_display_plugin->cover_window);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (cover_display_plugin->cover_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW (cover_display_plugin->cover_window), GTK_SHADOW_IN);

    g_signal_connect (gtkpod_app, SIGNAL_PLAYLIST_SELECTED, G_CALLBACK (coverart_display_update_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_TRACK_REMOVED, G_CALLBACK (coverart_display_track_removed_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_TRACKS_DISPLAYED, G_CALLBACK (coverart_display_set_tracks_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_TRACKS_SELECTED, G_CALLBACK (coverart_display_set_tracks_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_TRACK_UPDATED, G_CALLBACK (coverart_display_track_updated_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_TRACK_ADDED, G_CALLBACK (coverart_display_track_added_cb), NULL);

    coverart_init_display(cover_display_plugin->cover_window, cover_display_plugin->gladepath);
    anjuta_shell_add_widget(plugin->shell, cover_display_plugin->cover_window, "CoverDisplayPlugin", _("  Cover Artwork"), NULL, ANJUTA_SHELL_PLACEMENT_CENTER, NULL);

    coverart_block_change(FALSE);
    coverart_display_update(TRUE);

    return TRUE; /* FALSE if activation failed */
}

static gboolean deactivate_plugin(AnjutaPlugin *plugin) {
    AnjutaUI *ui;
    CoverDisplayPlugin *cover_display_plugin;

    coverart_block_change(TRUE);

    g_signal_handlers_disconnect_by_func(plugin->shell, G_CALLBACK (coverart_display_update_cb), NULL);
    g_signal_handlers_disconnect_by_func(plugin->shell, G_CALLBACK (coverart_display_track_removed_cb), NULL);
    g_signal_handlers_disconnect_by_func(plugin->shell, G_CALLBACK (coverart_display_set_tracks_cb), NULL);
    g_signal_handlers_disconnect_by_func(plugin->shell, G_CALLBACK (coverart_display_track_updated_cb), NULL);
    g_signal_handlers_disconnect_by_func(plugin->shell, G_CALLBACK (coverart_display_track_added_cb), NULL);

    cover_display_plugin = (CoverDisplayPlugin*) plugin;
    ui = anjuta_shell_get_ui(plugin->shell, NULL);

    /* Remove widgets from Shell */
    anjuta_shell_remove_widget(plugin->shell, cover_display_plugin->cover_window, NULL);

    /* Destroy the treeview */
    destroy_coverart_display();

    /* Unmerge UI */
    anjuta_ui_unmerge(ui, cover_display_plugin->uiid);

    /* Remove Action groups */
    anjuta_ui_remove_action_group(ui, cover_display_plugin->action_group);

    g_free(cover_display_plugin->gladepath);

    /* FALSE if plugin doesn't want to deactivate */
    return TRUE;
}

static void cover_display_plugin_instance_init(GObject *obj) {
    CoverDisplayPlugin *plugin = (CoverDisplayPlugin*) obj;
    plugin->uiid = 0;
    plugin->cover_window = NULL;
    plugin->action_group = NULL;
    plugin->gladepath = g_build_filename(get_glade_dir(), "cover_display.xml", NULL);
}

static void cover_display_plugin_class_init(GObjectClass *klass) {
    AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

    parent_class = g_type_class_peek_parent(klass);

    plugin_class->activate = activate_plugin;
    plugin_class->deactivate = deactivate_plugin;
}

static void ipreferences_merge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e) {
    GdkPixbuf *pixbuf;
    GError *error = NULL;

    CoverDisplayPlugin* plugin = COVER_DISPLAY_PLUGIN(ipref);
    plugin->prefs = init_cover_preferences(plugin->gladepath);
    if (plugin->prefs == NULL)
        return;

    pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), DEFAULT_COVER_ICON, 48, 0, &error);

    if (!pixbuf) {
        g_warning ("Couldn't load icon: %s", error->message);
        g_error_free(error);
    }
    anjuta_preferences_dialog_add_page(ANJUTA_PREFERENCES_DIALOG (anjuta_preferences_get_dialog (prefs)), "gtkpod-coverart-settings", TAB_NAME, pixbuf, plugin->prefs);
    g_object_unref(pixbuf);
}

static void ipreferences_unmerge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e) {
    anjuta_preferences_remove_page(prefs, TAB_NAME);
    CoverDisplayPlugin* plugin = COVER_DISPLAY_PLUGIN(ipref);
    gtk_widget_destroy(plugin->prefs);
}

static void ipreferences_iface_init(IAnjutaPreferencesIface* iface) {
    iface->merge = ipreferences_merge;
    iface->unmerge = ipreferences_unmerge;
}

ANJUTA_PLUGIN_BEGIN (CoverDisplayPlugin, cover_display_plugin);
        ANJUTA_PLUGIN_ADD_INTERFACE(ipreferences, IANJUTA_TYPE_PREFERENCES);ANJUTA_PLUGIN_END
;

ANJUTA_SIMPLE_PLUGIN (CoverDisplayPlugin, cover_display_plugin)
;
