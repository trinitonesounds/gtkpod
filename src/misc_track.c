/* Time-stamp: <2006-05-13 01:27:04 jcs>
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "display_itdb.h"
#include "md5.h"
#include "prefs.h"
#include "misc.h"
#include "misc_track.h"
#include "info.h"
#include "charset.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>


/* ------------------------------------------------------------ *\
|                                                                |
|         functions for md5 checksums                            |
|                                                                |
\* ------------------------------------------------------------ */

/**
 * Register all tracks in the md5 hash and remove duplicates (while
 * preserving playlists)
 */
void gp_md5_hash_tracks_itdb (iTunesDB *itdb)
{
   gint ns, count;
   gchar *buf;
   GList *gl;

   g_return_if_fail (itdb);

   if (!prefs_get_md5tracks ()) return;
   ns = itdb_tracks_number (itdb);   /* number of tracks */
   if (ns == 0)                 return;

   block_widgets (); /* block widgets -- this might take a while,
			so we'll do refreshs */
   md5_free (itdb);  /* release md5 hash */
   count = 0;
   /* populate the hash table */
   gl=itdb->tracks;
   while (gl)
   {
       Track *track=gl->data;
       Track *oldtrack = md5_track_exists_insert (itdb, track);

       /* need to get next track now because it might be a duplicate and
	  thus be removed when we call gp_duplicate_remove() */
       gl = gl->next;

       if (oldtrack)
       {
	   gp_duplicate_remove (oldtrack, track);
       }

       ++count;
       if (((count % 20) == 1) || (count == ns))
       { /* update for count == 1, 21, 41 ... and for count == n */
	   buf = g_strdup_printf (ngettext ("Hashed %d of %d track.",
					    "Hashed %d of %d tracks.", ns),
				  count, ns);
	   gtkpod_statusbar_message (buf);
	   while (widgets_blocked && gtk_events_pending ())
	       gtk_main_iteration ();
	   g_free (buf);
       }
   }
   gp_duplicate_remove (NULL, NULL); /* show info dialogue */
   release_widgets (); /* release widgets again */
}


/**
 * Call gp_hash_tracks_itdb() for each itdb.
 *
 */
void gp_md5_hash_tracks (void)
{
    GList *gl;
    struct itdbs_head *itdbs_head;

    g_return_if_fail (gtkpod_window);
    itdbs_head = g_object_get_data (G_OBJECT (gtkpod_window),
				    "itdbs_head");
    g_return_if_fail (itdbs_head);

    block_widgets ();
    for (gl=itdbs_head->itdbs; gl; gl=gl->next)
    {
	gp_md5_hash_tracks_itdb (gl->data);
    }
    release_widgets ();
}


/**
 * Call md5_free() for each itdb and delete md5 checksums in all tracks.
 *
 */
void gp_md5_free_hash (void)
{
    void rm_md5 (gpointer track, gpointer user_data)
	{
	    ExtraTrackData *etr;
	    g_return_if_fail (track);
	    etr = ((Track *)track)->userdata;
	    g_return_if_fail (etr);
	    C_FREE (etr->md5_hash);
	}
    GList *gl;
    struct itdbs_head *itdbs_head;

    g_return_if_fail (gtkpod_window);
    itdbs_head = g_object_get_data (G_OBJECT (gtkpod_window),
				    "itdbs_head");
    g_return_if_fail (itdbs_head);

    for (gl=itdbs_head->itdbs; gl; gl=gl->next)
    {
	iTunesDB *itdb = gl->data;
	g_return_if_fail (itdb);
	md5_free (itdb);
	g_list_foreach (itdb->tracks, rm_md5, NULL);
    }
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
 * The "added" timestamp is set to the older entry (unless that one
 * is 0).
 *
 * The "modified" and "last played" timestamps are set to the more
 * recent entry.
 *
 * You should call "gp_duplicate_remove (NULL, NULL)" to pop up the info
 * dialogue with the list of duplicate tracks afterwards. Call with
 * "NULL, (void *)-1" to just clean up without dialoge.
 *
 * If "track" does not exist in
 * the master play list, only a message is logged (to be displayed
 * later when called with "NULL, NULL" */
void gp_duplicate_remove (Track *oldtrack, Track *track)
{
   gchar *buf, *buf2;
   static gint deltrack_nr = 0;
   static gboolean removed = FALSE;
   static GString *str = NULL;

/*   printf ("%p, %p, '%s'\n", oldtrack, track, str?str->str:"empty");*/

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
		prefs_set_show_duplicates,
		                    /* ConfHandlerCA confirm_again_handler,*/
		CONF_NULL_HANDLER,  /* ConfHandler ok_handler,*/
		NULL,               /* don't show "Apply" button */
		NULL,               /* don't show "Cancel" button */
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

   if (oldtrack && track)
   {
       ExtraTrackData *oldetr = oldtrack->userdata;
       ExtraTrackData *etr = track->userdata;
       iTunesDB *itdb = oldtrack->itdb;
       g_return_if_fail (itdb);
       g_return_if_fail (oldetr);
       g_return_if_fail (etr);

       if (prefs_get_show_duplicates ())
       {
	   /* add info about it to str */
	   buf = get_track_info (track, TRUE);
	   buf2 = get_track_info (oldtrack, TRUE);
	   if (!str)
	   {
	       deltrack_nr = 0;
	       str = g_string_sized_new (2000); /* used to keep record
						 * of duplicate
						 * tracks */
	   }
	   g_string_append_printf (str, "'%s': identical to '%s'\n",
				   buf, buf2);
	   g_free (buf);
	   g_free (buf2);
       }
       /* Set playcount */
       oldtrack->playcount += track->playcount;
       oldtrack->recent_playcount += track->recent_playcount;
       /* Set rating */
       if (oldtrack->rating && track->rating)
	   oldtrack->rating =
	       floor((double)(oldtrack->rating + track->rating + ITDB_RATING_STEP) /
		     (2 * ITDB_RATING_STEP)) * ITDB_RATING_STEP;
       else
	   oldtrack->rating = MAX (oldtrack->rating, track->rating);
       /* Set 'modified' timestamp */
       oldtrack->time_modified =  MAX (oldtrack->time_modified,
				      track->time_modified);
       /* Set 'played' timestamp */
       oldtrack->time_played =  MAX (oldtrack->time_played, track->time_played);
       /* Set 'added' timestamp */
       oldtrack->time_added =  MIN (oldtrack->time_added, track->time_added);

       /* Update filename if new track has filename set (should be
	* always!?) */
       if (etr->pc_path_locale)
       {
	   g_free (oldetr->pc_path_locale);
	   g_free (oldetr->pc_path_utf8);
	   oldetr->pc_path_locale = g_strdup (etr->pc_path_locale);
	   oldetr->pc_path_utf8 = g_strdup (etr->pc_path_utf8);
       }
       if (itdb_playlist_contains_track (itdb_playlist_mpl (itdb),
					 track))
       { /* track is already added to memory -> replace with "oldtrack" */
	   /* check for "track" in all playlists (except for MPL) */
	   GList *gl;
	   gl = g_list_nth (itdb->playlists, 1);
	   while (gl)
	   {
	       Playlist *pl = gl->data;
	       g_return_if_fail (pl);
	       /* if "track" is in playlist pl, we remove it and add
		  the "oldtrack" instead (this way round we don't have
		  to worry about changing md5 hash entries */
	       if (itdb_playlist_contains_track (pl, track))
	       {
		   gp_playlist_remove_track (pl, track,
					     DELETE_ACTION_PLAYLIST);
		   if (!itdb_playlist_contains_track (pl, oldtrack))
		       gp_playlist_add_track (pl, oldtrack, TRUE);
	       }
	       gl = gl->next;
	   }
	   /* remove track from MPL, i.e. from the ipod (or the local
	    * database */
	   if (itdb->usertype & GP_ITDB_TYPE_IPOD)
	   {
	       gp_playlist_remove_track (NULL, track, DELETE_ACTION_IPOD);
	   }
	   if (itdb->usertype & GP_ITDB_TYPE_LOCAL)
	   {
	       gp_playlist_remove_track (NULL, track,
					 DELETE_ACTION_DATABASE);
	   }
	   removed = TRUE;
       }
       ++deltrack_nr; /* count duplicate tracks */
       data_changed (itdb);
   }
}


/**
 * Register all tracks in the md5 hash and remove duplicates (while
 * preserving playlists).
 * Call  remove_duplicate (NULL, NULL); to show an info dialogue
 */
void gp_itdb_hash (iTunesDB *itdb)
{
   gint ns, count, track_nr;
   gchar *buf;
   Track *track, *oldtrack;

   g_return_if_fail (itdb);

   if (!prefs_get_md5tracks ()) return;

   ns = itdb_tracks_number (itdb);
   if (ns == 0)                 return;

   block_widgets (); /* block widgets -- this might take a while,
			so we'll do refreshs */
   md5_free (itdb);  /* release md5 hash */
   count = 0;
   track_nr = 0;
   /* populate the hash table */
   while ((track = g_list_nth_data (itdb->tracks, track_nr)))
   {
       oldtrack = md5_track_exists_insert (itdb, track);
       ++count;
/*        printf("%d:%d:%p:%p\n", count, track_nr, track, oldtrack); */
       if (!prefs_get_block_display() &&
	   (((count % 20) == 1) || (count == ns)))
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
	   gp_duplicate_remove (oldtrack, track);
       }
       else
       { /* if we removed a track (above), we don't need to increment
	    the track_nr here */
	   ++track_nr;
       }
   }
   release_widgets (); /* release widgets again */
}




/* ------------------------------------------------------------ *\
|                                                                |
|         functions to locate tracks                             |
|                                                                |
\* ------------------------------------------------------------ */


/* Returns the track with the filename @name or NULL, if none can be
 * found. This function also works if @name is on the iPod. */
Track *gp_track_by_filename (iTunesDB *itdb, gchar *filename)
{
    gchar *musicdir = NULL;
    Track *result = NULL;

    g_return_val_if_fail (itdb, NULL);
    g_return_val_if_fail (filename, NULL);

    if (itdb->usertype & GP_ITDB_TYPE_IPOD)
    {
	gchar *mountpoint = get_itdb_prefs_string (itdb, "mountpoint");
	g_return_val_if_fail (mountpoint, NULL);
	musicdir = itdb_get_music_dir (mountpoint);
	g_free (mountpoint);
    }

    if ((itdb->usertype & GP_ITDB_TYPE_IPOD) &&
	(strncmp (filename, musicdir, strlen (musicdir)) == 0))
    {   /* handle track on iPod (in music dir) */
	GList *gl;
	for (gl=itdb->tracks; gl&&!result; gl=gl->next)
	{
	    Track *track = gl->data;
	    gchar *ipod_path;
	    g_return_val_if_fail (track, NULL);
	    ipod_path = itdb_filename_on_ipod (track);
	    if (ipod_path)
	    {
		if (strcmp (ipod_path, filename) == 0)
		{
		    result = track;
		}
		g_free (ipod_path);
	    }
	}
    }
    else
    {   /* handle track on local filesystem */
	GList *gl;
	for (gl=itdb->tracks; gl; gl=gl->next)
	{
	    Track *track = gl->data;
	    ExtraTrackData *etr;
	    g_return_val_if_fail (track, NULL);
	    etr = track->userdata;
	    g_return_val_if_fail (etr, NULL);
	    if (etr->pc_path_locale)
	    {
		if (strcmp (etr->pc_path_locale, filename) == 0)
		    result = track;
	    }
	}
    }
    return result;
}



/* ------------------------------------------------------------ *\
|                                                                |
|         functions to retrieve information from tracks          |
|                                                                |
\* ------------------------------------------------------------ */

/* return the address of the UTF8 field @t_item. @t_item is one of
 * (the applicable) T_* defined in track.h */
gchar **track_get_item_pointer (Track *track, T_item t_item)
{
    gchar **result = NULL;
    ExtraTrackData *etr;

    g_return_val_if_fail (track, NULL);
    etr = track->userdata;
    g_return_val_if_fail (etr, NULL);

    switch (t_item)
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
    case T_FILETYPE:
	result = &track->filetype;
	break;
    case T_IPOD_PATH:
	result = &track->ipod_path;
	break;
    case T_PC_PATH:
	result = &etr->pc_path_utf8;
	break;
    case T_YEAR:
	result = &etr->year_str;
	break;
    case T_GROUPING:
	result = &track->grouping;
	break;
    case T_CATEGORY:
	result = &track->category;
	break;
    case T_DESCRIPTION:
	result = &track->description;
	break;
    case T_PODCASTURL:
	result = &track->podcasturl;
	break;
    case T_PODCASTRSS:
	result = &track->podcastrss;
	break;
    case T_SUBTITLE:
	result = &track->subtitle;
	break;
    case T_ALL:
    case T_IPOD_ID:
    case T_TRACK_NR:
    case T_TRANSFERRED:
    case T_SIZE:
    case T_TRACKLEN:
    case T_BITRATE:
    case T_SAMPLERATE:
    case T_BPM:
    case T_PLAYCOUNT:
    case T_RATING:
    case T_TIME_ADDED:
    case T_TIME_PLAYED:
    case T_TIME_MODIFIED:
    case T_TIME_RELEASED:
    case T_VOLUME:
    case T_SOUNDCHECK:
    case T_CD_NR:
    case T_COMPILATION:
    case T_CHECKED:
    case T_ITEM_NUM:
	g_return_val_if_reached (NULL);
    }
    return result;
}







/* return the UTF8 item @t_item. @t_item is one of
   (the applicable) T_* defined in track.h */
const gchar *track_get_item (Track *track, T_item t_item)
{
    gchar **ptr;

    g_return_val_if_fail (track, NULL);

    ptr = track_get_item_pointer (track, t_item);

    if (ptr)     return *ptr;
    else         return NULL;
}


/* Copy item @item from @frtrack to @totrack.
   Return value:
     TRUE: @totrack was changed
     FALSE: @totrack is unchanged
*/
gboolean track_copy_item (Track *frtrack, Track *totrack, T_item item)
{
    gboolean changed = FALSE;
    const gchar *fritem;
    gchar **toitem_ptr;

    g_return_val_if_fail (frtrack, FALSE);
    g_return_val_if_fail (totrack, FALSE);
    g_return_val_if_fail ((item > 0) && (item < T_ITEM_NUM), FALSE);

    if (frtrack == totrack) return FALSE;

    switch (item)
    {
    case T_ALBUM:
    case T_ARTIST:
    case T_TITLE:
    case T_GENRE:
    case T_COMMENT:
    case T_COMPOSER:
    case T_FILETYPE:
    case T_IPOD_PATH:
    case T_PC_PATH:
    case T_YEAR:
    case T_GROUPING:
    case T_CATEGORY:
    case T_DESCRIPTION:
    case T_PODCASTURL:
    case T_PODCASTRSS:
    case T_SUBTITLE:
	fritem = track_get_item (frtrack, item);
	toitem_ptr = track_get_item_pointer (totrack, item);
	g_return_val_if_fail (fritem, FALSE);
	g_return_val_if_fail (toitem_ptr, FALSE);
	if ((*toitem_ptr == NULL) || (strcmp (fritem, *toitem_ptr) != 0))
	{
	    g_free (*toitem_ptr);
	    *toitem_ptr = g_strdup (fritem);
	    changed = TRUE;
	}
	break;
    case T_IPOD_ID:
	if (frtrack->id != totrack->id)
	{
	    totrack->id = frtrack->id;
	    changed = TRUE;
	}
	break;
    case T_TRACK_NR:
	if (frtrack->track_nr != totrack->track_nr)
	{
	    totrack->track_nr = frtrack->track_nr;
	    changed = TRUE;
	}
	if (frtrack->tracks != totrack->tracks)
	{
	    totrack->tracks = frtrack->tracks;
	    changed = TRUE;
	}
	break;
    case T_TRANSFERRED:
	if (frtrack->transferred != totrack->transferred)
	{
	    totrack->transferred = frtrack->transferred;
	    changed = TRUE;
	}
	break;
    case T_SIZE:
	if (frtrack->size != totrack->size)
	{
	    totrack->size = frtrack->size;
	    changed = TRUE;
	}
	break;
    case T_TRACKLEN:
	if (frtrack->tracklen != totrack->tracklen)
	{
	    totrack->tracklen = frtrack->tracklen;
	    changed = TRUE;
	}
	break;
    case T_BITRATE:
	if (frtrack->bitrate != totrack->bitrate)
	{
	    totrack->bitrate = frtrack->bitrate;
	    changed = TRUE;
	}
	break;
    case T_SAMPLERATE:
	if (frtrack->samplerate != totrack->samplerate)
	{
	    totrack->samplerate = frtrack->samplerate;
	    changed = TRUE;
	}
	break;
    case T_BPM:
	if (frtrack->BPM != totrack->BPM)
	{
	    totrack->BPM = frtrack->BPM;
	    changed = TRUE;
	}
	break;
    case T_PLAYCOUNT:
	if (frtrack->playcount != totrack->playcount)
	{
	    totrack->playcount = frtrack->playcount;
	    changed = TRUE;
	}
	break;
    case T_RATING:
	if (frtrack->rating != totrack->rating)
	{
	    totrack->rating = frtrack->rating;
	    changed = TRUE;
	}
	break;
    case T_TIME_ADDED:
    case T_TIME_PLAYED:
    case T_TIME_MODIFIED:
    case T_TIME_RELEASED:
	if (time_get_time (frtrack, item) !=
	    time_get_time (totrack, item))
	{
	    time_set_time (totrack, time_get_time (frtrack, item), item);
	    changed = TRUE;
	}
	break;
    case T_VOLUME:
	if (frtrack->volume != totrack->volume)
	{
	    totrack->volume = frtrack->volume;
	    changed = TRUE;
	}
	break;
    case T_SOUNDCHECK:
	if (frtrack->soundcheck != totrack->soundcheck)
	{
	    totrack->soundcheck = frtrack->soundcheck;
	    changed = TRUE;
	}
	break;
    case T_CD_NR:
	if (frtrack->cd_nr != totrack->cd_nr)
	{
	    totrack->cd_nr = frtrack->cd_nr;
	    changed = TRUE;
	}
	if (frtrack->cds != totrack->cds)
	{
	    totrack->cds = frtrack->cds;
	    changed = TRUE;
	}
	break;
    case T_COMPILATION:
	if (frtrack->compilation != totrack->compilation)
	{
	    totrack->compilation = frtrack->compilation;
	    changed = TRUE;
	}
	break;
    case T_CHECKED:
	if (frtrack->checked != totrack->checked)
	{
	    totrack->checked = frtrack->checked;
	    changed = TRUE;
	}
	break;
    case T_ITEM_NUM:
    case T_ALL:
	g_return_val_if_reached (FALSE);

    }	
    return changed;
}


/* return a pointer to the specified timestamp. @t_item is one of (the
   applicable) T_* defined in track.h.  If the parameters are illegal,
   "0" is returned. */
guint32 *track_get_timestamp_ptr (Track *track, T_item t_item)
{
    g_return_val_if_fail (track, NULL);

    switch (t_item)
    {
    case T_TIME_PLAYED:
	return &track->time_played;
    case T_TIME_MODIFIED:
	return &track->time_modified;
    case T_TIME_RELEASED:
	return &track->time_released;
    case T_TIME_ADDED:
	return &track->time_added;
    default:
	g_return_val_if_reached (0);
    }
}


/* return the specified timestamp. @t_item is one of
   (the * applicable) T_* defined in track.h. If the parameters are
   illegal, "0" is returned. */
guint32 track_get_timestamp (Track *track, T_item t_item)
{
    guint32 *ptr;
    g_return_val_if_fail (track, 0);

    ptr = track_get_timestamp_ptr (track, t_item);
    if (ptr)  return *ptr;
    else      return 0;
}


/* Return text for display. g_free() after use. */
gchar *track_get_text (Track *track, T_item item)
{
    gchar *text = NULL;
    ExtraTrackData *etr;
    iTunesDB *itdb;

    g_return_val_if_fail ((item > 0) && (item < T_ITEM_NUM), NULL);
    g_return_val_if_fail (track, NULL);
    etr = track->userdata;
    g_return_val_if_fail (etr, NULL);
    itdb = track->itdb;
    g_return_val_if_fail (itdb, NULL);

    switch (item)
    {
    case T_TITLE:
    case T_ARTIST:
    case T_ALBUM:
    case T_GENRE:
    case T_COMPOSER:
    case T_COMMENT:
    case T_FILETYPE:
    case T_GROUPING:
    case T_CATEGORY:
    case T_DESCRIPTION:
    case T_PODCASTURL:
    case T_PODCASTRSS:
    case T_SUBTITLE:
	text = g_strdup (track_get_item (track, item));
	break;
    case T_TRACK_NR:
	if (track->tracks == 0)
	    text = g_strdup_printf ("%d", track->track_nr);
	else
	    text = g_strdup_printf ("%d/%d",
				    track->track_nr, track->tracks);
	break;
    case T_CD_NR:
	if (track->cds == 0)
	    text = g_strdup_printf ("%d", track->cd_nr);
	else
	    text = g_strdup_printf ("%d/%d", track->cd_nr, track->cds);
	break;
    case T_IPOD_ID:
	if (track->id != -1)
	    text = g_strdup_printf ("%d", track->id);
	else
	    text = g_strdup ("--");
	break;
    case T_PC_PATH:
	text = g_strdup (etr->pc_path_utf8);
	break;
    case T_IPOD_PATH:
	if (itdb->usertype & GP_ITDB_TYPE_IPOD)
	{
	    text = g_strdup (track->ipod_path);
	}
	if (itdb->usertype & GP_ITDB_TYPE_LOCAL)
	{
	    text = g_strdup (_("Local Database"));
	}
	break;
    case T_SIZE:
	text = g_strdup_printf ("%d", track->size);
	break;
    case T_TRACKLEN:
	text = g_strdup_printf ("%d:%02d",
				track->tracklen/60000,
				(track->tracklen/1000)%60);
	break;
    case T_BITRATE:
	text = g_strdup_printf ("%dk", track->bitrate);
	break;
    case T_SAMPLERATE:
	text = g_strdup_printf ("%d", track->samplerate);
	break;
    case T_BPM:
	text = g_strdup_printf ("%d", track->BPM);
	break;
    case T_PLAYCOUNT:
	text = g_strdup_printf ("%d", track->playcount);
	break;
    case T_YEAR:
	text = g_strdup_printf ("%d", track->year);
	break;
    case T_RATING:
	text = g_strdup_printf ("%d", track->rating/ITDB_RATING_STEP);
	break;
    case T_TIME_PLAYED:
    case T_TIME_MODIFIED:
    case T_TIME_ADDED:
    case T_TIME_RELEASED:
	text = time_field_to_string (track, item);
	break;
    case T_VOLUME:
	text = g_strdup_printf ("%d", track->volume);
	break;
    case T_SOUNDCHECK:
	text = g_strdup_printf ("%0.2f", soundcheck_to_replaygain (track->soundcheck));
	break;
    case T_TRANSFERRED:
    case T_COMPILATION:
    case T_ALL:
    case T_CHECKED:
    case T_ITEM_NUM:
	break;
    }
    return text;
}



/* Set track data according to @new_text

   Return value: TRUE, if the track data was modified, FALSE otherwise
*/
gboolean track_set_text (Track *track, const gchar *new_text, T_item item)
{
    gboolean changed = FALSE;
    gchar **itemp_utf8;
    const gchar *str;
    ExtraTrackData *etr;
    gint32 nr;
    time_t t;

    g_return_val_if_fail (track, FALSE);
    g_return_val_if_fail (new_text, FALSE);

    etr = track->userdata;
    g_return_val_if_fail (etr, FALSE);


    switch(item)
    {
    case T_TITLE:
    case T_ALBUM:
    case T_ARTIST:
    case T_GENRE:
    case T_COMPOSER:
    case T_COMMENT:
    case T_FILETYPE:
    case T_GROUPING:
    case T_CATEGORY:
    case T_DESCRIPTION:
    case T_PODCASTURL:
    case T_PODCASTRSS:
    case T_SUBTITLE:
        itemp_utf8 = track_get_item_pointer (track, item);
        if (g_utf8_collate (*itemp_utf8, new_text) != 0)
        {
	    g_free (*itemp_utf8);
	    *itemp_utf8 = g_strdup (new_text);
	    changed = TRUE;
        }
        break;
    case T_TRACK_NR:
        nr = atoi (new_text);
        if ((nr >= 0) && (nr != track->track_nr))
        {
	    track->track_nr = nr;
	    changed = TRUE;
        }
	str = strrchr (new_text, '/');
	if (str)
	{
	    nr = atoi (str+1);
	    if ((nr >= 0) && (nr != track->tracks))
	    {
		track->tracks = nr;
		changed = TRUE;
	    }
	}
        break;
    case T_CD_NR:
        nr = atoi (new_text);
        if ((nr >= 0) && (nr != track->cd_nr))
        {
	    track->cd_nr = nr;
	    changed = TRUE;
        }
	str = strrchr (new_text, '/');
	if (str)
	{
	    nr = atoi (str+1);
	    if ((nr >= 0) && (nr != track->cds))
	    {
		track->cds = nr;
		changed = TRUE;
	    }
	}
        break;
    case T_YEAR:
        nr = atoi (new_text);
        if ((nr >= 0) && (nr != track->year))
        {
	    g_free (etr->year_str);
	    etr->year_str = g_strdup_printf ("%d", nr);
	    track->year = nr;
	    changed = TRUE;
        }
        break;
    case T_PLAYCOUNT:
        nr = atoi (new_text);
        if ((nr >= 0) && (nr != track->playcount))
        {
	    track->playcount = nr;
	    changed = TRUE;
        }
        break;
    case T_RATING:
        nr = atoi (new_text);
        if ((nr >= 0) && (nr <= 5) && (nr != track->rating))
        {
	    track->rating = nr*ITDB_RATING_STEP;
	    changed = TRUE;
        }
        break;
    case T_TIME_ADDED:
    case T_TIME_PLAYED:
    case T_TIME_MODIFIED:
    case T_TIME_RELEASED:
	t = time_string_to_time (new_text);
	if ((t != -1) && (t != time_get_time (track, item)))
	{
	    time_set_time (track, t, item);
	    changed = TRUE;
	}
	break;
    case T_VOLUME:
        nr = atoi (new_text);
        if (nr != track->volume)
        {
	    track->volume = nr;
	    changed = TRUE;
        }
        break;
    case T_SOUNDCHECK:
	nr = replaygain_to_soundcheck (atof (new_text));
/* 	printf("%d : %f\n", nr, atof (new_text)); */
        if (nr != track->soundcheck)
        {
	    track->soundcheck = nr;
	    changed = TRUE;
        }
        break;
    case T_SIZE:
        nr = atoi (new_text);
        if (nr != track->size)
        {
	    track->size = nr;
	    changed = TRUE;
        }
        break;
    case T_BITRATE:
        nr = atoi (new_text);
        if (nr != track->bitrate)
        {
	    track->bitrate = nr;
	    changed = TRUE;
        }
        break;
    case T_SAMPLERATE:
        nr = atoi (new_text);
        if (nr != track->samplerate)
        {
	    track->samplerate = nr;
	    changed = TRUE;
        }
        break;
    case T_BPM:
        nr = atoi (new_text);
        if (nr != track->BPM)
        {
	    track->BPM = nr;
	    changed = TRUE;
        }
        break;
    case T_TRACKLEN:
	str = strrchr (new_text, ':');
	if (str)
	{   /* MM:SS */
	    nr = 1000 * (60 * atoi (new_text) + atoi (str+1));
	}
	else
	{   /* SS */
	    nr = 1000 * atoi (new_text);
	}
	if (nr != track->tracklen)
	{
	    track->tracklen = nr;
	    changed = TRUE;
	}
	break;
    case T_PC_PATH:
    case T_IPOD_PATH:
    case T_IPOD_ID:
    case T_TRANSFERRED:
    case T_COMPILATION:
    case T_CHECKED:
    case T_ALL:
    case T_ITEM_NUM:
	gtkpod_warning ("Programming error: track_set_text() called with illegal argument (item: %d)\n", item);
	break;
    }

    return changed;
}




/* Fills @size with the size and @num with the number of
   non-transferred tracks. The size is in Bytes, minus the space taken
   by tracks that will be overwritten. */
/* @size and @num may be NULL */
void gp_info_nontransferred_tracks (iTunesDB *itdb,
				    gdouble *size, guint32 *num)
{
    GList *gl;

    if (size) *size = 0;
    if (num)  *num = 0;
    g_return_if_fail (itdb);

    for (gl = itdb->tracks; gl; gl=gl->next)
    {
	Track *tr = gl->data;
	ExtraTrackData *etr;
	g_return_if_fail (tr);
	etr = tr->userdata;
	g_return_if_fail (etr);
	if (!tr->transferred)
	{
	    if (size)  *size += tr->size - etr->oldsize;
	    if (num)   *num += 1;
	}
    }
}





/*------------------------------------------------------------------*\
 *                                                                  *
 *             DND to playlists                                     *
 *                                                                  *
\*------------------------------------------------------------------*/

/* DND: add either a GList of tracks or an ASCII list of tracks to
 * Playlist @pl */
static void add_tracks_to_playlist (Playlist *pl,
				    gchar *string, GList *tracks)
{
    void intern_add_track (Playlist *pl, Track *track)
	{
	    iTunesDB *from_itdb, *to_itdb;
	    Playlist *to_mpl;
	    from_itdb = track->itdb;
	    g_return_if_fail (from_itdb);
	    to_itdb = pl->itdb;
	    to_mpl = itdb_playlist_mpl (to_itdb);

/* 	    printf ("add tr %p to pl: %p\n", track, pl); */
	    if (from_itdb == to_itdb)
	    {   /* DND within the same itdb */

		/* set flags to 'podcast' if adding to podcast list */
		if (itdb_playlist_is_podcasts (pl))
		    gp_track_set_flags_podcast (track);

		if (!itdb_playlist_contains_track (to_mpl, track))
		{   /* add to MPL if not already present (will happen
		     * if dragged from the podcasts playlist */
		    gp_playlist_add_track (to_mpl, track, TRUE);
		}
		if (!itdb_playlist_is_mpl (pl))
		{
		    /* add to designated playlist -- unless adding
		     * to podcasts list and track already exists there */
		    if (itdb_playlist_is_podcasts (pl) &&
			g_list_find (pl->members, track))
		    {
			gchar *buf = get_track_info (track, FALSE);
			gtkpod_warning (_("Podcast already present: '%s'\n\n"), buf);
			g_free (buf);
		    }
		    else
		    {
			gp_playlist_add_track (pl, track, TRUE);
		    }
		}
	    }
	    else
	    {   /* DND between different itdbs -- need to duplicate the
		   track before inserting */
		Track *duptr, *addtr;
		/* duplicate track */
		duptr = itdb_track_duplicate (track);
		/* add to database -- if duplicate detection is on and the
		   same track already exists in the database, the already
		   existing track is returned and @duptr is freed */
		addtr = gp_track_add (to_itdb, duptr);

		/* set flags to 'podcast' if adding to podcast list */
		if (itdb_playlist_is_podcasts (pl))
		    gp_track_set_flags_podcast (addtr);

		if (addtr == duptr)
		{   /* no duplicate */
		    /* we need to add to the MPL if the track is no
		       duplicate and will not be added to the podcasts
		       playlist */
		    if (!itdb_playlist_is_podcasts (pl))
		    {   /* don't add to mpl if we add to the podcasts
			   playlist */
			gp_playlist_add_track (to_mpl, addtr, TRUE);
		    }
		}
		else
		{   /* duplicate */
		    /* we also need to add to the MPL if the track is a
		       duplicate, does not yet exist in the MPL and will
		       not be added to a podcast list (this happens if
		       it's already in the podcast list) */
		    if ((!itdb_playlist_contains_track (to_mpl, addtr)) &&
			(!itdb_playlist_is_podcasts (pl)))
		    {
			gp_playlist_add_track (to_mpl, addtr, TRUE);
		    }
		}
		/* add to designated playlist (if not mpl) -- unless
		 * adding to podcasts list and track already * exists
		 * there */
		if (!itdb_playlist_is_mpl (pl))
		{
		    if (itdb_playlist_is_podcasts (pl) &&
			g_list_find (pl->members, addtr))
		    {
			gchar *buf = get_track_info (addtr, FALSE);
			gtkpod_warning (_("Podcast already present: '%s'\n\n"), buf);
			g_free (buf);
		    }
		    else
		    {
			gp_playlist_add_track (pl, addtr, TRUE);
		    }
		}
	    }
	}


    g_return_if_fail (!(string && tracks));
    g_return_if_fail (pl);
    g_return_if_fail (pl->itdb);
    g_return_if_fail (itdb_playlist_mpl (pl->itdb));
    if (!(string || tracks)) return;

    if (string)
    {
	Track *track;
	gchar *str = string;
	while(parse_tracks_from_string(&str, &track))
	{
	    g_return_if_fail (track);
	    intern_add_track (pl, track);
	}
    }
    if (tracks)
    {
	GList *gl;
	for (gl=tracks; gl; gl=gl->next)
	{
	    Track *track = gl->data;
	    g_return_if_fail (track);
	    intern_add_track (pl, track);
	}
    }
}


/* DND: add a glist of tracks to Playlist @pl */
void add_trackglist_to_playlist (Playlist *pl, GList *tracks)
{
	add_tracks_to_playlist (pl, NULL, tracks);
}


/* DND: add a list of tracks to Playlist @pl */
void add_tracklist_to_playlist (Playlist *pl, gchar *string)
{
    add_tracks_to_playlist (pl, string, NULL);
}

/* DND: add a list of files to Playlist @pl.

   @pl: playlist to add to or NULL. If NULL, a "New Playlist" will be
   created and inserted at position @pl_pos for adding tracks and when
   adding a playlist file, a playlist with the name of the playlist
   file will be added.

   @pl_pos: position to add playlist file, ignored if @pl!=NULL.

   @trackaddfunc: passed on to add_track_by_filename() etc. */

/* Return value: playlist to where the tracks were added. Note: when
   adding playlist files, additional playlists may have been created */
Playlist *add_text_plain_to_playlist (iTunesDB *itdb, Playlist *pl,
				      gchar *str, gint pl_pos,
				      AddTrackFunc trackaddfunc,
				      gpointer data)
{
    gchar **files = NULL, **filesp = NULL;
    Playlist *pl_playlist = pl; /* playlist for playlist file */
    Playlist *pl_playlist_created = NULL;

    g_return_val_if_fail (itdb, NULL);

    if (!str)  return NULL;

    /*   printf("pl: %x, pl_pos: %d\n%s\n", pl, pl_pos, str);*/

    block_widgets ();

    files = g_strsplit (str, "\n", -1);
    if (files)
    {
	filesp = files;
	while (*filesp)
	{
	    gboolean added = FALSE;
	    gint file_len = -1;

	    gchar *file = NULL;
	    gchar *decoded_file = NULL;

	    file = *filesp;
	    /* file is in uri form (the ones we're looking for are
	       file:///), file can include the \n or \r\n, which isn't
	       a valid character of the filename and will cause the
	       uri decode / file test to fail, so we'll cut it off if
	       its there. */
	    file_len = strlen (file);
	    if (file_len && (file[file_len-1] == '\n'))
	    {
		file[file_len-1] = 0;
		--file_len;
	    }
	    if (file_len && (file[file_len-1] == '\r'))
	    {
		file[file_len-1] = 0;
		--file_len;
	    }

	    decoded_file = filename_from_uri (file, NULL, NULL);
	    if (decoded_file != NULL)
	    {
		if (g_file_test (decoded_file, G_FILE_TEST_IS_DIR))
		{   /* directory */
		    if (!pl)
		    {  /* no playlist yet -- create new one */
			pl = add_new_pl_user_name (itdb, NULL, pl_pos);
			if (!pl)  break; /* while (*filesp) */
		    }
		    add_directory_by_name (itdb, decoded_file, pl,
					   prefs_get_add_recursively (),
					   trackaddfunc, data);
		    added = TRUE;
		}
		if (g_file_test (decoded_file, G_FILE_TEST_IS_REGULAR))
		{   /* regular file */
		    FileType ftype = determine_file_type (decoded_file);
		    switch (ftype)
		    {
		    case FILE_TYPE_MP3:
		    case FILE_TYPE_M4A:
		    case FILE_TYPE_M4P:
		    case FILE_TYPE_M4B:
		    case FILE_TYPE_WAV:
		    case FILE_TYPE_M4V:
		    case FILE_TYPE_MP4:
		    case FILE_TYPE_MOV:
		    case FILE_TYPE_MPG:
			if (!pl)
			{  /* no playlist yet -- create new one */
			    pl = add_new_pl_user_name (itdb, NULL,
						       pl_pos);
			    if (!pl)  break; /* while (*filesp) */
			}
			add_track_by_filename (itdb, decoded_file, pl,
					       prefs_get_add_recursively (),
					       trackaddfunc, data);
			added = TRUE;
			break;
		    case FILE_TYPE_M3U:
		    case FILE_TYPE_PLS:
			pl_playlist_created = add_playlist_by_filename (
			    itdb, decoded_file,
			    pl_playlist, pl_pos, trackaddfunc, data);
			added = TRUE;
			break;
		    case FILE_TYPE_UNKNOWN:
		    case FILE_TYPE_DIRECTORY:
		    case FILE_TYPE_IMAGE:
			break;
		    }
		}
		g_free (decoded_file);
	    }
	    if (!added)
	    {
		if (strlen (*filesp) != 0)
		    gtkpod_warning (_("drag and drop: ignored '%s'\n"), *filesp);
	    }
	    ++filesp;
	}
	g_strfreev (files);
    }
    /* display log of non-updated tracks */
    display_non_updated (NULL, NULL);
    /* display log updated tracks */
    display_updated (NULL, NULL);
    /* display log of detected duplicates */
    gp_duplicate_remove (NULL, NULL);

    release_widgets ();

    if (pl) return pl;
    if (pl_playlist_created) return pl_playlist_created;
    return NULL;
}

/*------------------------------------------------------------------*\
 *                                                                  *
 * Functions setting default values on tracks                       *
 *                                                                  *
\*------------------------------------------------------------------*/

/* set podcast-specific flags for @track */
void gp_track_set_flags_podcast (Track *track)
{
    g_return_if_fail (track);
    track->flag2 = 0x01;  /* skip when shuffling */
    track->flag3 = 0x01;  /* remember playback position */
    track->flag4 = 0x01;  /* Show Title/Album on the 'Now Playing' page */
}

/* set podcast-specific flags for @track */
void gp_track_set_flags_default (Track *track)
{
    g_return_if_fail (track);
    track->flag2 = 0x00;  /* do not skip when shuffling */
    track->flag3 = 0x00;  /* do not remember playback position */
    track->flag4 = 0x00;  /* Show Title/Album/Artist on the 'Now
			     Playing' page */
}


/*------------------------------------------------------------------*\
 *                                                                  *
 *  Generic functions to "do" things on selected playlist / entry   *
 *  / tracks                                                         *
 *                                                                  *
\*------------------------------------------------------------------*/

/* Make a list of all selected tracks and call @do_func with that list
   as argument */
void gp_do_selected_tracks (void (*do_func)(GList *tracks))
{
    GList *selected_tracks = NULL;

    g_return_if_fail (do_func);

    /* I'm using ids instead of "Track *" -pointer because it would be
     * possible that a track gets removed during the process */
    selected_tracks = tm_get_selected_tracks ();
    do_func (selected_tracks);
    g_list_free (selected_tracks);
}


/* Make a list of all tracks in the currently selected entry of sort
   tab @inst and call @do_func with that list as argument */
void gp_do_selected_entry (void (*do_func)(GList *tracks), gint inst)
{
    GList *selected_tracks = NULL;
    TabEntry *entry;
    GList *gl;

    g_return_if_fail (do_func);

    g_return_if_fail ((inst >= 0) && (inst <= prefs_get_sort_tab_num()));

    entry = st_get_selected_entry (inst);
    if (entry == NULL)
    {  /* no entry selected */
	gtkpod_statusbar_message (_("No entry selected."));
	return;
    }
    for (gl=entry->members; gl; gl=gl->next)
    { /* make a list with all trackids in this entry */
	Track *track = gl->data;
	g_return_if_fail (track);
	selected_tracks = g_list_append (selected_tracks, track);
    }
    do_func (selected_tracks);
    g_list_free (selected_tracks);
}


/* Make a list of the tracks in the current playlist and call @do_func
   with that list as argument */
void gp_do_selected_playlist (void (*do_func)(GList *tracks))
{
    GList *selected_tracks = NULL;
    Playlist *pl;
    GList *gl;

    g_return_if_fail (do_func);

    pl = pm_get_selected_playlist();
    if (!pl)
    { /* no playlist selected */
	gtkpod_statusbar_message (_("No playlist selected."));
	return;
    }
    for (gl=pl->members; gl; gl=gl->next)
    { /* make a list with all trackids in this entry */
	Track *track = gl->data;
	g_return_if_fail (track);
	selected_tracks = g_list_append (selected_tracks, track);
    }
    do_func (selected_tracks);
    g_list_free (selected_tracks);
}


/* return some sensible input about the "track". Yo must free the
 * return string after use. */
gchar *get_track_info (Track *track, gboolean prefer_filename)
{
    ExtraTrackData *etr;

    g_return_val_if_fail (track, NULL);
    etr = track->userdata;
    g_return_val_if_fail (etr, NULL);

    if (prefer_filename)
    {
	if (etr->pc_path_utf8 && strlen(etr->pc_path_utf8))
	    return g_path_get_basename (etr->pc_path_utf8);
    }
    if ((track->title && strlen(track->title)))
	return g_strdup (track->title);
    if ((track->album && strlen(track->album)))
	return g_strdup (track->album);
    if ((track->artist && strlen(track->artist)))
	return g_strdup (track->artist);
    if ((track->composer && strlen(track->composer)))
	return g_strdup (track->composer);
    if (!prefer_filename)
    {
	if (etr->pc_path_utf8 && strlen(etr->pc_path_utf8))
	    return g_path_get_basename (etr->pc_path_utf8);
    }

    return g_strdup_printf ("iPod ID: %d", track->id);
}

