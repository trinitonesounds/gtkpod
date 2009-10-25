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

#include "types.h"

/* Note: the toggle buttons for tag_autoset and display_col in the
 * prefs_window are named after the the TM_COLUM_* numbers defined in
 * display.h (Title: tag_autoset0, Artist: tag_autoset1
 * etc.). ign_field is named after T_*. Since the labels to the
 * buttons are set in prefs_window.c when creating the window, you
 * only need to name the buttons in the intended order using
 * glade-2. There is no need to label them. */
/* Strings associated to the column headers */
static const gchar *t_strings[] = {
    N_("All"),               /*  0 */
    N_("Album"),
    N_("Artist"),
    N_("Title"),
    N_("Genre"),
    N_("Comment"),           /*  5 */
    N_("Composer"),
    N_("File type"),
    N_("PC File"),
    N_("iPod File"),
    N_("iPod ID"),           /* 10 */
    N_("Track Nr (#)"),
    N_("Transferred"),
    N_("File Size"),
    N_("Play Time"),
    N_("Bitrate"),           /* 15 */
    N_("Samplerate"),
    N_("BPM"),
    N_("Playcount"),
    N_("Rating"),
    N_("Date added"),        /* 20 */
    N_("Date played"),
    N_("Date modified"),
    N_("Volume"),
    N_("Soundcheck"),
    N_("Year"),              /* 25 */
    N_("CD Nr"),
    N_("Grouping"),
    N_("Compilation"),
    N_("Category"),
    N_("Description"),       /* 30 */
    N_("Podcast URL"),
    N_("Podcast RSS"),
    N_("Subtitle"),
    N_("Date released"),
    N_("Checked"),           /* 35 */
    N_("Start time"),
    N_("Stop time"),
    N_("Remember Playback Position"),
    N_("Skip when Shuffling"),
    N_("Artwork Path"),      /* 40 */
    N_("Media Type"),
    N_("TV Show"),
    N_("TV Episode"),
    N_("TV Network"),
    N_("Season Nr"),         /* 45 */
    N_("Episode Nr"),
    N_("Album Artist"),
    N_("Sort Artist"),
    N_("Sort Title"),
    N_("Sort Album"),        /* 50 */
    N_("Sort Album Artist"),
    N_("Sort Composer"),
    N_("Sort TV Show"),
    N_("Gapless Track Flag"),
    N_("Lyrics"),
    NULL };

/* Tooltips for prefs window */
static const gchar *t_tooltips[] = {
    NULL,                                              /*  0 */
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,                                              /*  5 */
    NULL,
    NULL,
    N_("Name of file on PC, if available"),
    N_("Name of file on the iPod"),
    NULL,                                              /* 10 */
    N_("Track Nr. and total number of tracks on CD"),
    N_("Whether the file has already been "
       "transferred to the iPod or not"),
    NULL,
    NULL,
    NULL,                                              /* 15 */
    NULL,
    N_("Beats per minute"),
    N_("Number of times the track has been played"),
    N_("Star rating from 0 to 5"),
    N_("Date and time track has been added"),          /* 20 */
    N_("Date and time track has last been played"),
    N_("Date and time track has last been modified"),
    N_("Manual volume adjust"),
    N_("Volume adjust in dB (replay gain) -- "
       "you need to activate 'soundcheck' on the iPod"),
    NULL,                                              /* 25 */
    N_("CD Nr. and total number of CDS in set"),
    NULL,
    NULL,
    N_("The category (e.g. 'Technology' or 'Music') where the podcast was located."),
    N_("Accessible by selecting the center button on the iPod."), /* 30 */
    NULL,
    NULL,
    NULL,
    N_("Release date (for podcasts displayed next to the title on the iPod)"),
    NULL,   /* 35 */
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,   /* 40 */
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,   /* 45 */
    NULL,
    NULL,
    N_("Used for sorting on the iPod"),
    N_("Used for sorting on the iPod"),
    N_("Used for sorting on the iPod"),  /* 50 */
    N_("Used for sorting on the iPod"),
    N_("Used for sorting on the iPod"),
    N_("Used for sorting on the iPod"),
    NULL,
    NULL
 };

/* translates a TM_COLUMN_... (defined in display.h) into a
 * T_... (defined in display.h). Returns -1 in case a translation is not
 * possible */
T_item TM_to_T (TM_item tm)
{
    switch (tm)
    {
    case TM_COLUMN_TITLE:         return T_TITLE;
    case TM_COLUMN_ARTIST:        return T_ARTIST;
    case TM_COLUMN_ALBUM:         return T_ALBUM;
    case TM_COLUMN_GENRE:         return T_GENRE;
    case TM_COLUMN_COMPOSER:      return T_COMPOSER;
    case TM_COLUMN_FILETYPE:      return T_FILETYPE;
    case TM_COLUMN_GROUPING:      return T_GROUPING;
    case TM_COLUMN_TRACK_NR:      return T_TRACK_NR;
    case TM_COLUMN_CD_NR:         return T_CD_NR;
    case TM_COLUMN_IPOD_ID:       return T_IPOD_ID;
    case TM_COLUMN_PC_PATH:       return T_PC_PATH;
    case TM_COLUMN_IPOD_PATH:     return T_IPOD_PATH;
    case TM_COLUMN_TRANSFERRED:   return T_TRANSFERRED;
    case TM_COLUMN_SIZE:          return T_SIZE;
    case TM_COLUMN_TRACKLEN:      return T_TRACKLEN;
    case TM_COLUMN_BITRATE:       return T_BITRATE;
    case TM_COLUMN_SAMPLERATE:    return T_SAMPLERATE;
    case TM_COLUMN_BPM:           return T_BPM;
    case TM_COLUMN_PLAYCOUNT:     return T_PLAYCOUNT;
    case TM_COLUMN_RATING:        return T_RATING;
    case TM_COLUMN_TIME_ADDED:    return T_TIME_ADDED;
    case TM_COLUMN_TIME_PLAYED:   return T_TIME_PLAYED;
    case TM_COLUMN_TIME_MODIFIED: return T_TIME_MODIFIED;
    case TM_COLUMN_VOLUME:        return T_VOLUME;
    case TM_COLUMN_SOUNDCHECK:    return T_SOUNDCHECK;
    case TM_COLUMN_YEAR:          return T_YEAR;
    case TM_COLUMN_COMPILATION:   return T_COMPILATION;
    case TM_COLUMN_COMMENT:       return T_COMMENT;
    case TM_COLUMN_CATEGORY:      return T_CATEGORY;
    case TM_COLUMN_DESCRIPTION:   return T_DESCRIPTION;
    case TM_COLUMN_PODCASTURL:    return T_PODCASTURL;
    case TM_COLUMN_PODCASTRSS:    return T_PODCASTRSS;
    case TM_COLUMN_SUBTITLE:      return T_SUBTITLE;
    case TM_COLUMN_TIME_RELEASED: return T_TIME_RELEASED;
    case TM_COLUMN_THUMB_PATH:    return T_THUMB_PATH;
    case TM_COLUMN_MEDIA_TYPE:    return T_MEDIA_TYPE;
    case TM_COLUMN_TV_SHOW:       return T_TV_SHOW;
    case TM_COLUMN_TV_EPISODE:    return T_TV_EPISODE;
    case TM_COLUMN_TV_NETWORK:    return T_TV_NETWORK;
    case TM_COLUMN_SEASON_NR:     return T_SEASON_NR;
    case TM_COLUMN_EPISODE_NR:    return T_EPISODE_NR;
    case TM_COLUMN_ALBUMARTIST:   return T_ALBUMARTIST;
    case TM_COLUMN_SORT_ARTIST:   return T_SORT_ARTIST;
    case TM_COLUMN_SORT_TITLE:    return T_SORT_TITLE;
    case TM_COLUMN_SORT_ALBUM:    return T_SORT_ALBUM;
    case TM_COLUMN_SORT_ALBUMARTIST: return T_SORT_ALBUMARTIST;
    case TM_COLUMN_SORT_COMPOSER: return T_SORT_COMPOSER;
    case TM_COLUMN_SORT_TVSHOW:   return T_SORT_TVSHOW;
    case TM_COLUMN_LYRICS:        return T_LYRICS;
    case TM_NUM_COLUMNS:          g_return_val_if_reached (-1);
    }
    return -1;
}

/* return descriptive string (non-localized -- pass through gettext()
 * for the localized version) for tm_item (usually used to name
 * buttons or column headers). */
const gchar *get_tm_string (TM_item tm)
{
    T_item t = TM_to_T (tm);

    g_return_val_if_fail (t != -1, "");

    return t_strings[t];
}


/* return string (non-localized -- pass through gettext()
 * for the localized version) for tm_item that can be used as a
 * tooltip */
const gchar *get_tm_tooltip (TM_item tm)
{
    T_item t = TM_to_T (tm);

    g_return_val_if_fail (t != -1, "");

    return t_tooltips[t];
}

/* return descriptive string (non-localized -- pass through gettext()
 * for the localized version) for tm_item (usually used to name
 * buttons or column headers). */
const gchar *get_t_string (T_item t)
{
    g_return_val_if_fail (t>=0 && t<T_ITEM_NUM, "");

    return t_strings[t];
}


/* return string (non-localized -- pass through gettext()
 * for the localized version) for tm_item that can be used as a
 * tooltip */
const gchar *get_t_tooltip (T_item t)
{
    if ((t >= 0) && (t<T_ITEM_NUM))   return t_tooltips[t];
    else                              return ("");
}

/* get the timestamp TM_COLUMN_TIME_CREATE/PLAYED/MODIFIED/RELEASED */
time_t time_get_time (Track *track, T_item t_item)
{
    guint32 mactime = 0;

    if (track) switch (t_item)
    {
    case T_TIME_ADDED:
    mactime = track->time_added;
    break;
    case T_TIME_PLAYED:
    mactime = track->time_played;
    break;
    case T_TIME_MODIFIED:
    mactime = track->time_modified;
    break;
    case T_TIME_RELEASED:
    mactime = track->time_released;
    break;
    default:
    mactime = 0;
    break;
    }
    return mactime;
}

static gchar *time_to_string_format (time_t t, const gchar *format)
{
    gchar buf[PATH_MAX+1];
    struct tm tm;
    size_t size;

    g_return_val_if_fail (format, NULL);

    if (t)
    {
    localtime_r (&t, &tm);
    size = strftime (buf, PATH_MAX, format, &tm);
    buf[size] = 0;
    return g_locale_to_utf8 (buf, -1, NULL, NULL, NULL);
    }
    return g_strdup ("--");
}

/* hopefully obvious */
gchar *time_field_to_string (Track *track, T_item t_item)
{
    time_t t = time_get_time (track, t_item);
    return time_to_string_format (t, DATE_FORMAT_LONG);
}

/* get the timestamp TM_COLUMN_TIME_CREATE/PLAYED/MODIFIED/RELEASED */
void time_set_time (Track *track, time_t timet, T_item t_item)
{
    g_return_if_fail (track);

    switch (t_item)
    {
    case T_TIME_ADDED:
    track->time_added = timet;
    break;
    case T_TIME_PLAYED:
    track->time_played = timet;
    break;
    case T_TIME_MODIFIED:
    track->time_modified = timet;
    break;
    case T_TIME_RELEASED:
    track->time_released = timet;
    break;
    default:
    break;
    }
}
