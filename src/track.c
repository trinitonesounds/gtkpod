/* Time-stamp: <2003-11-30 10:53:26 jcs>
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "prefs.h"
#include "song.h"
#include "misc.h"
#include "support.h"
#include "md5.h"
#include "confirmation.h"
#include "file.h"

/* List with all the tracks */
GList *tracks = NULL;

static guint32 free_ipod_id (guint32 id);
static void reset_ipod_id (void);

void
free_track(Track *track)
{
  if (track)
    {
      g_free (track->album);
      g_free (track->artist);
      g_free (track->title);
      g_free (track->genre);
      g_free (track->comment);
      g_free (track->composer);
      g_free (track->fdesc);
      g_free (track->album_utf16);
      g_free (track->artist_utf16);
      g_free (track->title_utf16);
      g_free (track->genre_utf16);
      g_free (track->comment_utf16);
      g_free (track->composer_utf16);
      g_free (track->fdesc_utf16);
      g_free (track->pc_path_utf8);
      g_free (track->pc_path_locale);
      g_free (track->ipod_path);
      g_free (track->ipod_path_utf16);
      g_free (track->md5_hash);
      g_free (track->hostname);
      g_free (track->charset);
      g_free (track->year_str);
      g_free (track);
    }
}

/* Append track to the list */
/* Note: adding to the display model is the responsibility of
   the playlist code */
/* Returns: pointer to the added track -- may be different in the case
   of duplicates. In that case a pointer to the already existing track
   is returned. */
Track *add_track (Track *track)
{
  Track *oldtrack, *result=NULL;

  /* fill in additional information from the extended info hash
     (extended info hash only exists during initial import of the
     database */
  fill_in_extended_info (track);
  if((oldtrack = md5_track_exists_insert (track)))
  {
      remove_duplicate (oldtrack, track);
      free_track(track);
      result = oldtrack;
  }
  else
  {
    /* Make sure all strings are initialised -- that way we don't 
     have to worry about it when we are handling the strings */
    /* exception: md5_hash, hostname, charset: these may be NULL. */
    validate_entries (track);
    if(!track->ipod_id) 
	track->ipod_id = free_ipod_id (0);  /* keep track of highest ID used */
    else
	free_ipod_id(track->ipod_id);
    
    tracks = g_list_append (tracks, track);
    result = track;
  }
  return result;
}


/* Make sure all strings are initialised -- that way we don't 
   have to worry about it when we are handling the strings.
   If a corresponding utf16 string is not set, it will be created from
   the utf8 string */
/* exception: md5_hash and hostname: these may be NULL. */
void validate_entries (Track *track)
{
    if (!track) return;

    if (track->album == NULL)           track->album = g_strdup ("");
    if (track->artist == NULL)          track->artist = g_strdup ("");
    if (track->title == NULL)           track->title = g_strdup ("");
    if (track->genre == NULL)           track->genre = g_strdup ("");
    if (track->comment == NULL)         track->comment = g_strdup ("");
    if (track->composer == NULL)        track->composer = g_strdup ("");
    if (track->fdesc == NULL)           track->fdesc = g_strdup ("");
    if (track->pc_path_utf8 == NULL)    track->pc_path_utf8 = g_strdup ("");
    if (track->pc_path_locale == NULL)  track->pc_path_locale = g_strdup ("");
    if (track->ipod_path == NULL)       track->ipod_path = g_strdup ("");
    if (track->album_utf16 == NULL)     
	track->album_utf16 = g_utf8_to_utf16 (track->album, -1, NULL, NULL, NULL);
    if (track->artist_utf16 == NULL)
	track->artist_utf16 = g_utf8_to_utf16 (track->artist, -1, NULL, NULL, NULL);
    if (track->title_utf16 == NULL)
	track->title_utf16 = g_utf8_to_utf16 (track->title, -1, NULL, NULL, NULL);
    if (track->genre_utf16 == NULL)
	track->genre_utf16 = g_utf8_to_utf16 (track->genre, -1, NULL, NULL, NULL);
    if (track->comment_utf16 == NULL)
	track->comment_utf16 = g_utf8_to_utf16 (track->comment, -1, NULL, NULL, NULL);
    if (track->composer_utf16 == NULL)
	track->composer_utf16 = g_utf8_to_utf16 (track->composer, -1, NULL, NULL, NULL);
    if (track->fdesc_utf16 == NULL)
	track->fdesc_utf16 = g_utf8_to_utf16 (track->fdesc, -1, NULL, NULL, NULL);
    if (track->ipod_path_utf16 == NULL)
	track->ipod_path_utf16 = g_utf8_to_utf16 (track->ipod_path, -1, NULL, NULL, NULL);
    /* Make sure year_str is identical to year */
    g_free (track->year_str);
    track->year_str = g_strdup_printf ("%d", track->year);
}

/**
 * remove_track_from_ipod - in order to delete a track from the system
 * we need to keep track of the Tracks we want to delete next time we export
 * the id. 
 * @track - the Track id we want to delete
 */
void
remove_track_from_ipod (Track *track)
{
    if (track->transferred)
    {
	tracks = g_list_remove(tracks, track);
	md5_track_removed (track);
	mark_track_for_deletion (track); /* will be deleted on next export */
    }
    else
    {
	remove_track (track);
    }
}

/* Remove track from the list. */
void remove_track (Track *track)
{
  tracks = g_list_remove (tracks, track);
  md5_track_removed (track);
  free_track(track);
}


/* Remove all tracks from the list using remove_track () */
void remove_all_tracks (void)
{
  Track *track;

  while (tracks != NULL)
    {
      track = g_list_nth_data (tracks, 0);
      remove_track (track);
    }
}


/* Returns the number of tracks stored in the list */
guint get_nr_of_tracks (void)
{
  return g_list_length (tracks);
}

/* Returns the number of tracks not yet transferred to the ipod */
guint get_nr_of_nontransferred_tracks (void)
{
    guint n = 0;
    Track *track;
    GList *gl_track;

    for (gl_track = tracks; gl_track; gl_track=gl_track->next)
    {
	track = (Track *)gl_track->data;
	if (!track->transferred)   ++n;
    }
    return n;
}

/* in Bytes, minus the space taken by tracks that will be overwritten
 * during copying. If != NULL, @num will be filled with the number of
 * non-transferred tracks */
double get_filesize_of_nontransferred_tracks(guint32 *num)
{
    double size = 0;
    guint32 n = 0;
    Track *track;
    GList *gl_track;


    for (gl_track = tracks; gl_track; gl_track=gl_track->next)
    {
	track = (Track *)gl_track->data;
	if (!track->transferred)
	{
	    size += track->size - track->oldsize;
	    ++n;
	}
    }
    if (num) *num = n;
    return size;
}


/* Returns the n_th track */
Track *get_track_by_nr (guint32 n)
{
  return g_list_nth_data (tracks, n);
}

/* Gets the next track (i=1) or the first track (i=0)
   This function is optimized for speed, caching a pointer to the last
   link returned.
   Make sure that the list of tracks does not get changed during
   ceonsecutive calls with i=1 or this function might crash. */
Track *get_next_track (gint i)
{
    static GList *lastlink = NULL;
    Track *result;

    if (i==0)           lastlink = tracks;
    else if (lastlink)  lastlink = lastlink->next;

    if (lastlink)       result = (Track *)lastlink->data;
    else                result = NULL;

    return result;
}	


/* Returns the track with ID "id". We need to get the last occurence of
 * "id" in case we're currently importing the iTunesDB. In that case
 * there might be duplicate ids */
Track *get_track_by_id (guint32 id)
{
  GList *l;
  Track *s;

  for (l=g_list_last(tracks); l; l=l->prev)
  {
      s = (Track *)l->data;
      if (s->ipod_id == id)  return s;
  }
  return NULL;
}


/* Returns the track with the local filename @name or NULL, if none can
 * be found. */
Track *get_track_by_filename (gchar *name)
{
  GList *l;

  if (!name) return NULL;

  for (l=tracks; l; l=l->next)
  {
      Track *track = (Track *)l->data;
      if (track->pc_path_locale)
	  if (strcmp (track->pc_path_locale, name) == 0) return track;
  }
  return NULL;
}


/* Check if @track is (still) in the track list
 *
 * Return TRUE if present
 */
gboolean track_is_valid (Track *track)
{
    if (g_list_find (tracks, track)) return TRUE;
    else                           return FALSE;
}


/* renumber ipod ids (used from withing handle_import() */
void renumber_ipod_ids ()
{
    GList *gl_track;

    /* We simply re-ID all tracks. That way we don't run into
       trouble if some "funny" guy put a "-1" into the itunesDB */
    reset_ipod_id (); /* reset lowest unused ipod ID */
    for (gl_track = tracks; gl_track; gl_track=gl_track->next)
    {
	((Track *)gl_track->data)->ipod_id = free_ipod_id (0);
	/*we need to tell the display that the ID has changed (if ID
	 * is displayed) */
	/* pm_track_changed (track);*/
    }
}

/* used by free_ipod_id() and reset_ipod_id() */
static guint32 get_id (guint32 id, gboolean reset)
{
  static gint32 max_id = 52; /* the lowest valid ID is 53 (itunes,
			      * MusicMatch: 2 */

  if (reset)
  { /* reset ipod IDs */
      max_id = 52;
  }
  else
  { /* register ID */
      if (id > max_id)     max_id = id;
      if (id == 0)   ++max_id;
  }
  /* return lowest free ID (discarded if reset == TRUE) */
  return max_id;
}



/*  Call with 0 to get a unused ID. It will be registered.
    Call with anything else to register a used ID. The highest ID use
    will then be returned */
/* Called each time by add_track with the ipod_id used.
   That's how it keeps track of the largest ID used. */
static guint32 free_ipod_id (guint32 id)
{
    return get_id (id, FALSE);
}

/* reset lowest free ipod ID */
static void reset_ipod_id (void)
{
    get_id (0, TRUE);
}

/* ------------------------------------------------------------ *\
|                                                                |
|         functions for md5 checksums                            |
|                                                                |
\* ------------------------------------------------------------ */

/**
 * Register all tracks in the md5 hash and remove duplicates (while
 * preserving playlists)
 */
void hash_tracks(void)
{
   gint ns, count, track_nr;
   gchar *buf;
   Track *track, *oldtrack;

   if (!prefs_get_md5tracks ()) return;
   ns = get_nr_of_tracks ();
   if (ns == 0)                 return;

   block_widgets (); /* block widgets -- this might take a while,
			so we'll do refreshs */
   md5_unique_file_free (); /* release md5 hash */
   count = 0;
   track_nr = 0;
   /* populate the hash table */
   while ((track = get_track_by_nr (track_nr)))
   {
       oldtrack = md5_track_exists_insert(track);
       ++count;
       if (((count % 20) == 1) || (count == ns))
       { /* update for count == 1, 21, 41 ... and for count == n */
	   buf = g_strdup_printf (ngettext ("Hashed %d of %d track.",
					    "Hashed %d of %d tracks.", ns),
				  count, ns);
	   gtkpod_statusbar_message(buf);
	   while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
	   g_free (buf);
       }
       if (oldtrack)
       {
	   remove_duplicate (oldtrack, track);
       }
       else
       { /* if we removed a track (above), we don't need to increment
	    the track_nr here */
	   ++track_nr;
       }
   }
   remove_duplicate (NULL, NULL); /* show info dialogue */
   release_widgets (); /* release widgets again */
}


/* This function removes a duplicate track "track" from memory while
 * preserving the playlists.
 *
 * The md5 hash is not modified.  
 *
 * The playcount/recent_playcount are modified to show the cumulative
 * playcounts for that track.
 *
 * The star rating is set to the average of both star ratings if both
 * ratings are not 0, or the higher rating if one of the ratings is 0
 * (it is assumed that a rating of "0" means that no rating has been
 * set).
 *
 * The "created" timestamp is set to the older entry (unless that one
 * is 0).
 *
 * The "modified" and "last played" timestamps are set to the more
 * recent entry.
 *
 * You should call "remove_duplicate (NULL, NULL)" to pop up the info
 * dialogue with the list of duplicate tracks afterwards. Call with
 * "NULL, (void *)-1" to just clean up without dialoge.
 *
 * If "track" does not exist in
 * the master play list, only a message is logged (to be displayed
 * later when called with "NULL, NULL" */
void remove_duplicate (Track *oldtrack, Track *track)
{
   gchar *buf, *buf2;
   static gint deltrack_nr = 0;
   static gboolean removed = FALSE;
   static GString *str = NULL;

   if (prefs_get_show_duplicates() && !oldtrack && !track && str)
   {
       if (str->len)
       { /* Some tracks have been deleted. Print a notice */
	   if (removed)
	   {	       
	       buf = g_strdup_printf (
		   ngettext ("The following duplicate track has been removed.",
			     "The following %d duplicate tracks have been removed.",
			     deltrack_nr), deltrack_nr);
	   }
	   else
	   {
	       buf = g_strdup_printf (
		   ngettext ("The following duplicate track has not been added to the master play list.",
			     "The following %d duplicate tracks have not been added to the master play list.",
			     deltrack_nr), deltrack_nr);
	   }
	   gtkpod_confirmation
	       (-1,                      /* gint id, */
		FALSE,                   /* gboolean modal, */
		_("Duplicate detection"),/* title */
		buf,                     /* label */
		str->str,                /* scrolled text */
		NULL, 0, NULL,      /* option 1 */
		NULL, 0, NULL,      /* option 2 */
		TRUE,               /* gboolean confirm_again, */
		NULL,               /* ConfHandlerCA confirm_again_handler,*/
		NULL,               /* ConfHandler ok_handler,*/
		CONF_NO_BUTTON,     /* don't show "Apply" button */
		CONF_NO_BUTTON,     /* cancel_handler,*/
		NULL,               /* gpointer user_data1,*/
		NULL);              /* gpointer user_data2,*/
	   g_free (buf);
       }
   }
   if (oldtrack == NULL)
   { /* clean up */
       if (str)       g_string_free (str, TRUE);
       str = NULL;
       removed = FALSE;
       deltrack_nr = 0;
       gtkpod_tracks_statusbar_update();
   }
   if (prefs_get_show_duplicates() && oldtrack && track)
   {
       /* add info about it to str */
       buf = get_track_info (track);
       buf2 = get_track_info (oldtrack);
       if (!str)
       {
	   deltrack_nr = 0;
	   str = g_string_sized_new (2000); /* used to keep record of
					     * duplicate tracks */
       }
       g_string_append_printf (str, "'%s': identical to '%s'\n",
			       buf, buf2);
       g_free (buf);
       g_free (buf2);
       /* Set playcount */
       oldtrack->playcount += track->playcount;
       oldtrack->recent_playcount += track->recent_playcount;
       /* Set rating */
       if (oldtrack->rating && track->rating)
	   oldtrack->rating =
	       floor((double)(oldtrack->rating + track->rating + RATING_STEP) /
		     (2 * RATING_STEP)) * RATING_STEP;
       else
	   oldtrack->rating = MAX (oldtrack->rating, track->rating);
       /* Set 'modified' timestamp */
       oldtrack->time_modified =  MAX (oldtrack->time_modified,
				      track->time_modified);
       /* Set 'played' timestamp */
       oldtrack->time_played =  MAX (oldtrack->time_played, track->time_played);

       /* Update filename if new track has filename set */
       if (track->pc_path_locale && *oldtrack->pc_path_locale)
       {
	   g_free (oldtrack->pc_path_locale);
	   g_free (oldtrack->pc_path_utf8);
	   oldtrack->pc_path_utf8 = g_strdup (track->pc_path_utf8);
	   oldtrack->pc_path_locale = g_strdup (track->pc_path_locale);
       }
       if (track_is_in_playlist (NULL, track))
       { /* track is already added to memory -> replace with "oldtrack" */
	   /* check for "track" in all playlists (except for MPL) */
	   gint np = get_nr_of_playlists ();
	   gint pl_nr;
	   for (pl_nr=1; pl_nr<np; ++pl_nr)
	   {
	       Playlist *pl = get_playlist_by_nr (pl_nr);
	       /* if "track" is in playlist pl, we remove it and add
		  the "oldtrack" instead (this way round we don't have
		  to worry about changing md5 hash entries */
	       if (remove_track_from_playlist (pl, track))
	       {
		   if (!track_is_in_playlist (pl, oldtrack))
		       add_track_to_playlist (pl, oldtrack, TRUE);
	       }
	   }
	   /* remove track from MPL, i.e. from the ipod */
	   remove_track_from_playlist (NULL, track);
	   removed = TRUE;
       }
       ++deltrack_nr; /* count duplicate tracks */
   }
}


/* delete all md5 checkums from the tracks */
void clear_md5_hash_from_tracks (void)
{
    GList *l;

    for (l = tracks; l; l = l->next)
    {
	C_FREE (((Track *)l->data)->md5_hash);
    }
}


/* return the address of the UTF8 field @t_item. @t_item is one of
 * (the applicable) T_* defined in track.h */
gchar **track_get_item_pointer_utf8 (Track *track, T_item t_item)
{
    gchar **result = NULL;

    if (track) switch (t_item)
    {
    case T_ALBUM:
	result = &track->album;
	break;
    case T_ARTIST:
	result = &track->artist;
	break;
    case T_TITLE:
	result = &track->title;
	break;
    case T_GENRE:
	result = &track->genre;
	break;
    case T_COMMENT:
	result = &track->comment;
	break;
    case T_COMPOSER:
	result = &track->composer;
	break;
    case T_FDESC:
	result = &track->fdesc;
	break;
    case T_IPOD_PATH:
	result = &track->ipod_path;
	break;
    case T_YEAR:
	result = &track->year_str;
	break;
    default:
	result = NULL;
    }
    return result;
}

/* return the UTF8 item @t_item. @t_item is one of
   (the applicable) T_* defined in track.h */
gchar *track_get_item_utf8 (Track *track, T_item t_item)
{
    gchar **address = track_get_item_pointer_utf8 (track, t_item);

    if (address) return *address;
    else         return NULL;
}


/* return the address of the UTF16 field @t_item. @t_item is one of
(the * applicable) T_* defined in track.h */
gunichar2 **track_get_item_pointer_utf16 (Track *track, T_item t_item)
{
    gunichar2 **result = NULL;

    if (track) switch (t_item)
    {
    case T_ALBUM:
	result = &track->album_utf16;
	break;
    case T_ARTIST:
	result = &track->artist_utf16;
	break;
    case T_TITLE:
	result = &track->title_utf16;
	break;
    case T_GENRE:
	result = &track->genre_utf16;
	break;
    case T_COMMENT:
	result = &track->comment_utf16;
	break;
    case T_COMPOSER:
	result = &track->composer_utf16;
	break;
    case T_FDESC:
	result = &track->fdesc_utf16;
	break;
    case T_IPOD_PATH:
	result = &track->ipod_path_utf16;
	break;
    default:
	result = NULL;
    }
    return result;
}

/* return the UTF16 item @t_item. @t_item is one of
(the * applicable) T_* defined in track.h */
gunichar2 *track_get_item_utf16 (Track *track, T_item t_item)
{
    gunichar2 **address = track_get_item_pointer_utf16 (track, t_item);

    if (address) return *address;
    else         return NULL;
}


/* return a pointer to the specified timestamp. @t_item is one of (the
   applicable) T_* defined in track.h.  If the parameters are illegal,
   "0" is returned. */
guint32 *track_get_timestamp_ptr (Track *track, T_item t_item)
{
    if (track)
    {
	switch (t_item)
	{
	  case T_TIME_PLAYED:
	    return &track->time_played;
	  case T_TIME_MODIFIED:
	    return &track->time_modified;
	default:
	    break;
	}
    }
    return NULL;
}


/* return the specified timestamp. @t_item is one of
   (the * applicable) T_* defined in track.h. If the parameters are
   illegal, "0" is returned. */
guint32 track_get_timestamp (Track *track, T_item t_item)
{
    guint32 *ptr = track_get_timestamp_ptr (track, t_item);
    if (ptr)  return *ptr;
    else      return 0;
}


/* ------------------------------------------------------------------- */
/* functions used by itunesdb (so we can refresh the display during
 * import */
gboolean it_add_track (Track *track)
{
    static gint count = 0;
    Track *result;

    /* fix timestamp (up to V0.51 I used a dummy timestamp -- replace
       that with the current time) */
    if (track && (track->time_modified == 0x8c3abf9b))
	track->time_modified = 0;

    /* fix bitrate (up to V0.51 I used bps instead of kbps) */
    if (track && (track->bitrate >= 10000))
	track->bitrate /= 1000;

    result = add_track (track);

    ++count;
    if ((count % 20) == 0)     gtkpod_tracks_statusbar_update();

    while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();

    if (result) return TRUE;
    else        return FALSE;
}

Track *it_get_track_by_nr (guint32 n)
{
    Track *track = get_track_by_nr (n);
    while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
    return track;
}
