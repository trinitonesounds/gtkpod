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

#ifndef PLUGIN_H_
#define PLUGIN_H_

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <libanjuta/anjuta-plugin.h>

/* Stock IDs */
#define PLAYLIST_DISPLAY_PHOTO_ICON_STOCK_ID "playlist_display-photo-icon"
#define PLAYLIST_DISPLAY_READ_ICON_STOCK_ID "playlist_display-read-icon"
#define PLAYLIST_DISPLAY_ADD_DIRS_ICON_STOCK_ID "playlist_display-add-dirs-icon"
#define PLAYLIST_DISPLAY_ADD_FILES_ICON_STOCK_ID "playlist_display-add-files-icon"
#define PLAYLIST_DISPLAY_ADD_PLAYLISTS_ICON_STOCK_ID "playlist_display-add-playlists-icon"
#define PLAYLIST_DISPLAY_SYNC_ICON_STOCK_ID "playlist_display-sync-icon"

/* Action IDs */
#define ACTION_LOAD_IPOD "ActionLoadiPod"
#define ACTION_SAVE_CHANGES "ActionSaveChanges"
#define ACTION_ADD_FILES "ActionAddFiles"
#define ACTION_ADD_DIRECTORY "ActionAddDirectory"
#define ACTION_ADD_PLAYLIST "ActionAddPlaylist"
#define ACTION_NEW_PLAYLIST "ActionNewPlaylist"
#define ACTION_NEW_PLAYLIST_MENU "ActionNewPlaylistMenu"

extern GType playlist_display_plugin_get_type (GTypeModule *module);
#define PLAYLIST_DISPLAY_TYPE_PLUGIN         (playlist_display_plugin_get_type (NULL))
#define PLAYLIST_DISPLAY_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), PLAYLIST_DISPLAY_TYPE_PLUGIN, PlaylistDisplayPlugin))
#define PLAYLIST_DISPLAY_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), PLAYLIST_DISPLAY_TYPE_PLUGIN, PlaylistDisplayPluginClass))
#define PLAYLIST_DISPLAY_IS_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), PLAYLIST_DISPLAY_TYPE_PLUGIN))
#define PLAYLIST_DISPLAY_IS_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), PLAYLIST_DISPLAY_TYPE_PLUGIN))
#define PLAYLIST_DISPLAY_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), PLAYLIST_DISPLAY_TYPE_PLUGIN, PlaylistDisplayPluginClass))

typedef struct _PlaylistDisplayPlugin PlaylistDisplayPlugin;
typedef struct _PlaylistDisplayPluginClass PlaylistDisplayPluginClass;

struct _PlaylistDisplayPlugin {
    AnjutaPlugin parent;
    GtkWidget *pl_window;
    GtkWidget *playlist_view;
    gint uiid;
    GtkActionGroup *action_group;
    GtkWidget *prefs;
};

struct _PlaylistDisplayPluginClass {
    AnjutaPluginClass parent_class;
};

#endif /* PLUGIN_H_ */
