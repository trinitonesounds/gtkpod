/*
|  Copyright (C) 2002 Adrian Ulrich <pab at blinkenlights.ch>
|                2002-2003 Jörg Schuler  <jcsjcs at users.sourceforge.net>
|
|  Part of the gtkpod project.
| 
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
*/

#ifndef __PLAYLIST_H__
#define __PLAYLIST_H__


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include "song.h"

typedef struct
{
  gchar *name;
  gunichar2 *name_utf16;
  guint32 type;         /* 1: master play list (PL_TYPE_MPL) */
  GList *members;       /* songs in playlist (Song *) */
} Playlist;

enum {
  PL_TYPE_NORM = 0,
  PL_TYPE_MPL = 1
};


void create_mpl (void);
void add_new_playlist (void);
void free_playlist(Playlist *playlist);
void create_playlist (Playlist *plitem);
Playlist *it_add_playlist (Playlist *plitem);
Playlist *add_playlist (Playlist *plitem);
void it_add_songid_to_playlist (Playlist *plitem, guint32 id);
void add_songid_to_playlist (Playlist *plitem, guint32 id);
void add_song_to_playlist (Playlist *plitem, Song *song);
void remove_songid_from_playlist (Playlist *plitem, guint32 id);
void remove_song_from_playlist (Playlist *plitem, Song *song);
void remove_playlist (Playlist *playlist);
void remove_all_playlists (void);
#define it_get_nr_of_playlists get_nr_of_playlists
guint32 get_nr_of_playlists (void);
#define it_get_playlist_by_nr get_playlist_by_nr
Playlist *get_playlist_by_nr (guint32 n);
#define it_get_nr_of_songs_in_playlist get_nr_of_songs_in_playlist
guint32 get_nr_of_songs_in_playlist (Playlist *plitem);
Song *it_get_song_in_playlist_by_nr (Playlist *plitem, guint32 n);
Song *get_song_in_playlist_by_nr (Playlist *plitem, guint32 n);
void reset_playlists_to_new_list(GList *new_l);

#endif __PLAYLIST_H__
