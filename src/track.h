/* Time-stamp: <2004-07-20 00:46:19 jcs>
|
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
|
|  $Id$
*/

#ifndef __SONG_H__
#define __SONG_H__


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib.h>

typedef struct
{
  gchar   *album;            /* album (utf8)           */
  gchar   *artist;           /* artist (utf8)          */
  gchar   *title;            /* title (utf8)           */
  gchar   *genre;            /* genre (utf8)           */
  gchar   *comment;          /* comment (utf8)         */
  gchar   *composer;         /* Composer (utf8)        */
  gchar   *fdesc;            /* ? (utf8)               */
  gchar   *ipod_path;        /* name of file on iPod: uses ":" instead of "/"*/
  gunichar2 *album_utf16;    /* album (utf16)          */
  gunichar2 *artist_utf16;   /* artist (utf16)         */
  gunichar2 *title_utf16;    /* title (utf16)          */
  gunichar2 *genre_utf16;    /* genre (utf16)          */
  gunichar2 *comment_utf16;  /* comment (utf16)        */
  gunichar2 *composer_utf16; /* Composer (utf16)       */
  gunichar2 *fdesc_utf16;    /* ? (utf16)              */
  gunichar2 *ipod_path_utf16;/* name of file on iPod: uses ":" instead of "/"*/
  gchar   *pc_path_utf8;     /* PC filename in utf8 encoding   */
  gchar   *pc_path_locale;   /* PC filename in locale encoding */
  guint32 ipod_id;           /* unique ID of track     */
  gint32  size;              /* size of file in bytes  */
  gint32  oldsize;           /* used when updating tracks: size on iPod */
  gint32  tracklen;          /* Length of track in ms  */
  gint32  cd_nr;             /* CD number              */
  gint32  cds;               /* number of CDs          */
  gint32  track_nr;          /* track number           */
  gint32  tracks;            /* number of tracks       */
  gint32  bitrate;           /* bitrate                */
  guint16 samplerate;        /* samplerate (CD: 44100) */
  gint32  year;              /* year                   */
  gchar   *year_str;         /* year as string -- always identical to year */
  gint32  volume;            /* volume adjustment              */
  gint32  soundcheck;        /* volume adjustment "soundcheck" */
  guint32 peak_signal;	     /* LAME Peak Signal * 0x800000    */
  gint32  radio_gain;	     /* RadioGain in dB*10
				(as defined by www.replaygain.org) */
  gint32  audiophile_gain;   /* AudiophileGain in dB*10 
				(as defined by www.replaygain.org)  */
  gboolean peak_signal_set;  /* has the peak signal been set?       */
  gboolean radio_gain_set;   /* has the radio gain been set?        */
  gboolean audiophile_gain_set;/* has the audiophile gain been set? */
  guint32 time_created;      /* time when added (Mac type)          */
  guint32 time_played;       /* time of last play (Mac type)        */
  guint32 time_modified;     /* time of last modification (Mac type)*/
  guint32 rating;            /* star rating (stars * RATING_STEP (20))     */
  guint32 playcount;         /* number of times track was played    */
  guint32 recent_playcount;  /* times track was played since last sync     */
  gchar   *hostname;         /* name of host this file has been imported on*/
  gboolean transferred;      /* has file been transferred to iPod?  */
  gchar   *md5_hash;         /* md5 hash of file (or NULL)          */
  gchar   *charset;          /* charset used for ID3 tags           */
  gint16 BPM;                /* supposed to vary the playback speed */
/* present in the mhit but not used by gtkpod yet */
  guint32 unk020, unk024, unk084, unk100, unk108, unk112, unk116, unk124;
  guint32 unk128, unk132, unk136, unk140, unk144, unk148, unk152;
  guint8  app_rating;        /* star rating set by appl. (not iPod) */
  guint16 type;
  guint8  compilation;
  guint32 starttime;
  guint32 stoptime;
  guint8  checked;
} Track;
/* !Don't forget to add fields read from the file to copy_new_info() in
 * file.c! */


/* one star is how much (track->rating) */
#define RATING_STEP 20

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
} T_item;

void free_track(Track *track);
gboolean it_add_track (Track *track);
Track *add_track (Track *track);
void validate_entries (Track *track);
void remove_track (Track *track);
void remove_all_tracks (void);
#define it_get_nr_of_tracks get_nr_of_tracks
guint get_nr_of_tracks (void);
guint get_nr_of_nontransferred_tracks (void);
double get_filesize_of_nontransferred_tracks (guint32 *num);
Track *it_get_track_by_nr (guint32 n);
Track *get_next_track (gint i);
Track *get_track_by_nr (guint32 n);
Track *get_track_by_id (guint32 id);
Track *get_track_by_filename (gchar *name);
void remove_track_from_ipod (Track *track);
void hash_tracks(void);
void remove_duplicate (Track *oldtrack, Track *track);
void clear_md5_hash_from_tracks (void);
void renumber_ipod_ids ();
gchar **track_get_item_pointer_utf8 (Track *track, T_item t_item);
gchar *track_get_item_utf8 (Track *track, T_item t_item);
gunichar2 **track_get_item_pointer_utf16 (Track *track, T_item t_item);
gunichar2 *track_get_item_utf16 (Track *track, T_item t_item);
guint32 *track_get_timestamp_ptr (Track *track, T_item t_item);
guint32 track_get_timestamp (Track *track, T_item t_item);
gboolean track_increase_playcount (gchar *md5, gchar *file, gint num);
#endif
