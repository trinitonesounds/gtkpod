/* Time-stamp: <2005-05-25 00:02:55 jcs>
|
|  Copyright (C) 2002-2004 Jorg Schuler <jcsjcs at users.sourceforge.net>
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
|
|  $Id$
*/

#ifndef __DISPLAY_ITDB_H__
#define __DISPLAY_ITDB_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <itdb.h>
#include <gtk/gtk.h>

struct itdbs_head
{
    GList *itdbs;
};

typedef struct
{
    struct itdbs_head *itdbs_head; /* pointer to the master itdbs_head */
    GHashTable *md5hash;           /* md5 hash information             */
    GList *pending_deletion;       /* tracks marked for removal from
				      media                            */
    gchar *offline_filename;       /* filename for offline database
				      (only for GP_ITDP_TYPE_IPOD)     */
    gboolean data_changed;         /* data changed since import?       */
    gboolean itdb_imported;        /* has in iTunesDB been imported?   */
} ExtraiTunesDBData;

typedef struct
{
    glong size;
} ExtraPlaylistData;

typedef struct
{
  gint32  oldsize;        /* used when updating tracks: size on iPod */
  gchar   *year_str;      /* year as string -- always identical to year */
  guint32 peak_signal;	  /* LAME Peak Signal * 0x800000            */
  gdouble radio_gain;	  /* RadioGain in dB
			     (as defined by www.replaygain.org)     */
  gdouble audiophile_gain;/* AudiophileGain in dB 
			     (as defined by www.replaygain.org)     */
  gboolean peak_signal_set;    /* has the peak signal been set?     */
  gboolean radio_gain_set;     /* has the radio gain been set?      */
  gboolean audiophile_gain_set;/* has the audiophile gain been set? */
  gchar   *pc_path_locale;/* path on PC (local encoding)            */
  gchar   *pc_path_utf8;  /* PC filename in utf8 encoding           */
  gchar   *hostname;      /* name of host this file has been imported on*/
  gchar   *md5_hash;      /* md5 hash of file (or NULL)             */
  gchar   *charset;       /* charset used for ID3 tags              */
} ExtraTrackData;

/* types for iTunesDB */
typedef enum
{
    GP_ITDB_TYPE_LOCAL = 1<<0,   /* local browsing */
    GP_ITDB_TYPE_IPOD  = 1<<1,    /* iPod */
} GpItdbType;

/* Delete actions */
typedef enum
{
    /* remove from playlist only -- cannot be used on MPL */
    DELETE_ACTION_PLAYLIST = 0,
    /* remove from iPod (implicates removing from database) */
    DELETE_ACTION_IPOD,
    /* remove from local harddisk (implicates removing from database) */
    DELETE_ACTION_LOCAL,
    /* remove from database only */
    DELETE_ACTION_DATABASE
} DeleteAction;

struct DeleteData
{
    iTunesDB *itdb;
    Playlist *pl;
    GList *selected_tracks;
    DeleteAction deleteaction;
};

void init_data (GtkWidget *window);

iTunesDB *gp_itdb_new (void);
void gp_itdb_add (iTunesDB *itdb, gint pos);
void gp_replace_itdb (iTunesDB *old_itdb, iTunesDB *new_itdb);
void gp_itdb_add_extra (iTunesDB *itdb);
void gp_itdb_add_extra_full (iTunesDB *itdb);

Track *gp_track_new (void);
#define gp_track_free itdb_track_free
Track *gp_track_add (iTunesDB *itdb, Track *track);
void gp_track_add_extra (Track *track);
void gp_track_validate_entries (Track *track);

Playlist *gp_playlist_new (const gchar *title, gboolean spl);
void gp_playlist_add (iTunesDB *itdb, Playlist *pl, gint32 pos);
void gp_playlist_remove (Playlist *pl);
guint gp_playlist_remove_by_name (iTunesDB *itdb, gchar *pl_name);
Playlist *gp_playlist_add_new (iTunesDB *itdb, gchar *name,
			       gboolean spl, gint32 pos);
Playlist *gp_playlist_by_name_or_add (iTunesDB *itdb, gchar *pl_name,
				      gboolean spl);
void gp_playlist_remove_track (Playlist *plitem, Track *track,
			       DeleteAction deleteaction);
void gp_playlist_add_track (Playlist *pl, Track *track, gboolean display);

void gp_playlist_add_extra (Playlist *pl);

gboolean gp_increase_playcount (gchar *md5, gchar *file, gint num);
iTunesDB *gp_get_active_itdb (void);
iTunesDB *gp_get_ipod_itdb (void);
void gp_update_itdb_prefs (void);
#endif
