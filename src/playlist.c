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
#include "support.h"


GList *playlists;

/* Creates a new playlist */
void add_new_playlist (void)
{
  Playlist *plitem;

  plitem = g_malloc0 (sizeof (Playlist));
  plitem->type = PL_TYPE_NORM;
  plitem->name = g_strdup ("New Playlist");
  plitem->name_utf16 = g_utf8_to_utf16 (plitem->name, -1, NULL, NULL, NULL);
  add_playlist (plitem);
  data_changed (); /* indicate that data has changed in memory */
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
void remove_songid_from_playlist (Playlist *plitem, guint32 id)
{
  Song *song;

  song = get_song_by_id (id);
  /* printf ("id: %4d song: %x\n", id, song); */
  remove_song_from_playlist (plitem, song);
}

/* This function removes the song "song" from the 
   playlist "plitem". It then lets the display model know.
   No action is taken if "song" is not in the playlist.
   If "plitem" == NULL, remove from master playlist.
   If the song is removed from the MPL, it's also removed 
   from memory completely */
void remove_song_from_playlist (Playlist *plitem, Song *song)
{
    if (song == NULL) return;
    if (plitem == NULL)  plitem = get_playlist_by_nr (0);
    if (g_list_find (plitem->members, song) == NULL) return; /* not a member */
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

