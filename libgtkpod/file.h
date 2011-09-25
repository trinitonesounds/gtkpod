/* Time-stamp: <2008-11-08 18:11:02 jcs>
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

#ifndef __FILE_H__
#define __FILE_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <stdio.h>
#include "itdb.h"
#include "filetype_iface.h"

/* Don't change the order of this enum -- when exporting playlists the
   file requester depends on having these in order because the toggle
   buttons are arranged that way */
typedef enum
{
    SOURCE_PREFER_LOCAL = 0,
    SOURCE_LOCAL,
    SOURCE_IPOD,
    SOURCE_PREFER_IPOD
} FileSource;


typedef void (*AddTrackFunc)(Playlist *plitem, Track *track, gpointer data);

FileType *determine_filetype (const gchar *path);

/*
 * Used inside file functions but also used in file_itunesdb for importing
 * itdbs. Embedded artwork can be read using this.
 */
Track *get_track_info_from_file(gchar *name, Track *orig_track, GError **error);

gboolean add_track_by_filename (iTunesDB *itdb, gchar *name, Playlist *plitem, gboolean descend, AddTrackFunc addtrackfunc, gpointer data, GError **error);
gboolean add_directory_by_name (iTunesDB *itdb, gchar *name, Playlist *plitem, gboolean descend, AddTrackFunc addtrackfunc, gpointer data, GError **error);
Playlist *add_playlist_by_filename (iTunesDB *itdb, gchar *plfile, Playlist *plitem, gint plitem_pos, AddTrackFunc addtrackfunc, gpointer data, GError **error);
gboolean write_tags_to_file(Track *s);
void update_track_from_file (iTunesDB *itdb, Track *track);
void update_tracks (GList *selected_tracks);
void display_non_updated (Track *track, gchar *txt);
void display_updated (Track *track, gchar *txt);
iTunesDB *gp_import_itdb (iTunesDB *old_itdb, const gint type,
			  const gchar *mp, const gchar *name_off,
			  const gchar *name_loc);
void gp_load_ipods (void);
iTunesDB *gp_load_ipod (iTunesDB *itdb);
gboolean gp_eject_ipod(iTunesDB *itdb);
gboolean gp_save_itdb (iTunesDB *itdb);
gboolean gp_create_extended_info(iTunesDB *itdb);
void handle_export (void);
void data_changed (iTunesDB *itdb);
void data_unchanged (iTunesDB *itdb);
gboolean files_are_saved (void);
gchar *get_file_name_from_source (Track *track, FileSource source);
gchar* get_preferred_track_name_format(Track *s);
void mark_track_for_deletion (iTunesDB *itdb, Track *track);
void gp_info_deleted_tracks (iTunesDB *itdb,
			     gdouble *size, guint32 *num);
void update_charset_info (Track *track);
void parse_offline_playcount (void);

gboolean read_soundcheck (Track *track);

gboolean read_lyrics_from_file (Track *track, gchar **lyrics);
gboolean write_lyrics_to_file (Track *track);

gchar *fileselection_get_file_or_dir(const gchar *title, const gchar *cur_file, GtkFileChooserAction action);
#endif
