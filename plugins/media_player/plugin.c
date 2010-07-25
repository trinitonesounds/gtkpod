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
#include "libgtkpod/gtkpod_app_iface.h"
#include "libgtkpod/directories.h"
#include "libgtkpod/stock_icons.h"
#include "plugin.h"
#include "media_player.h"

#define PLAYER_VOLUME_ICON "media_player-volume-control"
#define PLAYER_VOLUME_ICON_STOCK_ID "media_player-volume-control-icon"

/* Parent class. Part of standard class definition */
static gpointer parent_class;

static GtkActionEntry cover_actions[] =
    { };

static gboolean activate_plugin(AnjutaPlugin *plugin) {
    AnjutaUI *ui;
    MediaPlayerPlugin *media_player_plugin;
    GtkActionGroup* action_group;

    register_icon_path(get_plugin_dir(), "media_player");
    register_stock_icon(PLAYER_VOLUME_ICON, PLAYER_VOLUME_ICON_STOCK_ID);

    media_player_plugin = (MediaPlayerPlugin*) plugin;
    ui = anjuta_shell_get_ui(plugin->shell, NULL);

    /* Add our cover_actions */
    action_group
            = anjuta_ui_add_action_group_entries(ui, "ActionGroupMediaPlayer", _("Cover Display"), cover_actions, G_N_ELEMENTS (cover_actions), GETTEXT_PACKAGE, TRUE, plugin);
    media_player_plugin->action_group = action_group;

    /* Merge UI */
    gchar *uipath = g_build_filename(get_ui_dir(), "media_player.ui", NULL);
    media_player_plugin->uiid = anjuta_ui_merge(ui, uipath);
    g_free(uipath);

    /* Add widget in Shell. Any number of widgets can be added */
    media_player_plugin->media_player_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_ref(media_player_plugin->media_player_window);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (media_player_plugin->media_player_window), GTK_POLICY_NEVER, GTK_POLICY_NEVER);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW (media_player_plugin->media_player_window), GTK_SHADOW_IN);

    g_signal_connect (gtkpod_app, SIGNAL_TRACKS_DISPLAYED, G_CALLBACK (media_player_set_tracks_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_TRACK_REMOVED, G_CALLBACK (media_player_track_removed_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_TRACKS_SELECTED, G_CALLBACK (media_player_set_tracks_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_TRACK_UPDATED, G_CALLBACK (media_player_track_updated_cb), NULL);

    init_media_player(media_player_plugin->media_player_window);

    // Do not show all as video widget is initially invisible
    gtk_widget_show(media_player_plugin->media_player_window);
    anjuta_shell_add_widget(plugin->shell, media_player_plugin->media_player_window, "MediaPlayerPlugin", "Media Player", NULL, ANJUTA_SHELL_PLACEMENT_BOTTOM, NULL);

    return TRUE; /* FALSE if activation failed */
}

static gboolean deactivate_plugin(AnjutaPlugin *plugin) {
    AnjutaUI *ui;
    MediaPlayerPlugin *media_player_plugin;

    media_player_plugin = (MediaPlayerPlugin*) plugin;
    ui = anjuta_shell_get_ui(plugin->shell, NULL);

    /* Destroy the browser */
    destroy_media_player();

    /* Remove widgets from Shell */
    anjuta_shell_remove_widget(plugin->shell, media_player_plugin->media_player_window, NULL);

    /* Unmerge UI */
    anjuta_ui_unmerge(ui, media_player_plugin->uiid);

    /* Remove Action groups */
    anjuta_ui_remove_action_group(ui, media_player_plugin->action_group);

    /* FALSE if plugin doesn't want to deactivate */
    return TRUE;
}

static void media_player_plugin_instance_init(GObject *obj) {
    MediaPlayerPlugin *plugin = (MediaPlayerPlugin*) obj;
    plugin->uiid = 0;
    plugin->media_player_window = NULL;
    plugin->action_group = NULL;
}

static void media_player_plugin_class_init(GObjectClass *klass) {
    AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

    parent_class = g_type_class_peek_parent(klass);

    plugin_class->activate = activate_plugin;
    plugin_class->deactivate = deactivate_plugin;
}

//static void ipreferences_merge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e) {
//    GdkPixbuf *pixbuf;
//    GError *error = NULL;
//
//    MediaPlayerPlugin* plugin = MEDIA_PLAYER_PLUGIN(ipref);
//    plugin->prefs = init_media_player_preferences(plugin->glade_path);
//    if (plugin->prefs == NULL)
//        return;
//
//    pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), PREFERENCE_ICON, 48, 0, &error);
//
//    if (!pixbuf) {
//        g_warning ("Couldn't load icon: %s", error->message);
//        g_error_free(error);
//    }
//    anjuta_preferences_dialog_add_page(ANJUTA_PREFERENCES_DIALOG (anjuta_preferences_get_dialog (prefs)), "gtkpod-media_player-settings", _(TAB_NAME), pixbuf, plugin->prefs);
//    g_object_unref(pixbuf);
//}
//
//static void ipreferences_unmerge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e) {
//    anjuta_preferences_remove_page(prefs, _(TAB_NAME));
//    MediaPlayerPlugin* plugin = MEDIA_PLAYER_PLUGIN(ipref);
//    gtk_widget_destroy(plugin->prefs);
//}
//
//static void ipreferences_iface_init(IAnjutaPreferencesIface* iface) {
//    iface->merge = ipreferences_merge;
//    iface->unmerge = ipreferences_unmerge;
//}

ANJUTA_PLUGIN_BEGIN (MediaPlayerPlugin, media_player_plugin);
//        ANJUTA_PLUGIN_ADD_INTERFACE(ipreferences, IANJUTA_TYPE_PREFERENCES);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (MediaPlayerPlugin, media_player_plugin);
