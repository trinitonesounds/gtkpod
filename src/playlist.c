/*
|  Copyright (C) 2002 Adrian Ulrich <pab at blinkenlights.ch>
|                2002-2003 Jörg Schuler  <jcsjcs at users.sourceforge.net>
|
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

#include <gtk/gtk.h>
#include <string.h>
#include "prefs.h"
#include "playlist.h"
#include "display.h"
#include "misc.h"
#include "file.h"
#include "support.h"
#include "id3_tag.h"


GList *playlists;

/* Creates a new playlist */
Playlist *add_new_playlist (gchar *plname, gint position)
{
  Playlist *plitem;

  plitem = g_malloc0 (sizeof (Playlist));
  plitem->type = PL_TYPE_NORM;
  plitem->name = g_strdup (plname);
  plitem->size = 0.0;
  plitem->name_utf16 = g_utf8_to_utf16 (plname, -1, NULL, NULL, NULL);
  data_changed (); /* indicate that data has changed in memory */
  return add_playlist (plitem, position);
}


/* initializes the playlists by creating the master playlist */
void create_mpl (void)
{
  Playlist *plitem;

  if (playlists != NULL) return;  /* already entries! */
  plitem = g_malloc0 (sizeof (Playlist));
  plitem->type = PL_TYPE_MPL;  /* MPL! */
  plitem->name = g_strdup ("gtkpod");
  plitem->size = 0.0;
  plitem->name_utf16 = g_utf8_to_utf16 (plitem->name, -1, NULL, NULL, NULL);
  add_playlist (plitem, -1);
}

/* This function stores the new "plitem" in the global list
   and adds it to the display model.
   "plitem" must have "name", "name_utf16", and "type" initialized
   and all other members set to zero.
   If "plitem" is a master playlist, and another MPL
   already exists, only the name of the new playlist is copied
   and a pointer to the original MPL is returned. This pointer
   has to be used for consequent calls of add_track(id)_to_playlist()
   */
/* @position: if != -1, playlist will be inserted at that position */
Playlist *add_playlist (Playlist *plitem, gint position)
{
    Playlist *mpl;

    if (plitem->type == PL_TYPE_MPL)
    {
	/* it's the MPL */
	mpl = get_playlist_by_nr (0);
	if (mpl != NULL)
	{
	    if (mpl->name)       g_free (mpl->name);
	    if (mpl->name_utf16) g_free (mpl->name_utf16);
	    mpl->name = plitem->name;
	    mpl->name_utf16 = plitem->name_utf16;
	    g_free (plitem);
	    pm_name_changed (mpl);  /* Tell the model about the
				       name change */
	    return mpl;
	}
    }
    if (position == -1)  playlists = g_list_append (playlists, plitem);
    else  playlists = g_list_insert (playlists, plitem, position);
    pm_add_playlist (plitem, position);
    return plitem;
}

/* This function appends the track with id "id" to the 
   playlist "plitem". It then lets the display model know.
   If "plitem" == NULL, add to master playlist
   @display: if TRUE, track is added the display.  Otherwise it's only
   added to memory */
void add_trackid_to_playlist (Playlist *plitem, guint32 id, gboolean display)
{
  Track *track;

  track = get_track_by_id (id);
  /* printf ("id: %4d track: %x\n", id, track); */
  add_track_to_playlist (plitem, track, display);
}

/* This function appends the track "track" to the 
   playlist "plitem". It then lets the display model know.
   If "plitem" == NULL, add to master playlist
   @display: if TRUE, track is added the display.  Otherwise it's only
   added to memory */
void add_track_to_playlist (Playlist *plitem, Track *track, gboolean display)
{
  if (track == NULL) return;
  if (plitem == NULL)  plitem = get_playlist_by_nr (0);
  plitem->members = g_list_append (plitem->members, track);
  ++plitem->num;  /* increase track counter */
  /* it's more convenient to store the pointer to the track than
     the ID, because id=track->ipod_id -- it takes more computing
     power to do it the other way round */
  if (display)  pm_add_track (plitem, track, TRUE);
}

/* This function removes the track with id "id" from the 
   playlist "plitem". It then lets the display model know.
   No action is taken if "track" is not in the playlist.
   If "plitem" == NULL, remove from master playlist */
gboolean remove_trackid_from_playlist (Playlist *plitem, guint32 id)
{
  Track *track;

  track = get_track_by_id (id);
  /* printf ("id: %4d track: %x\n", id, track); */
  return remove_track_from_playlist (plitem, track);
}

/* This function removes the track "track" from the 
   playlist "plitem". It then lets the display model know.
   No action is taken if "track" is not in the playlist.
   If "plitem" == NULL, remove from master playlist.
   If the track is removed from the MPL, it's also removed 
   from memory completely

   Return value: FALSE, if track was not a member, TRUE otherwise */
gboolean remove_track_from_playlist (Playlist *plitem, Track *track)
{
    if (track == NULL) return FALSE;
    if (plitem == NULL)  plitem = get_playlist_by_nr (0);
    if (g_list_find (plitem->members, track) == NULL)
	return FALSE; /* not a member */
    pm_remove_track (plitem, track);
    plitem->members = g_list_remove (plitem->members, track);
    --plitem->num; /* decrease number of tracks */
    if (plitem->type == PL_TYPE_MPL)
    { /* if it's the MPL, we remove the track permanently */
	gint i, n;
	n = get_nr_of_playlists ();
	for (i = 1; i<n; ++i)
	{  /* first we remove the track from all other playlists (i=1:
	    * skip MPL or we loop */
	    remove_track_from_playlist (get_playlist_by_nr (i), track);
	}
	remove_track_from_ipod (track);	
    }
    return TRUE; /* track was a member */
}

/* checks if "track" is in playlist "plitem". TRUE, if yes, FALSE
   otherwise */
gboolean track_is_in_playlist (Playlist *plitem, Track *track)
{
    if (track == NULL) return FALSE;
    if (plitem == NULL)  plitem = get_playlist_by_nr (0);
    if (g_list_find (plitem->members, track) == NULL)
	return FALSE; /* not a member */
    return TRUE;
}


/* Returns the number of playlists
   (including the MPL, which is nr. 0) */
guint32 get_nr_of_playlists (void)
{
  return g_list_length (playlists);
}

/* Returns the n_th playlist.
   Useful for itunesdb: Nr. 0 should be the MLP */
Playlist *get_playlist_by_nr (guint32 n)
{
  return g_list_nth_data (playlists, n);
}

/* Returns the number of tracks in "plitem" */
/* Stupid function... but please use it :-) */
guint32 get_nr_of_tracks_in_playlist (Playlist *plitem)
{
  return plitem->num;
  /* return g_list_length (plitem->members);*/
}

/* Returns the track nur "n" in playlist "plitem" */
/* If "n >= number of tracks in playlist" return NULL */
Track *get_track_in_playlist_by_nr (Playlist *plitem, guint32 n)
{
  return g_list_nth_data (plitem->members, n);
}

/**
 * 
 * You must free this yourself
 */
/* Remove playlist from the list. First it's removed from any display
   model using remove_playlist_from_model (), then the entry itself
   is removed from the GList *playlists */
void remove_playlist (Playlist *playlist)
{
  pm_remove_playlist (playlist, TRUE);
  data_changed ();
  free_playlist(playlist);
}

/* Move @playlist to the position @pos. It will not move the master
 * playlist, and will not move anything before the master playlist */
void move_playlist (Playlist *playlist, gint pos)
{
    if (!playlist) return;
    if (pos == 0)  return; /* don't move before MPL */

    if (playlist->type == PL_TYPE_MPL)  return; /* don't move MPL */
    playlists = g_list_remove (playlists, playlist);
    playlists = g_list_insert (playlists, playlist, pos);
}

/**
 * free_playlist - free the associated memory a playlist contains
 * @playlist - the playlist we'd like to free
 */
void
free_playlist(Playlist *playlist)
{
  playlists = g_list_remove (playlists, playlist);
  if (playlist->name)            g_free (playlist->name);
  if (playlist->name_utf16)      g_free (playlist->name_utf16);
  g_list_free (playlist->members);
  g_free (playlist);
}

  /* Remove all playlists from the list using remove_playlist () */
void remove_all_playlists (void)
{
  Playlist *playlist;

  while (playlists != NULL)
    {
      playlist = g_list_nth_data (playlists, 0);
      pm_remove_playlist (playlist, FALSE);
      free_playlist (playlist);
    }
}

/***
 * playlist_exists(): return TRUE if the playlist @pl exists, FALSE
 * otherwise
 ***/
gboolean playlist_exists (Playlist *pl)
{
    if (g_list_find (playlists, pl))  return TRUE;
    else                              return FALSE;
}


/* FIXME: this is a bit dangerous. . . we delete all
 * playlists with titles @pl_name and return how many
 * pl have been removed.
 ***/
guint remove_playlist_by_name(gchar *pl_name){
    Playlist *pl;
    guint i;
    guint pl_removed=0;
    
    for(i = 1; i < get_nr_of_playlists(); i++)
    {
        pl = get_playlist_by_nr (i);
        if(pl->name && (strcmp (pl->name, pl_name) == 0))
        {
            remove_playlist(pl);
         /* we just deleted the ith element of playlists, so
          * we must examine the new ith element. */
            pl_removed++;
            i--;
        }
    }
    return pl_removed;
}


/* ------------------------------------------------------------------- */
/* functions used by itunesdb (so we can refresh the display during
 * import */
void it_add_trackid_to_playlist (Playlist *plitem, guint32 id)
{
    static Playlist *last_pl = NULL;
    static GTimeVal *time;
    static float max_count = REFRESH_INIT_COUNT;
    static gint count = REFRESH_INIT_COUNT - 1;
    static gint count_s = 0;
    float ms;
    gchar *buf;

    if (!time) time = g_malloc (sizeof (GTimeVal));

    if (plitem != last_pl)
    {
	max_count = REFRESH_INIT_COUNT;
	count = REFRESH_INIT_COUNT - 1;
	count_s = 0; /* nr of tracks in current playlist */
	g_get_current_time (time);
	last_pl = plitem;
    }
    add_trackid_to_playlist (plitem, id, TRUE);
    --count;
    ++count_s;
    if ((count < 0) && widgets_blocked)
    {
	buf = g_strdup_printf (ngettext ("Added %d+ tracks to playlist '%s'",
					 "Added %d+ tracks to playlist '%s'",
					 count_s), count_s, plitem->name);
	gtkpod_statusbar_message(buf);
	g_free (buf);
	while (gtk_events_pending ())  gtk_main_iteration ();
	ms = get_ms_since (time, TRUE);
	/* average the new and the old max_count */
	max_count *= (1 + 2 * REFRESH_MS / ms) / 3;
	count = max_count - 1;
#if DEBUG_TIMING
	printf("it_a_s ms: %f mc: %f\n", ms, max_count);
#endif
    }
}

Track *it_get_track_in_playlist_by_nr (Playlist *plitem, guint32 n)
{
    Track *track = get_track_in_playlist_by_nr (plitem, n);
    while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
    return track;
}

Playlist *it_add_playlist (Playlist *plitem)
{
    Playlist *pl = add_playlist (plitem, -1);
    gtkpod_tracks_statusbar_update();
    while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
    return pl;
}
