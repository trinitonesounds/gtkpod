/* -*- coding: utf-8; -*-
|  Time-stamp: <2005-01-20 00:33:10 jcs>
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "display_itdb.h"

/* ------------------------------------------------------------ *\
|                                                                |
|         functions for md5 checksums                            |
|                                                                |
\* ------------------------------------------------------------ */

/**
 * Register all tracks in the md5 hash and remove duplicates (while
 * preserving playlists)
 */
void gp_hash_tracks_itdb (iTunesDB *itdb)
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
void gp_hash_tracks (void)
{
    GList *gl;

    g_assert (itdbs_head);

    block_widgets ();
    for (gl=itdbs_head->itdbs; gl; gl=gl->next)
    {
	gp_hash_tracks_itdb (gl->data);
    }
    release_widgets ();
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
       iTunesDB *itdb = oldtrack->itdb;
       g_return_if_fail (itdb);

       if (prefs_get_show_duplicates ())
       {
	   /* add info about it to str */
	   buf = get_track_info (track);
	   buf2 = get_track_info (oldtrack);
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
	       floor((double)(oldtrack->rating + track->rating + RATING_STEP) /
		     (2 * RATING_STEP)) * RATING_STEP;
       else
	   oldtrack->rating = MAX (oldtrack->rating, track->rating);
       /* Set 'modified' timestamp */
       oldtrack->time_modified =  MAX (oldtrack->time_modified,
				      track->time_modified);
       /* Set 'played' timestamp */
       oldtrack->time_played =  MAX (oldtrack->time_played, track->time_played);
       /* Set 'created' timestamp */
       oldtrack->time_created =  MIN (oldtrack->time_created, track->time_created);

       /* Update filename if new track has filename set (should be
	* always!?) */
       if (track->pc_path_locale)
       {
	   ExtraTrackData *oldetr = oldtrack->userdata;
	   ExtraTrackData *etr = track->userdata;
	   g_return_if_fail (oldetr);
	   g_return_if_fail (etr);
	   g_free (oldtrack->pc_path_locale);
	   g_free (oldetr->pc_path_utf8);
	   oldtrack->pc_path_locale = g_strdup (track->pc_path_locale);
	   oldetr->pc_path_utf8 = g_strdup (etr->pc_path_utf8);
       }
       if (itdb_playlist_contains_track (NULL, track))
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
		   gp_playlist_remove_track (pl, track);
	       {
		   if (!itdb_playlist_contains_track (pl, oldtrack))
		       gp_playlist_add_track (pl, oldtrack, TRUE);
	       }
	       gl = gl->next;
	   }
	   /* remove track from MPL, i.e. from the ipod */
	   gp_playlist_remove_track (NULL, track);
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
       oldtrack = md5_track_exists_insert (track);
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
	   remove_duplicate (oldtrack, track);
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
    g_return_val_if_fail (itdb, NULL);
    g_return_val_if_fail (filename, NULL);

    if ((itdb->usertype == GP_ITDB_TYPE_IPOD) &&
	(strncmp (filename, prefs_get_ipod_mount (),
		  strlen (prefs_get_ipod_mount ())) == 0))
    {   /* handle track on iPod */
	GList *gl;
	for (gl=itdb->tracks; gl; gl=gl->next)
	{
	    Track *track = gl->data;
	    gchar *mount;
	    gchar *ipod_path;
	    g_return_val_if_fail (track, NULL);
	    mount = charset_from_utf8 (prefs_get_ipod_mount ());
	    ipod_path = itdb_filename_on_ipod (mount, track);
	    g_free (mount);
	    if (ipod_path)
	    {
		if (strcmp (ipod_path, filename) == 0)
		{
		    g_free (ipod_path);
		    return track;
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
		    return track;
	    }
	}
    }
    return NULL;
}



/* ------------------------------------------------------------ *\
|                                                                |
|         functions to retrieve information from tracks          |
|                                                                |
\* ------------------------------------------------------------ */

/* return the address of the UTF8 field @t_item. @t_item is one of
 * (the applicable) T_* defined in track.h */
gchar **track_get_item_pointer_utf8 (Track *track, T_item t_item)
{
    gchar **result = NULL;

    g_return_val_if_fail (track, NULL);

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
    case T_FDESC:
	result = &track->fdesc;
	break;
    case T_GROUPING:
	result = &track->grouping;
	break;
    case T_IPOD_PATH:
	result = &track->ipod_path;
	break;
    case T_YEAR:
	result = &track->year_str;
	break;
    default:
	g_return_val_if_reached (NULL);
    }
    return result;
}

/* return the UTF8 item @t_item. @t_item is one of
   (the applicable) T_* defined in track.h */
gchar *track_get_item_utf8 (Track *track, T_item t_item)
{
    gchar **ptr;

    g_return_val_if_fail (track, NULL);

    ptr = track_get_item_pointer_utf8 (track, t_item);

    if (ptr)     return *ptr;
    else         return NULL;
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
    case T_TIME_CREATED:
	return &track->time_created;
    default:
	g_return_val_if_reached (0);
    }
}


/* return the specified timestamp. @t_item is one of
   (the * applicable) T_* defined in track.h. If the parameters are
   illegal, "0" is returned. */
guint32 track_get_timestamp (Track *track, T_item t_item)
{
    g_return_val_if_fail (track, 0);

    guint32 *ptr = track_get_timestamp_ptr (track, t_item);
    if (ptr)  return *ptr;
    else      return 0;
}




/*------------------------------------------------------------------*\
 *                                                                  *
 *             DND to playlists                                     *
 *                                                                  *
\*------------------------------------------------------------------*/

/* DND: add a list of iPod IDs to Playlist @pl */
void add_idlist_to_playlist (Playlist *pl, gchar *string)
{
    guint32 id = 0;
    gchar *str = g_strdup (string);

    if (!pl) return;
    while(parse_ipod_id_from_string(&str,&id))
    {
	add_trackid_to_playlist(pl, id, TRUE);
    }
    data_changed();
    g_free (str);
}

/* DND: add a list of files to Playlist @pl.  @pl: playlist to add to
   or NULL. If NULL, a "New Playlist" will be created for adding
   tracks and when adding a playlist file, a playlist with the name of the
   playlist file will be added.
   @trackaddfunc: passed on to add_track_by_filename() etc. */
void add_text_plain_to_playlist (Playlist *pl, gchar *str, gint pl_pos,
				 AddTrackFunc trackaddfunc, gpointer data)
{
    gchar **files = NULL, **filesp = NULL;
    Playlist *pl_playlist = pl; /* playlist for playlist file */

    if (!str)  return;

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
			pl = add_new_pl_user_name (NULL, pl_pos);
			if (!pl)  break; /* while (*filesp) */
		    }
		    add_directory_by_name (decoded_file, pl,
					   prefs_get_add_recursively (),
					   trackaddfunc, data);
		    added = TRUE;
		}
		if (g_file_test (decoded_file, G_FILE_TEST_IS_REGULAR))
		{   /* regular file */
		    gint ftype = determine_file_type (decoded_file);
		    switch (ftype)
		    {
		    case FILE_TYPE_MP3:
		    case FILE_TYPE_M4A:
		    case FILE_TYPE_M4P:
		    case FILE_TYPE_M4B:
		    case FILE_TYPE_WAV:
			if (!pl)
			{  /* no playlist yet -- create new one */
			    pl = add_new_pl_user_name (NULL,
						       pl_pos);
			    if (!pl)  break; /* while (*filesp) */
			}
			add_track_by_filename (decoded_file, pl,
					       prefs_get_add_recursively (),
					       trackaddfunc, data);
			added = TRUE;
			break;
		    case FILE_TYPE_M3U:
		    case FILE_TYPE_PLS:
			add_playlist_by_filename (decoded_file,
						  pl_playlist,
						  trackaddfunc, data);
			added = TRUE;
			break;
		    case FILE_TYPE_ERROR:
		    case FILE_TYPE_UNKNOWN:
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
    remove_duplicate (NULL, NULL);

    release_widgets ();
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

    g_return_if_fail ((inst >= 0) && (inst <= prefs_get_sort_tab_num));

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
	selected_trackids = g_list_append (selected_tracks, track);
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
gchar *get_track_info (Track *track)
{
    ExtraTrackData *etr;

    g_return_val_if_fail (track, NULL);
    etr = track->userdata;
    g_return_val_if_fail (etr, NULL);

    if (etr->pc_path_utf8 && strlen(etr->pc_path_utf8))
	return g_path_get_basename (etr->pc_path_utf8);
    if ((track->title && strlen(track->title)))
	return g_strdup (track->title);
    if ((track->album && strlen(track->album)))
	return g_strdup (track->album);
    if ((track->artist && strlen(track->artist)))
	return g_strdup (track->artist);
    if ((track->composer && strlen(track->composer)))
	return g_strdup (track->composer);
    return g_strdup_printf ("iPod ID: %d", track->ipod_id);
}

