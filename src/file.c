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

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "file.h"
#include "id3_tag.h"
#include "mp3file.h"
#include "md5.h"
#include "misc.h"
#include "prefs.h"
#include "support.h"
#include "charset.h"



/*------------------------------------------------------------------*\
 *                                                                  *
 *      Add Playlists                                               *
 *                                                                  *
\*------------------------------------------------------------------*/


/* Add all files specified in playlist @plfile. Will create a new
 * playlist with the name "basename (plfile)", even if one of the same
 * name already exists.
 * It will then add all songs listed in @plfile. If set in the prefs,
 * duplicates will be detected (and the song already present in the
 * database will be added to the playlist instead). */
gboolean add_playlist_by_filename (gchar *plfile)
{
    enum {
	PLT_M3U,   /* M3U playlist file */
	PLT_PLS,   /* PLS playlist file */
	PLT_MISC   /* something else -- assume it works the same as M3U */
    };
    gchar *plname, *bufp, *plfile_utf8;
    gchar buf[PATH_MAX];
    Playlist *plitem;
    gint type = PLT_MISC; /* type of playlist file */
    gint line, songs;
    FILE *fp;
    gboolean error;

    if (!plfile)  return TRUE;

    if (g_file_test (plfile, G_FILE_TEST_IS_REGULAR) == FALSE)
    {
	/* FIXME: Status */
	return FALSE;  /* definitely not! */
    }

    plfile_utf8 = charset_to_utf8 (plfile);

    plname = g_path_get_basename (plfile_utf8);
    bufp = g_strrstr (plname, "."); /* find last occurence of '.' */
    if (bufp)
    {
	*bufp = 0;          /* truncate playlist name */
	++bufp;
	if (g_utf8_collate (g_utf8_casefold (bufp, -1), "m3u") == 0)
	    type = PLT_M3U;
	else if (g_utf8_collate (g_utf8_casefold (bufp, -1), "pls") == 0)
	    type = PLT_PLS;
	else if (g_utf8_collate (g_utf8_casefold (bufp, -1), "mp3") == 0)
	{
	    /* FIXME: Status */
	    return FALSE;  /* definitely not! */
	}
	else if (g_utf8_collate (g_utf8_casefold (bufp, -1), "wav") == 0)
	{
	    /* FIXME: Status */
	    return FALSE;  /* definitely not! */
	}
    }

    /* attempt to open playlist file */
    if (!(fp = fopen (plfile, "r")))
    {
	/* FIXME: Status */
	return FALSE;  /* definitely not! */
    }
    /* create playlist */
    plitem = add_new_playlist (plname);

    /* for now assume that all playlist files will be line-based
       all of these are line based -- add different code for different
       playlist files */
    line = 0; /* nr of line being read */
    songs = 0; /* nr of songs added */
    error = FALSE;
    while (!error && fgets (buf, PATH_MAX, fp))
    {
	gchar *bufp = buf;
	gint len = strlen (bufp); /* remove newline */
	if((len>0) && (bufp[len-1] == 0x0a))  bufp[len-1] = 0;
	switch (type)
	{
	case PLT_MISC:
	    /* skip whitespace */
	    while (isspace (*bufp)) ++bufp;
	    /* assume comments start with ';' or '#' */
	    if ((*bufp == ';') || (*bufp == '#')) break;
	    /* assume the rest of the line is a filename */
	    if (add_song_by_filename (bufp, plitem)) ++songs;
	    break;
	case PLT_M3U:
	    /* comments start with '#' */
	    if (*bufp == '#') break;
	    /* assume the rest of the line is a filename */
	    if (add_song_by_filename (bufp, plitem)) ++songs;
	    break;
	case PLT_PLS:
	    /* I don't know anything about pls playlist files and just
	       looked at one example produced by xmms -- please
	       correct the code if you know more */ 
	    if (line == 0)
	    { /* check for "[playlist]" */
		if (strncasecmp (bufp, "[playlist]", 10) != 0)
		    error = TRUE;
	    }
	    else if (strncasecmp (bufp, "File", 4) == 0)
	    { /* looks like a file entry */
		bufp = strchr (bufp, '=');
		if (bufp) ++bufp;
		if (add_song_by_filename (bufp, plitem)) ++songs;
	    }
	    break;
	}
	++line;
    }
    fclose (fp);

    /* I don't think it's too interesting to pop up the list of
       duplicates -- but we should reset the list. */
    remove_duplicate (NULL, (void *)-1);
    return !error;
}



/*------------------------------------------------------------------*\
 *                                                                  *
 *      Add Dir                                                     *
 *                                                                  *
\*------------------------------------------------------------------*/

/* Add all files in directory and subdirectories.
   If @name is a regular file, just add that.
   If @name == NULL, just return
   If @plitem != NULL, add songs also to Playlist @plitem */
/* Not nice: the return value has not much meaning. TRUE: all files
 * were added successfully. FALSE: some files could not be
   added (e.g: duplicates)  */
gboolean add_directory_recursively (gchar *name, Playlist *plitem)
{
  GDir *dir;
  G_CONST_RETURN gchar *next;
  gchar *nextfull;
  gboolean result = TRUE;

  if (name == NULL) return TRUE;
  if (g_file_test (name, G_FILE_TEST_IS_REGULAR))
      return (add_song_by_filename (name, plitem));
  if (g_file_test (name, G_FILE_TEST_IS_DIR))
  {
      block_widgets ();
      dir = g_dir_open (name, 0, NULL);
      if (dir != NULL) {
	  do {
	      next = g_dir_read_name (dir);
	      if (next != NULL)
	      {
		  nextfull = concat_dir (name, next);
		  result &= add_directory_recursively (nextfull, plitem);
		  g_free (nextfull);
	      }
	  } while (next != NULL);
	  g_dir_close (dir);
      }
      release_widgets ();
  }
  return result;
}



/*------------------------------------------------------------------*\
 *                                                                  *
 *      Helper function for add_song_by_filename ()                 *
 *                                                                  *
\*------------------------------------------------------------------*/


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

    if (prefs_get_tag_autoset (column) &&
	song->pc_path_utf8 && strlen (song->pc_path_utf8))
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
	case SM_COLUMN_GENRE:
	    str = g_path_get_basename (song->pc_path_utf8);
	    set_entry (&song->genre, &song->genre_utf16, str);
	    break;
	}
    }
}


/*------------------------------------------------------------------*\
 *                                                                  *
 *      Add File                                                    *
 *                                                                  *
\*------------------------------------------------------------------*/

/* Append file @name to the list of songs.
   @name is in the current locale
   If @plitem != NULL, add song to plitem as well */
/* Not nice: currently only accepts files ending on .mp3 */
gboolean add_song_by_filename (gchar *name, Playlist *plitem)
{
  static gint count = 0; /* do a gtkpod_songs_statusbar_update() every
			    10 songs */
  Song *song, *added_song;
  File_Tag *filetag;
  gint len;
  gchar str[PATH_MAX];
  mp3metadata_t *mp3meta;

  if (name == NULL) return TRUE;

  if (g_file_test (name, G_FILE_TEST_IS_DIR))
  {
      return add_directory_recursively (name, NULL);
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
      song->pc_path_utf8 = charset_to_utf8 (name);
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
      else set_entry_from_filename (song, SM_COLUMN_GENRE);
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
      mp3meta = get_mp3metadata_from_file (name, song->size);
      if (mp3meta)
      {
	  song->bitrate = mp3meta->Bitrate;
	  if (song->songlen == 0)
	      song->songlen = mp3meta->PlayLength * 1000;
	  g_free (mp3meta);
      }
      if (song->songlen == 0)
      {
	  /* Songs with zero play length are ignored by iPod... */
	  gtkpod_warning (_("File \"%s\" has zero play length. Ignoring.\n"),
			 name);
	  g_free (filetag);
	  g_free (song);
	  while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
	  return FALSE;
      }
      song->ipod_id = 0;
      song->transferred = FALSE;
      if (gethostname (str, PATH_MAX-2) == 0)
	{
	  str[PATH_MAX-1] = 0;
	  song->hostname = g_strdup (str);
	}
      added_song = add_song (song);
      if(added_song)                   /* add song to memory */
      {
	  /* add song to master playlist (if it hasn't been done before) */
	  if (added_song == song) add_song_to_playlist (NULL, added_song);
	  /* add song to specified playlist */
	  if (plitem)  add_song_to_playlist (plitem, added_song);
	  /* indicate that non-transferred files exist */
	  data_changed ();
	  ++count;
	  if (count >= 10)  /* update every ten songs added */
	  {
	      gtkpod_songs_statusbar_update();
	      count = 0;
	  }
      }
    }
  g_free (filetag);
  while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
  return TRUE;
}


/*------------------------------------------------------------------*\
 *                                                                  *
 *      Write Tags                                                  *
 *                                                                  *
\*------------------------------------------------------------------*/

/* Write changed tags to file.
   "tag_id": specify which tags should be changed (one of
   S_... defined in song.h) */
gboolean write_tags_to_file (Song *song, gint tag_id)
{
    File_Tag *filetag;
    gint i, len;
    gchar *ipod_file, *ipod_fullpath, track[20];
    Song *oldsong;

    filetag = g_malloc0 (sizeof (File_Tag));
    if ((tag_id == S_ALL) || (tag_id = S_ALBUM))
	filetag->album = song->album;
    if ((tag_id == S_ALL) || (tag_id = S_ARTIST))
	filetag->artist = song->artist;
    if ((tag_id == S_ALL) || (tag_id = S_TITLE))
	filetag->title = song->title;
    if ((tag_id == S_ALL) || (tag_id = S_GENRE))
	filetag->genre = song->genre;
    if ((tag_id == S_ALL) || (tag_id = S_COMMENT))
	filetag->comment = song->comment;
    if ((tag_id == S_ALL) || (tag_id = S_TRACK_NR))
    {
	snprintf(track, 20, "%d", song->track_nr);
	filetag->track = track;
    }
    if (song->pc_path_locale && (strlen (song->pc_path_locale) > 0))
      {
	if (Id3tag_Write_File_Tag (song->pc_path_locale, filetag) == FALSE)
	  {
	    gtkpod_warning (_("Couldn't change tags of file: %s\n"),
			    song->pc_path_locale);
	  }
      }
    if (!prefs_get_offline () &&
	song->transferred &&
	song->ipod_path &&
	(g_utf8_strlen (song->ipod_path, -1) > 0))
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
    /* remove song from md5 hash and reinsert it (hash value has changed!) */
    md5_song_removed (song);
    C_FREE (song->md5_hash);  /* need to remove the old value manually! */
    oldsong = md5_song_exists (song);
    if (oldsong) { /* song exists, remove and register the new version */
	md5_song_removed (oldsong);
	remove_duplicate (song, oldsong);
	md5_song_exists (song);
    }
    g_free (filetag);
    return(TRUE);
}
