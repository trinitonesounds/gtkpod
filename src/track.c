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
#include "itunesdb.h"
#include "display.h"
#include "confirmation.h"
#include "charset.h"

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
static void reset_ipod_id (void);

/* Thread specific */
static  GMutex *mutex = NULL;
static GCond  *cond = NULL;
static gboolean mutex_data = FALSE;

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

/* Append song to the list */
/* Note: adding to the display model is the responsibility of
   the playlist code */
/* Returns: pointer to the added song -- may be different in the case
   of duplicates. In that case a pointer to the already existing song
   is returned. */
Song *add_song (Song *song)
{
  gint sz = sizeof (gunichar2);
  gint ipod_id;
  Song *oldsong, *result=NULL;
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

  if((oldsong = md5_song_exists (song)))
  {
      remove_duplicate (oldsong, song);
      free_song(song);
      result = oldsong;
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
	free_ipod_id(song->ipod_id);
    
    songs = g_list_append (songs, song);
    result = song;
  }
  return result;
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
	pending_deletion = g_list_append(pending_deletion, song);
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


/* Returns the n_th song */
Song *get_song_by_nr (guint32 n)
{
  return g_list_nth_data (songs, n);
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
static gpointer th_copy (gpointer song)
{
    gboolean result;

    result = copy_song_to_ipod (cfg->ipod_mount,
				(Song *)song,
				((Song *)song)->pc_path_locale);
    g_mutex_lock (mutex);
    mutex_data = TRUE;   /* signal that thread will end */
    g_cond_signal (cond);
    g_mutex_unlock (mutex);
    return (gpointer)result;
}
#endif G_THREADS_ENABLED

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
  gint count, n;
  gchar *buf;
  Song  *song;
  GList *gl_song;
  gchar *filename = NULL;
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
	      thread = g_thread_create (th_remove, filename, FALSE, NULL);
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
		  printf ("Thread creation failed, falling back to default.\n");
		  remove (filename);
	      }
#else
	      remove(filename);
/*	      fprintf(stderr, "Removed %s-%s(%d)\n%s\n", song->artist,
						    song->title, song->ipod_id,
						    filename);*/
#endif G_THREADS_ENABLED
	  }
	  g_free(filename);
      }
      free_song(song);
      pending_deletion = g_list_delete_link (pending_deletion, pending_deletion);
      while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
  }

  /* we now have as much space as we're gonna have, copy files to ipod */

  /* count number of songs to be transferred */
  n = 0;
  gl_song = g_list_first (songs);
  while (gl_song != NULL) {
    song = (Song *)gl_song->data;
    if (!song->transferred)
	++n;
    gl_song = g_list_next (gl_song);
    while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
  }

  count = 0; /* songs transferred */
  gl_song = g_list_first (songs);
  while (!abort && gl_song) {
    song = (Song *)gl_song->data;
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
	    printf ("Thread creation failed, falling back to default.\n");
	    result &= copy_song_to_ipod (cfg->ipod_mount, song, song->pc_path_locale);
	}
#else
	result &= copy_song_to_ipod (cfg->ipod_mount, song, song->pc_path_locale);
#endif G_THREADS_ENABLED
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
    gl_song = g_list_next (gl_song);
  } /* while (gl_song) */
  prefs_set_statusbar_timeout (0);
  gtk_widget_destroy (dialog);
  if (abort)      result = FALSE;   /* negative result if user aborted */
  while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
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
    gboolean success, md5songs;
    guint32 n;
    GList *gl_song;

    if (itunes_import_ok () == FALSE)
    {
	gtkpod_warning (_("You cannot import an iTunesDB again!\n"));
	return;
    }

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
    /* Change: we simply re-ID all songs. That way we don't run into
       trouble if some "funny" guy put a "-1" into the itunesDB */
    reset_ipod_id (); /* reset lowest unused ipod ID */
    for (gl_song = songs; gl_song; gl_song=gl_song->next)
    {
	((Song *)gl_song->data)->ipod_id = free_ipod_id (0);
	/*we need to tell the display that the ID has changed (if ID
	 * is displayed) */
	/* pm_song_changed (song);*/
    }
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
	result = get_song_name_on_ipod (s);
	if(!result &&
	   (s->pc_path_locale) && (strlen(s->pc_path_locale) > 0))
	{
	    result = g_strdup (s->pc_path_locale);
	} 
    }
    return(result);
}

/* same as get_song_name_on_disk(), but only return a valid path to a
   song on the ipod */
gchar *get_song_name_on_ipod (Song *s)
{
    gchar *result = NULL;

    if(s)
    {
	if(!prefs_get_offline () &&
	   s->ipod_path &&
	   (strlen(s->ipod_path) > 0))
	{
	    guint i = 0, size = 0;
	    gchar *buf = g_strdup (s->ipod_path);
	    size = strlen(buf);
	    for(i = 0; i < size; i++)
		if(buf[i] == ':') buf[i] = '/';
	    result = concat_dir(cfg->ipod_mount, buf);
	    g_free (buf);
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
      if (!(success=flush_songs ()))
	  gtkpod_statusbar_message (_("Some songs were not written to iPod. Export aborted!"));
      /* else: write iTunesDB to iPod */
      else if (!(success=itunesdb_write (cfg->ipod_mount)))
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
       oldsong = md5_song_exists(song);
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
	    the song_nr */
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

   if ((oldsong == NULL) && (song == NULL) && str)
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
		   ngettext ("The following duplicate song has been skipped.",
			     "The following %d duplicate songs have been skipped.",
			     delsong_nr), delsong_nr);
	   }
	   gtkpod_confirmation
	       (-1,                      /* gint id, */
		FALSE,                   /* gboolean modal, */
		_("Duplicate detection"),/* title */
		buf,                     /* label */
		str->str,                /* scrolled text */
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
   if (oldsong && song)
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
		       add_song_to_playlist (pl, oldsong);
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


