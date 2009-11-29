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

#ifndef TYPES_H_
#define TYPES_H_

#define DATE_FORMAT_LONG "%x %X"
#define DATE_FORMAT_SHORT "%x"

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
#include "itdb.h"

/* max. number of stars */
#define RATING_MAX 5

/* number of entries with "autoset empty tag to filename " feature */
#define TM_NUM_TAGS_PREFS (5)

/* Column numbers in track model */
/* Note: add corresponding entries to T_item and TM_to_T() as well
 * (below and in misc_conversion.c).
 * IMPORTANT: Do not change the order -- always add new entries at the
 * end */
typedef enum  {
  TM_COLUMN_TITLE = 0,
  TM_COLUMN_ARTIST,
  TM_COLUMN_ALBUM,
  TM_COLUMN_GENRE,
  TM_COLUMN_COMPOSER,
  TM_COLUMN_TRACK_NR,         /*  5 */
  TM_COLUMN_IPOD_ID,
  TM_COLUMN_PC_PATH,
  TM_COLUMN_TRANSFERRED,
  TM_COLUMN_SIZE,
  TM_COLUMN_TRACKLEN,         /* 10 */
  TM_COLUMN_BITRATE,
  TM_COLUMN_PLAYCOUNT,
  TM_COLUMN_RATING,
  TM_COLUMN_TIME_PLAYED,
  TM_COLUMN_TIME_MODIFIED,    /* 15 */
  TM_COLUMN_VOLUME,
  TM_COLUMN_YEAR,
  TM_COLUMN_CD_NR,
  TM_COLUMN_TIME_ADDED,
  TM_COLUMN_IPOD_PATH,        /* 20 */
  TM_COLUMN_SOUNDCHECK,
  TM_COLUMN_SAMPLERATE,
  TM_COLUMN_BPM,
  TM_COLUMN_FILETYPE,
  TM_COLUMN_GROUPING,         /* 25 */
  TM_COLUMN_COMPILATION,
  TM_COLUMN_COMMENT,
  TM_COLUMN_CATEGORY,
  TM_COLUMN_DESCRIPTION,
  TM_COLUMN_PODCASTURL,       /* 30 */
  TM_COLUMN_PODCASTRSS,
  TM_COLUMN_SUBTITLE,
  TM_COLUMN_TIME_RELEASED,
  TM_COLUMN_THUMB_PATH,
  TM_COLUMN_MEDIA_TYPE,       /* 35 */
  TM_COLUMN_TV_SHOW,
  TM_COLUMN_TV_EPISODE,
  TM_COLUMN_TV_NETWORK,
  TM_COLUMN_SEASON_NR,
  TM_COLUMN_EPISODE_NR,       /* 40 */
  TM_COLUMN_ALBUMARTIST,
  TM_COLUMN_SORT_ARTIST,
  TM_COLUMN_SORT_TITLE,
  TM_COLUMN_SORT_ALBUM,
  TM_COLUMN_SORT_ALBUMARTIST, /* 45 */
  TM_COLUMN_SORT_COMPOSER,
  TM_COLUMN_SORT_TVSHOW,
  TM_COLUMN_LYRICS,
  TM_NUM_COLUMNS
} TM_item;

/* A means to address the fields by uniform IDs. May be extended as
 * needed. You should extend "track_get_item_pointer()" defined in
 * track.c as well for string fields. */
/* Add corresponding entries to t_strings[] and t_tooltips[] in
   misc_conversion.c! */
/* Used in prefs_window.c to label the sort_ign_field<num> buttons */
/* Used in details.c to label the detail_label_<num> labels */
typedef enum {
    T_ALL = 0,      /* all fields */
    T_ALBUM,
    T_ARTIST,
    T_TITLE,
    T_GENRE,
    T_COMMENT,      /*  5 */
    T_COMPOSER,
    T_FILETYPE,
    T_PC_PATH,
    T_IPOD_PATH,
    T_IPOD_ID,      /* 10 */
    T_TRACK_NR,
    T_TRANSFERRED,
    T_SIZE,
    T_TRACKLEN,
    T_BITRATE,      /* 15 */
    T_SAMPLERATE,
    T_BPM,
    T_PLAYCOUNT,
    T_RATING,
    T_TIME_ADDED,   /* 20 */
    T_TIME_PLAYED,
    T_TIME_MODIFIED,
    T_VOLUME,
    T_SOUNDCHECK,
    T_YEAR,         /* 25 */
    T_CD_NR,
    T_GROUPING,
    T_COMPILATION,
    T_CATEGORY,
    T_DESCRIPTION,  /* 30 */
    T_PODCASTURL,
    T_PODCASTRSS,
    T_SUBTITLE,
    T_TIME_RELEASED,
    T_CHECKED,      /* 35 */
    T_STARTTIME,
    T_STOPTIME,
    T_REMEMBER_PLAYBACK_POSITION,
    T_SKIP_WHEN_SHUFFLING,
    T_THUMB_PATH,   /* 40 */
    T_MEDIA_TYPE,
    T_TV_SHOW,
    T_TV_EPISODE,
    T_TV_NETWORK,
    T_SEASON_NR,    /* 45 */
    T_EPISODE_NR,
    T_ALBUMARTIST,
    T_SORT_ARTIST,
    T_SORT_TITLE,
    T_SORT_ALBUM,   /* 50 */
    T_SORT_ALBUMARTIST,
    T_SORT_COMPOSER,
    T_SORT_TVSHOW,
    T_GAPLESS_TRACK_FLAG,
    T_LYRICS,
    T_ITEM_NUM,
} T_item;

T_item TM_to_T (TM_item tm);
const gchar *get_tm_string (TM_item tm);
const gchar *get_tm_tooltip (TM_item tm);
const gchar *get_t_string (T_item t);
const gchar *get_t_tooltip (T_item t);
time_t time_get_time (Track *track, T_item t_item);
gchar *time_field_to_string (Track *track, T_item t_item);
void time_set_time (Track *track, time_t timet, T_item t_item);

#endif /* TYPES_H_ */
