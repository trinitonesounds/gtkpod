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

gboolean add_song_by_filename (gchar *name, Playlist *plitem);
gboolean add_directory_recursively (gchar *name, Playlist *plitem);
gboolean add_playlist_by_filename (gchar *plfile, Playlist *plitem);
gboolean write_tags_to_file(Song *s, gint tag_id);
void update_song_from_file (Song *song);
void update_selected_songs (void);
void update_selected_entry (gint inst);
void update_selected_playlist (void);
void display_non_updated (Song *song, gchar *txt);
void display_updated (Song *song, gchar *txt);
void handle_import (void);
void handle_export (void);
gboolean files_are_saved (void);
void data_changed (void);
gchar* get_song_name_on_disk(Song *s);
gchar* get_song_name_on_ipod(Song *s);
gchar* get_preferred_song_name_format(Song *s);
void mark_song_for_deletion (Song *song);
void fill_in_extended_info (Song *song);
#endif __FILE_H__
