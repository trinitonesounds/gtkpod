/* Time-stamp: <2005-01-08 13:46:41 jcs>
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
    GHashTable *md5hash;           /* md5 hash information       */
    gboolean data_changed;         /* data changed since import? */
} ExtraiTunesDBData;

typedef struct
{
    glong size;
} ExtraPlaylistData;

typedef struct
{
  gint32  oldsize;        /* used when updating tracks: size on iPod */
  gchar   *year_str;      /* year as string -- always identical to year */
  guint32 peak_signal;	  /* LAME Peak Signal * 0x800000    */
  gdouble radio_gain;	  /* RadioGain in dB
			     (as defined by www.replaygain.org) */
  gdouble audiophile_gain;/* AudiophileGain in dB 
			     (as defined by www.replaygain.org)  */
  gboolean peak_signal_set;    /* has the peak signal been set?       */
  gboolean radio_gain_set;     /* has the radio gain been set?        */
  gboolean audiophile_gain_set;/* has the audiophile gain been set? */
  gchar   *pc_path_utf8;  /* PC filename in utf8 encoding   */
  gchar   *hostname;      /* name of host this file has been imported on*/
  gchar   *md5_hash;      /* md5 hash of file (or NULL)          */
  gchar   *charset;       /* charset used for ID3 tags           */
} ExtraTrackData;

/* A means to address the fields by uniform IDs. May be extended as
 * needed. You should extend "track_get_item_pointer()" defined in
 * track.c as well for string fields. */
typedef enum {
    T_ALL = 0,      /* all fields */
    T_ALBUM,
    T_ARTIST,
    T_TITLE,
    T_GENRE,
    T_COMMENT,
    T_COMPOSER,
    T_FDESC,
    T_PC_PATH,
    T_IPOD_PATH,
    T_IPOD_ID,
    T_TRACK_NR,
    T_TRANSFERRED,
    T_SIZE,
    T_TRACKLEN,
    T_BITRATE,
    T_SAMPLERATE,
    T_BPM,
    T_PLAYCOUNT,
    T_RATING,
    T_TIME_CREATED,
    T_TIME_PLAYED,
    T_TIME_MODIFIED,
    T_VOLUME,
    T_SOUNDCHECK,
    T_YEAR,
    T_CD_NR,
    T_GROUPING,
    T_COMPILATION,
} T_item;

/* types for iTunesDB */
typedef enum
{
    GP_ITDB_TYPE_IPOD,    /* iPod */
    GP_ITDB_TYPE_LOCAL,   /* local browsing */
} GpItdbType;


void init_data (GtkWidget *window);

iTunesDB *gp_itdb_new (void);
void gp_itdb_add (iTunesDB *itdb);

Track *gp_track_new (void);
Track *gp_track_add (iTunesDB *itdb, Track *track);

Playlist *gp_playlist_new (const gchar *title, gboolean spl);
void gp_playlist_add (iTunesDB *itdb, Playlist *pl, gint32 pos);
Playlist *gp_playlist_add_new (iTunesDB *itdb, gchar *name,
			       gboolean spl, gint32 pos);
void gp_playlist_remove_track (Playlist *plitem, Track *track);
void gp_playlist_add_track (Playlist *pl, Track *track, gboolean display);

gboolean gp_increase_playcount (gchar *md5, gchar *file, gint num);

#endif
