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
#include <libanjuta/interfaces/ianjuta-preferences.h>
#include "libgtkpod/stock_icons.h"
#include "libgtkpod/directories.h"
#include "libgtkpod/gtkpod_app_iface.h"
#include "libgtkpod/gp_private.h"
#include "libgtkpod/prefs.h"
#include "plugin.h"
#include "display_tracks.h"
#include "track_display_actions.h"
#include "track_display_preferences.h"

#define PREFERENCE_ICON "track_display-track-category"
#define PREFERENCE_ICON_STOCK_ID "track_display-preference-icon"
#define TAB_NAME "Track Display"

/* Parent class. Part of standard class definition */
static gpointer parent_class;

static GtkActionEntry track_actions[] =
    {
        {
            "ActionDeleteSelectedTracksFromPlaylist", GTK_STOCK_DELETE, N_("Selected Tracks from Playlist"), NULL,
            NULL, G_CALLBACK (on_delete_selected_tracks_from_playlist) },
        {
            "ActionDeleteSelectedTracksFromDatabase", GTK_STOCK_DELETE, N_("Selected Tracks from Database"), NULL,
            NULL, G_CALLBACK (on_delete_selected_tracks_from_database) },
        {
            "ActionDeleteSelectedTracksFromDevice", GTK_STOCK_DELETE, N_("Selected Tracks from Device"), NULL, NULL,
            G_CALLBACK (on_delete_selected_tracks_from_device) },
        {
            "ActionUpdateTracks", GTK_STOCK_REFRESH, N_("Selected Tracks"), NULL, NULL,
            G_CALLBACK (on_update_selected_tracks) },
        {
            "ActionUpdateMservTracks", GTK_STOCK_REFRESH, N_("Selected Tracks"), NULL, NULL,
            G_CALLBACK (on_update_mserv_selected_tracks)
        },
    };

static void set_default_preferences() {
    gint int_buf;

    /* sm_autostore renamed to tm_autostore */
    if (prefs_get_int_value("sm_autostore", &int_buf)) {
        prefs_set_int("tm_autostore", int_buf);
        prefs_set_string("sm_autostore", NULL);
    }

    /* sm_sortcol renamed to tm_sortcol */
    if (prefs_get_int_value("sm_sortcol", &int_buf)) {
        prefs_set_int("tm_sortcol", int_buf);
        prefs_set_string("sm_sortcol", NULL);
    }

    /* sm_sort_ renamed to tm_sort */
    if (prefs_get_int_value("sm_sort_", &int_buf)) {
        prefs_set_int("tm_sort", int_buf);
        prefs_set_string("sm_sort_", NULL);
    }

    if (prefs_get_int_value("tm_autostore", NULL))
        prefs_set_int("tm_autostore", FALSE);

    if (prefs_get_int_value("tm_sortcol", NULL))
        prefs_set_int("tm_sortcol", TM_COLUMN_TITLE);

    if (prefs_get_int_value("tm_sort", NULL))
        prefs_set_int("tm_sort", SORT_NONE);

}

static gboolean activate_track_display_plugin(AnjutaPlugin *plugin) {
    AnjutaUI *ui;
    TrackDisplayPlugin *track_display_plugin;
    GtkActionGroup* action_group;

    register_icon_path(get_plugin_dir(), "track_display");
    register_stock_icon(PREFERENCE_ICON, PREFERENCE_ICON_STOCK_ID);

    track_display_plugin = (TrackDisplayPlugin*) plugin;
    ui = anjuta_shell_get_ui(plugin->shell, NULL);

    /* Add our track_actions */
    action_group
            = anjuta_ui_add_action_group_entries(ui, "ActionGroupTrackDisplay", _("Track Display"), track_actions, G_N_ELEMENTS (track_actions), GETTEXT_PACKAGE, TRUE, plugin);
    track_display_plugin->action_group = action_group;

    /* Merge UI */
    gchar *uipath = g_build_filename(get_ui_dir(), "track_display.ui", NULL);
    track_display_plugin->uiid = anjuta_ui_merge(ui, uipath);
    g_free(uipath);

    set_default_preferences();

    /* Add widget in Shell. Any number of widgets can be added */
    track_display_plugin->track_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (track_display_plugin->track_window), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW (track_display_plugin->track_window), GTK_SHADOW_IN);
    tm_create_track_display(track_display_plugin->track_window);

    g_signal_connect (gtkpod_app, SIGNAL_TRACKS_DISPLAYED, G_CALLBACK (track_display_set_tracks_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_PLAYLIST_SELECTED, G_CALLBACK (track_display_set_playlist_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_SORT_ENABLEMENT, G_CALLBACK (track_display_set_sort_enablement), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_TRACK_REMOVED, G_CALLBACK (track_display_track_removed_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_TRACK_UPDATED, G_CALLBACK (track_display_track_updated_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_PREFERENCE_CHANGE, G_CALLBACK (track_display_preference_changed_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_TRACKS_REORDERED, G_CALLBACK (track_display_tracks_reordered_cb), NULL);

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

static void ipreferences_merge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e) {
    GdkPixbuf *pixbuf;
    GError *error = NULL;

    TrackDisplayPlugin* plugin = TRACK_DISPLAY_PLUGIN(ipref);
    plugin->prefs = init_track_display_preferences();
    if (plugin->prefs == NULL)
        return;

    pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), PREFERENCE_ICON, 48, 0, &error);

    if (!pixbuf) {
        g_warning ("Couldn't load icon: %s", error->message);
        g_error_free(error);
    }

    anjuta_preferences_dialog_add_page(ANJUTA_PREFERENCES_DIALOG (anjuta_preferences_get_dialog (prefs)), "gtkpod-track-display-settings", _(TAB_NAME), pixbuf, plugin->prefs);
    g_object_unref(pixbuf);
}

static void ipreferences_unmerge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e) {
    anjuta_preferences_remove_page(prefs, _(TAB_NAME));
    TrackDisplayPlugin* plugin = TRACK_DISPLAY_PLUGIN(ipref);
    gtk_widget_destroy(plugin->prefs);
}

static void ipreferences_iface_init(IAnjutaPreferencesIface* iface) {
    iface->merge = ipreferences_merge;
    iface->unmerge = ipreferences_unmerge;
}

ANJUTA_PLUGIN_BEGIN (TrackDisplayPlugin, track_display_plugin);
        ANJUTA_PLUGIN_ADD_INTERFACE(ipreferences, IANJUTA_TYPE_PREFERENCES);ANJUTA_PLUGIN_END
;

ANJUTA_SIMPLE_PLUGIN (TrackDisplayPlugin, track_display_plugin)
;
