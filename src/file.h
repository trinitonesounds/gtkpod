/*
|  Copyright (C) 2002-2003 Jorg Schuler <jcsjcs at users.sourceforge.net>
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

#ifndef __FILE_H__
#define __FILE_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <stdio.h>
#include "song.h"
#include "playlist.h"

typedef void (*AddSongFunc)(Playlist *plitem, Song *song, gpointer data);

gboolean add_song_by_filename (gchar *name, Playlist *plitem, gboolean descend,
			       AddSongFunc addsongfunc, gpointer data);
gboolean add_directory_by_name (gchar *name, Playlist *plitem, 
				gboolean descend,
				AddSongFunc addsongfunc, gpointer data);
gboolean add_playlist_by_filename (gchar *plfile, Playlist *plitem,
				   AddSongFunc addsongfunc, gpointer data);
gboolean write_tags_to_file(Song *s, S_item tag_id);
void update_song_from_file (Song *song);
void do_selected_songs (void (*do_func)(GList *songids));
void do_selected_entry (void (*do_func)(GList *songids), gint inst);
void do_selected_playlist (void (*do_func)(GList *songids));
void update_songids (GList *selected_songids);
void sync_songids (GList *selected_songids);
void display_non_updated (Song *song, gchar *txt);
void display_updated (Song *song, gchar *txt);
void handle_import (void);
void handle_export (void);
gboolean files_are_saved (void);
void data_changed (void);
gchar *get_track_name_on_disk_verified (Song *song);
gchar* get_track_name_on_disk(Song *s);
gchar* get_track_name_on_ipod(Song *s);
gchar* get_preferred_track_name_format(Song *s);
void mark_song_for_deletion (Song *song);
void fill_in_extended_info (Song *song);
glong get_filesize_of_deleted_songs(void);

#endif
