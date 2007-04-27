/*
|  Copyright (C) 2002-2007 Jorg Schuler <jcsjcs at users sourceforge net>
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

#ifndef __MISC_TRACK_H__
#define __MISC_TRACK_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "display.h"

void gp_duplicate_remove (Track *oldtrack, Track *track);
void gp_sha1_hash_tracks_itdb (iTunesDB *itdb);
void gp_sha1_hash_tracks (void);
void gp_sha1_free_hash (void);
Track *gp_track_by_filename (iTunesDB *itdb, gchar *filename);

void gp_itdb_pc_path_hash_init (ExtraiTunesDBData *eitdb);
void gp_itdb_pc_path_hash_destroy (ExtraiTunesDBData *eitdb);
void gp_itdb_pc_path_hash_add_track (Track *track);
void gp_itdb_pc_path_hash_remove_track (Track *track);
GList *gp_itdb_pc_path_hash_find_tracks (iTunesDB *itdb, const gchar *filename);
GList *gp_itdb_find_same_tracks (iTunesDB *itdb, Track *track);
GList *gp_itdb_find_same_tracks_in_itdbs (Track *track);
GList *gp_itdb_find_same_tracks_in_local_itdbs (Track *track);

gchar **track_get_item_pointer (Track *track, T_item t_item);
const gchar *track_get_item (Track *track, T_item t_item);
time_t *track_get_timestamp_ptr (Track *track, T_item t_item);
time_t track_get_timestamp (Track *track, T_item t_item);
gchar *track_get_text (Track *track, T_item item);
gboolean track_set_text (Track *track,
			 const gchar *new_text, T_item item);
gboolean track_copy_item (Track *frtrack, Track *totrack, T_item item);
void gp_info_nontransferred_tracks (iTunesDB *itdb,
				    gdouble *size, guint32 *num);

void add_tracklist_to_playlist (Playlist *pl, gchar *str);
void add_trackglist_to_playlist (Playlist *pl, GList *tracks);
Playlist *add_text_plain_to_playlist (iTunesDB *itdb, Playlist *pl,
				      gchar *str, gint position,
				      AddTrackFunc trackaddfunc,
				      gpointer data);
void gp_do_selected_tracks (void (*do_func)(GList *tracks));
void gp_do_selected_entry (void (*do_func)(GList *tracks), gint inst);
void gp_do_selected_playlist (void (*do_func)(GList *tracks));

void gp_track_set_flags_podcast (Track *track);
void gp_track_set_flags_default (Track *track);

gchar *get_track_info (Track *track, gboolean prefer_filename);
#endif
