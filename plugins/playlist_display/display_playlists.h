/* Time-stamp: <2007-03-19 23:11:13 jcs>
|
|  Copyright (C) 2002-2005 Jorg Schuler <jcsjcs at users sourceforge net>
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

#ifndef __DISPLAY_PLAYLIST_H__
#define __DISPLAY_PLAYLIST_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "libgtkpod/gtkpod_app_iface.h"
#include "libgtkpod/itdb.h"

/* "Column numbers" in playlist model */
typedef enum  {
  PM_COLUMN_ITDB = 0,
  PM_COLUMN_TYPE,
  PM_COLUMN_PLAYLIST,
  PM_COLUMN_PHOTOS,
  PM_NUM_COLUMNS
} PM_column_type;

GtkWidget *pm_create_playlist_view(GtkActionGroup *action_group);
void pm_destroy_playlist_view(void);
void pm_select_playlist(Playlist *playlist);
void pm_set_selected_playlist(Playlist *pl);
void pm_remove_all_playlists (gboolean clear_sort);
void pm_add_all_itdbs (void);
Playlist* pm_get_selected_playlist(void);
void pm_stop_editing(gboolean cancel);

void playlist_display_itdb_added_cb(GtkPodApp *app, gpointer itdb, gint32 pos, gpointer data);
void playlist_display_itdb_removed_cb(GtkPodApp *app, gpointer itdb, gpointer data);
void playlist_display_update_itdb_cb (GtkPodApp *app, gpointer olditdb, gpointer newitdb, gpointer data);
void playlist_display_select_playlist_cb (GtkPodApp *app, gpointer pl, gpointer data);
void playlist_display_playlist_added_cb(GtkPodApp *app, gpointer pl, gint32 pos, gpointer data);
void playlist_display_playlist_removed_cb(GtkPodApp *app, gpointer pl, gpointer data);
void playlist_display_track_removed_cb(GtkPodApp *app, gpointer tk, gpointer data);
void playlist_display_preference_changed_cb(GtkPodApp *app, gpointer pfname, gpointer value, gpointer data);
void playlist_display_itdb_data_changed_cb(GtkPodApp *app, gpointer itdb, gpointer data);

#endif /* __DISPLAY_PLAYLIST_H__ */
