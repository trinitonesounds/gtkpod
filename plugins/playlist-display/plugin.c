/*
|  Copyright (C) 2002-2009 Jorg Schuler <jcsjcs at users sourceforge net>
|                          Paul Richardson <phantom_sf at users sourceforge net>
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
#include "plugin.h"
/* Project configuration file */
#include <config.h>
#include "display_playlists.h"
#include "stock_icons.h"

/* Parent class. Part of standard class definition */
static gpointer parent_class;

#define UI_FILE GTKPOD_UI_DIR"/playlist-display.ui"

static GtkActionEntry actions[] = {/* Empty at moment add for playlist display */};

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
    GtkWidget *wid;
    AnjutaUI *ui;
    PlaylistDisplayPlugin *playlist_display_plugin;
    GtkActionGroup* action_group;

    /* Prepare the icons for the playlist */
    register_stock_icon ("playlist-photo", GPHOTO_PLAYLIST_ICON_STOCK_ID);
    register_stock_icon ("playlist", TUNES_PLAYLIST_ICON_STOCK_ID);

    playlist_display_plugin = (PlaylistDisplayPlugin*) plugin;
    ui = anjuta_shell_get_ui (plugin->shell, NULL);

//    /* Create hello plugin widget */
//    wid = gtk_label_new ("Hello World Plugin!!");
//    hello_plugin->widget = wid;
//
    /* Add our actions */
    action_group = anjuta_ui_add_action_group_entries (ui,
                                                        "ActionGroupPlaylistDisplay",
                                                        _("Playlist Display"),
                                                        actions,
                                                        G_N_ELEMENTS (actions),
                                                        GETTEXT_PACKAGE, TRUE,
                                                        plugin);
    playlist_display_plugin->action_group = action_group;

    /* Merge UI */
    playlist_display_plugin->uiid = anjuta_ui_merge (ui, UI_FILE);

    /* Add widget in Shell. Any number of widgets can be added */
    pm_create_treeview ();

    anjuta_shell_add_widget (plugin->shell, wid,
                             "PlaylistDisplayPlugin",
                             _("PlaylistDisplayPlugin"),
                             GTK_STOCK_ABOUT,
                             ANJUTA_SHELL_PLACEMENT_LEFT,
                             NULL);

    return TRUE; /* FALSE if activation failed */
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
    AnjutaUI *ui;
    PlaylistDisplayPlugin *playlist_display_plugin;

    playlist_display_plugin = (PlaylistDisplayPlugin*) plugin;
    ui = anjuta_shell_get_ui (plugin->shell, NULL);

    /* Remove widgets from Shell */
    anjuta_shell_remove_widget (plugin->shell,
                                playlist_display_plugin->widget,
                                NULL);
    /* Unmerge UI */
    anjuta_ui_unmerge (ui, playlist_display_plugin->uiid);

    /* Remove Action groups */
    anjuta_ui_remove_action_group (ui, playlist_display_plugin->action_group);

    /* FALSE if plugin doesn't want to deactivate */
    return TRUE;
}

static void
playlist_display_plugin_instance_init (GObject *obj)
{
    PlaylistDisplayPlugin *plugin = (PlaylistDisplayPlugin*) obj;
    plugin->uiid = 0;
    plugin->widget = NULL;
    plugin->action_group = NULL;
}

static void
playlist_display_plugin_class_init (GObjectClass *klass)
{
    AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

    parent_class = g_type_class_peek_parent (klass);

    plugin_class->activate = activate_plugin;
    plugin_class->deactivate = deactivate_plugin;
}

/* This line will change when we implement interfaces */
ANJUTA_PLUGIN_BOILERPLATE (PlaylistDisplayPlugin, playlist_display_plugin);

/* This sets up codes to register our plugin */
ANJUTA_SIMPLE_PLUGIN (PlaylistDisplayPlugin, playlist_display_plugin);
