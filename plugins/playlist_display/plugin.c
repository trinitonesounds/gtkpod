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
#include "libgtkpod/stock_icons.h"
#include "libgtkpod/gtkpod_app_iface.h"
//#include "libgtkpod/gtkpod_selection_listener.h"
#include "plugin.h"
#include "display_playlists.h"
#include "playlist_display_actions.h"


/* Parent class. Part of standard class definition */
static gpointer parent_class;

static GtkActionEntry playlist_actions[] =
    {
        {
            "ActionLoadiPod",         /* Action name */
            PLAYLIST_DISPLAY_GTKPOD_READ_ICON_STOCK_ID, /* Stock icon */
            N_("_Load iPod(s)"),      /* Display label */
            NULL,                           /* short-cut */
            NULL,                           /* Tooltip */
            G_CALLBACK (on_load_ipods_mi)         /* callback */
        }
    };

static gboolean activate_plugin(AnjutaPlugin *plugin) {
    AnjutaUI *ui;
    PlaylistDisplayPlugin *playlist_display_plugin;
    GtkActionGroup* action_group;

    /* Prepare the icons for the playlist */
    register_stock_icon("playlist_display-photo", PLAYLIST_DISPLAY_PHOTO_ICON_STOCK_ID);
    register_stock_icon("playlist_display-playlist", PLAYLIST_DISPLAY_PLAYLIST_ICON_STOCK_ID);
    register_stock_icon("playlist_display-gtkpod-read", PLAYLIST_DISPLAY_GTKPOD_READ_ICON_STOCK_ID);

    playlist_display_plugin = (PlaylistDisplayPlugin*) plugin;
    ui = anjuta_shell_get_ui(plugin->shell, NULL);

    /* Add our playlist_actions */
    action_group
            = anjuta_ui_add_action_group_entries(ui, "ActionGroupPlaylistDisplay", _("Playlist Display"), playlist_actions, G_N_ELEMENTS (playlist_actions), GETTEXT_PACKAGE, TRUE, plugin);
    playlist_display_plugin->action_group = action_group;

    /* Merge UI */
    playlist_display_plugin->uiid = anjuta_ui_merge(ui, UI_FILE);

    /* Add widget in Shell. Any number of widgets can be added */
    g_warning("Creating tree view\n");
    playlist_display_plugin->pl_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (playlist_display_plugin->pl_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW (playlist_display_plugin->pl_window), GTK_SHADOW_IN);

    playlist_display_plugin->playlist_view = pm_create_treeview();

    g_signal_connect (gtkpod_app, "playlist_selected", G_CALLBACK (playlist_display_select_playlist_cb), NULL);
    g_signal_connect (gtkpod_app, "itdb_updated", G_CALLBACK (playlist_display_update_itdb_cb), NULL);

    gtk_container_add(GTK_CONTAINER (playlist_display_plugin->pl_window), GTK_WIDGET (playlist_display_plugin->playlist_view));
    gtk_widget_show_all(playlist_display_plugin->pl_window);
    anjuta_shell_add_widget(plugin->shell, playlist_display_plugin->pl_window, "PlaylistDisplayPlugin", "", NULL, ANJUTA_SHELL_PLACEMENT_LEFT, NULL);

    return TRUE; /* FALSE if activation failed */
}

static gboolean deactivate_plugin(AnjutaPlugin *plugin) {
    AnjutaUI *ui;
    PlaylistDisplayPlugin *playlist_display_plugin;

    playlist_display_plugin = (PlaylistDisplayPlugin*) plugin;
    ui = anjuta_shell_get_ui(plugin->shell, NULL);

    /* Remove widgets from Shell */
    anjuta_shell_remove_widget(plugin->shell, playlist_display_plugin->pl_window, NULL);

    /* Destroy the treeview */
    playlist_display_plugin->playlist_view = NULL;
    destroy_treeview();

    /* Unmerge UI */
    anjuta_ui_unmerge(ui, playlist_display_plugin->uiid);

    /* Remove Action groups */
    anjuta_ui_remove_action_group(ui, playlist_display_plugin->action_group);

    /* FALSE if plugin doesn't want to deactivate */
    return TRUE;
}

static void playlist_display_plugin_instance_init(GObject *obj) {
    PlaylistDisplayPlugin *plugin = (PlaylistDisplayPlugin*) obj;
    plugin->uiid = 0;
    plugin->pl_window = NULL;
    plugin->playlist_view = NULL;
    plugin->action_group = NULL;
}

static void playlist_display_plugin_class_init(GObjectClass *klass) {
    AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

    parent_class = g_type_class_peek_parent(klass);

    plugin_class->activate = activate_plugin;
    plugin_class->deactivate = deactivate_plugin;
}

ANJUTA_PLUGIN_BEGIN (PlaylistDisplayPlugin, playlist_display_plugin);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (PlaylistDisplayPlugin, playlist_display_plugin)
;
