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
#include <unistd.h>
#include <string.h>
#include "prefs.h"
#include "song.h"
#include "misc.h"
#include "id3_tag.h"
#include "mp3file.h"
#include "support.h"
#include "md5.h"
#include "itunesdb.h"
#include "display.h"

/* only used when reading extended info from file */
struct song_extended_info
{
    guint ipod_id;
    gchar *pc_path_locale;
    gchar *pc_path_utf8;
    gchar *md5_hash;
    gchar *hostname;
    gchar *ipod_path;
    gboolean transferred;
};

/* List with all the songs */
GList *songs = NULL;
/* List with songs pending deletion */
static GList *pending_deletion = NULL;
/* Flag to indicate if it's safe to quit (i.e. all songs exported or
   at least a offline database written). It's state is changed in
   handle_export() and add_song_by_filename(), as well as the
   xy_cell_edited() functions in display.c.
   It's state can be accessed by the public function
   file_are_saved(). It can be set to FALSE by calling
   data_changed() */
static gboolean files_saved = TRUE;

static guint32 free_ipod_id (guint32 id);

/* Used to keep the "extended information" until the iTunesDB is 
   loaded */
static GHashTable *extendedinfohash = NULL;
static gboolean read_extended_info (gchar *name, gchar *itunes);

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


/* Used by set_entry_from_filename() */
static void set_entry (gchar **entry_utf8, gunichar2 **entry_utf16, gchar *str)
{
  C_FREE (*entry_utf8);
  C_FREE (*entry_utf16);
  *entry_utf8 = str;
  *entry_utf16 = g_utf8_to_utf16 (*entry_utf8, -1, NULL, NULL, NULL);
}

/* Set entry "column" (SM_COLUMN_TITLE etc) according to filename */
/* TODO: make the TAG extraction more intelligent -- if possible, this
   should be user configurable. */
static void set_entry_from_filename (Song *song, gint column)
{
  gchar *str;

  if (song->pc_path_utf8 && strlen (song->pc_path_utf8))
    {
      switch (column)
	{
	case SM_COLUMN_TITLE:
	  str = g_path_get_basename (song->pc_path_utf8);
	  set_entry (&song->title, &song->title_utf16, str);
	  break;
	case SM_COLUMN_ALBUM:
	  str = g_path_get_basename (song->pc_path_utf8);
	  set_entry (&song->album, &song->album_utf16, str);
	  break;
	case SM_COLUMN_ARTIST:
	  str = g_path_get_basename (song->pc_path_utf8);
	  set_entry (&song->artist, &song->artist_utf16, str);
	  break;
	}	  
    }
}


/* Append song to the list */
/* Note: adding to the display model is the responsibility of
   the playlist code */
gboolean add_song (Song *song)
{
  gint sz = sizeof (gunichar2);
  gboolean result = FALSE;
  gint ipod_id;
  gchar *str;
  struct song_extended_info *sei;

  /* fill in additional information from the extended info hash */
  if (extendedinfohash && song->ipod_id)
    {
      ipod_id = song->ipod_id;
      sei = g_hash_table_lookup (extendedinfohash, &ipod_id);
      if (sei) /* found info for this id! */
	{
	  if (sei->pc_path_locale && !song->pc_path_locale)
	      song->pc_path_locale = g_strdup (sei->pc_path_locale);
	  if (sei->pc_path_utf8 && !song->pc_path_utf8)
	      song->pc_path_utf8 = g_strdup (sei->pc_path_utf8);
	  if (sei->md5_hash && !song->md5_hash)
	      song->md5_hash = g_strdup (sei->md5_hash);
	  if (sei->hostname && !song->hostname)
	      song->hostname = g_strdup (sei->hostname);
	  song->transferred = sei->transferred;
	  g_hash_table_remove (extendedinfohash, &ipod_id);
	}
    }

  if((str = md5_song_exists_on_ipod(song)))
  {
    gtkpod_warning (_("Song (%s) already exists on iPod! (%s)\n"), song->pc_path_utf8, str);
    free_song(song);
  }
  else
  {
    /* Make sure all strings are initialised -- that way we don't 
     have to worry about it when we are handling the strings */
    /* exception: md5_hash and hostname: these may be NULL. */
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
    
    if(!song->ipod_id) 
	song->ipod_id = free_ipod_id (0);  /* keep track of highest ID used */
    else
	song->ipod_id = free_ipod_id(song->ipod_id);
    
    songs = g_list_append (songs, song);
    result = TRUE;
  }
  return(result);
}

/**
 * remove_song_from_ipod_by_id - in order to delete a song from the system
 * we need to keep track of the Songs we want to delete next time we export
 * the id. 
 * @id - the Song id we want to delete
 */
void
remove_song_from_ipod_by_id(guint32 id)
{
    if(id > 50)
    {
	Song *song = NULL;
	if((song = get_song_by_id(id)))
	{
	    if (song->transferred)
	    {
		songs = g_list_remove(songs, song);
		pending_deletion = g_list_append(pending_deletion, song);
		if(cfg->md5songs)
		    md5_song_removed_from_ipod(song);
	    }
	    else
	    {
		remove_song (song);
	    }
	}
    }
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
  gchar str[PATH_MAX];

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
      /* to make g_filename_to_utf8 () work with filenames in latin1 etc.
         export G_BROKEN_FILENAMES=1 in your shell.  See README for details. */
      song->pc_path_utf8 = g_filename_to_utf8 (name, -1, NULL, NULL, NULL);
      song->pc_path_locale = g_strdup (name);
      if (filetag->album)
	{
	  song->album = filetag->album;
	  song->album_utf16 = g_utf8_to_utf16 (song->album, -1, NULL, NULL, NULL);
	}
      else set_entry_from_filename (song, SM_COLUMN_ALBUM);
      if (filetag->artist)
	{
	  song->artist = filetag->artist;
	  song->artist_utf16 = g_utf8_to_utf16 (song->artist, -1, NULL, NULL, NULL);
	}
      else set_entry_from_filename (song, SM_COLUMN_ARTIST);
      if (filetag->title)
	{
	  song->title = filetag->title;
	  song->title_utf16 = g_utf8_to_utf16 (song->title, -1, NULL, NULL, NULL);
	}
      else set_entry_from_filename (song, SM_COLUMN_TITLE);
      if (filetag->genre)
	{
	  song->genre = filetag->genre;
	  song->genre_utf16 = g_utf8_to_utf16 (song->genre, -1, NULL, NULL, NULL);
	}
      if (filetag->comment)
	{
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
      song->ipod_id = 0;
      song->transferred = FALSE;
      if (gethostname (str, PATH_MAX-2) == 0)
	{
	  str[PATH_MAX-1] = 0;
	  song->hostname = g_strdup (str);
	}
      if(add_song (song))                   /* add song to memory */
      {
	  /* add song to master playlist */
	  add_song_to_playlist (NULL, song);
	  /* indicate that non-transferred files exist */
	  data_changed ();
      }
    }
  g_free (filetag);
  return TRUE;
}


/* Remove song from the list. */
void remove_song (Song *song)
{
  /*  remove_song_from_model (song); Must be done by playlist handling! */
  songs = g_list_remove (songs, song);
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
  Song  *song;
  GList *gl_song;
  gchar *filename = NULL;
  gboolean result = TRUE;
  
  /* lets clean up those pending deletions */
  for(gl_song = pending_deletion; gl_song; gl_song = gl_song->next)
  {
      song = (Song*)gl_song->data;
      if((filename = get_song_name_on_disk(song)))
      {
	  if(g_strstr_len(filename, strlen(cfg->ipod_mount), cfg->ipod_mount))
	  {
	      remove(filename);
/*	      fprintf(stderr, "Removed %s-%s(%d)\n%s\n", song->artist,
						    song->title, song->ipod_id,
						    filename);*/
	  }
	  g_free(filename);
      }
      free_song(song);
      gl_song->data = NULL;
  }
  g_list_free(pending_deletion);
  pending_deletion = NULL;  

  /* we now have as much space as we're gonna have, copy files to ipod */
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

/* Used to free the memory of hash data */
static void hash_delete (gpointer data)
{
    struct song_extended_info *sei = data;

    if (sei)
    {
	C_FREE (sei->pc_path_locale);
	C_FREE (sei->pc_path_utf8);
	C_FREE (sei->md5_hash);
	C_FREE (sei->hostname);
	C_FREE (sei->ipod_path);
	g_free (sei);
    }
}

static void destroy_extendedinfohash (void)
{
    if (extendedinfohash)
	g_hash_table_destroy (extendedinfohash);
    extendedinfohash = NULL;
}

/* Read extended info from "name" and check if "itunes" is the
   corresponding iTunesDB (using the itunes_hash value in "name").
   The data is stored in a hash table with the ipod_id as key.
   This hash table is used by add_song() to fill in missing information */
/* Return TRUE on success, FALSE otherwise */
static gboolean read_extended_info (gchar *name, gchar *itunes)
{
    gchar *md5, buf[PATH_MAX], *arg, *line, *bufp;
    gboolean success = TRUE;
    gboolean expect_hash;
    gint len;
    struct song_extended_info *sei = NULL;
    FILE *fp, *fpit;


    fpit = fopen (itunes, "r");
    if (!fpit)
    {
	gtkpod_warning (_("Could not open \"%s\" for reading extended info.\n"),
			itunes);
	return FALSE;
    }
    md5 = md5_hash_on_file (fpit);
    fclose (fpit);
    if (!md5)
    {
	fprintf (stderr, "Programming error: Could not create hash value from itunesdb\n");
	return FALSE;
    }
    fp = fopen (name, "r");
    if (!fp)
    {
	gtkpod_warning (_("Could not open \"%s\" for reading extended info.\n"),
			name);
	g_free (md5);
	return FALSE;
    }
    /* Create hash table */
    if (extendedinfohash) destroy_extendedinfohash ();
    extendedinfohash = g_hash_table_new_full (g_int_hash, g_int_equal,
					      NULL, hash_delete);
    expect_hash = TRUE; /* next we expect the hash value (checksum) */
    while (success && fgets (buf, PATH_MAX, fp))
    {
	/* allow comments */
	if ((buf[0] == ';') || (buf[0] == '#')) continue;
	arg = strchr (buf, '=');
	if (!arg || (arg == buf))
	{
	    gtkpod_warning (_("Error while reading extended info: %s\n"), buf);
	    continue;
	}
	/* skip whitespace (isblank() is a GNU extension... */
	bufp = buf;
	while ((*bufp == ' ') || (*bufp == 0x09)) ++bufp;
	line = g_strndup (buf, arg-bufp);
	++arg;
	len = strlen (arg); /* remove newline */
	if((len>0) && (arg[len-1] == 0x0a))  arg[len-1] = 0;
	if (expect_hash)
	{
	    if(g_ascii_strcasecmp (line, "itunesdb_hash") == 0)
	    {
		if (strcmp (arg, md5) != 0)
		{
		    gtkpod_warning (_("iTunesDB (%s)\ndoes not match checksum in extended information file (%s)\n"), itunes, name);
		    success = FALSE;
		    break;
		}
		else expect_hash = FALSE;
	    }
	    else
	    {
		gtkpod_warning (_("%s:\nExpected \"itunesdb_hash=\" but got:\"%s\"\n"), name, buf);
		success = FALSE;
		break;
	    }
	}
	else
	    if(g_ascii_strcasecmp (line, "id") == 0)
	    { /* found new id */
		if (sei)
		{
		    if (sei->ipod_id != 0)
		    { /* normal extended information */
			g_hash_table_insert (extendedinfohash,
					     &sei->ipod_id, sei);
		    }
		    else
		    { /* this is a deleted song that hasn't yet been
		         removed from the iPod's hard drive */
			Song *song = g_malloc0 (sizeof (Song));
			song->ipod_path = g_strdup (sei->ipod_path);
			pending_deletion = g_list_append (pending_deletion,
							  song);
			hash_delete ((gpointer)sei); /* free sei */
		    }
		    sei = NULL;
		}
		if (strcmp (arg, "xxx") != 0)
		{
		    sei = g_malloc0 (sizeof (struct song_extended_info));
		    sei->ipod_id = atoi (arg);
		}
	    }
	    else if (sei == NULL)
	    {
		gtkpod_warning (_("%s:\nFormat error:%s\n"), name, buf);
		success = FALSE;
		break;
	    }
	    else if (g_ascii_strcasecmp (line, "hostname") == 0)
		sei->hostname = g_strdup (arg);
	    else if (g_ascii_strcasecmp (line, "filename_locale") == 0)
		sei->pc_path_locale = g_strdup (arg);
	    else if (g_ascii_strcasecmp (line, "filename_utf8") == 0)
		sei->pc_path_utf8 = g_strdup (arg);
	    else if (g_ascii_strcasecmp (line, "md5_hash") == 0)
		sei->md5_hash = g_strdup (arg);
	    else if (g_ascii_strcasecmp (line, "transferred") == 0)
		sei->transferred = atoi (arg);
	    else if (g_ascii_strcasecmp (line, "filename_ipod") == 0)
		sei->ipod_path = g_strdup (arg);
    }
    g_free (md5);
    fclose (fp);
    if (!success) destroy_extendedinfohash ();
    return success;
}


/* Handle the function "Import iTunesDB" */
void handle_import (void)
{
    gchar *name1, *name2, *cfgdir;
    gboolean success;
    guint32 n,i;
    Song *song;

    if (itunes_import_ok () == FALSE)
    {
	gtkpod_warning (_("You cannot import an iTunesDB again!\n"));
	return;
    }

    n = get_nr_of_songs (); /* how many songs are alread there? */

    if (!cfg->offline)
    { /* iPod is connected */
	if (prefs_get_write_extended_info())
	{
	    name1 = concat_dir (cfg->ipod_mount,
				"iPod_Control/iTunes/iTunesDB.ext");
	    name2 = concat_dir (cfg->ipod_mount,
				"iPod_Control/iTunes/iTunesDB");
	    success = read_extended_info (name1, name2);
	    g_free (name1);
	    g_free (name2);
	    if (!success) 
	    {
		gtkpod_warning (_("Extended info will not be used.\n"));
	    }
	}
	itunesdb_parse (cfg->ipod_mount);
    }
    else
    { /* offline - requires extended info */
	if ((cfgdir = prefs_get_cfgdir ()))
	{
	    name1 = concat_dir (cfgdir, "/iTunesDB.ext");
	    name2 = concat_dir (cfgdir, "iTunesDB");
	    success = read_extended_info (name1, name2);
	    g_free (name1);
	    if (!success) 
	    {
		gtkpod_warning (_("Extended info will not be used. If you have non-transferred songs,\nthese will be lost.\n"));
	    }
	    itunesdb_parse_file (name2);
	    g_free (name2);
	    g_free (cfgdir);
	}
	else
	{
	    gtkpod_warning (_("Import aborted.\n"));
	    return;
	}
    }

    destroy_extendedinfohash (); /* delete hash information (if available) */

    if (itunes_import_ok () == FALSE)
    { /* Import was successfull, block menu item and button */
	disable_gtkpod_import_buttons();
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
    /* if(cfg->md5songs)    done with add_song ();
       unique_file_repository_init(get_song_list()); */
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
	    guint i = 0, size = 0;
	    result = concat_dir(cfg->ipod_mount, s->ipod_path);
	    size = strlen(result);
	    for(i = 0; i < size; i++)
		if(result[i] == ':') result[i] = '/';
	}
	else if((s->pc_path_locale) && (strlen(s->pc_path_locale) > 0))
	{
	    result = g_strdup_printf("%s",s->pc_path_locale);
	} 
    }
    return(result);
}

/**
 * get_preferred_song_name_format - useful for generating the preferred
 * output filename for any track.  
 * FIXME Eventually this should check your prefs for the displayed
 * attributes in the song model and generate track names based on that
 * @s - The Song reference we're generating the filename for
 * Returns - The preferred filename, you must free it yourself.
 */
gchar *
get_preferred_song_name_format(Song *s)
{
    gchar buf[PATH_MAX];
    gchar *result = NULL;
    if(s)
    {
	if(s->track_nr < 10)
	    snprintf(buf, PATH_MAX, "0%d-%s-%s.mp3", s->track_nr,
						s->title,s->artist);
	else
	    snprintf(buf, PATH_MAX, "%d-%s-%s.mp3", s->track_nr,
						s->title,s->artist);
	result = g_strdup(buf);
    }
    return(result);
}


/* Writes extended info (md5 hash, PC-filename...) into file "name".
   "itunes" is the filename of the corresponding iTunesDB */
static gboolean write_extended_info (gchar *name, gchar *itunes)
{
  FILE *fp, *fpit;
  guint n,i;
  Song *song;
  gchar *md5;

  fp = fopen (name, "w");
  if (!fp)
    {
      gtkpod_warning (_("Could not open \"%s\" for writing extended info.\n"),
		      name);
      return FALSE;
    }
  fpit = fopen (itunes, "r");
  if (!fpit)
    {
      gtkpod_warning (_("Could not open \"%s\" for writing extended info.\n"),
		      itunes);
      fclose (fp);
      return FALSE;
    }
  md5 = md5_hash_on_file (fpit);
  fclose (fpit);
  if (md5)
    {
      fprintf(fp, "itunesdb_hash=%s\n", md5);
      g_free (md5);
    }
  else
    {
      fprintf (stderr, "Programming error: Could not create hash value from itunesdb\n");
      fclose (fp);
      return FALSE;
    }
  n = get_nr_of_songs ();
  for (i=0; i<n; ++i)
    {
      song = get_song_by_nr (i);
      fprintf (fp, "id=%d\n", song->ipod_id);
      if (song->hostname)
	fprintf (fp, "hostname=%s\n", song->hostname);
      if (strlen (song->pc_path_locale) != 0)
	fprintf (fp, "filename_locale=%s\n", song->pc_path_locale);
      if (strlen (song->pc_path_utf8) != 0)
	fprintf (fp, "filename_utf8=%s\n", song->pc_path_utf8);
      if (song->md5_hash)
	fprintf (fp, "md5_hash=%s\n", song->md5_hash);
      fprintf (fp, "transferred=%d\n", song->transferred);
    }
  if (prefs_get_offline())
  { /* we are offline and also need to export the list of songs that
       are to be deleted */
      GList *gl_song;
      for(gl_song = pending_deletion; gl_song; gl_song = gl_song->next)
      {
	  song = (Song*)gl_song->data;
	  fprintf (fp, "id=000\n");  /* our sign for songs pending
					deletion */
	  fprintf (fp, "filename_ipod=%s\n", song->ipod_path);
      }
  }
  fprintf (fp, "id=xxx\n");
  fclose (fp);
  return TRUE;
}


/* used to handle export of database */
void handle_export (void)
{
  gchar *ipt, *ipe, *cft=NULL, *cfe=NULL, *cfgdir;
  gboolean success = TRUE;

  cfgdir = prefs_get_cfgdir ();
  ipt = concat_dir (cfg->ipod_mount, "iPod_Control/iTunes/iTunesDB");
  ipe = concat_dir (cfg->ipod_mount, "iPod_Control/iTunes/iTunesDB.ext");
  if (cfgdir)
    {
      cft = concat_dir (cfgdir, "iTunesDB");
      cfe = concat_dir (cfgdir, "iTunesDB.ext");
    }

  if(!prefs_get_offline ())
    {
      /* write songs to iPod */
      if (!(success=flush_songs ()))
	  gtkpod_warning (_("Could not write songs to iPod. Export aborted!\n"));
      /* else: write iTunesDB to iPod */
      else if (!(success=itunesdb_write (cfg->ipod_mount)))
	  gtkpod_warning (_("Error writing iTunesDB to iPod. Export aborted!\n"));
      /* else: write extended info (PC filenames, md5 hash) to iPod */
      else if (prefs_get_write_extended_info ())
	{
	  if(!(success = write_extended_info (ipe, ipt)))
	    gtkpod_warning (_("Extended information not written\n"));
	}
      /* else: delete extended information file, if it exists */
      else if (g_file_test (ipe, G_FILE_TEST_EXISTS))
	{
	  if (remove (ipe) != 0)
	    {
	      gtkpod_warning (_("Could not delete extended information file: \"%s\"\n"), ipe);
	    }
	}
      /* if everything was successful, copy files to ~/.gtkpod */
      if (success && prefs_get_keep_backups ())
	{
	  if (cfgdir)
	    {
	      success = cp (ipt, cft);
	      if (success && prefs_get_write_extended_info())
		success = cp (ipe, cfe);
	    }
	  if ((cfgdir == NULL) || (!success))
	    gtkpod_warning (_("Backups could not be created!\n"));
	}
      if (success && !prefs_get_keep_backups() && cfgdir)
	cleanup_backup_and_extended_files ();
    }
  else
    { /* we are offline -> only write database to ~/.gtkpod */
      /* offline implies "extended information" */
      if (cfgdir)
	{
	  success = itunesdb_write_to_file (cft);
	  if (success)
	    success = write_extended_info (cfe, cft);
	}
      if ((cfgdir == NULL) || (!success))
	{
	  gtkpod_warning (_("Export not successful!\n"));
	  success = FALSE;
	}
    }

  /* indicate that files and/or database is saved */
  if (success)   files_saved = TRUE;

  C_FREE (cfgdir);
  C_FREE (cft);
  C_FREE (cfe);
  C_FREE (ipt);
  C_FREE (ipe);
}


/* make state of "files_saved" available to functions */
gboolean files_are_saved (void)
{
  return files_saved;
}

/* set the state of "files_saved" to FALSE */
void data_changed (void)
{
  files_saved = FALSE;
}
