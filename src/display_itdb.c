/* Time-stamp: <2005-01-04 00:02:11 jcs>
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
#include "display.h"
#include "md5.h"
#include "support.h"

/* list with available iTunesDBs. A pointer to this list is stored in
 * gtkpod_window as "itdbs" data (see . */
static GList *itdbs=NULL;


void gp_itdb_extra_destroy (ExtraiTunesDBData *eitdb)
{
    if (eitdb)
    {
	md5_free (eitdb);
	g_free (eitdb);
    }
}

void gp_playlist_extra_destroy (ExtraPlaylistData *epl)
{
    if (epl)
    {
	g_free (epl);
    }
}

void gp_track_extra_destroy (ExtraTrackData *etrack)
{
    if (etrack)
    {
	g_free (etrack->year_str);
	g_free (etrack->pc_path_utf8);
	g_free (etrack->hostname);
	g_free (etrack->md5_hash);
	g_free (etrack->charset);
	g_free (etrack);
    }
}

iTunesDB *gp_itdb_new (void)
{
    iTunesDB *itdb = itdb_new ();
    itdb->userdata = g_new0 (ExtraiTunesDBData, 1);
    itdb->userdata_destroy =
	(ItdbUserDataDestroyFunc)gp_itdb_extra_destroy;
    return itdb;
}

Playlist *gp_playlist_new (const gchar *title, gboolean spl)
{
    Playlist *pl = itdb_playlist_new (title, spl);
    pl->userdata = g_new0 (ExtraPlaylistData, 1);
    pl->userdata_destroy =
	(ItdbUserDataDestroyFunc)gp_playlist_extra_destroy;
    return pl;
}

Track *gp_track_new (void)
{
    Track *track = itdb_track_new ();
    track->userdata = g_new0 (ExtraTrackData, 1);
    track->userdata_destroy =
	(ItdbUserDataDestroyFunc)gp_track_extra_destroy;
    return track;
}

/* add itdb to itdbs */
void gp_itdb_add (iTunesDB *itdb)
{
    g_return_if_fail (itdb);

    itdbs = g_list_append (itdbs, itdb);
}

/* add playlist to itdb and to display */
void gp_playlist_add (iTunesDB *itdb, Playlist *pl, gint32 pos)
{
    g_return_if_fail (itdb);
    g_return_if_fail (pl);

    itdb_playlist_add (itdb, pl, pos);
    pm_add_playlist (pl, pos);
}

void init_data (GtkWidget *window)
{
    iTunesDB *itdb;
    Playlist *pl;

    g_assert (window);
    g_return_if_fail (itdbs == NULL);

    g_object_set_data (G_OBJECT (window), "itdbs", &itdbs);

    itdb = gp_itdb_new ();
    itdb->usertype = GP_ITDB_TYPE_IPOD;
    gp_itdb_add (itdb);
    pl = gp_playlist_new (_("gtkpod"), FALSE);
    pl->type = PL_TYPE_MPL;  /* MPL! */
    gp_playlist_add (itdb, pl, -1);

    itdb = gp_itdb_new ();
    itdb->usertype = GP_ITDB_TYPE_LOCAL;
    gp_itdb_add (itdb);
    pl = gp_playlist_new (_("Local"), FALSE);
    pl->type = PL_TYPE_MPL;  /* MPL! */
    gp_playlist_add (itdb, pl, -1);
}


/* Increase playcount of filename <file> by <num>. If md5 is activated,
   use md5 to find the track. Otherwise use the filename. If @md5 is
   set, this value is used directly to look up the track in the
   database (instead of calculating it from the file).

   Return value:
   TRUE: OK (playcount increased in GP_ITDB_TYPE_IPOD)
   FALSE: file could not be found in the GP_ITDB_TYPE_IPOD */
gboolean gp_increase_playcount (gchar *md5, gchar *file, gint num)
{
    gboolean result = FALSE;
    Track *track = NULL;
    GList *gl;

    for (gl=itdbs; gl; gl=gl->next)
    {
	iTunesDB *itdb = gl->data;
	g_return_val_if_fail (itdb, FALSE);

	if (md5) track = md5_md5_exists (itdb, md5);
	else     track = md5_file_exists (itdb, file, TRUE);
	if (!track)	  track = gp_track_by_filename (itdb, file);
	if (track)
	{
	    gchar *buf1, *buf;
	    track->playcount += num;
	    data_changed ();
	    pm_track_changed (track);
	    buf1 = get_track_info (track);
	    buf = g_strdup_printf (_("Increased playcount for '%s'"), buf1);
	    gtkpod_statusbar_message (buf);
	    g_free (buf);
	    g_free (buf1);
	    if (itdb->usertype == GP_ITDB_TYPE_IPOD)    result = TRUE;
	}
    }
    return result;
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
void hash_tracks (void)
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
/*        printf("%d:%d:%p:%p\n", count, track_nr, track, oldtrack); */
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
	   gp_duplicate_remove (oldtrack, track);
       }
       else
       { /* if we removed a track (above), we don't need to increment
	    the track_nr here */
	   ++track_nr;
       }
   }
   gp_duplicate_remove (NULL, NULL); /* show info dialogue */
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
       data_changed ();
   }
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
	    gchar *mount = charset_from_utf8 (prefs_get_ipod_mount ());
	    gchar *ipod_path = itdb_filename_on_ipod (mount, track);
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
	    if (track->pc_path)
	    {
		if (strcmp (track->pc_path, filename) == 0)
		    return track;
	    }
	}
    }
    return NULL;
}
