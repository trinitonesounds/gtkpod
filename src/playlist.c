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
   has to be used for consequent calls of add_song(id)_to_playlist()
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

/* This function appends the song with id "id" to the 
   playlist "plitem". It then lets the display model know.
   If "plitem" == NULL, add to master playlist
   @display: if TRUE, song is added the display.  Otherwise it's only
   added to memory */
void add_songid_to_playlist (Playlist *plitem, guint32 id, gboolean display)
{
  Song *song;

  song = get_song_by_id (id);
  /* printf ("id: %4d song: %x\n", id, song); */
  add_song_to_playlist (plitem, song, display);
}

/* This function appends the song "song" to the 
   playlist "plitem". It then lets the display model know.
   If "plitem" == NULL, add to master playlist
   @display: if TRUE, song is added the display.  Otherwise it's only
   added to memory */
void add_song_to_playlist (Playlist *plitem, Song *song, gboolean display)
{
  if (song == NULL) return;
  if (plitem == NULL)  plitem = get_playlist_by_nr (0);
  plitem->members = g_list_append (plitem->members, song);
  ++plitem->num;  /* increase song counter */
  /* it's more convenient to store the pointer to the song than
     the ID, because id=song->ipod_id -- it takes more computing
     power to do it the other way round */
  if (display)  pm_add_song (plitem, song, TRUE);
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
    --plitem->num; /* decrease number of songs */
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
  return plitem->num;
  /* return g_list_length (plitem->members);*/
}

/* Returns the song nur "n" in playlist "plitem" */
/* If "n >= number of songs in playlist" return NULL */
Song *get_song_in_playlist_by_nr (Playlist *plitem, guint32 n)
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

/* FIXME: should this function have handling for utf16 
 * genres and playlist titles? */
void generate_genre_playlists (void)
{
    Playlist *master_pl;
    int i, j; 

    /* FIXME: this is a bit dangerous. . . we delete all
     * playlists with titles that start with '[' and end 
     * with ']'.  We assume that they are previously generated
     * (and possibly out of date) genre playlists. */
    for(i = 0; i < g_list_length(playlists); i++) {
        Playlist *pl = g_list_nth_data(playlists, i);

        if(pl->name[0] == '[' && 
                pl->name[strlen(pl->name)-1] == ']') {
            remove_playlist(pl);
            /* we just deleted the ith element of playlists, so
             * we must examine the new ith element. */
            i--;
        }
    }

   master_pl = g_list_nth_data(playlists, 0);
    
    for(i = 0; i < master_pl->num ; i++) {
        Song *song = g_list_nth_data(master_pl->members, i);
        Playlist *genre_pl = NULL;
        gchar genre[ID3V2_MAX_STRING_LEN+3]; 
        int playlists_len = g_list_length(playlists);

        /* some songs have empty strings in the genre field */
        if(song->genre[0] == '\0') {
            sprintf(genre, "[%s]", _("Unknown Genre"));
        } else {
            sprintf(genre, "[%s]", song->genre);
        }

        /* look for genre playlist */
        for(j = 0; j < playlists_len; j++) {
            Playlist *pl = g_list_nth_data(playlists, j);

            if(!g_ascii_strcasecmp(pl->name, genre)) {
                genre_pl = pl;
                break;
            }
        }

        /* or, create genre playlist */
        if(!genre_pl) {
            genre_pl = add_new_playlist(genre, -1);
        }

        add_song_to_playlist(genre_pl, song, TRUE);
    }
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
    add_songid_to_playlist (plitem, id, TRUE);
    --count;
    ++count_s;
    if ((count < 0) && widgets_blocked)
    {
	buf = g_strdup_printf (ngettext ("Added %d+ songs to playlist '%s'",
					 "Added %d+ songs to playlist '%s'",
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
    Playlist *pl = add_playlist (plitem, -1);
    gtkpod_songs_statusbar_update();
    while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
    return pl;
}
