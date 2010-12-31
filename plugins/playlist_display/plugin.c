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
#include <libanjuta/interfaces/ianjuta-preferences.h>
#include "libgtkpod/stock_icons.h"
#include "libgtkpod/directories.h"
#include "libgtkpod/misc_playlist.h"
#include "libgtkpod/gtkpod_app_iface.h"
#include "libgtkpod/tool_menu_action.h"
#include "libgtkpod/misc.h"
#include "libgtkpod/gp_private.h"
#include "libgtkpod/prefs.h"
#include "plugin.h"
#include "display_playlists.h"
#include "playlist_display_actions.h"
#include "playlist_display_preferences.h"

#define PREFERENCE_ICON "playlist_display-playlist-category"
#define PREFERENCE_ICON_STOCK_ID "playlist_display-preference-icon"
#define TAB_NAME N_("Playlist Display")

/* Parent class. Part of standard class definition */
static gpointer parent_class;

static GtkActionEntry playlist_actions[] =
    {
        {
            ACTION_LOAD_IPOD, /* Action name */
            PLAYLIST_DISPLAY_READ_ICON_STOCK_ID, /* Stock icon */
            N_("_Load iPod(s)"), /* Display label */
            NULL, /* short-cut */
            N_("Load all currently listed iPods"), /* Tooltip */
            G_CALLBACK (on_load_ipods_mi) /* callback */
        },
        {
            ACTION_SAVE_CHANGES, /* Action name */
            PLAYLIST_DISPLAY_SYNC_ICON_STOCK_ID, /* Stock icon */
            N_("_Save Changes"), /* Display label */
            NULL, /* short-cut */
            N_("Save all changes"), /* Tooltip */
            G_CALLBACK (on_save_changes) /* callback */
        },
        {
            ACTION_ADD_FILES, /* Action name */
            PLAYLIST_DISPLAY_ADD_FILES_ICON_STOCK_ID, /* Stock icon */
            N_("Add _Files"), /* Display label */
            NULL, /* short-cut */
            N_("Add files to selected iPod"), /* Tooltip */
            G_CALLBACK (on_create_add_files) /* callback */
        },
        {
            ACTION_ADD_DIRECTORY, /* Action name */
            PLAYLIST_DISPLAY_ADD_DIRS_ICON_STOCK_ID, /* Stock icon */
            N_("Add Fol_der"), /* Display label */
            NULL, /* short-cut */
            N_("Add folder contents to selected iPod"), /* Tooltip */
            G_CALLBACK (on_create_add_directory) /* callback */
        },
        {
            ACTION_ADD_PLAYLIST, /* Action name */
            PLAYLIST_DISPLAY_ADD_PLAYLISTS_ICON_STOCK_ID, /* Stock icon */
            N_("Add _Playlist"), /* Display label */
            NULL, /* short-cut */
            N_("Add playlist to selected iPod"), /* Tooltip */
            G_CALLBACK (on_create_add_playlists) /* callback */
        },
        {
            "ActionSyncPlaylistWithDir",
            GTK_STOCK_REFRESH,
            N_("Sync Playlist with Dir(s)"),
            NULL,
            NULL,
            G_CALLBACK (on_sync_playlist_with_dirs)
        },
        {
            ACTION_NEW_PLAYLIST_MENU,
            NULL,
            N_("_New Playlist"),
            NULL,
            NULL,
            NULL
        },
        {
            "ActionNewEmptyPlaylist",
            GTK_STOCK_NEW,
            N_("Empty Playlist"),
            NULL,
            N_("Create an empty playlist"),
            G_CALLBACK (on_new_playlist_activate)
        },
        {
            "ActionNewSmartPlaylist",
            NULL,
            N_("Smart Playlist"),
            NULL,
            N_("Create a new smart playlist"),
            G_CALLBACK (on_smart_playlist_activate)
        },
        {
            "ActionNewRandomPlaylist",
            NULL,
            N_("Random Playlist from Displayed Tracks"),
            NULL,
            N_("Create a random playlist from the displayed tracks"),
            G_CALLBACK (on_random_playlist_activate)
        },
        {
            "ActionNewContainingDisplayedPlaylist",
            NULL,
            N_("Containing Displayed Tracks"),
            NULL,
            N_("Create a playlist containing the displayed tracks"),
            G_CALLBACK (on_pl_containing_displayed_tracks_activate)
        },
        {
            "ActionNewContainingSelectedPlaylist",
            NULL,
            N_("Containing Selected Tracks"),
            NULL,
            N_("Create a playlist containing the selected tracks"),
            G_CALLBACK (on_pl_containing_selected_tracks_activate)
        },
        {
            "ActionNewBestRatedPlaylist",
            NULL,
            N_("Best Rated Tracks"),
            NULL,
            N_("Create a playlist containing the best rated tracks"),
            G_CALLBACK (on_most_rated_tracks_playlist_s1_activate)
        },
        {
            "ActionNewTracksMostOftenPlaylist",
            NULL,
            N_("Tracks Most Often Listened To"),
            NULL,
            N_("Create a playlist containing the tracks most often listened to"),
            G_CALLBACK (on_most_listened_tracks1_activate)
        },
        {
            "ActionNewMostRecentPlayledPlaylist",
            NULL,
            N_("Most Recently Played Tracks"),
            NULL,
            N_("Create a playlist containing the most recently played tracks"),
            G_CALLBACK (on_most_recent_played_tracks_activate)
        },
        {
            "ActionNewAllPlayedSinceLastTimePlaylist",
            NULL,
            N_("All Tracks Played Since Last Time"),
            NULL,
            N_("Create a playlist containing all tracks played since last time"),
            G_CALLBACK (on_played_since_last_time1_activate)
        },
        {
            "ActionNewAllNeverListenedPlaylist",
            NULL,
            N_("All Tracks Never Listened To"),
            NULL,
            N_("Create a playlist of all tracks never listened to"),
            G_CALLBACK (on_all_tracks_never_listened_to1_activate)
        },
        {
            "ActionAllNeverListedPlaylist",
            NULL,
            N_("All Tracks not Listed in any Playlist"),
            NULL,
            N_("Create a playlist of tracks not list in any other playlist"),
            G_CALLBACK (on_all_tracks_not_listed_in_any_playlist1_activate)
        },
        {
            "ActionNewOnePerArtistPlaylist",
            NULL,
            N_("One for each Artist"),
            NULL,
            N_("Create a playlist for each artist"),
            G_CALLBACK (on_pl_for_each_artist_activate)
        },
        {
            "ActionNewOnePerAlbumPlaylist",
            NULL,
            N_("One for each Album"),
            NULL,
            N_("Create a playlist for each album"),
            G_CALLBACK (on_pl_for_each_album_activate)
        },
        {
            "ActionNewOnePerGenrePlaylist",
            NULL,
            N_("One for each Genre"),
            NULL,
            N_("Create a playlist for each genre"),
            G_CALLBACK (on_pl_for_each_genre_activate)
        },
        {
            "ActionNewOnePreComposerPlaylist",
            NULL,
            N_("One for each Composer"),
            NULL,
            N_("Create a playlist for each composer"),
            G_CALLBACK (on_pl_for_each_composer_activate)
        },
        {
            "ActionNewOnePerYearPlaylist",
            NULL,
            N_("One for each Year"),
            NULL,
            N_("Create a playlist for each year"),
            G_CALLBACK (on_pl_for_each_year_activate)
        },
        {
            "ActionNewOnePerRatingPlaylist",
            NULL,
            N_("One for each Rating"),
            NULL,
            N_("Create a playlist for each rating"),
            G_CALLBACK (on_pl_for_each_rating_activate)
        },
        {
            "ActionDeleteSelectedPlaylist",
            GTK_STOCK_DELETE,
            N_("Selected Playlist"),
            NULL,
            NULL,
            G_CALLBACK (on_delete_selected_playlist)
        },
        {
            "ActionDeleteSelectedPlaylistIncDb",
            GTK_STOCK_DELETE,
            N_("Selected Playlist including Tracks from Database"),
            NULL,
            NULL,
            G_CALLBACK (on_delete_selected_playlist_including_tracks_from_database)
        },
        {
            "ActionDeleteSelectedPlaylistIncDev",
            GTK_STOCK_DELETE,
            N_("Selected Playlist including Tracks from Device"),
            NULL,
            NULL,
            G_CALLBACK (on_delete_selected_playlist_including_tracks_from_device)
        },
        {
            "ActionUpdatePlaylist",
            GTK_STOCK_REFRESH,
            N_("Selected Playlist"),
            NULL,
            NULL,
            G_CALLBACK (on_update_selected_playlist)
        }
    };

static void set_default_preferences() {
    if (! prefs_get_int_value("pm_sort", NULL))
        prefs_set_int("pm_sort", SORT_NONE);

    if (! prefs_get_int_value("pm_case_sensitive", NULL))
        prefs_set_int("pm_case_sensitive", FALSE);
}

static gboolean activate_plugin(AnjutaPlugin *plugin) {
    AnjutaUI *ui;
    PlaylistDisplayPlugin *playlist_display_plugin;
    GtkActionGroup* action_group;
    GtkAction *new_playlist_action;

    /* Set preferences */
    set_default_preferences();

    /* Prepare the icons for the playlist */
    register_icon_path(get_plugin_dir(), "playlist_display");
    register_stock_icon(PREFERENCE_ICON, PREFERENCE_ICON_STOCK_ID);

    register_stock_icon("playlist_display-photo", PLAYLIST_DISPLAY_PHOTO_ICON_STOCK_ID);
    register_stock_icon("playlist_display-playlist", PLAYLIST_DISPLAY_PLAYLIST_ICON_STOCK_ID);
    register_stock_icon("playlist_display-read", PLAYLIST_DISPLAY_READ_ICON_STOCK_ID);
    register_stock_icon("playlist_display-add-dirs", PLAYLIST_DISPLAY_ADD_DIRS_ICON_STOCK_ID);
    register_stock_icon("playlist_display-add-files", PLAYLIST_DISPLAY_ADD_FILES_ICON_STOCK_ID);
    register_stock_icon("playlist_display-add-playlists", PLAYLIST_DISPLAY_ADD_PLAYLISTS_ICON_STOCK_ID);
    register_stock_icon("playlist_display-sync", PLAYLIST_DISPLAY_SYNC_ICON_STOCK_ID);

    playlist_display_plugin = (PlaylistDisplayPlugin*) plugin;
    ui = anjuta_shell_get_ui(plugin->shell, NULL);

    /* Add our playlist_actions */
    action_group
        = anjuta_ui_add_action_group_entries(ui, "ActionGroupPlaylistDisplay", _("Playlist Display"), playlist_actions, G_N_ELEMENTS (playlist_actions), GETTEXT_PACKAGE, TRUE, plugin);
    playlist_display_plugin->action_group = action_group;

    new_playlist_action = tool_menu_action_new (ACTION_NEW_PLAYLIST, _("New Playlist"), _("Create a new playlist for the selected iPod"), GTK_STOCK_NEW);
    g_signal_connect(new_playlist_action, "activate", G_CALLBACK(on_new_playlist_activate), NULL);
    gtk_action_group_add_action (playlist_display_plugin->action_group, GTK_ACTION (new_playlist_action));

    /* Merge UI */
    gchar *uipath = g_build_filename(get_ui_dir(), "playlist_display.ui", NULL);
    playlist_display_plugin->uiid = anjuta_ui_merge(ui, uipath);
    g_free(uipath);

    playlist_display_plugin->playlist_view = pm_create_playlist_view(action_group);

    g_signal_connect (gtkpod_app, SIGNAL_PLAYLIST_SELECTED, G_CALLBACK (playlist_display_select_playlist_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_PLAYLIST_ADDED, G_CALLBACK (playlist_display_playlist_added_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_PLAYLIST_REMOVED, G_CALLBACK (playlist_display_playlist_removed_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_ITDB_ADDED, G_CALLBACK (playlist_display_itdb_added_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_ITDB_REMOVED, G_CALLBACK (playlist_display_itdb_removed_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_ITDB_UPDATED, G_CALLBACK (playlist_display_update_itdb_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_PREFERENCE_CHANGE, G_CALLBACK (playlist_display_preference_changed_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_ITDB_DATA_CHANGED, G_CALLBACK (playlist_display_itdb_data_changed_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_ITDB_DATA_SAVED, G_CALLBACK (playlist_display_itdb_data_changed_cb), NULL);

    gtk_widget_show_all(playlist_display_plugin->playlist_view);
    // Add widget directly as scrolling is handled internally by the widget
    anjuta_shell_add_widget(plugin->shell, playlist_display_plugin->playlist_view, "PlaylistDisplayPlugin", _("  iPod Repositories"), PLAYLIST_DISPLAY_PLAYLIST_ICON_STOCK_ID, ANJUTA_SHELL_PLACEMENT_LEFT, NULL);

    return TRUE; /* FALSE if activation failed */
}

static gboolean deactivate_plugin(AnjutaPlugin *plugin) {
    AnjutaUI *ui;
    PlaylistDisplayPlugin *playlist_display_plugin;

    playlist_display_plugin = (PlaylistDisplayPlugin*) plugin;
    ui = anjuta_shell_get_ui(plugin->shell, NULL);

    g_signal_handlers_disconnect_by_func (plugin->shell, G_CALLBACK (playlist_display_select_playlist_cb), plugin);
    g_signal_handlers_disconnect_by_func (plugin->shell, G_CALLBACK (playlist_display_playlist_added_cb), plugin);
    g_signal_handlers_disconnect_by_func (plugin->shell, G_CALLBACK (playlist_display_playlist_removed_cb), plugin);
    g_signal_handlers_disconnect_by_func (plugin->shell, G_CALLBACK (playlist_display_itdb_added_cb), plugin);
    g_signal_handlers_disconnect_by_func (plugin->shell, G_CALLBACK (playlist_display_itdb_removed_cb), plugin);
    g_signal_handlers_disconnect_by_func (plugin->shell, G_CALLBACK (playlist_display_update_itdb_cb), plugin);
    g_signal_handlers_disconnect_by_func (plugin->shell, G_CALLBACK (playlist_display_preference_changed_cb), plugin);
    g_signal_handlers_disconnect_by_func (plugin->shell, G_CALLBACK (playlist_display_itdb_data_changed_cb), plugin);
    g_signal_handlers_disconnect_by_func (plugin->shell, G_CALLBACK (playlist_display_itdb_data_changed_cb), plugin);

    /* Unmerge UI */
    anjuta_ui_unmerge(ui, playlist_display_plugin->uiid);

    /* Remove Action groups */
    anjuta_ui_remove_action_group(ui, playlist_display_plugin->action_group);

    /* Remove widgets from Shell */
    anjuta_shell_remove_widget(plugin->shell, playlist_display_plugin->playlist_view, NULL);

    /* Destroy the treeview */
    pm_destroy_playlist_view();
    playlist_display_plugin->playlist_view = NULL;

    /* FALSE if plugin doesn't want to deactivate */
    return TRUE;
}

static void playlist_display_plugin_instance_init(GObject *obj) {
    PlaylistDisplayPlugin *plugin = (PlaylistDisplayPlugin*) obj;
    plugin->uiid = 0;
    plugin->playlist_view = NULL;
    plugin->action_group = NULL;
}

static void playlist_display_plugin_class_init(GObjectClass *klass) {
    AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

    parent_class = g_type_class_peek_parent(klass);

    plugin_class->activate = activate_plugin;
    plugin_class->deactivate = deactivate_plugin;
}

static void ipreferences_merge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e) {
    GdkPixbuf *pixbuf;
    GError *error = NULL;

    PlaylistDisplayPlugin* plugin = PLAYLIST_DISPLAY_PLUGIN(ipref);
    plugin->prefs = init_playlist_display_preferences();
    if (plugin->prefs == NULL)
        return;

    pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), PREFERENCE_ICON, 48, 0, &error);

    if (!pixbuf) {
        g_warning (N_("Couldn't load icon: %s"), error->message);
        g_error_free(error);
    }

    anjuta_preferences_dialog_add_page(ANJUTA_PREFERENCES_DIALOG (anjuta_preferences_get_dialog (prefs)), "gtkpod-track-display-settings", _(TAB_NAME), pixbuf, plugin->prefs);
    g_object_unref(pixbuf);
}

static void ipreferences_unmerge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e) {
    anjuta_preferences_remove_page(prefs, TAB_NAME);
    PlaylistDisplayPlugin* plugin = PLAYLIST_DISPLAY_PLUGIN(ipref);
    gtk_widget_destroy(plugin->prefs);
}

static void ipreferences_iface_init(IAnjutaPreferencesIface* iface) {
    iface->merge = ipreferences_merge;
    iface->unmerge = ipreferences_unmerge;
}

ANJUTA_PLUGIN_BEGIN (PlaylistDisplayPlugin, playlist_display_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE(ipreferences, IANJUTA_TYPE_PREFERENCES);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (PlaylistDisplayPlugin, playlist_display_plugin);
