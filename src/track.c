/*
|  Copyright (C) 2002 Jorg Schuler <jcsjcs at sourceforge.net>
|  Part of the gtkpod project.
| 
|  URL: 
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
#include "prefs.h"
#include "song.h"
#include "misc.h"
#include "id3_tag.h"
#include "mp3file.h"
#include "support.h"
#include "md5.h"
#include "itunesdb.h"
#include "display.h"

GList *songs = NULL;

static guint32 free_ipod_id (guint32 id);

/* Append song to the list */
/* Note: adding to the display model is the responsibility of
   the playlist code */
gboolean add_song (Song *song)
{
  gint sz = sizeof (gunichar2);
  gboolean result = FALSE;
    
  if(song_exists_on_ipod(song))
  {
    fprintf(stderr, "song already exists on ipod !!!\n");
    remove_song(song);
  }
  else
  {
    /* Make sure all strings are initialised -- that way we don't 
     have to worry about it when we are handling the strings */
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
    
    song->ipod_id = free_ipod_id (0);  /* keep track of highest ID used */
    
    songs = g_list_append (songs, song);
    result = TRUE;
  }
  return(result);
}


/* Add all files in directory and subdirectories.
   If name is a regular file, just add that.
   If name == NULL, just return */
/* Not nice: the return value has no meaning */
gboolean add_directory_recursively (gchar *name)
{
  GDir *dir;
  G_CONST_RETURN gchar *next;
  gchar *nextfull;
  gboolean result = TRUE;

  if (name == NULL) return TRUE;
  if (g_file_test (name, G_FILE_TEST_IS_REGULAR))
    return (add_song_by_filename (name));
  if (g_file_test (name, G_FILE_TEST_IS_DIR)) {
    dir = g_dir_open (name, 0, NULL);
    if (dir != NULL) {
      do {
	next = g_dir_read_name (dir);
	if (next != NULL)
	  {
	    nextfull = concat_dir (name, next);
	    result &= add_directory_recursively (nextfull);
	    g_free (nextfull);
	  }
      } while (next != NULL);
      g_dir_close (dir);
    }
  }
  return TRUE;
}



/* Append file "name" to the list of songs.
   "name" is in the current locale */
/* Not nice: currently only accepts files ending on .mp3 */
gboolean add_song_by_filename (gchar *name)
{
  Song *song;
  File_Tag *filetag;
  gint len;

  if (name == NULL) return TRUE;

  if (g_file_test (name, G_FILE_TEST_IS_DIR)) {
    add_directory_recursively (name);
    return TRUE;
  }

  if (g_file_test (name, G_FILE_TEST_IS_REGULAR) == FALSE) return FALSE;

  /* check if filename ends on ".mp3" */
  len = strlen (name);
  if (len < 4) return FALSE;
  if (strcmp (&name[len-4], ".mp3") != 0) return FALSE;

  filetag = g_malloc0 (sizeof (File_Tag));
  if (Id3tag_Read_File_Tag (name, filetag) == TRUE)
    {
      song = g_malloc0 (sizeof (Song));
      if (filetag->album) {
	song->album = filetag->album;
	song->album_utf16 = g_utf8_to_utf16 (song->album, -1, NULL, NULL, NULL);
      }
      if (filetag->artist) {
	song->artist = filetag->artist;
	song->artist_utf16 = g_utf8_to_utf16 (song->artist, -1, NULL, NULL, NULL);
      }
      if (filetag->title) {
	song->title = filetag->title;
	song->title_utf16 = g_utf8_to_utf16 (song->title, -1, NULL, NULL, NULL);
      }
      if (filetag->genre) {
	song->genre = filetag->genre;
	song->genre_utf16 = g_utf8_to_utf16 (song->genre, -1, NULL, NULL, NULL);
      }
      if (filetag->comment) {
	song->comment = filetag->comment;
	song->comment_utf16 = g_utf8_to_utf16 (song->comment, -1, NULL, NULL, NULL);
      }

      if (filetag->year == NULL)
	{
	  song->year = 0;
	}
      else
	{
	  song->year = atoi(filetag->year);
	  g_free (filetag->year);
	}
      if (filetag->track == NULL)
	{
	  song->track_nr = 0;
	}
      else
	{
	  song->track_nr = atoi(filetag->track);
	  g_free (filetag->track);
	}
      if (filetag->track_total == NULL)
	{
	  song->tracks = 0;
	}
      else
	{
	  song->tracks = atoi(filetag->track_total);
	  g_free (filetag->track_total);
	}
      song->songlen = filetag->songlen;
      song->size = filetag->size;
      if (song->songlen == 0) {
	song->songlen = length_from_file (name, song->size);
	if (song->songlen == 0) {
	  /* Songs with zero play length are ignored by iPod... */
	  gtkpod_warning (_("File \"%s\" has zero play length. Ignoring.\n"),
			 name);
	  g_free (filetag);
	  g_free (song);
	  return FALSE;
	}
      }
      /* to make g_filename_to_utf8 () work with filenames in latin1 etc.
         export G_BROKEN_FILENAMES=1 in your shell.  See README for details. */
      song->pc_path_utf8 = g_filename_to_utf8 (name, -1, NULL, NULL, NULL);
      song->pc_path_locale = g_strdup (name);
      song->ipod_id = 0;
      song->transferred = FALSE;
      if(add_song (song))                   /* add song to memory */
      {
	  add_song_to_playlist (NULL, song);  
	  /* add song to master playlist */
      }
    }
  g_free (filetag);
  return TRUE;
}


/* Remove song from the list. First it's removed from any display
   model using remove_song_from_model (), then the entry itself
   is removed from the GList *songs */
void remove_song (Song *song)
{
  /*  remove_song_from_model (song); Must be done by playlist handling! */
  songs = g_list_remove (songs, song);
  if (song->album)            g_free (song->album);
  if (song->artist)           g_free (song->artist);
  if (song->title)            g_free (song->title);
  if (song->genre)            g_free (song->genre);
  if (song->comment)          g_free (song->comment);
  if (song->composer)         g_free (song->composer);
  if (song->fdesc)            g_free (song->fdesc);
  if (song->album_utf16)      g_free (song->album_utf16);
  if (song->artist_utf16)     g_free (song->artist_utf16);
  if (song->title_utf16)      g_free (song->title_utf16);
  if (song->genre_utf16)      g_free (song->genre_utf16);
  if (song->comment_utf16)    g_free (song->comment_utf16);
  if (song->composer_utf16)   g_free (song->composer_utf16);
  if (song->fdesc_utf16)      g_free (song->fdesc_utf16);
  if (song->pc_path_utf8)     g_free (song->pc_path_utf8);
  if (song->pc_path_locale)   g_free (song->pc_path_locale);
  if (song->ipod_path)        g_free (song->ipod_path);
  if (song->ipod_path_utf16)  g_free (song->ipod_path_utf16);
  g_free (song);
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


/* Return a pointer to the song list, so other modules can
   read!-access this information */
GList *get_song_list (void)
{
  return songs;
}


/* Returns the number of songs stored in the list */
guint get_nr_of_songs (void)
{
  return g_list_length (songs);
}


/* Returns the n_th song */
Song *get_song_by_nr (guint32 n)
{
  return g_list_nth_data (songs, n);
}


/* This code needs explaining...
   Function used by get_song_by_id() to compare 
   song->ipod_id with the id currently searched for */
static gint get_song_by_id_comp_func (gconstpointer song, gconstpointer id)
{
  return (((Song *)song)->ipod_id - *((guint32 *)id));
}

/* Returns the song with ID "id" */
Song *get_song_by_id (guint32 id)
{
  GList *song_l;

  song_l = g_list_find_custom (songs, &id, get_song_by_id_comp_func);
  if (song_l == NULL) return NULL;
  return (Song *)song_l->data;
}



/* Flushes all non-transferred songs to the iPod filesystem
   Returns TRUE on success, FALSE if some error occured */
gboolean flush_songs (void)
{
  GList *gl_song;
  Song  *song;
  gboolean result = TRUE;

  gl_song = g_list_first (songs);
  while (gl_song != NULL) {
    song = (Song *)gl_song->data;
    result &= copy_song_to_ipod (cfg->ipod_mount, song, song->pc_path_locale);
    gl_song = g_list_next (gl_song);
  }
  return result;
}


/* Checks if it is OK to import an iTunesDB. It's not OK, if
   some of the songs have already been transferred to the iPod
   (i.e. because an iTunesDB had been imported before) */
static gboolean itunes_import_ok (void)
{
  GList *gl_song;
  Song  *song;

  gl_song = g_list_first (songs);
  while (gl_song != NULL) {
    song = (Song *)gl_song->data;
    if (song->transferred) break;
    gl_song = g_list_next (gl_song);
  }
  if (gl_song != NULL) return FALSE;
  return TRUE;
}


/* Handle the function "Import iTunesDB" */
void handle_import_itunes (void)
{
  guint32 n,i;
  Song *song;

  n = get_nr_of_songs ();

  if (itunes_import_ok () == FALSE)
    {
      gtkpod_warning (_("You cannot import an iTunesDB again!\n"));
    }
  else
    {
      itunesdb_parse (cfg->ipod_mount);
      if (itunes_import_ok () == FALSE)
	{ /* Import was successfull, block menu item and button */
	  ;
	}
      /* We need to make sure that the songs that already existed
	 in the DB when we called itunesdb_parse() do not duplicate
	 any existing ID */
      for (i=0; i<n; ++i)
	{
	  song = get_song_by_nr (i);
	  song->ipod_id = free_ipod_id (0);
	  /*we need to tell the display that the ID has changed */
	  pm_song_changed (song);
	}
	
      /* setup our md5 hashness for unique files */
      if(cfg->md5songs)
	unique_file_repository_init(get_song_list());
    }
}


gboolean write_tags_to_file (Song *song)
{
    File_Tag *filetag;
    gint i, len;
    gchar *ipod_file, *ipod_fullpath;

    filetag = g_malloc0 (sizeof (File_Tag));
    filetag->album = song->album;
    filetag->artist = song->artist;
    filetag->title = song->title;
    filetag->genre = song->genre;
    filetag->comment = song->comment;
    if (song->pc_path_locale && strlen (song->pc_path_locale) > 0)
      {
	if (Id3tag_Write_File_Tag (song->pc_path_locale, filetag) == FALSE)
	  {
	    gtkpod_warning (_("Couldn't change tags of file: %s\n"),
			    song->pc_path_locale);
	  }
      }
    if (song->transferred &&
	song->ipod_path &&
	g_utf8_strlen (song->ipod_path, -1) > 0)
      {
	/* need to get ipod filename */
	ipod_file = g_locale_from_utf8 (song->ipod_path, -1, NULL, NULL, NULL);
	/* FIXME? locale might be utf8 again, but filename should always be
	   "US-ASCII", so we could just forget about utf8_to_locale() */
	len = strlen (ipod_file);
	for (i=0; i<len; ++i)     /* replace ':' by '/' */
	  if (ipod_file[i] == ':')  ipod_file[i] = '/';
	/* ipod_file+1: ipod_file always starts with ":", i.e. "/" */
	ipod_fullpath = concat_dir (cfg->ipod_mount, ipod_file+1);
	if (Id3tag_Write_File_Tag (ipod_fullpath, filetag) == FALSE)
	  {
	    gtkpod_warning (_("Couldn't change tags of file: %s\n"),
			    ipod_fullpath);
	  }
	g_free (ipod_file);
	g_free (ipod_fullpath);
      }
    g_free (filetag);
    return(TRUE);
}    


/*  Call with 0 to get a unused ID. It will be registered.
    Call with anything else to register a used ID. The highest ID use
    will then be returned */
/* Called each time by add_song with the ipod_id used.
   That's how it keeps track of the largest ID used. */
static guint32 free_ipod_id (guint32 id)
{
  static gint32 max_id = 52; /* the lowest valid ID is 53 */

  if (id > max_id)     max_id = id;
  if (id == 0)   ++max_id;
  return max_id;
}

/**
 * get_song_name_on_disk
 * Function to retrieve the filename on disk for the specified Song.  It
 * returns the valid filename whether the file has been copied to the ipod,
 * or has yet to be copied.  So it's useful for file operations on a song.
 * @s - The Song data structure we want the on disk file for
 * Returns - the filename for this Song
 */
gchar* get_song_name_on_disk(Song *s)
{
    gchar *result = NULL;
    if(s)
    {
	if((s->ipod_path) && (strlen(s->ipod_path) > 0))
	{
	    result = g_strdup_printf("%s%s",cfg->ipod_mount, s->ipod_path);
	}
	else if(strlen(s->pc_path_utf8) > 0)
	{
	    result = g_strdup_printf("%s",s->pc_path_utf8);
	} 
	if(result)
	{
	    guint i = 0, size = 0;
	    size = strlen(result);
	    for(i = 0; i < size; i++)
		if(result[i] == ':') result[i] = '/';
	}
    }
    return(result);

}

