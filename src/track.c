/*
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
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include "prefs.h"
#include "song.h"
#include "misc.h"
#include "support.h"
#include "md5.h"
#include "confirmation.h"
#include "charset.h"
#include "file.h"

/* List with all the songs */
GList *songs = NULL;

static guint32 free_ipod_id (guint32 id);
static void reset_ipod_id (void);

void
free_song(Song *song)
{
  if (song)
    { /* C_FREE defined in misc.h */
      C_FREE (song->album);
      C_FREE (song->artist);
      C_FREE (song->title);
      C_FREE (song->genre);
      C_FREE (song->comment);
      C_FREE (song->composer);
      C_FREE (song->fdesc);
      C_FREE (song->album_utf16);
      C_FREE (song->artist_utf16);
      C_FREE (song->title_utf16);
      C_FREE (song->genre_utf16);
      C_FREE (song->comment_utf16);
      C_FREE (song->composer_utf16);
      C_FREE (song->fdesc_utf16);
      C_FREE (song->pc_path_utf8);
      C_FREE (song->pc_path_locale);
      C_FREE (song->ipod_path);
      C_FREE (song->ipod_path_utf16);
      C_FREE (song->md5_hash);
      C_FREE (song->hostname);
      g_free (song);
    }
}

/* Append song to the list */
/* Note: adding to the display model is the responsibility of
   the playlist code */
/* Returns: pointer to the added song -- may be different in the case
   of duplicates. In that case a pointer to the already existing song
   is returned. */
Song *add_song (Song *song)
{
  Song *oldsong, *result=NULL;

  /* fill in additional information from the extended info hash */
  fill_in_extended_info (song);
  if((oldsong = md5_song_exists_insert (song)))
  {
      remove_duplicate (oldsong, song);
      free_song(song);
      result = oldsong;
  }
  else
  {
    /* Make sure all strings are initialised -- that way we don't 
     have to worry about it when we are handling the strings */
    /* exception: md5_hash, hostname, charset: these may be NULL. */
    validate_entries (song);
    if(!song->ipod_id) 
	song->ipod_id = free_ipod_id (0);  /* keep track of highest ID used */
    else
	free_ipod_id(song->ipod_id);
    
    songs = g_list_append (songs, song);
    result = song;
  }
  return result;
}


/* Make sure all strings are initialised -- that way we don't 
   have to worry about it when we are handling the strings */
/* exception: md5_hash and hostname: these may be NULL. */
void validate_entries (Song *song)
{
    gint sz = sizeof (gunichar2);

    if (!song) return;

    if (song->album == NULL)           song->album = g_strdup ("");
    if (song->artist == NULL)          song->artist = g_strdup ("");
    if (song->title == NULL)           song->title = g_strdup ("");
    if (song->genre == NULL)           song->genre = g_strdup ("");
    if (song->comment == NULL)         song->comment = g_strdup ("");
    if (song->composer == NULL)        song->composer = g_strdup ("");
    if (song->fdesc == NULL)           song->fdesc = g_strdup ("");
    if (song->album_utf16 == NULL)     song->album_utf16 = g_malloc0 (sz);
    if (song->artist_utf16 == NULL)    song->artist_utf16 = g_malloc0 (sz);
    if (song->title_utf16 == NULL)     song->title_utf16 = g_malloc0 (sz);
    if (song->genre_utf16 == NULL)     song->genre_utf16 = g_malloc0 (sz);
    if (song->comment_utf16 == NULL)   song->comment_utf16 = g_malloc0 (sz);
    if (song->composer_utf16 == NULL)  song->composer_utf16 = g_malloc0 (sz);
    if (song->fdesc_utf16 == NULL)     song->fdesc_utf16 = g_malloc0 (sz);
    if (song->pc_path_utf8 == NULL)    song->pc_path_utf8 = g_strdup ("");
    if (song->pc_path_locale == NULL)  song->pc_path_locale = g_strdup ("");
    if (song->ipod_path == NULL)       song->ipod_path = g_strdup ("");
    if (song->ipod_path_utf16 == NULL) song->ipod_path_utf16 = g_malloc0 (sz);
}

/**
 * remove_song_from_ipod - in order to delete a song from the system
 * we need to keep track of the Songs we want to delete next time we export
 * the id. 
 * @song - the Song id we want to delete
 */
void
remove_song_from_ipod (Song *song)
{
    if (song->transferred)
    {
	songs = g_list_remove(songs, song);
	md5_song_removed (song);
	mark_song_for_deletion (song); /* will be deleted on next export */
    }
    else
    {
	remove_song (song);
    }
}

/* Remove song from the list. */
void remove_song (Song *song)
{
  songs = g_list_remove (songs, song);
  md5_song_removed (song);
  free_song(song);
}


/* Remove all songs from the list using remove_song () */
void remove_all_songs (void)
{
  Song *song;

  while (songs != NULL)
    {
      song = g_list_nth_data (songs, 0);
      remove_song (song);
    }
}


/* Returns the number of songs stored in the list */
guint get_nr_of_songs (void)
{
  return g_list_length (songs);
}

/* Returns the number of songs not yet transferred to the ipod */
guint get_nr_of_nontransferred_songs (void)
{
    guint n = 0;
    Song *song;
    GList *gl_song;

    for (gl_song = songs; gl_song; gl_song=gl_song->next)
    {
	song = (Song *)gl_song->data;
	if (!song->transferred)   ++n;
    }
    return n;
}

/* in Bytes, minus the space taken by songs that will be overwritten
 * during copying */
glong get_filesize_of_nontransferred_songs(void)
{
    glong n = 0;
    Song *song;
    GList *gl_song;

    for (gl_song = songs; gl_song; gl_song=gl_song->next)
    {
	song = (Song *)gl_song->data;
	if (!song->transferred)   n += song->size - song->oldsize;
    }
    return n;
}


/* Returns the n_th song */
Song *get_song_by_nr (guint32 n)
{
  return g_list_nth_data (songs, n);
}

/* Gets the next song (i=1) or the first song (i=0)
   This function is optimized for speed, caching a pointer to the last
   link returned.
   Make sure that the list of songs does not get changed during
   ceonsecutive calls with i=1 or this function might crash. */
Song *get_next_song (gint i)
{
    static GList *lastlink = NULL;
    Song *result;

    if (i==0)           lastlink = songs;
    else if (lastlink)  lastlink = lastlink->next;

    if (lastlink)       result = (Song *)lastlink->data;
    else                result = NULL;

    return result;
}	


/* Returns the song with ID "id". We need to get the last occurence of
 * "id" in case we're currently importing the iTunesDB. In that case
 * there might be duplicate ids */
Song *get_song_by_id (guint32 id)
{
  GList *l;
  Song *s;

  for (l=g_list_last(songs); l; l=l->prev)
  {
      s = (Song *)l->data;
      if (s->ipod_id == id)  return s;
  }
  return NULL;
}


/* Returns the song with the local filename @name or NULL, if none can
 * be found. */

Song *get_song_by_filename (gchar *name)
{
  GList *l;

  if (!name) return NULL;

  for (l=songs; l; l=l->next)
  {
      Song *song = (Song *)l->data;
      if (song->pc_path_locale)
	  if (strcmp (song->pc_path_locale, name) == 0) return song;
  }
  return NULL;
}


/* renumber ipod ids (used from withing handle_import() */
void renumber_ipod_ids ()
{
    GList *gl_song;

    /* We simply re-ID all songs. That way we don't run into
       trouble if some "funny" guy put a "-1" into the itunesDB */
    reset_ipod_id (); /* reset lowest unused ipod ID */
    for (gl_song = songs; gl_song; gl_song=gl_song->next)
    {
	((Song *)gl_song->data)->ipod_id = free_ipod_id (0);
	/*we need to tell the display that the ID has changed (if ID
	 * is displayed) */
	/* pm_song_changed (song);*/
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
/* Called each time by add_song with the ipod_id used.
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
 * Register all songs in the md5 hash and remove duplicates (while
 * preserving playlists)
 */
void hash_songs(void)
{
   gint ns, count, song_nr;
   gchar *buf;
   Song *song, *oldsong;

   if (!prefs_get_md5songs ()) return;
   ns = get_nr_of_songs ();
   if (ns == 0)                 return;

   block_widgets (); /* block widgets -- this might take a while,
			so we'll do refreshs */
   md5_unique_file_free (); /* release md5 hash */
   count = 0;
   song_nr = 0;
   /* populate the hash table */
   while ((song = get_song_by_nr (song_nr)))
   {
       oldsong = md5_song_exists_insert(song);
       ++count;
       if (((count % 20) == 1) || (count == ns))
       { /* update for count == 1, 21, 41 ... and for count == n */
	   buf = g_strdup_printf (ngettext ("Hashed %d of %d song.",
					    "Hashed %d of %d songs.", ns),
				  count, ns);
	   gtkpod_statusbar_message(buf);
	   while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
	   g_free (buf);
       }
       if (oldsong)
       {
	   remove_duplicate (oldsong, song);
       }
       else
       { /* if we removed a song (above), we don't need to increment
	    the song_nr here */
	   ++song_nr;
       }
   }
   remove_duplicate (NULL, NULL); /* show info dialogue */
   release_widgets (); /* release widgets again */
}


/* This function removes a duplicate song "song" from memory while
 * preserving the playlists. The md5 hash is not modified.  You should
 * call "remove_duplicate (NULL, NULL)" to pop up the info dialogue
 * with the list of duplicate songs afterwards. Call with "NULL, (void
 * *)-1" to just clean up without dialoge. If "song" does not exist in
 * the master play list, only a message is logged (to be displayed
 * later when called with "NULL, NULL" */
void remove_duplicate (Song *oldsong, Song *song)
{
   gchar *buf, *buf2;
   static gint delsong_nr = 0;
   static gboolean removed = FALSE;
   static GString *str = NULL;

   if (prefs_get_show_duplicates() && (oldsong == NULL) && (song == NULL) && str)
   {
       if (str->len)
       { /* Some songs have been deleted. Print a notice */
	   if (removed)
	   {	       
	       buf = g_strdup_printf (
		   ngettext ("The following duplicate song has been removed.",
			     "The following %d duplicate songs have been removed.",
			     delsong_nr), delsong_nr);
	   }
	   else
	   {
	       buf = g_strdup_printf (
		   ngettext ("The following duplicate song has not been added to the master play list.",
			     "The following %d duplicate songs have not been added to the master play list.",
			     delsong_nr), delsong_nr);
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
   if (oldsong == NULL)
   { /* clean up */
       if (str)       g_string_free (str, TRUE);
       str = NULL;
       removed = FALSE;
       delsong_nr = 0;
       gtkpod_songs_statusbar_update();
   }
   if (prefs_get_show_duplicates() && oldsong && song)
   {
       /* add info about it to str */
       buf = get_song_info (song);
       buf2 = get_song_info (oldsong);
       if (!str)
       {
	   delsong_nr = 0;
	   str = g_string_sized_new (2000); /* used to keep record of
					     * duplicate songs */
       }
       g_string_append_printf (str, "'%s': identical to '%s'\n",
			       buf, buf2);
       g_free (buf);
       g_free (buf2);
       if (song_is_in_playlist (NULL, song))
       { /* song is already added to memory -> replace with "oldsong" */
	   /* check for "song" in all playlists (except for MPL) */
	   gint np = get_nr_of_playlists ();
	   gint pl_nr;
	   for (pl_nr=1; pl_nr<np; ++pl_nr)
	   {
	       Playlist *pl = get_playlist_by_nr (pl_nr);
	       /* if "song" is in playlist pl, we remove it and add
		  the "oldsong" instead (this way round we don't have
		  to worry about changing md5 hash entries */
	       if (remove_song_from_playlist (pl, song))
	       {
		   if (!song_is_in_playlist (pl, oldsong))
		       add_song_to_playlist (pl, oldsong, TRUE);
	       }
	   }
	   /* remove song from MPL, i.e. from the ipod */
	   remove_song_from_playlist (NULL, song);
	   removed = TRUE;
       }
       ++delsong_nr; /* count duplicate songs */
   }
}


/* delete all md5 checkums from the songs */
void clear_md5_hash_from_songs (void)
{
    GList *l;

    for (l = songs; l; l = l->next)
    {
	C_FREE (((Song *)l->data)->md5_hash);
    }
}


/* return the address of the UTF8 field @s_item. @s_item is one of
 * (the applicable) S_* defined in song.h */
gchar **song_get_item_pointer_utf8 (Song *song, S_item s_item)
{
    gchar **result = NULL;

    if (song) switch (s_item)
    {
    case S_ALBUM:
	result = &song->album;
	break;
    case S_ARTIST:
	result = &song->artist;
	break;
    case S_TITLE:
	result = &song->title;
	break;
    case S_GENRE:
	result = &song->genre;
	break;
    case S_COMMENT:
	result = &song->comment;
	break;
    case S_COMPOSER:
	result = &song->composer;
	break;
    case S_FDESC:
	result = &song->fdesc;
	break;
    case S_IPOD_PATH:
	result = &song->ipod_path;
	break;
    default:
	result = NULL;
    }
    return result;
}

/* return the UTF8 item @s_item. @s_item is one of
(the * applicable) S_* defined in song.h */
gchar *song_get_item_utf8 (Song *song, S_item s_item)
{
    gchar **address = song_get_item_pointer_utf8 (song, s_item);

    if (address) return *address;
    else         return NULL;
}


/* return the address of the UTF16 field @s_item. @s_item is one of
(the * applicable) S_* defined in song.h */
gunichar2 **song_get_item_pointer_utf16 (Song *song, S_item s_item)
{
    gunichar2 **result = NULL;

    if (song) switch (s_item)
    {
    case S_ALBUM:
	result = &song->album_utf16;
	break;
    case S_ARTIST:
	result = &song->artist_utf16;
	break;
    case S_TITLE:
	result = &song->title_utf16;
	break;
    case S_GENRE:
	result = &song->genre_utf16;
	break;
    case S_COMMENT:
	result = &song->comment_utf16;
	break;
    case S_COMPOSER:
	result = &song->composer_utf16;
	break;
    case S_FDESC:
	result = &song->fdesc_utf16;
	break;
    case S_IPOD_PATH:
	result = &song->ipod_path_utf16;
	break;
    default:
	result = NULL;
    }
    return result;
}

/* return the UTF16 item @s_item. @s_item is one of
(the * applicable) S_* defined in song.h */
gunichar2 *song_get_item_utf16 (Song *song, S_item s_item)
{
    gunichar2 **address = song_get_item_pointer_utf16 (song, s_item);

    if (address) return *address;
    else         return NULL;
}



/* ------------------------------------------------------------------- */
/* functions used by itunesdb (so we can refresh the display during
 * import */
gboolean it_add_song (Song *song)
{
    static gint count = 0;
    Song *result = add_song (song);
    ++count;
    if ((count % 20) == 0)     gtkpod_songs_statusbar_update();
    while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
    if (result) return TRUE;
    else        return FALSE;
}

Song *it_get_song_by_nr (guint32 n)
{
    Song *song = get_song_by_nr (n);
    while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
    return song;
}
