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
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include "prefs.h"
#include "playlist.h"
#include "display.h"
#include "misc.h"
#include "file.h"
#include "support.h"


GList *playlists;

/* Creates a new playlist */
Playlist *add_new_playlist (gchar *plname)
{
  Playlist *plitem;

  plitem = g_malloc0 (sizeof (Playlist));
  plitem->type = PL_TYPE_NORM;
  plitem->name = g_strdup (plname);
  plitem->name_utf16 = g_utf8_to_utf16 (plname, -1, NULL, NULL, NULL);
  data_changed (); /* indicate that data has changed in memory */
  return add_playlist (plitem);
}


/* initializes the playlists by creating the master playlist */
void create_mpl (void)
{
  Playlist *plitem;

  if (playlists != NULL) return;  /* already entries! */
  plitem = g_malloc0 (sizeof (Playlist));
  plitem->type = PL_TYPE_MPL;  /* MPL! */
  plitem->name = g_strdup ("gtkpod");
  plitem->name_utf16 = g_utf8_to_utf16 (plitem->name, -1, NULL, NULL, NULL);
  add_playlist (plitem);
}

/* This function stores the new "plitem" in the global list
   and adds it to the display model.
   "plitem" must have "name", "name_utf16", and "type" initialized
   and all other members set to zero.
   If "plitem" is a master playlist, and another MPL
   already exists, only the name of the new playlist is copied
   and a pointer to the original MPL is returned. This pointer
   has to be used for consequent calls of add_song(id)_to_playlist() */
Playlist *add_playlist (Playlist *plitem)
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
  playlists = g_list_append (playlists, plitem);
  pm_add_playlist (plitem);
  return plitem;
}

/* This function appends the song with id "id" to the 
   playlist "plitem". It then lets the display model know.
   If "plitem" == NULL, add to master playlist */
void add_songid_to_playlist (Playlist *plitem, guint32 id)
{
  Song *song;

  song = get_song_by_id (id);
  /* printf ("id: %4d song: %x\n", id, song); */
  add_song_to_playlist (plitem, song);
}

/* This function appends the song "song" to the 
   playlist "plitem". It then lets the display model know.
   If "plitem" == NULL, add to master playlist */
void add_song_to_playlist (Playlist *plitem, Song *song)
{
  if (song == NULL) return;
  if (plitem == NULL)  plitem = get_playlist_by_nr (0);
  plitem->members = g_list_append (plitem->members, song);
  /*  ++plitem->num;  */ /* increase song counter */
  /* it's more convenient to store the pointer to the song than
     the ID, because id=song->ipod_id -- it takes more computing
     power to do it the other way round */
  pm_add_song (plitem, song);
}

/* This function removes the song with id "id" from the 
   playlist "plitem". It then lets the display model know.
   No action is taken if "song" is not in the playlist.
   If "plitem" == NULL, remove from master playlist */
gboolean remove_songid_from_playlist (Playlist *plitem, guint32 id)
{
  Song *song;

  song = get_song_by_id (id);
  /* printf ("id: %4d song: %x\n", id, song); */
  return remove_song_from_playlist (plitem, song);
}

/* This function removes the song "song" from the 
   playlist "plitem". It then lets the display model know.
   No action is taken if "song" is not in the playlist.
   If "plitem" == NULL, remove from master playlist.
   If the song is removed from the MPL, it's also removed 
   from memory completely

   Return value: FALSE, if song was not a member, TRUE otherwise */
gboolean remove_song_from_playlist (Playlist *plitem, Song *song)
{
    if (song == NULL) return FALSE;
    if (plitem == NULL)  plitem = get_playlist_by_nr (0);
    if (g_list_find (plitem->members, song) == NULL)
	return FALSE; /* not a member */
    pm_remove_song (plitem, song);
    plitem->members = g_list_remove (plitem->members, song);
    if (plitem->type == PL_TYPE_MPL)
    { /* if it's the MPL, we remove the song permanently */
	gint i, n;
	n = get_nr_of_playlists ();
	for (i = 1; i<n; ++i)
	{  /* first we remove the song from all other playlists (i=1:
	    * skip MPL or we loop */
	    remove_song_from_playlist (get_playlist_by_nr (i), song);
	}
	remove_song_from_ipod (song);	
    }
    return TRUE; /* song was a member */
}

/* checks if "song" is in playlist "plitem". TRUE, if yes, FALSE
   otherwise */
gboolean song_is_in_playlist (Playlist *plitem, Song *song)
{
    if (song == NULL) return FALSE;
    if (plitem == NULL)  plitem = get_playlist_by_nr (0);
    if (g_list_find (plitem->members, song) == NULL)
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

/* Returns the number of songs in "plitem" */
/* Stupid function... but please use it :-) */
guint32 get_nr_of_songs_in_playlist (Playlist *plitem)
{
  /*  return plitem->num;*/
  return g_list_length (plitem->members);
}

/* Returns the song nur "n" in playlist "plitem" */
/* If "n >= number of songs in playlist" return NULL */
Song *get_song_in_playlist_by_nr (Playlist *plitem, guint32 n)
{
  return g_list_nth_data (plitem->members, n);
}

/* Remove playlist from the list. First it's removed from any display
   model using remove_playlist_from_model (), then the entry itself
   is removed from the GList *playlists */
void remove_playlist (Playlist *playlist)
{
  pm_remove_playlist (playlist, TRUE);
  free_playlist(playlist);
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

/**
 *
 * @new_l - the new list we want our internal represenation to be set to
 */
void 
reset_playlists_to_new_list(GList *new_l)
{
    if(playlists) g_list_free(playlists);
    playlists = new_l;
}


/* ------------------------------------------------------------------- */
/* functions used by itunesdb (so we can refresh the display during
 * import */
void it_add_songid_to_playlist (Playlist *plitem, guint32 id)
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
	count_s = 0; /* nr of songs in current playlist */
	g_get_current_time (time);
	last_pl = plitem;
    }
    add_songid_to_playlist (plitem, id);
    --count;
    ++count_s;
    if ((count < 0) && widgets_blocked)
    {
	buf = g_strdup_printf (ngettext ("Added %d+ song to playlist '%s'",
					 "Added %d+ songs to playlists '%s'",
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

Song *it_get_song_in_playlist_by_nr (Playlist *plitem, guint32 n)
{
    Song *song = get_song_in_playlist_by_nr (plitem, n);
    while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
    return song;
}

Playlist *it_add_playlist (Playlist *plitem)
{
    Playlist *pl = add_playlist (plitem);
    gtkpod_songs_statusbar_update();
    while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
    return pl;
}
