/* Time-stamp: <2005-06-18 00:08:51 jcs>
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


/* values below -1 are private to individual functions */
typedef enum
{
    FILE_TYPE_MISC = -2,
    FILE_TYPE_ERROR = -1,
    FILE_TYPE_UNKNOWN = 0,
    FILE_TYPE_MP3,
    FILE_TYPE_M4A,
    FILE_TYPE_M4P,
    FILE_TYPE_M4B,
    FILE_TYPE_WAV,
    FILE_TYPE_M3U,
    FILE_TYPE_PLS
} FileType;

typedef enum
{
    SOURCE_PREFER_LOCAL = 0,
    SOURCE_LOCAL,
    SOURCE_IPOD
} FileSource;


typedef void (*AddTrackFunc)(Playlist *plitem, Track *track, gpointer data);

gint determine_file_type(gchar *path);
gboolean add_track_by_filename (iTunesDB *itdb, gchar *name,
				Playlist *plitem, gboolean descend,
				AddTrackFunc addtrackfunc, gpointer data);
gboolean add_directory_by_name (iTunesDB *itdb, gchar *name,
				Playlist *plitem, gboolean descend,
				AddTrackFunc addtrackfunc, gpointer data);
Playlist *add_playlist_by_filename (iTunesDB *itdb, gchar *plfile,
				    Playlist *plitem, gint plitem_pos,
				    AddTrackFunc addtrackfunc,
				    gpointer data);
gboolean write_tags_to_file(Track *s);
void update_track_from_file (iTunesDB *itdb, Track *track);
void update_tracks (GList *selected_tracks);
void mserv_from_file_tracks (GList *selected_tracks);
void sync_tracks (GList *selected_tracks);
void display_non_updated (Track *track, gchar *txt);
void display_updated (Track *track, gchar *txt);
void display_mserv_problems (Track *track, gchar *txt);
iTunesDB *gp_import_itdb (iTunesDB *old_itdb, const gint type,
			  const gchar *mp, const gchar *name_off,
			  const gchar *name_loc);
void gp_merge_ipod_itdbs (void);
void gp_merge_itdb (iTunesDB *old_itdb);
void handle_export (void);
void data_changed (iTunesDB *itdb);
gboolean files_are_saved (void);
void gp_itdb_set_mountpoint (const gchar *mp);
gchar* get_file_name(Track *tr);
gchar* get_file_name_on_ipod(Track *tr);
gchar *get_file_name_on_harddisk (Track *tr);
gchar *get_file_name_verified (Track *tr);
gchar *get_file_name_from_source (Track *track, FileSource source);
gchar* get_preferred_track_name_format(Track *s);
void mark_track_for_deletion (iTunesDB *itdb, Track *track);
void unmark_track_for_deletion (iTunesDB *itdb, Track *track);
void gp_info_deleted_tracks (iTunesDB *itdb,
			     gdouble *size, guint32 *num);
Track *get_track_info_from_file (gchar *name, Track *or_track);
void update_charset_info (Track *track);
gchar *resolve_path(const gchar *,const gchar * const *);
void parse_offline_playcount (void);

gboolean get_gain(Track *track);

/* file_export.c */
void export_files_init (GList *tracks, GList **filenames,
			gboolean display, gchar *message);
void export_playlist_file_init (GList *tracks);
GList *export_tracklist_when_necessary (iTunesDB *itdb_s,
					iTunesDB *itdb_d,
					gchar *data);
GList *export_trackglist_when_necessary (iTunesDB *itdb_s,
					 iTunesDB *itdb_d,
					 GList *tracks);
/* needed to adapt the prefs structure */
extern const gchar *EXPORT_FILES_SPECIAL_CHARSET;
extern const gchar *EXPORT_FILES_CHECK_EXISTING;
extern const gchar *EXPORT_FILES_PATH;
extern const gchar *EXPORT_FILES_TPL;
#endif
