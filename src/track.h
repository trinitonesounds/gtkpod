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

#ifndef __SONG_H__
#define __SONG_H__


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

typedef struct
{
  gchar   *album;            /* album (utf8)          */
  gchar   *artist;           /* artist (utf8)         */
  gchar   *title;            /* title (utf8)          */
  gchar   *genre;            /* genre (utf8)          */
  gchar   *comment;          /* comment (utf8)        */
  gchar   *composer;         /* Composer (utf8)       */
  gchar   *fdesc;            /* ? (utf8)              */
  gchar   *ipod_path;        /* name of file on iPod: uses ":" instead of "/"*/
  gunichar2 *album_utf16;    /* album (utf16)         */
  gunichar2 *artist_utf16;   /* artist (utf16)        */
  gunichar2 *title_utf16;    /* title (utf16)         */
  gunichar2 *genre_utf16;    /* genre (utf16)         */
  gunichar2 *comment_utf16;  /* comment (utf16)       */
  gunichar2 *composer_utf16; /* Composer (utf16)      */
  gunichar2 *fdesc_utf16;    /* ? (utf16)             */
  gunichar2 *ipod_path_utf16;/* name of file on iPod: uses ":" instead of "/"*/
  gchar   *pc_path_utf8;     /* PC filename in utf8 encoding   */
  gchar   *pc_path_locale;   /* PC filename in locale encoding */
  guint32 ipod_id;           /* unique ID of song     */
  gint32  size;              /* size of file in bytes */
  gint32  songlen;           /* Length of song in ms  */
  gint32  cd_nr;             /* CD number             */
  gint32  cds;               /* number of CDs         */
  gint32  track_nr;          /* track number          */
  gint32  tracks;            /* number of tracks      */
  gint32  year;              /* year                  */
  gint32  bitrate;           /* bitrate               */
  gchar   *hostname;         /* name of host this file has been imported from */
  gboolean transferred;      /* has file been transferred to iPod? */
  gchar   *md5_hash;         /* md5 hash of file (or NULL)         */
  guint   pos;               /* position of this song in selected playlist */
} Song;

/* A means to address the fields by uniform IDs. May be extended as
 * needed. You should extend "song_get_item_pointer()" defined in
 * song.c as well. */
typedef enum {
    S_ALL,      /* all fields */
    S_ALBUM,
    S_ARTIST,
    S_TITLE,
    S_GENRE,
    S_COMMENT,
    S_COMPOSER,
    S_FDESC,
    S_PC_PATH,
    S_IPOD_PATH,
    S_IPOD_ID,
    S_TRACK_NR,
    S_TRANSFERRED,
    S_NONE
} S_item;

void free_song(Song *song);
gboolean it_add_song (Song *song);
Song *add_song (Song *song);
void validate_entries (Song *song);
void remove_song (Song *song);
void remove_all_songs (void);
#define it_get_nr_of_songs get_nr_of_songs
guint get_nr_of_songs (void);
guint get_nr_of_nontransferred_songs (void);
Song *it_get_song_by_nr (guint32 n);
Song *get_song_by_nr (guint32 n);
Song *get_song_by_id (guint32 id);
Song *get_song_by_filename (gchar *name);
void remove_song_from_ipod (Song *song);
void hash_songs(void);
void remove_duplicate (Song *oldsong, Song *song);
void clear_md5_hash_from_songs (void);
void renumber_ipod_ids ();
gchar **song_get_item_pointer_utf8 (Song *song, S_item s_item);
gchar *song_get_item_utf8 (Song *song, S_item s_item);
gunichar2 **song_get_item_pointer_utf16 (Song *song, S_item s_item);
gunichar2 *song_get_item_utf16 (Song *song, S_item s_item);
#endif __SONG_H__
