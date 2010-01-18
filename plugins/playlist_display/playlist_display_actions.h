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
#ifndef __PLAYLIST_DISPLAY_H__
#define __PLAYLIST_DISPLAY_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include "plugin.h"

void on_load_ipods_mi(GtkAction* action, PlaylistDisplayPlugin* plugin);
void on_save_changes(GtkAction *action, PlaylistDisplayPlugin* plugin);
void on_create_add_files(GtkAction *action, PlaylistDisplayPlugin* plugin);
void on_create_add_directory (GtkAction *action, PlaylistDisplayPlugin* plugin);
void on_create_add_playlists (GtkAction *action, PlaylistDisplayPlugin* plugin);

void on_new_playlist_activate(GtkAction *action, PlaylistDisplayPlugin* plugin);
void on_smart_playlist_activate(GtkAction *action, PlaylistDisplayPlugin* plugin);
void on_random_playlist_activate(GtkAction *action, PlaylistDisplayPlugin* plugin);
void on_pl_containing_displayed_tracks_activate(GtkAction *action, PlaylistDisplayPlugin* plugin);
void on_pl_containing_selected_tracks_activate(GtkAction *action, PlaylistDisplayPlugin* plugin);
void on_most_rated_tracks_playlist_s1_activate(GtkAction *action, PlaylistDisplayPlugin* plugin);
void on_most_listened_tracks1_activate(GtkAction *action, PlaylistDisplayPlugin* plugin);
void on_most_recent_played_tracks_activate(GtkAction *action, PlaylistDisplayPlugin* plugin);
void on_played_since_last_time1_activate(GtkAction *action, PlaylistDisplayPlugin* plugin);
void on_all_tracks_never_listened_to1_activate(GtkAction *action, PlaylistDisplayPlugin* plugin);
void on_all_tracks_not_listed_in_any_playlist1_activate(GtkAction *action, PlaylistDisplayPlugin* plugin);
void on_pl_for_each_artist_activate(GtkAction *action, PlaylistDisplayPlugin* plugin);
void on_pl_for_each_album_activate(GtkAction *action, PlaylistDisplayPlugin* plugin);
void on_pl_for_each_genre_activate(GtkAction *action, PlaylistDisplayPlugin* plugin);
void on_pl_for_each_composer_activate(GtkAction *action, PlaylistDisplayPlugin* plugin);
void on_pl_for_each_year_activate(GtkAction *action, PlaylistDisplayPlugin* plugin);
void on_pl_for_each_rating_activate(GtkAction *action, PlaylistDisplayPlugin* plugin);

#endif
