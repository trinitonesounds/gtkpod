/* Time-stamp: <2003-06-19 23:09:47 jcs>
|
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
#include "itunesdb.h"
#include "md5.h"
#include "misc.h"
#include "prefs.h"
#include "support.h"
#include "charset.h"
#include "confirmation.h"

/* only used when reading extended info from file */
struct song_extended_info
{
    guint ipod_id;
    gchar *pc_path_locale;
    gchar *pc_path_utf8;
    gchar *md5_hash;
    gchar *charset;
    gchar *hostname;
    gchar *ipod_path;
    gint32 oldsize;
    guint32 playcount;
    guint32 rating;
    gboolean transferred;
};
/* List with songs pending deletion */
static GList *pending_deletion = NULL;
/* Flag to indicate if it's safe to quit (i.e. all songs exported or
   at least a offline database written). It's state is set to TRUE in
   handle_export().  It's state can be accessed by the public function
   file_are_saved(). It can be set to FALSE by calling
   data_changed() */
static gboolean files_saved = TRUE;
#ifdef G_THREADS_ENABLED
/* Thread specific */
static  GMutex *mutex = NULL;
static GCond  *cond = NULL;
static gboolean mutex_data = FALSE;
#endif 
/* Used to keep the "extended information" until the iTunesDB is 
   loaded */
static GHashTable *extendedinfohash = NULL;


/*------------------------------------------------------------------*\
 *                                                                  *
 *      Add Playlists                                               *
 *                                                                  *
\*------------------------------------------------------------------*/


/* Add all files specified in playlist @plfile. Will create a new
 * playlist with the name "basename (plfile)", even if one of the same
 * name already exists (if @plitem is != NULL, all songs will be added
 * to @plitem and no new playlist will be created).
 * It will then add all songs listed in @plfile. If set in the prefs,
 * duplicates will be detected (and the song already present in the
 * database will be added to the playlist instead). */
/* @addsongfunc: if != NULL this will be called instead of
   "add_song_to_playlist () -- used for dropping songs at a specific
   position in the song view */
gboolean add_playlist_by_filename (gchar *plfile, Playlist *plitem,
				   AddSongFunc addsongfunc, gpointer data)
{
    enum {
	PLT_M3U,   /* M3U playlist file */
	PLT_PLS,   /* PLS playlist file */
	PLT_MISC   /* something else -- assume it works the same as M3U */
    };
    gchar *plname, *bufp, *plfile_utf8;
    gchar buf[PATH_MAX];
    gint type = PLT_MISC; /* type of playlist file */
    gint line, songs;
    FILE *fp;
    gboolean error;

    if (!plfile)  return TRUE;

    if (g_file_test (plfile, G_FILE_TEST_IS_DIR))
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
	if (compare_string_case_insensitive (bufp, "m3u") == 0)
	    type = PLT_M3U;
	else if (compare_string_case_insensitive (bufp, "pls") == 0)
	    type = PLT_PLS;
	else if (compare_string_case_insensitive (bufp, "mp3") == 0)
	{
	    /* FIXME: Status */
	    return FALSE;  /* definitely not! */
	}
	else if (compare_string_case_insensitive (bufp, "wav") == 0)
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
    /* create playlist (if none is specified) */
    if (!plitem)   plitem = add_new_playlist (plname, -1);

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
	    if (add_song_by_filename (bufp, plitem,
				      prefs_get_add_recursively (),
				      addsongfunc, data))
		++songs;
	    break;
	case PLT_M3U:
	    /* comments start with '#' */
	    if (*bufp == '#') break;
	    /* assume the rest of the line is a filename */
	    if (add_song_by_filename (bufp, plitem,
				      prefs_get_add_recursively (),
				      addsongfunc, data))
		++songs;
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
		if (add_song_by_filename (bufp, plitem,
					  prefs_get_add_recursively (),
					  addsongfunc, data))
		    ++songs;
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
   If @plitem != NULL, add songs also to Playlist @plitem
   @descend: TRUE: add recursively
             FALSE: don't enter subdirectories */
/* Not nice: the return value has not much meaning. TRUE: all files
 * were added successfully. FALSE: some files could not be
   added (e.g: duplicates)  */
/* @addsongfunc: if != NULL this will be called instead of
   "add_song_to_playlist () -- used for dropping songs at a specific
   position in the song view */
gboolean add_directory_by_name (gchar *name, Playlist *plitem,
				gboolean descend,
				AddSongFunc addsongfunc, gpointer data)
{
  gboolean result = TRUE;

  if (name == NULL) return TRUE;
  if (g_file_test (name, G_FILE_TEST_IS_DIR))
  {
      GDir *dir = g_dir_open (name, 0, NULL);
      block_widgets ();
      if (dir != NULL) {
	  G_CONST_RETURN gchar *next;
	  do {
	      next = g_dir_read_name (dir);
	      if (next != NULL)
	      {
		  gchar *nextfull = concat_dir (name, next);
		  if (descend ||
		      !g_file_test (nextfull, G_FILE_TEST_IS_DIR))
		      result &= add_directory_by_name (nextfull, plitem,
						       descend,
						       addsongfunc, data);
		  g_free (nextfull);
	      }
	  } while (next != NULL);
	  g_dir_close (dir);
      }
      release_widgets ();
  }
  else
  {
      result = add_song_by_filename (name, plitem, descend, addsongfunc, data);
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
	case SM_COLUMN_COMPOSER:
	    str = g_path_get_basename (song->pc_path_utf8);
	    set_entry (&song->composer, &song->composer_utf16, str);
	    break;
	}
    }
}


/*------------------------------------------------------------------*\
 *                                                                  *
 *      Fill in song struct with data from file                     *
 *                                                                  *
\*------------------------------------------------------------------*/

/* update the song->charset info with the currently used charset */
static void update_charset_info (Song *song)
{
    if (song)
    {
	G_CONST_RETURN gchar *charset = prefs_get_charset ();
	C_FREE (song->charset);
	if (!charset || !strlen (charset))
	{    /* use standard locale charset */
	    g_get_charset (&charset);
	}
	/* test for Japanese auto detection */
	if (charset && (strcmp (charset, GTKPOD_JAPAN_AUTOMATIC) == 0))
	{
	    if (song->auto_charset)  charset = song->auto_charset;
	}
	song->auto_charset = NULL;
	song->charset = g_strdup (charset);
    }
}


/* Fills the supplied @or_song with data from the file @name. If
 * @or_song is NULL, a new song struct is created The entries
 * pc_path_utf8 and pc_path_locale are not changed if an entry already
 * exists */ 
/* turns NULL on error, a pointer to the Song otherwise */
static Song *get_song_info_from_file (gchar *name, Song *or_song)
{
    Song *song = NULL;
    File_Tag filetag;
    mp3info *mp3info;
    gint len;

    if (!name) return NULL;

    if (g_file_test (name, G_FILE_TEST_IS_DIR)) return NULL;
    if (!g_file_test (name, G_FILE_TEST_EXISTS)) return NULL;

    /* check if filename ends on ".mp3" */
    len = strlen (name);
    if (len < 4) return NULL;
    if (strcasecmp (&name[len-4], ".mp3") != 0) return NULL;

    if (Id3tag_Read_File_Tag (name, &filetag) == TRUE)
    {
	struct stat si;

	if (or_song)    song = or_song;
	else            song = g_malloc0 (sizeof (Song));

	C_FREE (song->pc_path_utf8);
	C_FREE (song->pc_path_locale);
	song->pc_path_utf8 = charset_to_utf8 (name);
	song->pc_path_locale = g_strdup (name);

	/* Set modification date to modification date of file */
	if (stat (name, &si) == 0)
	    song->time_modified = itunesdb_time_host_to_mac (si.st_mtime);

	C_FREE (song->fdesc);
	C_FREE (song->fdesc_utf16);
	song->fdesc = g_strdup ("MPEG audio file");
	song->fdesc_utf16 = g_utf8_to_utf16 (song->fdesc, -1, NULL, NULL, NULL);
	C_FREE (song->album);
	C_FREE (song->album_utf16);
	if (filetag.album)
	{
	    song->album = filetag.album;
	    song->album_utf16 = g_utf8_to_utf16 (song->album, -1, NULL, NULL, NULL);
	}
	else set_entry_from_filename (song, SM_COLUMN_ALBUM);

	C_FREE (song->artist);
	C_FREE (song->artist_utf16);
	if (filetag.artist)
	{
	    song->artist = filetag.artist;
	    song->artist_utf16 = g_utf8_to_utf16 (song->artist, -1, NULL, NULL, NULL);
	}
	else set_entry_from_filename (song, SM_COLUMN_ARTIST);

	C_FREE (song->title);
	C_FREE (song->title_utf16);
	if (filetag.title)
	{
	    song->title = filetag.title;
	    song->title_utf16 = g_utf8_to_utf16 (song->title, -1, NULL, NULL, NULL);
	}
	else set_entry_from_filename (song, SM_COLUMN_TITLE);

	C_FREE (song->genre);
	C_FREE (song->genre_utf16);
	if (filetag.genre)
	{
	    song->genre = filetag.genre;
	    song->genre_utf16 = g_utf8_to_utf16 (song->genre, -1, NULL, NULL, NULL);
	}
	else set_entry_from_filename (song, SM_COLUMN_GENRE);

	C_FREE (song->comment);
	C_FREE (song->comment_utf16);
	if (filetag.comment)
	{
	    song->comment = filetag.comment;
	    song->comment_utf16 = g_utf8_to_utf16 (song->comment, -1, NULL, NULL, NULL);
	}
	if (filetag.year == NULL)
	{
	    song->year = 0;
	}
	else
	{
	    song->year = atoi(filetag.year);
	    g_free (filetag.year);
	}

	if (filetag.track == NULL)
	{
	    song->track_nr = 0;
	}
	else
	{
	    song->track_nr = atoi(filetag.track);
	    g_free (filetag.track);
	}

	if (filetag.track_total == NULL)
	{
	    song->tracks = 0;
	}
	else
	{
	    song->tracks = atoi(filetag.track_total);
	    g_free (filetag.track_total);
	}
	song->size = filetag.size;
	song->auto_charset = filetag.auto_charset;
    }

    if (song)
    {
	/* Get additional info (play time and bitrate */
	mp3info = mp3file_get_info (name);
	if (mp3info)
	{
	    song->songlen = mp3info->milliseconds;
	    song->bitrate = (gint)(mp3info->vbr_average);
	    g_free (mp3info);
	}
	/* Fall back to xmms code if songlen is 0 */
	if (song->songlen == 0)
	{
	    song->songlen = get_song_time (name);
	    if (song->songlen)
		song->bitrate = (float)song->size*8/song->songlen;
	}

	/* set charset used */
	update_charset_info (song);

	/* Make sure all strings are initialised -- that way we don't 
	   have to worry about it when we are handling the strings */
	/* exception: md5_hash, charset and hostname: these may be NULL. */
	validate_entries (song);

	if (song->songlen == 0)
	{
	    /* Songs with zero play length are ignored by iPod... */
	    gtkpod_warning (_("File \"%s\" has zero play length. Ignoring.\n"),
			    name);
	    if (!or_song)
	    {  /* don't delete the song that was passed to us */
		g_free (song);
	    }
	    song = NULL;
	    while (widgets_blocked && gtk_events_pending ())
		gtk_main_iteration ();
	}
    }
    return song;
}


/*------------------------------------------------------------------*\
 *                                                                  *
 *      Update Song Data from File                                  *
 *                                                                  *
\*------------------------------------------------------------------*/


/* reads info from file and updates the ID3 tags of
   @selected_songids. */
void update_songids (GList *selected_songids)
{
    GList *gl_id;

    if (g_list_length (selected_songids) == 0)
    {
	gtkpod_statusbar_message(_("Nothing to update"));
	return;
    }

    block_widgets ();
    for (gl_id = selected_songids; gl_id; gl_id = gl_id->next)
    {
	guint32 id = (guint32)gl_id->data;
	Song *song = get_song_by_id (id);
	gchar *buf = g_strdup_printf (_("Updating %s"), get_song_info (song));
	gtkpod_statusbar_message (buf);
	g_free (buf);
	update_song_from_file (song);
	/* FIXME: how can we find (easily) out whether the data has
	 * actually changed */
	data_changed ();
    }
    release_widgets ();
    /* display log of non-updated songs */
    display_non_updated (NULL, NULL);
    /* display log of updated songs */
    display_updated (NULL, NULL);
    /* display log of detected duplicates */
    remove_duplicate (NULL, NULL);
    gtkpod_statusbar_message(_("Updated selected songs with info from file."));
}


/*------------------------------------------------------------------*\
 *                                                                  *
 *      Sync Song Dirs                                              *
 *                                                                  *
\*------------------------------------------------------------------*/

/* Callback for adding songs (makes sure song isn't added to playlist
 * again if it already exists */
static void sync_addsongfunc (Playlist *plitem, Song *song, gpointer data)
{
    if (!song) return;
    if (!plitem) plitem = get_playlist_by_nr (0); /* NULL/0: MPL */

    /* only add if @song isn't already a member of the current
       playlist */
    if (!g_list_find (plitem->members, song))
	add_song_to_playlist (plitem, song, TRUE);
}


/* Synchronize the directory @key (gchar *dir). Called by
 * sync_songids().
 * @user_data: selected playlist */
static void sync_dir (gpointer key,
		      gpointer value,
		      gpointer user_data)
{
    gchar *dir = (gchar *)key;
    gchar *charset = (gchar *)value;
    Playlist *pl = (Playlist *)user_data;
    gchar *buf = NULL;
    gchar *dir_utf8 = NULL;
    /* old charset */
    gchar *prefs_charset = NULL;

    /* Assertions */
    if (!dir) return;
    if (!g_file_test (dir, G_FILE_TEST_IS_DIR))  return;

    /* change default charset if prefs don't say otherwise */
    if (!prefs_get_update_charset () && charset)
    {   /* we should use the initial charset for the update */
	if (prefs_get_charset ())
	{   /* remember the charset originally set */
	    prefs_charset = g_strdup (prefs_get_charset ());
	}
	/* use the charset used for the directory */
	prefs_set_charset (charset);
    }	

    /* report status */
    /* convert dir to UTF8. Use @charset if specified */
    if (charset)
	dir_utf8 = charset_to_charset (charset, "UTF8", dir);
    else
	dir_utf8 = charset_to_utf8 (dir);
    buf = g_strdup_printf (_("Syncing directory '%s'"), dir_utf8);
    gtkpod_statusbar_message (buf);
    g_free (buf);
    g_free (dir_utf8);

    /* sync dir */
    add_directory_by_name (dir, pl, FALSE, sync_addsongfunc, NULL);

    /* reset charset */
    if (!prefs_get_update_charset () && charset)
	prefs_set_charset (prefs_charset);
    g_free (prefs_charset);
}


/* ok handler for sync_remove */
/* @user_data1 is NULL, @user_data2 are the selected songids */
static void sync_remove_ok (gpointer user_data1, gpointer user_data2)
{
    if (prefs_get_sync_remove ())
	delete_song_ok (NULL, user_data2);
}


/* cancel handler for sync_remove */
/* @user_data1 is NULL, @user_data2 are the selected songids */
static void sync_remove_cancel (gpointer user_data1, gpointer user_data2)
{
    GList *id_list = user_data2;

    g_list_free (id_list);

    gtkpod_statusbar_message (_("Syncing completed. No files deleted."));
}


static void sync_dir_ok (gpointer user_data1, gpointer user_data2)
{
    GHashTable *hash = (GHashTable *)user_data1;
    /* currently selected playlist */
    Playlist *playlist = (Playlist *)user_data2;
    /* state of "update existing" */
    gboolean update;

    /* assertion */
    if (!hash) return;

    /* set "update existing" to TRUE */
    update = prefs_get_update_existing();
    prefs_set_update_existing (TRUE);

    /* sync all dirs stored in the hash */
    g_hash_table_foreach (hash, sync_dir, playlist);

    /* reset "update existing" */
    prefs_set_update_existing (update);

    /* display log of non-updated songs */
    display_non_updated (NULL, NULL);
    /* display log updated songs */
    display_updated (NULL, NULL);
    /* display log of detected duplicates */
    remove_duplicate (NULL, NULL);

    if (prefs_get_sync_remove ())
    {   /* remove tracks that are no longer present in the dirs */
	GList *id_list = NULL;
	GString *str;
	gchar *label, *title;
	Song *s;
	GList *l;

	/* add all songs that are no longer present in the dirs */
	for (s=get_next_song (0); s; s=get_next_song (1))
	{
	    if (s->pc_path_locale && *s->pc_path_locale)
	    {
		gchar *dirname = g_path_get_dirname (s->pc_path_locale);
		if (g_hash_table_lookup (hash, dirname) &&
		    !g_file_test (s->pc_path_locale, G_FILE_TEST_EXISTS))
		{
		    id_list = g_list_append (id_list, (gpointer)s->ipod_id);
		}
	    }
	}
	if (g_list_length (id_list) > 0)
	{
	    /* populate "title" and "label" */
	    delete_populate_settings (NULL, id_list,
				      &label, &title,
				      NULL, NULL, NULL);
	    /* create a list of songs */
	    str = g_string_sized_new (2000);
	    for(l = id_list; l; l=l->next)
	    {
		s = get_song_by_id ((guint32)l->data);
		if (s)
		    g_string_append_printf (str, "%s (%s-%s)\n",
					    s->pc_path_utf8,
					    s->artist, s->title);
	    }
	    /* open window */
	    gtkpod_confirmation
		(-1,                   /* gint id, */
		 FALSE,                /* gboolean modal, */
		 title,                /* title */
		 label,                /* label */
		 str->str,             /* scrolled text */
		 _("Never delete any files when syncing"),/* opt 1 text */
		 CONF_STATE_INVERT_FALSE,                 /* opt 1 state */
		 prefs_set_sync_remove,                   /* opt 1 handler */
		 NULL, 0, NULL,        /* option 2 */
		 prefs_get_sync_remove_confirm(), /* gboolean confirm_again, */
		 prefs_set_sync_remove_confirm,   /* confirm_again_handler,*/
		 sync_remove_ok,       /* ConfHandler ok_handler,*/
		 CONF_NO_BUTTON,       /* don't show "Apply" button */
		 sync_remove_cancel,   /* cancel_handler,*/
		 NULL,                 /* gpointer user_data1,*/
		 id_list);             /* gpointer user_data2,*/

	    g_free (label);
	    g_string_free (str, TRUE);
	}
	else sync_remove_cancel (hash, id_list);
    }
    else
    {
	gtkpod_statusbar_message (_("Syncing completed."));
    }
    /* destroy the hash table (and free the memory taken by the
       dirnames */
    g_hash_table_destroy (hash);
}


/* sync dirs cancelled */
static void sync_dir_cancel (gpointer user_data1, gpointer user_data2)
{
    if (user_data1)  g_hash_table_destroy ((GHashTable *)user_data1);
    gtkpod_statusbar_message (_("Syncing aborted"));
}


/* Append @key (gchar *dir) to GString *user_data. Called by
 * sync_songids(). Charset to use is @value. If @value is not set, use
 * the charset specified in the preferences */
static void sync_add_dir_to_string  (gpointer key,
				     gpointer value,
				     gpointer user_data)
{
    gchar *dir = (gchar *)key;
    gchar *charset = (gchar *)value;
    GString *str = (GString *)user_data;
    gchar *buf;

    if (!dir || !str) return;

    /* convert to UTF8. Use @charset if specified */
    if (charset)
	buf = charset_to_charset (charset, "UTF8", dir);
    else
	buf = charset_to_utf8 (dir);
    g_string_append_printf (str, "%s\n", buf);
    g_free (buf);
}


/* Sync all directories referred to by the pc_path_locale entries in
   the selected songs */
void sync_songids (GList *selected_songids)
{
    GHashTable *hash;
    GList *gl_id;
    guint32 dirnum = 0;

    if (g_list_length (selected_songids) == 0)
    {
	gtkpod_statusbar_message (_("No songs in selection"));
	return;
    }

    /* Create a hash to keep the directory names ("key", and "value"
       to be freed with g_free). key is dirname, value is charset used
       */
    hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

    /* Get the dirs of all songs selected and enter them into the hash
       table if the dir exists. Using a hash table automatically
       removes duplicates */
    for (gl_id = selected_songids; gl_id; gl_id = gl_id->next)
    {
	guint32 id = (guint32)gl_id->data;
	Song *song = get_song_by_id (id);
	if (song && song->pc_path_locale && *song->pc_path_locale)
	{
	    gchar *dirname = g_path_get_dirname (song->pc_path_locale);

	    ++dirnum;
	    if (g_file_test (dirname, G_FILE_TEST_IS_DIR))
	    {
		if (song->charset && *song->charset)
		{   /* charset set -- just insert the entry */
		    gchar *charset = g_strdup (song->charset);
		    g_hash_table_insert (hash, dirname, charset);
		}
		else
		{   /* no charset -- make sure we don't replace a dir
		     * entry that had the charset set */
		    if (!g_hash_table_lookup (hash, dirname))
			g_hash_table_insert (hash, dirname, NULL);
		}
	    }
	    else
	    {
		if (g_file_test (dirname, G_FILE_TEST_EXISTS))
		    gtkpod_warning (_("'%s' is not a directory. Ignored.\n"),
				    dirname);
		else
		    gtkpod_warning (_("'%s' does not exist. Ignored.\n"),
				    dirname);
	    }
	}
    }
    if (g_hash_table_size (hash) == 0)
    {   /* no directory names available */
	if (dirnum == 0) gtkpod_warning (_("\
No directory names were stored. Make sure that you enable 'Write extended information' in the Export section of the preferences at the time of importing files.\n\
\n\
To synchronize directories now, activate the 'duplicate detection' in the Import section and add the directories you want to sync again.\n"));
	else	         gtkpod_warning (_("\
No valid directories have been found. Sync aborted.\n"));
    }
    else
    {
	GString *str = g_string_sized_new (2000);
	/* build a string with all directories in the hash */
	g_hash_table_foreach (hash, sync_add_dir_to_string, str);
	if (!gtkpod_confirmation (
		-1,                     /* gint id, */
		FALSE,                  /* gboolean modal, */
		_("Synchronize directories"), /* title */
		_("OK to synchronize the following directories?"),
		str->str,               /* text */
		NULL, 0, NULL,          /* option 1 */
		NULL, 0, NULL,          /* option 2 */
		prefs_get_show_sync_dirs(),/* gboolean confirm_again, */
		prefs_set_show_sync_dirs,/* confirm_again_handler*/
		sync_dir_ok,            /* ConfHandler ok_handler,*/
		CONF_NO_BUTTON,         /* don't show "Apply" */
		sync_dir_cancel,        /* cancel_handler,*/
		hash,                   /* gpointer user_data1,*/
		pm_get_selected_playlist())) /* gpointer user_data2,*/
	{ /* creation failed */
	    g_hash_table_destroy (hash);
	}
	g_string_free (str, TRUE);
    }
}


/*------------------------------------------------------------------*\
 *                                                                  *
 *  Generic functions to "do" things on selected playlist / entry   *
 *  / songs                                                         *
 *                                                                  *
\*------------------------------------------------------------------*/

/* Make a list of all selected songs and call @do_func with that list
   as argument */
void do_selected_songs (void (*do_func)(GList *songids))
{
    GList *selected_songids = NULL;

    if (!do_func) return;

    /* I'm using ids instead of "Song *" -pointer because it would be
     * possible that a song gets removed during the process */
    selected_songids = sm_get_selected_songids();
    do_func (selected_songids);
    g_list_free (selected_songids);
}


/* Make a list of all songs in the currently selected entry of sort
   tab @inst and call @do_func with that list as argument */
void do_selected_entry (void (*do_func)(GList *songids), gint inst)
{
    GList *selected_songids = NULL;
    TabEntry *entry;
    GList *gl;

    if (!do_func) return;

    if ((inst < 0) || (inst > prefs_get_sort_tab_num ()))   return;

    entry = st_get_selected_entry (inst);
    if (entry == NULL)
    {  /* no entry selected */
	gtkpod_statusbar_message (_("No entry selected."));
	return;
    }
    for (gl=entry->members; gl; gl=gl->next)
    { /* make a list with all songids in this entry */
	Song *song = (Song *)gl->data;
	if (song)
	    selected_songids = g_list_append (selected_songids,
					      (gpointer)song->ipod_id);
    }
    do_func (selected_songids);
    g_list_free (selected_songids);
}


/* Make a list of the songs in the current playlist and call @do_func
   with that list as argument */
void do_selected_playlist (void (*do_func)(GList *songids))
{
    GList *selected_songids = NULL;
    Playlist *pl;
    GList *gl;

    if (!do_func) return;

    pl = pm_get_selected_playlist();
    if (!pl)
    { /* no playlist selected */
	gtkpod_statusbar_message (_("No playlist selected."));
	return;
    }
    for (gl=pl->members; gl; gl=gl->next)
    { /* make a list with all songids in this entry */
	Song *song = (Song *)gl->data;
	if (song)
	    selected_songids = g_list_append (selected_songids,
					      (gpointer)song->ipod_id);
    }
    do_func (selected_songids);
    g_list_free (selected_songids);
}


/* Logs songs (@song) that could not be updated from file for some
   reason. Pop up a window with the log by calling with @song = NULL,
   or remove the log by calling with @song = -1.
   @txt (if available)w is added as an explanation as to why it was
   impossible to update the song */
void display_non_updated (Song *song, gchar *txt)
{
   gchar *buf;
   static gint song_nr = 0;
   static GString *str = NULL;

   if ((song == NULL) && str)
   {
       if (prefs_get_show_non_updated() && str->len)
       { /* Some songs have not been updated. Print a notice */
	   buf = g_strdup_printf (
	       ngettext ("The following song could not be updated",
			 "The following %d songs could not be updated",
			 song_nr), song_nr);
	   gtkpod_confirmation
	       (-1,                 /* gint id, */
		FALSE,              /* gboolean modal, */
		_("Failed Song Update"),   /* title */
		buf,                /* label */
		str->str,           /* scrolled text */
		NULL, 0, NULL,          /* option 1 */
		NULL, 0, NULL,          /* option 2 */
		TRUE,               /* gboolean confirm_again, */
		prefs_set_show_non_updated,/* confirm_again_handler,*/
		NULL,               /* ConfHandler ok_handler,*/
		CONF_NO_BUTTON,     /* don't show "Apply" button */
		CONF_NO_BUTTON,     /* cancel_handler,*/
		NULL,               /* gpointer user_data1,*/
		NULL);              /* gpointer user_data2,*/
	   g_free (buf);
       }
       display_non_updated ((void *)-1, NULL);
   }

   if (song == (void *)-1)
   { /* clean up */
       if (str)       g_string_free (str, TRUE);
       str = NULL;
       song_nr = 0;
       gtkpod_songs_statusbar_update();
   }
   else if (prefs_get_show_non_updated() && song)
   {
       /* add info about it to str */
       buf = get_song_info (song);
       if (!str)
       {
	   song_nr = 0;
	   str = g_string_sized_new (2000); /* used to keep record of
					     * non-updated songs */
       }
       if (txt) g_string_append_printf (str, "%s (%s)\n", buf, txt);
       else     g_string_append_printf (str, "%s\n", buf);
       g_free (buf);
       ++song_nr; /* count songs */
   }
}


/* Logs songs (@song) that could be updated from file. Pop up a window
   with the log by calling with @song = NULL, or remove the log by
   calling with @song = -1.
   @txt (if available)w is added as an explanation as to why it was
   impossible to update the song */
void display_updated (Song *song, gchar *txt)
{
   gchar *buf;
   static gint song_nr = 0;
   static GString *str = NULL;

   if (prefs_get_show_updated() && (song == NULL) && str)
   {
       if (str->len)
       { /* Some songs have been updated. Print a notice */
	   buf = g_strdup_printf (
	       ngettext ("The following song has been updated",
			 "The following %d songs have been updated",
			 song_nr), song_nr);
	   gtkpod_confirmation
	       (-1,                 /* gint id, */
		FALSE,              /* gboolean modal, */
		_("Successful Song Update"),   /* title */
		buf,                /* label */
		str->str,           /* scrolled text */
		NULL, 0, NULL,          /* option 1 */
		NULL, 0, NULL,          /* option 2 */
		TRUE,               /* gboolean confirm_again, */
		prefs_set_show_updated,/* confirm_again_handler,*/
		NULL,               /* ConfHandler ok_handler,*/
		CONF_NO_BUTTON,     /* don't show "Apply" button */
		CONF_NO_BUTTON,     /* cancel_handler,*/
		NULL,               /* gpointer user_data1,*/
		NULL);              /* gpointer user_data2,*/
	   g_free (buf);
       }
       display_updated ((void *)-1, NULL);
   }

   if (song == (void *)-1)
   { /* clean up */
       if (str)       g_string_free (str, TRUE);
       str = NULL;
       song_nr = 0;
       gtkpod_songs_statusbar_update();
   }
   else if (prefs_get_show_updated() && song)
   {
       /* add info about it to str */
       buf = get_song_info (song);
       if (!str)
       {
	   song_nr = 0;
	   str = g_string_sized_new (2000); /* used to keep record of
					     * non-updated songs */
       }
       if (txt) g_string_append_printf (str, "%s (%s)\n", buf, txt);
       else     g_string_append_printf (str, "%s\n", buf);
       g_free (buf);
       ++song_nr; /* count songs */
   }
}


/* Update information of @song from data in original file. This
   requires that the original filename is available, and that the file
   exists.

   A list of non-updated songs can be displayed by calling
   display_non_updated (NULL, NULL). This list can be deleted by
   calling display_non_updated ((void *)-1, NULL);

   It is also possible that duplicates get detected in the process --
   a list of those can be displayed by calling "remove_duplicate
   (NULL, NULL)", that list can be deleted by calling
   "remove_duplicate (NULL, (void *)-1)"*/
void update_song_from_file (Song *song)
{
    Song *oldsong;
    gchar *prefs_charset = NULL;
    gchar *songpath = NULL;
    gint32 oldsize = 0;
    gchar *charset;

    if (!song) return;

    /* store size of song on iPod */
    if (song->transferred) oldsize = song->size;
    else                   oldsize = 0;

    charset = song->charset;
    if (!prefs_get_update_charset () && charset)
    {   /* we should use the initial charset for the update */
	if (prefs_get_charset ())
	{   /* remember the charset originally set */
	    prefs_charset = g_strdup (prefs_get_charset ());
	}
	/* use the charset used when first importing the song */
	prefs_set_charset (song->charset);
    }
    else
    {   /* we should update the song->charset information */
	update_charset_info (song);
    }

    if (song->pc_path_locale)
    {   /* need to copy because we cannot pass song->pc_path_locale to
	get_song_info_from_file () since song->pc_path gets g_freed
	there */
	songpath = g_strdup (song->pc_path_locale);
    }

    if (!(song->pc_path_locale && *song->pc_path_locale))
    { /* no path available */
	display_non_updated (song, _("no filename available"));
    }
    else if (get_song_info_from_file (songpath, song))
    { /* update successfull */
	/* remove song from md5 hash and reinsert it
	   (hash value may have changed!) */
	gchar *oldhash = song->md5_hash;
	md5_song_removed (song);
	song->md5_hash = NULL;  /* need to remove the old value manually! */
	oldsong = md5_song_exists_insert (song);
	if (oldsong) { /* song exists, remove old song
			  and register the new version */
	    md5_song_removed (oldsong);
	    remove_duplicate (song, oldsong);
	    md5_song_exists_insert (song);
	}
	/* song may have to be copied to iPod on next export */
	/* since it will copied under the same name as before, we
	   don't have to manually remove it */
	if (oldhash && song->md5_hash)
	{   /* do we really have to copy the song again? */
	    if (strcmp (oldhash, song->md5_hash) != 0)
		song->transferred = FALSE;
	}
	else song->transferred = FALSE; /* no hash available -- copy! */
	/* set old size if song has to be transferred (for free space
	 * calculation) */
	if (!song->transferred) song->oldsize = oldsize;
	/* notify display model */
	pm_song_changed (song);
	display_updated (song, NULL);
        C_FREE (oldhash);
    }
    else
    { /* update not successful -- log this song for later display */
	if (g_file_test (songpath,
			 G_FILE_TEST_EXISTS) == FALSE)
	{
	    display_non_updated (song, _("file not found"));
	}
	else
	{
	    display_non_updated (song, _("format not supported"));
	}
    }

    if (!prefs_get_update_charset () && charset)
    {   /* reset charset */
	prefs_set_charset (prefs_charset);
    }
    C_FREE (songpath);

    while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
}


/*------------------------------------------------------------------*\
 *                                                                  *
 *      Add File                                                    *
 *                                                                  *
\*------------------------------------------------------------------*/

/* Append file @name to the list of songs.
   @name is in the current locale
   @plitem: if != NULL, add song to plitem as well (unless it's the MPL)
   descend: TRUE:  add directories recursively
            FALSE: add contents of directories passed but don't descend
                   into its subdirectories */
/* Not nice: currently only accepts files ending on .mp3 */
/* @addsongfunc: if != NULL this will be called instead of
   "add_song_to_playlist () -- used for dropping songs at a specific
   position in the song view */
gboolean add_song_by_filename (gchar *name, Playlist *plitem, gboolean descend,
			       AddSongFunc addsongfunc, gpointer data)
{
  static gint count = 0; /* do a gtkpod_songs_statusbar_update() every
			    10 songs */
  Song *song;
  gchar str[PATH_MAX];
  gint len;

  if (name == NULL) return TRUE;

  if (g_file_test (name, G_FILE_TEST_IS_DIR))
  {
      return add_directory_by_name (name, NULL, descend, addsongfunc, data);
  }

  /* check if file is a playlist */
  len = strlen (name);
  if (len >= 4)
  {
      if ((strcasecmp (&name[len-4], ".pls") == 0) ||
	  (strcasecmp (&name[len-4], ".m3u") == 0))
      {
	  return add_playlist_by_filename (name, plitem, addsongfunc, data);
      }
  }

  song = get_song_info_from_file (name, NULL);

  if (song && prefs_get_update_existing ())
  {  /* If a file is added again, update the information of the
      * existing song */
      Song *oldsong = get_song_by_filename (name);

      if (oldsong)
      {
	  update_song_from_file (oldsong);
	  if (plitem && (plitem->type != PL_TYPE_MPL))
	  {
	      if (addsongfunc)
		  addsongfunc (plitem, oldsong, data);
	      else
		  add_song_to_playlist (plitem, oldsong, TRUE);
	  }
	  free_song (song);
	  song = NULL;
      }
  }

  if (song)
  {
      Song *added_song = NULL;

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
	  if (addsongfunc)
	  {
	      if (!plitem || (plitem->type == PL_TYPE_MPL))
	      {   /* add song to master playlist (if it hasn't been
		   * done before) */
		  if (added_song == song) addsongfunc (plitem, added_song,
						       data);
	      }
	      else
	      {   /* (plitem != NULL) && (type == NORM) */
		  /* add song to master playlist (if it hasn't been
		   * done before) */
		  if (added_song == song)
		      add_song_to_playlist (NULL, added_song, TRUE);
		  /* add song to specified playlist */
		  addsongfunc (plitem, added_song, data);
	      }
	  }
	  else  /* no addsongfunc */
	  {
	      /* add song to master playlist (if it hasn't been done before) */
	      if (added_song == song) add_song_to_playlist (NULL, added_song,
							    TRUE);
	      /* add song to specified playlist, but not to MPL */
	      if (plitem && (plitem->type != PL_TYPE_MPL))
		  add_song_to_playlist (plitem, added_song, TRUE);
	  }
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
gboolean write_tags_to_file (Song *song, S_item tag_id)
{
    File_Tag *filetag;
    gchar *ipod_fullpath, track[20];
    gchar *prefs_charset = NULL;
    Song *oldsong;
    gchar *charset;

    if (!song) return FALSE;

    /* if we are to use the charset used when first importing
       the song, change the prefs settings temporarily */
    charset = song->charset;
    if (!prefs_get_write_charset () && charset)
    {   /* we should use the initial charset for the update */
	if (prefs_get_charset ())
	{   /* remember the charset originally set */
	    prefs_charset = g_strdup (prefs_get_charset ());
	}
	/* use the charset used when first importing the song */
	prefs_set_charset (song->charset);
    }
    else
    {   /* we should update the song->charset information */
	update_charset_info (song);
    }

    filetag = g_malloc0 (sizeof (File_Tag));
    if ((tag_id == S_ALL) || (tag_id == S_ALBUM))
	filetag->album = song->album;
    if ((tag_id == S_ALL) || (tag_id == S_ARTIST))
	filetag->artist = song->artist;
    if ((tag_id == S_ALL) || (tag_id == S_TITLE))
	filetag->title = song->title;
    if ((tag_id == S_ALL) || (tag_id == S_GENRE))
	filetag->genre = song->genre;
    if ((tag_id == S_ALL) || (tag_id == S_COMMENT))
	filetag->comment = song->comment;
    if ((tag_id == S_ALL) || (tag_id == S_TRACK_NR))
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
	ipod_fullpath = get_song_name_on_ipod (song);
	if (Id3tag_Write_File_Tag (ipod_fullpath, filetag) == FALSE)
	  {
	    gtkpod_warning (_("Couldn't change tags of file: %s\n"),
			    ipod_fullpath);
	  }
	g_free (ipod_fullpath);
      }
    /* remove song from md5 hash and reinsert it (hash value has changed!) */
    md5_song_removed (song);
    C_FREE (song->md5_hash);  /* need to remove the old value manually! */
    oldsong = md5_song_exists_insert (song);
    if (oldsong) { /* song exists, remove and register the new version */
	md5_song_removed (oldsong);
	remove_duplicate (song, oldsong);
	md5_song_exists_insert (song);
    }
    g_free (filetag);

    if (!prefs_get_update_charset () && charset)
    {   /* reset charset */
	prefs_set_charset (prefs_charset);
    }
    return TRUE;
}


/*------------------------------------------------------------------*\
 *                                                                  *
 *      Handle Import of iTunesDB                                   *
 *                                                                  *
\*------------------------------------------------------------------*/


/* fills in extended info if available (called from add_song()) */
void fill_in_extended_info (Song *song)
{
  gint ipod_id;
  struct song_extended_info *sei;

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
	  if (sei->charset && !song->charset)
	      song->charset = g_strdup (sei->charset);
	  if (sei->hostname && !song->hostname)
	      song->hostname = g_strdup (sei->hostname);
	  song->oldsize = sei->oldsize;
	  song->playcount += sei->playcount;
	  /* FIXME: This means that the rating can never be reset to 0
	   * by the iPod */
	  if (song->rating == 0)
	      song->rating = sei->rating;
	  song->transferred = sei->transferred;
	  g_hash_table_remove (extendedinfohash, &ipod_id);
	}
    }
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
	C_FREE (sei->charset);
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
	g_warning ("Programming error: Could not create hash value from itunesdb\n");
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
		gtkpod_warning (_("%s:\nFormat error: %s\n"), name, buf);
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
	    else if (g_ascii_strcasecmp (line, "charset") == 0)
		sei->charset = g_strdup (arg);
	    else if (g_ascii_strcasecmp (line, "oldsize") == 0)
		sei->oldsize = atoi (arg);
	    else if (g_ascii_strcasecmp (line, "playcount") == 0)
		sei->playcount = atoi (arg);
	    else if (g_ascii_strcasecmp (line, "rating") == 0)
		sei->rating = atoi (arg);
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
/* The import button is disabled once you have imported an existing
   database or exported your new data. Therefore, no specific check
   has to be performed on whether it's OK to import or not. */
void handle_import (void)
{
    gchar *name1, *name2, *cfgdir;
    gboolean success, md5songs;
    guint32 n;


    /* we should switch off duplicate detection during import --
     * otherwise we mess up the playlists */

    md5songs = prefs_get_md5songs ();
    prefs_set_md5songs (FALSE);

    n = get_nr_of_songs (); /* how many songs are alread there? */

    block_widgets ();
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
	if(itunesdb_parse (cfg->ipod_mount))
	    gtkpod_statusbar_message(_("iPod Database Successfully Imported"));
	else
	    gtkpod_statusbar_message(_("iPod Database Import Failed"));
    }
    else
    { /* offline - requires extended info */
	if ((cfgdir = prefs_get_cfgdir ()))
	{
	    name1 = concat_dir (cfgdir, "iTunesDB.ext");
	    name2 = concat_dir (cfgdir, "iTunesDB");
	    success = read_extended_info (name1, name2);
	    g_free (name1);
	    if (!success) 
	    {
		gtkpod_warning (_("Extended info will not be used. If you have non-transferred songs,\nthese will be lost.\n"));
	    }
	    if(itunesdb_parse_file (name2))
		gtkpod_statusbar_message(
			_("Offline iPod Database Successfully Imported"));
	    else
		gtkpod_statusbar_message(
			_("Offline iPod Database Import Failed"));
	    g_free (name2);
	    g_free (cfgdir);
	}
	else
	{
	    gtkpod_warning (_("Import aborted.\n"));
	    return;
	    release_widgets ();
	}
    }
    destroy_extendedinfohash (); /* delete hash information (if set up) */

    /* We need to make sure that the songs that already existed
       in the DB when we called itunesdb_parse() do not duplicate
       any existing ID */
    renumber_ipod_ids ();

    gtkpod_songs_statusbar_update();
    if (n != get_nr_of_songs ())
    { /* Import was successfull, block menu item and button */
	disable_gtkpod_import_buttons();
    }
    /* reset duplicate detection -- this will also detect and correct
     * all duplicate songs currently in the database */
    prefs_set_md5songs (md5songs);
    release_widgets ();
}


/* Like get_song_name_on_disk(), but verifies the song actually exists
   Must g_free return value after use */
gchar *get_song_name_on_disk_verified (Song *song)
{
    gchar *name = NULL;

    if (song)
    {
	name = get_song_name_on_ipod (song);
	if (name)
	{
	    if (!g_file_test (name, G_FILE_TEST_EXISTS))
	    {
		g_free (name);
		name = NULL;
	    }
	}
	if(!name && song->pc_path_locale && (*song->pc_path_locale))
	{
	    if (g_file_test (song->pc_path_locale, G_FILE_TEST_EXISTS))
		name = g_strdup (song->pc_path_locale);
	} 
    }
    return name;
}

/**
 * get_song_name_on_disk
 * Function to retrieve the filename on disk for the specified Song.  It
 * returns the valid filename whether the file has been copied to the ipod,
 * or has yet to be copied.  So it's useful for file operations on a song.
 * @s - The Song data structure we want the on disk file for
 * Returns - the filename for this Song. Must be g_free'd.
 */
gchar* get_song_name_on_disk(Song *s)
{
    gchar *result = NULL;

    if(s)
    {
	result = get_song_name_on_ipod (s);
	if(!result &&
	   (s->pc_path_locale) && (strlen(s->pc_path_locale) > 0))
	{
	    result = g_strdup (s->pc_path_locale);
	} 
    }
    return(result);
}


/* Return the full iPod filename as stored in @s.
   @s: song
   Return value: full filename to @s on the iPod or NULL if no
   filename is set in @s. NOTE: the file does not necessarily
   exist. NOTE: the in itunesdb.c code works around a problem on some
   systems (see below) and might return a filename with different case
   than the original filename. Don't copy it back to @s */
gchar *get_song_name_on_ipod (Song *s)
{
    gchar *result = NULL;

    if(s &&  !prefs_get_offline ())
    {
	gchar *path = prefs_get_ipod_mount ();
	result = itunesdb_get_song_name_on_ipod (path, s);
	C_FREE (path);
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


/*------------------------------------------------------------------*\
 *                                                                  *
 *      Functions concerning deletion of songs                      *
 *                                                                  *
\*------------------------------------------------------------------*/


/* in Bytes, minus the space taken by songs that will be overwritten
 * during copying */
glong get_filesize_of_deleted_songs(void)
{
    glong n = 0;
    Song *song;
    GList *gl_song;

    for (gl_song = pending_deletion; gl_song; gl_song=gl_song->next)
    {
	song = (Song *)gl_song->data;
	if (song->transferred)   n += song->size;
    }
    return n;
}

void mark_song_for_deletion (Song *song)
{
    pending_deletion = g_list_append(pending_deletion, song);
}


/*------------------------------------------------------------------*\
 *                                                                  *
 *      Handle Export of iTunesDB                                   *
 *                                                                  *
\*------------------------------------------------------------------*/


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
      g_warning ("Programming error: Could not create hash value from itunesdb\n");
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
      if (song->charset)
	fprintf (fp, "charset=%s\n", song->charset);
      if (!song->transferred && song->oldsize)
	  fprintf (fp, "oldsize=%d\n", song->oldsize);
      fprintf (fp, "playcount=%d\n", song->playcount);
/* rating is now written in iTunesDB */
/*      fprintf (fp, "rating=%d\n", song->rating);*/
      fprintf (fp, "transferred=%d\n", song->transferred);
      while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
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
	  while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
      }
  }
  fprintf (fp, "id=xxx\n");
  fclose (fp);
  return TRUE;
}



#ifdef G_THREADS_ENABLED
/* Threaded remove file */
static gpointer th_remove (gpointer filename)
{
    int result;

    result = remove ((gchar *)filename);
    g_mutex_lock (mutex);
    mutex_data = TRUE; /* signal that thread will end */
    g_cond_signal (cond);
    g_mutex_unlock (mutex);
    return (gpointer)result;
}
/* Threaded copy of ipod song */
static gpointer th_copy (gpointer s)
{
    gboolean result;
    Song *song = (Song *)s;

    result = itunesdb_copy_song_to_ipod (cfg->ipod_mount,
					 song,
					 song->pc_path_locale);
    /* delete old size */
    if (song->transferred) song->oldsize = 0;
    g_mutex_lock (mutex);
    mutex_data = TRUE;   /* signal that thread will end */
    g_cond_signal (cond);
    g_mutex_unlock (mutex);
    return (gpointer)result;
}
#endif 

/* This function is called when the user presses the abort button
 * during flush_songs() */
static void flush_songs_abort (gboolean *abort)
{
    *abort = TRUE;
}

/* Flushes all non-transferred songs to the iPod filesystem
   Returns TRUE on success, FALSE if some error occured */
gboolean flush_songs (void)
{
  gint count, n, nrs;
  gchar *buf;
  Song  *song;
  gchar *filename = NULL;
  gint rmres;
  gboolean result = TRUE;
  static gboolean abort;
  GtkWidget *dialog;

#ifdef G_THREADS_ENABLED
  GThread *thread = NULL;
  GTimeVal time;
  if (!mutex) mutex = g_mutex_new ();
  if (!cond) cond = g_cond_new ();
#endif

  abort = FALSE;
  /* Set up dialogue to abort */
  dialog = gtk_message_dialog_new (
      GTK_WINDOW (gtkpod_window),
      GTK_DIALOG_DESTROY_WITH_PARENT,
      GTK_MESSAGE_INFO,
      GTK_BUTTONS_CANCEL,
      _("Press button to abort.\nExport can be continued at a later time."));
  /* Indicate that user wants to abort */
  g_signal_connect_swapped (GTK_OBJECT (dialog), "response",
			    G_CALLBACK (flush_songs_abort),
			    &abort);
  gtk_widget_show (dialog);
  while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();

  /* lets clean up those pending deletions */
  while (!abort && pending_deletion)
  {
      song = (Song*)pending_deletion->data;
      if((filename = get_song_name_on_ipod(song)))
      {
	  if(g_strstr_len(filename, strlen(cfg->ipod_mount), cfg->ipod_mount))
	  {
#ifdef G_THREADS_ENABLED
	      mutex_data = FALSE;
	      thread = g_thread_create (th_remove, filename, TRUE, NULL);
	      if (thread)
	      {
		  g_mutex_lock (mutex);
		  do
		  {
		      while (widgets_blocked && gtk_events_pending ())
			  gtk_main_iteration ();
		      /* wait a maximum of 10 ms */
		      g_get_current_time (&time);
		      g_time_val_add (&time, 20000);
		      g_cond_timed_wait (cond, mutex, &time);
		  } while(!mutex_data);
		  g_mutex_unlock (mutex);
		  rmres = (gint)g_thread_join (thread);
		  if (rmres == -1) result = FALSE;
	      }
	      else {
		  g_warning ("Thread creation failed, falling back to default.\n");
		  remove (filename);
	      }
#else
	      rmres = remove(filename);
	      if (rmres == -1) result = FALSE;
/*	      fprintf(stderr, "Removed %s-%s(%d)\n%s\n", song->artist,
						    song->title, song->ipod_id,
						    filename);*/
#endif 
	  }
	  g_free(filename);
      }
      free_song(song);
      pending_deletion = g_list_delete_link (pending_deletion, pending_deletion);
      while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
  }
  if (abort) result = FALSE;
  if (result == FALSE)
  {
      gtkpod_statusbar_message (_("Some songs could not be deleted from the iPod. Export aborted!"));
  }
  else
  {
      /* we now have as much space as we're gonna have, copy files to ipod */

      /* count number of songs to be transferred */
      n = get_nr_of_nontransferred_songs ();
      if (n != 0)  disable_gtkpod_import_buttons();
      count = 0; /* songs transferred */
      nrs = 0;
      while (!abort &&  (song = get_song_by_nr (nrs))) {
	  ++nrs;
	  if (!song->transferred)
	  {
#ifdef G_THREADS_ENABLED
	      mutex_data = FALSE;
	      thread = g_thread_create (th_copy, song, TRUE, NULL);
	      if (thread)
	      {
		  g_mutex_lock (mutex);
		  do
		  {
		      while (widgets_blocked && gtk_events_pending ())
			  gtk_main_iteration ();
		      /* wait a maximum of 10 ms */
		      g_get_current_time (&time);
		      g_time_val_add (&time, 20000);
		      g_cond_timed_wait (cond, mutex, &time);
		  } while(!mutex_data);
		  g_mutex_unlock (mutex);
		  result &= (gboolean)g_thread_join (thread);
	      }
	      else {
		  g_warning ("Thread creation failed, falling back to default.\n");
		  result &= itunesdb_copy_song_to_ipod (cfg->ipod_mount,
							song, song->pc_path_locale);
		  /* delete old size */
		  if (song->transferred) song->oldsize = 0;
	      }
#else
	      result &= itunesdb_copy_song_to_ipod (cfg->ipod_mount,
						    song, song->pc_path_locale);
	      /* delete old size */
	      if (song->transferred) song->oldsize = 0;

#endif 
	      ++count;
	      if (count == 1)  /* we need longer timeout */
		  prefs_set_statusbar_timeout (3*STATUSBAR_TIMEOUT);
	      if (count == n)  /* we need to reset timeout */
		  prefs_set_statusbar_timeout (0);
	      buf = g_strdup_printf (ngettext ("Copied %d of %d new song.",
					       "Copied %d of %d new songs.", n),
				     count, n);
	      gtkpod_statusbar_message(buf);
	      g_free (buf);
	  }
	  while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
      } /* while (gl_song) */
      if (abort)      result = FALSE;   /* negative result if user aborted */
      if (result == FALSE)
	  gtkpod_statusbar_message (_("Some songs were not written to iPod. Export aborted!"));
  }
  prefs_set_statusbar_timeout (0);
  gtk_widget_destroy (dialog);
  while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
  return result;
}


/* used to handle export of database */
void handle_export (void)
{
  gchar *ipt, *ipe, *cft=NULL, *cfe=NULL, *cfgdir;
  gboolean success = TRUE;
  gchar *buf;

  block_widgets (); /* block user input */
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
      if ((success=flush_songs ()))
      { /* write iTunesDB to iPod */
	  if (!(success=itunesdb_write (cfg->ipod_mount)))
	      gtkpod_statusbar_message (_("Error writing iTunesDB to iPod. Export aborted!"));
	  /* else: write extended info (PC filenames, md5 hash) to iPod */
	  else if (prefs_get_write_extended_info ())
	  {
	      if(!(success = write_extended_info (ipe, ipt)))
		  gtkpod_statusbar_message (_("Extended information not written"));
	  }
	  /* else: delete extended information file, if it exists */
	  else if (g_file_test (ipe, G_FILE_TEST_EXISTS))
	  {
	      if (remove (ipe) != 0)
	      {
		  buf = g_strdup_printf (_("Extended information file not deleted: '%s\'"), ipe);
		  gtkpod_statusbar_message (buf);
		  g_free (buf);
	      }
	  }
      }
      /* if everything was successful, copy files to ~/.gtkpod */
      if (success && prefs_get_keep_backups ())
      {
	  if (cfgdir)
	  {
	      success = itunesdb_cp (ipt, cft);
	      if (success && prefs_get_write_extended_info())
		  success = itunesdb_cp (ipe, cfe);
	  }
	  if ((cfgdir == NULL) || (!success))
	      gtkpod_statusbar_message (_("Backups could not be created!"));
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
	  gtkpod_statusbar_message (_("Export not successful!"));
	  success = FALSE;
	}
    }

  /* indicate that files and/or database is saved */
  if (success)   
  {
      files_saved = TRUE;
      /* block menu item and button */
      disable_gtkpod_import_buttons();
      gtkpod_statusbar_message(_("iPod Database Saved"));
  }

  C_FREE (cfgdir);
  C_FREE (cft);
  C_FREE (cfe);
  C_FREE (ipt);
  C_FREE (ipe);

  release_widgets (); /* Allow input again */
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
