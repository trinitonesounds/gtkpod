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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib.h>
#include "libgtkpod/stock_icons.h"
#include "libgtkpod/gtkpod_app_iface.h"
#include "plugin.h"
#include "display_tracks.h"
#include "track_display_actions.h"

/* Parent class. Part of standard class definition */
static gpointer parent_class;

static GtkActionEntry track_actions[] =
    { };

static gboolean activate_track_display_plugin(AnjutaPlugin *plugin) {
    AnjutaUI *ui;
    TrackDisplayPlugin *track_display_plugin;
    GtkActionGroup* action_group;

    /* Prepare the icons for the track */

    track_display_plugin = (TrackDisplayPlugin*) plugin;
    ui = anjuta_shell_get_ui(plugin->shell, NULL);

    /* Add our track_actions */
    action_group
            = anjuta_ui_add_action_group_entries(ui, "ActionGroupTrackDisplay", _("Track Display"), track_actions, G_N_ELEMENTS (track_actions), GETTEXT_PACKAGE, TRUE, plugin);
    track_display_plugin->action_group = action_group;

    /* Merge UI */
    track_display_plugin->uiid = anjuta_ui_merge(ui, UI_FILE);

    /* Add widget in Shell. Any number of widgets can be added */
    track_display_plugin->track_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (track_display_plugin->track_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW (track_display_plugin->track_window), GTK_SHADOW_IN);
    tm_create_track_display (track_display_plugin->track_window);

    g_signal_connect (gtkpod_app, "tracks_selected", G_CALLBACK (track_display_set_tracks_cb), NULL);
    g_signal_connect (gtkpod_app, "playlist_selected", G_CALLBACK (track_display_set_playlist_cb), NULL);
    g_signal_connect (gtkpod_app, "sort_enablement", G_CALLBACK (track_display_set_sort_enablement), NULL);

    gtk_widget_show_all(track_display_plugin->track_window);
    anjuta_shell_add_widget(plugin->shell, track_display_plugin->track_window, "TrackDisplayPlugin", "Playlist Tracks", NULL, ANJUTA_SHELL_PLACEMENT_TOP, NULL);

    return TRUE; /* FALSE if activation failed */
}

static gboolean deactivate_track_display_plugin(AnjutaPlugin *plugin) {
    AnjutaUI *ui;
    TrackDisplayPlugin *track_display_plugin;

    track_display_plugin = (TrackDisplayPlugin*) plugin;
    ui = anjuta_shell_get_ui(plugin->shell, NULL);

    /* Remove widgets from Shell */
    anjuta_shell_remove_widget(plugin->shell, track_display_plugin->track_window, NULL);

    /* Destroy the treeview */
    tm_destroy_widgets();
    track_display_plugin->track_view = NULL;
    track_display_plugin->track_window = NULL;

    /* Unmerge UI */
    anjuta_ui_unmerge(ui, track_display_plugin->uiid);

    /* Remove Action groups */
    anjuta_ui_remove_action_group(ui, track_display_plugin->action_group);

    /* FALSE if plugin doesn't want to deactivate */
    return TRUE;
}

static void track_display_plugin_instance_init(GObject *obj) {
    TrackDisplayPlugin *plugin = (TrackDisplayPlugin*) obj;
    plugin->uiid = 0;
    plugin->track_view = NULL;
    plugin->track_window = NULL;
    plugin->action_group = NULL;
}

static void track_display_plugin_class_init(GObjectClass *klass) {
    AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

    parent_class = g_type_class_peek_parent(klass);

    plugin_class->activate = activate_track_display_plugin;
    plugin_class->deactivate = deactivate_track_display_plugin;
}

ANJUTA_PLUGIN_BEGIN (TrackDisplayPlugin, track_display_plugin);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (TrackDisplayPlugin, track_display_plugin);

