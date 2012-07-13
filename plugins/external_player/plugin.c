/*
 |  Copyright (C) 2002-2012 Paul Richardson <phantom_sf at users.sourceforge.net>
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
#include "libgtkpod/prefs.h"
#include <libanjuta/interfaces/ianjuta-preferences.h>
#include "libgtkpod/directories.h"
#include "plugin.h"
#include "external_player.h"

#define TAB_NAME _("External Media Player")

/* Parent class. Part of standard class definition */
static gpointer parent_class;

static GtkActionEntry external_player_actions[] =
    { };

static void set_default_preferences() {
    if (! prefs_get_string_value("path_play_now", NULL))
        prefs_set_string ("path_play_now", "xmms -e %s");

    if (! prefs_get_int_value("path_play_enqueue", NULL))
        prefs_set_string ("path_play_enqueue", "xmms -e %s");
}

static gboolean activate_plugin(AnjutaPlugin *plugin) {
    AnjutaUI *ui;
    ExternalPlayerPlugin *external_player_plugin;
    GtkActionGroup* action_group;

    /* Set preferences */
    set_default_preferences();

    external_player_plugin = (ExternalPlayerPlugin*) plugin;
    ui = anjuta_shell_get_ui(plugin->shell, NULL);

    /* Add our cover_actions */
    action_group
            = anjuta_ui_add_action_group_entries(ui, "ActionGroupExternalPlayer", _("External Player"), external_player_actions, G_N_ELEMENTS (external_player_actions), GETTEXT_PACKAGE, TRUE, plugin);
    external_player_plugin->action_group = action_group;

    /* Merge UI */
    gchar *uipath = g_build_filename(get_ui_dir(), "external_player.ui", NULL);
    external_player_plugin->uiid = anjuta_ui_merge(ui, uipath);
    g_free(uipath);

    gtkpod_register_track_command(TRACK_COMMAND(external_player_plugin));

    return TRUE; /* FALSE if activation failed */
}

static gboolean deactivate_plugin(AnjutaPlugin *plugin) {
    AnjutaUI *ui;
    ExternalPlayerPlugin *external_player_plugin;

    external_player_plugin = (ExternalPlayerPlugin*) plugin;
    ui = anjuta_shell_get_ui(plugin->shell, NULL);

    gtkpod_unregister_track_command(TRACK_COMMAND(external_player_plugin));

    /* Unmerge UI */
    anjuta_ui_unmerge(ui, external_player_plugin->uiid);

    /* Remove Action groups */
    anjuta_ui_remove_action_group(ui, external_player_plugin->action_group);

    /* FALSE if plugin doesn't want to deactivate */
    return TRUE;
}

static void external_player_plugin_instance_init(GObject *obj) {
    ExternalPlayerPlugin *plugin = (ExternalPlayerPlugin*) obj;
    plugin->uiid = 0;
    plugin->action_group = NULL;
    plugin->prefs = NULL;
}

static void external_player_plugin_class_init(GObjectClass *klass) {
    AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

    parent_class = g_type_class_peek_parent(klass);

    plugin_class->activate = activate_plugin;
    plugin_class->deactivate = deactivate_plugin;
}

static void track_command_iface_init(TrackCommandInterface *iface) {
    iface->id = "external_player_play_track_command";
    iface->text = _("Play with preferred app...");
    iface->execute = external_player_play_tracks;
}

static void ipreferences_merge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e) {
    GdkPixbuf *pixbuf;
    GdkPixbuf *scaled;

    ExternalPlayerPlugin* plugin = EXTERNAL_PLAYER_PLUGIN(ipref);
    plugin->prefs = init_external_player_preferences();
    if (plugin->prefs == NULL)
        return;

    pixbuf = gtk_widget_render_icon_pixbuf(plugin->prefs, GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_LARGE_TOOLBAR);
    if (!pixbuf) {
        g_warning (N_("Couldn't load theme media player icon"));
    }

    scaled = gdk_pixbuf_scale_simple(pixbuf, 48, 48, GDK_INTERP_BILINEAR);

    anjuta_preferences_dialog_add_page(ANJUTA_PREFERENCES_DIALOG (anjuta_preferences_get_dialog (prefs)), "gtkpod-external-player-settings", TAB_NAME, scaled, plugin->prefs);
    g_object_unref(scaled);
    g_object_unref(pixbuf);
}

static void ipreferences_unmerge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e) {
    anjuta_preferences_remove_page(prefs, TAB_NAME);
    ExternalPlayerPlugin* plugin = EXTERNAL_PLAYER_PLUGIN(ipref);
    gtk_widget_destroy(plugin->prefs);
}

static void ipreferences_iface_init(IAnjutaPreferencesIface* iface) {
    iface->merge = ipreferences_merge;
    iface->unmerge = ipreferences_unmerge;
}

ANJUTA_PLUGIN_BEGIN (ExternalPlayerPlugin, external_player_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE(track_command, TRACK_COMMAND_TYPE);
ANJUTA_PLUGIN_ADD_INTERFACE(ipreferences, IANJUTA_TYPE_PREFERENCES);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (ExternalPlayerPlugin, external_player_plugin);
