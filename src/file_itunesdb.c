/* Time-stamp: <2004-11-19 00:02:23 jcs>
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
|
|  $Id$
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>
#include <stdlib.h>
#include "charset.h"
#include "display.h"
#include "file.h"
#include "itunesdb.h"
#include "md5.h"
#include "misc.h"
#include "prefs.h"
#include "support.h"
#include "tools.h"

/*------------------------------------------------------------------*\
 *                                                                  *
 *      Handle Import of iTunesDB                                   *
 *                                                                  *
\*------------------------------------------------------------------*/

/* only used when reading extended info from file */
/* see definition of Track in track.h for explanations */
struct track_extended_info
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
    guint32 rating;        /* still read but never written */
    guint32 peak_signal;
    gdouble radio_gain;
    gdouble audiophile_gain;
    gboolean peak_signal_set;
    gboolean radio_gain_set;
    gboolean audiophile_gain_set;
    gboolean transferred;
};

/* List with tracks pending deletion */
static GList *pending_deletion = NULL;
/* Flag to indicate if it's safe to quit (i.e. all tracks exported or
   at least a offline database written). It's state is set to TRUE in
   handle_export().  It's state can be accessed by the public function
   file_are_saved(). It can be set to FALSE by calling
   data_changed() */
static gboolean files_saved = TRUE;
/* Flag to indicated that an iTunesDB has been read */
static gboolean itunesdb_read = FALSE;

#ifdef G_THREADS_ENABLED
/* Thread specific */
static  GMutex *mutex = NULL;
static GCond  *cond = NULL;
static gboolean mutex_data = FALSE;
#endif
/* Used to keep the "extended information" until the iTunesDB is
   loaded */
static GHashTable *extendedinfohash = NULL;
static GHashTable *extendedinfohash_md5 = NULL;
static float extendedinfoversion = 0.0;


/* Has the iTunesDB already been read? */
gboolean file_itunesdb_read (void)
{
    return itunesdb_read;
}


/* fills in extended info if available (called from add_track()) */
void fill_in_extended_info (Track *track)
{
  gint ipod_id=0;
  struct track_extended_info *sei=NULL;

  if (extendedinfohash && track->ipod_id)
  {
      ipod_id = track->ipod_id;
      sei = g_hash_table_lookup (extendedinfohash, &ipod_id);
  }
  if (extendedinfohash_md5)
  {
      if (!track->md5_hash)
      {
	  gchar *filename = get_track_name_on_ipod (track);
	  track->md5_hash = md5_hash_on_filename (filename, FALSE);
	  g_free (filename);
      }
      if (track->md5_hash)
      {
	  sei = g_hash_table_lookup (extendedinfohash_md5, track->md5_hash);
      }
  }
  if (sei) /* found info for this id! */
  {
      if (sei->pc_path_locale && !track->pc_path_locale)
	  track->pc_path_locale = g_strdup (sei->pc_path_locale);
      if (sei->pc_path_utf8 && !track->pc_path_utf8)
	  track->pc_path_utf8 = g_strdup (sei->pc_path_utf8);
      if (sei->md5_hash && !track->md5_hash)
	  track->md5_hash = g_strdup (sei->md5_hash);
      if (sei->charset && !track->charset)
	  track->charset = g_strdup (sei->charset);
      if (sei->hostname && !track->hostname)
	  track->hostname = g_strdup (sei->hostname);
      track->oldsize = sei->oldsize;
      track->playcount += sei->playcount;
      if (sei->peak_signal_set)
      {
	  track->peak_signal_set = sei->peak_signal_set;
	  track->peak_signal = sei->peak_signal;
      }
      if (extendedinfoversion > 0.81)
      {
	  /* before 0.82 we used gint instead of double, so re-reading
	     the tags is safer (0.81 was CVS only and was a bit messed
	     up for a while) */
	  if (sei->radio_gain_set)
	  {
	      track->radio_gain_set = sei->radio_gain_set;
	      track->radio_gain = sei->radio_gain;
	  }
	  if (sei->audiophile_gain_set)
	  {
	      track->audiophile_gain_set = sei->audiophile_gain_set;
	      track->audiophile_gain = sei->audiophile_gain;
	  }
      }
      /* FIXME: This means that the rating can never be reset to 0
       * by the iPod */
      if (track->rating == 0)
	  track->rating = sei->rating;
      track->transferred = sei->transferred;
      /* don't remove the md5-hash -- there may be duplicates... */
      if (extendedinfohash)
	  g_hash_table_remove (extendedinfohash, &ipod_id);
  }
}


/* Used to free the memory of hash data */
static void hash_delete (gpointer data)
{
    struct track_extended_info *sei = data;

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
    if (extendedinfohash_md5)
	g_hash_table_destroy (extendedinfohash_md5);
    extendedinfohash = NULL;
    extendedinfohash_md5 = NULL;
    extendedinfoversion = 0.0;
}

/* Read extended info from "name" and check if "itunes" is the
   corresponding iTunesDB (using the itunes_hash value in "name").
   The data is stored in a hash table with the ipod_id as key.
   This hash table is used by add_track() to fill in missing information */
/* Return TRUE on success, FALSE otherwise */
static gboolean read_extended_info (gchar *name, gchar *itunes)
{
    gchar *md5, buf[PATH_MAX], *arg, *line, *bufp;
    gboolean success = TRUE;
    gboolean expect_hash, hash_matched=FALSE;
    gint len;
    struct track_extended_info *sei = NULL;
    FILE *fp;


    fp = fopen (name, "r");
    if (!fp)
    {
	gtkpod_warning (_("Could not open \"iTunesDB.ext\" for reading extended info.\n"),
			name);
	return FALSE;
    }
    md5 = md5_hash_on_filename (itunes, FALSE);
    if (!md5)
    {
	gtkpod_warning (_("Could not create hash value from itunesdb\n"));
	fclose (fp);
	return FALSE;
    }
    /* Create hash table */
    destroy_extendedinfohash ();
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
		    hash_matched = FALSE;
		    gtkpod_warning (_("iTunesDB '%s' does not match checksum in extended information file '%s'\ngtkpod will try to match the information using MD5 checksums. If successful, this may take a while.\n\n"), itunes, name);
/*		    success = FALSE;
		    break;*/
		}
		else
		{
		    hash_matched = TRUE;
		}
		expect_hash = FALSE;
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
			if (hash_matched)
			{
			    if (!extendedinfohash)
			    {
				extendedinfohash = g_hash_table_new_full (
				    g_int_hash,g_int_equal, NULL,hash_delete);
			    }
			    g_hash_table_insert (extendedinfohash,
						 &sei->ipod_id, sei);
			}
			else if (sei->md5_hash)
			{
			    if (!extendedinfohash_md5)
			    {
				extendedinfohash_md5 = g_hash_table_new_full (
				    g_str_hash,g_str_equal, NULL,hash_delete);
			    }
			    g_hash_table_insert (extendedinfohash_md5,
						 sei->md5_hash, sei);
			}
			else
			{
			    hash_delete ((gpointer)sei);
			}
		    }
		    else
		    { /* this is a deleted track that hasn't yet been
		         removed from the iPod's hard drive */
			Track *track = itunesdb_new_track ();
			track->ipod_path = g_strdup (sei->ipod_path);
			pending_deletion = g_list_append (pending_deletion,
							  track);
			hash_delete ((gpointer)sei); /* free sei */
		    }
		    sei = NULL;
		}
		if (strcmp (arg, "xxx") != 0)
		{
		    sei = g_malloc0 (sizeof (struct track_extended_info));
		    sei->ipod_id = atoi (arg);
		}
	    }
	    else if (g_ascii_strcasecmp (line, "version") == 0)
	    { /* found version */
		extendedinfoversion = g_ascii_strtod (arg, NULL);
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
	    {   /* only accept hash value if version is >= 0.53 or
		   PATH_MAX is 4096 -- in 0.53 I changed the MD5 hash
		   routine to using blocks of 4096 Bytes in
		   length. Before it was PATH_MAX, which might be
		   different on different architectures. */
		if ((extendedinfoversion >= 0.53) || (PATH_MAX == 4096))
		    sei->md5_hash = g_strdup (arg);
	    }
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
	    else if (g_ascii_strcasecmp (line, "peak_signal") == 0)
	    {
		sei->peak_signal_set = TRUE;
		sei->peak_signal = g_ascii_strtod (arg, NULL);
	    }
	    else if (g_ascii_strcasecmp (line, "radio_gain") == 0)
	    {
		sei->radio_gain_set = TRUE;
		sei->radio_gain = g_ascii_strtod (arg, NULL);
	    }
	    else if (g_ascii_strcasecmp (line, "audiophile_gain") == 0)
	    {
		sei->audiophile_gain_set = TRUE;
		sei->audiophile_gain = g_ascii_strtod (arg, NULL);
	    }
    }
    g_free (md5);
    fclose (fp);
    if (success && !hash_matched && !extendedinfohash_md5)
    {
	gtkpod_warning (_("No MD5 checksums on individual tracks are available.\n\nTo avoid this situation in the future either switch on duplicate detection (will provide MD5 checksums) or avoid using the iPod with programs other than gtkpod.\n\n"), itunes, name);
	success = FALSE;
    }
    if (!success) destroy_extendedinfohash ();
    return success;
}


/* Handle the function "Import iTunesDB" */
/* The import button is disabled once you have imported an existing
   database or exported your new data. Therefore, no specific check
   has to be performed on whether it's OK to import or not. */
void handle_import (void)
{
    gchar *cfgdir;
    gboolean success, md5tracks;
    guint32 n;

    /* we must switch off duplicate detection during import --
     * otherwise we mess up the playlists */

    md5tracks = prefs_get_md5tracks ();
    prefs_set_md5tracks (FALSE);

    n = get_nr_of_tracks (); /* how many tracks are already there? */

    if (!prefs_get_block_display ())  block_widgets ();
    if (!prefs_get_offline())
    { /* iPod is connected */
	const gchar *ext_db[] = { "iPod_Control","iTunes","iTunesDB.ext",NULL};
	const gchar *db[] = {"iPod_Control","iTunes","iTunesDB",NULL};
	gchar *ipod_mount_filename = charset_from_utf8(prefs_get_ipod_mount());
	gchar *name_ext = resolve_path(ipod_mount_filename,ext_db);
	gchar *name_db = resolve_path(ipod_mount_filename,db);
	if (name_db)
	{
	    if (prefs_get_write_extended_info())
	    {
		success = read_extended_info (name_ext, name_db);
		if (!success)
		{
		    gtkpod_warning (_("Extended info will not be used.\n"));
		}
	    }
	    display_enable_disable_view_sort (FALSE);
	    if(itunesdb_parse (ipod_mount_filename))
		gtkpod_statusbar_message(_("iPod Database Successfully Imported"));
	    else
		gtkpod_statusbar_message(_("iPod Database Import Failed"));
	    display_enable_disable_view_sort (TRUE);
	}
	else
	{
	    gchar *name = g_build_filename (
		ipod_mount_filename,
		"iPod_Control","iTunes","iTunesDB",NULL);
	    gtkpod_warning (_("'%s' does not exist. Import aborted.\n\n"), name);
	    g_free (name);
	}
	g_free (name_ext);
	g_free (name_db);
	g_free(ipod_mount_filename);
    }
    else
    { /* offline - requires extended info */
	if ((cfgdir = prefs_get_cfgdir ()))
	{
	    gchar *name_ext = g_build_filename (cfgdir, "iTunesDB.ext", NULL);
	    gchar *name_db = g_build_filename (cfgdir, "iTunesDB", NULL);
	    if (g_file_test (name_db, G_FILE_TEST_EXISTS))
	    {
		success = read_extended_info (name_ext, name_db);
		if (!success)
		{
		    gtkpod_warning (_("Extended info will not be used. If you have non-transferred tracks,\nthese will be lost.\n"));
		}
		display_enable_disable_view_sort (FALSE);
		if(itunesdb_parse_file (name_db))
		{
		    gtkpod_statusbar_message(
			_("Offline iPod Database Successfully Imported"));
		}
		else
		{
		    gtkpod_statusbar_message(
			_("Offline iPod Database Import Failed"));
		}
		display_enable_disable_view_sort (TRUE);
	    }
	    else
	    {
		gtkpod_warning (_("'%s' does not exist. Import aborted.\n\n"), name_db);
	    }
	    g_free (name_ext);
	    g_free (name_db);
	    g_free (cfgdir);
	}
	else
	{
	    gtkpod_warning (_("Import aborted.\n"));
	}
    }
    /* We need to make sure that the tracks that already existed
       in the DB when we called itunesdb_parse() do not duplicate
       any existing ID */
    renumber_ipod_ids ();

    gtkpod_tracks_statusbar_update();

    if (n != get_nr_of_tracks ())
    { /* Import was successfull, block menu item and button */
	display_disable_gtkpod_import_buttons();
	itunesdb_read = TRUE;
    }
    /* reset duplicate detection -- this will also detect and correct
     * all duplicate tracks currently in the database */
    prefs_set_md5tracks (md5tracks);
    destroy_extendedinfohash (); /* delete hash information (if present) */

    /* run update of offline playcounts */
    parse_offline_playcount ();

    display_set_check_ipod_menu ();/* taking care about 'Check IPOD files'mi */

    space_data_update ();          /* update space display */

    if (!prefs_get_block_display ())  release_widgets ();
}


/* Like get_track_name_on_disk(), but verifies the track actually
   exists.
   Must g_free return value after use */
gchar *get_track_name_on_disk_verified (Track *track)
{
    gchar *name = NULL;

    if (track)
    {
	if (!prefs_get_offline ())
	{
	    name = get_track_name_on_ipod (track);
	    if (name)
	    {
		if (!g_file_test (name, G_FILE_TEST_EXISTS))
		{
		    g_free (name);
		    name = NULL;
		}
	    }
	}
	if(!name && track->pc_path_locale && (*track->pc_path_locale))
	{
	    if (g_file_test (track->pc_path_locale, G_FILE_TEST_EXISTS))
		name = g_strdup (track->pc_path_locale);
	}
    }
    return name;
}


/* Get track name from source @source */
gchar *get_track_name_from_source (Track *track, FileSource source)
{
    gchar *result = NULL;

    g_return_val_if_fail (track != NULL, result);

    switch (source)
    {
    case SOURCE_PREFER_LOCAL:
	if (track->pc_path_locale && (*track->pc_path_locale))
	{
	    if (g_file_test (track->pc_path_locale,G_FILE_TEST_EXISTS))
	    {
		result = g_strdup (track->pc_path_locale);
	    }
	}
	if (!result) result = get_track_name_on_ipod (track);
	break;
    case SOURCE_LOCAL:
	if (track->pc_path_locale && (*track->pc_path_locale))
	{
	    if (g_file_test (track->pc_path_locale,G_FILE_TEST_EXISTS))
	    {
		result = g_strdup (track->pc_path_locale);
	    }
	}
	break;
    case SOURCE_IPOD:
	result = get_track_name_on_ipod (track);
	break;
    }
    return result;
}



/*------------------------------------------------------------------*\
 *                                                                  *
 *      Functions concerning deletion of tracks                      *
 *                                                                  *
\*------------------------------------------------------------------*/


/* in Bytes, minus the space taken by tracks that will be overwritten
 * during copying */
/* @num will be filled with the number of tracks if != NULL */
double get_filesize_of_deleted_tracks (guint32 *num)
{
    double size = 0;
    guint32 n = 0;
    Track *track;
    GList *gl_track;

    for (gl_track = pending_deletion; gl_track; gl_track=gl_track->next)
    {
	track = (Track *)gl_track->data;
	if (track->transferred)   size += track->size;
	++n;
    }
    if (num)  *num = n;
    return size;
}

void mark_track_for_deletion (Track *track)
{
    pending_deletion = g_list_append(pending_deletion, track);
}

/* It might be necessary to unmark for deletion like in case of
   dangling tracks with no real files on ipod */
void unmark_track_for_deletion (Track *track)
{
    if (track != NULL)
        pending_deletion = g_list_remove(pending_deletion, track);
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
  FILE *fp;
  guint n,i;
  Track *track;
  gchar *md5;

  space_data_update ();

  fp = fopen (name, "w");
  if (!fp)
  {
      gtkpod_warning (_("Could not open \"%s\" for writing extended info.\n"),
		      name);
      return FALSE;
  }
  md5 = md5_hash_on_filename (itunes, FALSE);
  if (md5)
  {
      fprintf(fp, "itunesdb_hash=%s\n", md5);
      g_free (md5);
  }
  else
  {
      gtkpod_warning (_("Aborted writing of extended info.\n"));
      fclose (fp);
      return FALSE;
  }
  fprintf (fp, "version=%s\n", VERSION);
  n = get_nr_of_tracks ();
  for (i=0; i<n; ++i)
  {
      track = get_track_by_nr (i);
      fprintf (fp, "id=%d\n", track->ipod_id);
      if (track->hostname)
	  fprintf (fp, "hostname=%s\n", track->hostname);
      if (strlen (track->pc_path_locale) != 0)
	  fprintf (fp, "filename_locale=%s\n", track->pc_path_locale);
      if (strlen (track->pc_path_utf8) != 0)
	  fprintf (fp, "filename_utf8=%s\n", track->pc_path_utf8);
      /* this is just for convenience for people looking for a track
	 on the ipod away from gktpod/itunes etc. */
      if (strlen (track->ipod_path) != 0)
	  fprintf (fp, "filename_ipod=%s\n", track->ipod_path);
      if (track->md5_hash)
	  fprintf (fp, "md5_hash=%s\n", track->md5_hash);
      if (track->charset)
	  fprintf (fp, "charset=%s\n", track->charset);
      if (!track->transferred && track->oldsize)
	  fprintf (fp, "oldsize=%d\n", track->oldsize);
      if (track->peak_signal_set)
      {
	  gchar buf[20];
	  g_ascii_dtostr (buf, 20, (gdouble)track->peak_signal);
	  fprintf (fp, "peak_signal=%s\n", buf);
      }
      if (track->radio_gain_set)
      {
	  gchar buf[20];
	  g_ascii_dtostr (buf, 20, track->radio_gain);
	  fprintf (fp, "radio_gain=%s\n", buf);
      }
      if (track->audiophile_gain_set)
      {
	  gchar buf[20];
	  g_ascii_dtostr (buf, 20, track->audiophile_gain);
	  fprintf (fp, "audiophile_gain=%s\n", buf);
      }
      fprintf (fp, "transferred=%d\n", track->transferred);
      while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
  }
  if (prefs_get_offline())
  { /* we are offline and also need to export the list of tracks that
       are to be deleted */
      GList *gl_track;
      for(gl_track = pending_deletion; gl_track; gl_track = gl_track->next)
      {
	  track = (Track*)gl_track->data;
	  fprintf (fp, "id=000\n");  /* our sign for tracks pending
					deletion */
	  fprintf (fp, "filename_ipod=%s\n", track->ipod_path);
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
/* Threaded copy of ipod track */
static gpointer th_copy (gpointer s)
{
    gboolean result;
    Track *track = (Track *)s;
    gchar *mount = charset_from_utf8 (prefs_get_ipod_mount ());

    result = itunesdb_copy_track_to_ipod (mount,
					 track,
					 track->pc_path_locale);
    g_free (mount);
    /* delete old size */
    if (track->transferred) track->oldsize = 0;
    g_mutex_lock (mutex);
    mutex_data = TRUE;   /* signal that thread will end */
    g_cond_signal (cond);
    g_mutex_unlock (mutex);
    return (gpointer)result;
}
#endif

/* This function is called when the user presses the abort button
 * during flush_tracks() */
static void flush_tracks_abort (gboolean *abort)
{
    *abort = TRUE;
}


/* check if iPod directory stucture is present */
static gboolean ipod_dirs_present (void)
{
    gchar *ipod_path_as_filename = 
      charset_from_utf8(prefs_get_ipod_mount ());
    const gchar *music[] = {"iPod_Control","Music",NULL},
      *itunes[] = {"iPod_Control","iTunes",NULL};
    gchar *file;
    gboolean result = TRUE;

    file = resolve_path(ipod_path_as_filename,music);
    if(!file || !g_file_test(file,G_FILE_TEST_IS_DIR))
      result = FALSE;
    g_free(file);
    
    file = resolve_path(ipod_path_as_filename,itunes);
    if(!file || !g_file_test(file,G_FILE_TEST_IS_DIR))
      result = FALSE;
    g_free(file);
    
    g_free(ipod_path_as_filename);

    return result;
}


/* Flushes all non-transferred tracks to the iPod filesystem
   Returns TRUE on success, FALSE if some error occured */
static gboolean flush_tracks (void)
{
  gint count, n, nrs;
  gchar *buf;
  Track  *track;
  gchar *filename = NULL;
  gint rmres;
  gboolean result = TRUE;
  static gboolean abort;
  GtkWidget *dialog, *progress_bar, *label, *image, *hbox;
  time_t diff, start, fullsecs, hrs, mins, secs;
  gchar *progtext = NULL;

#ifdef G_THREADS_ENABLED
  GThread *thread = NULL;
  GTimeVal gtime;
  if (!mutex) mutex = g_mutex_new ();
  if (!cond) cond = g_cond_new ();
#endif

  n = get_nr_of_nontransferred_tracks ();

  if (n==0 && !pending_deletion) return TRUE;

  abort = FALSE;
  /* create the dialog window */
  dialog = gtk_dialog_new_with_buttons (_("Information"),
                                         GTK_WINDOW (gtkpod_window),
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_STOCK_CANCEL,
                                         GTK_RESPONSE_NONE,
                                         NULL);

  /* emulate gtk_message_dialog_new */
  image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_INFO,
				    GTK_ICON_SIZE_DIALOG);
  label = gtk_label_new (
      _("Press button to abort.\nExport can be continued at a later time."));

  gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);

  /* hbox to put the image+label in */
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  /* Create the progress bar */
  progress_bar = gtk_progress_bar_new ();
  progtext = g_strdup (_("deleting..."));
  gtk_progress_bar_set_text(GTK_PROGRESS_BAR (progress_bar), progtext);
  g_free (progtext);

  /* Indicate that user wants to abort */
  g_signal_connect_swapped (GTK_OBJECT (dialog), "response",
			    G_CALLBACK (flush_tracks_abort),
			    &abort);

  /* Add the image/label + progress bar to dialog */
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
                      hbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
                      progress_bar, FALSE, FALSE, 0);
  gtk_widget_show_all (dialog);

  while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();

  /* lets clean up those pending deletions */
  while (!abort && pending_deletion)
  {
      track = (Track*)pending_deletion->data;
      if((filename = get_track_name_on_ipod(track)))
      {
	  const gchar *mp = prefs_get_ipod_mount ();
	  if(mp && g_strstr_len(filename, strlen(mp), mp))
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
		      /* wait a maximum of 20 ms */
		      g_get_current_time (&gtime);
		      g_time_val_add (&gtime, 20000);
		      g_cond_timed_wait (cond, mutex, &gtime);
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
/*	      fprintf(stderr, "Removed %s-%s(%d)\n%s\n", track->artist,
						    track->title, track->ipod_id,
						    filename);*/
#endif
	  }
	  g_free(filename);
      }
      free_track(track);
      pending_deletion = g_list_delete_link (pending_deletion, pending_deletion);
      while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
  }
  if (abort) result = FALSE;
  if (result == FALSE)
  {
      gtkpod_statusbar_message (_("Some tracks could not be deleted from the iPod. Export aborted!"));
  }
  else
  {
      /* we now have as much space as we're gonna have, copy files to ipod */

      progtext = g_strdup (_("preparing to copy..."));
      gtk_progress_bar_set_text(GTK_PROGRESS_BAR (progress_bar), progtext);
      g_free (progtext);
      while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();

      /* count number of tracks to be transferred */
      if (n != 0)  display_disable_gtkpod_import_buttons();
      count = 0; /* tracks transferred */
      nrs = 0;
      start = time(NULL);
      while (!abort &&  (track = get_track_by_nr (nrs))) {
	  ++nrs;
	  if (!track->transferred)
	  {
#ifdef G_THREADS_ENABLED
	      mutex_data = FALSE;
	      thread = g_thread_create (th_copy, track, TRUE, NULL);
	      if (thread)
	      {
		  g_mutex_lock (mutex);
		  do
		  {
		      while (widgets_blocked && gtk_events_pending ())
			  gtk_main_iteration ();
		      /* wait a maximum of 10 ms */
		      g_get_current_time (&gtime);
		      g_time_val_add (&gtime, 20000);
		      g_cond_timed_wait (cond, mutex, &gtime);
		  } while(!mutex_data);
		  g_mutex_unlock (mutex);
		  result &= (gboolean)g_thread_join (thread);
	      }
	      else {
		  gchar *mount = charset_from_utf8 (prefs_get_ipod_mount ());
		  g_warning ("Thread creation failed, falling back to default.\n");
		  result &= itunesdb_copy_track_to_ipod (
		      mount, track, track->pc_path_locale);
		  /* delete old size */
		  if (track->transferred) track->oldsize = 0;
		  g_free (mount);
	      }
#else
	      gchar *mount = charset_from_utf8 (prefs_get_ipod_mount ());
	      result &= itunesdb_copy_track_to_ipod (mount, track,
						     track->pc_path_locale);
	      /* delete old size */
	      if (track->transferred) track->oldsize = 0;
	      g_free (mount);
#endif
	      data_changed (); /* otherwise new free space status from
	      iPod is never read and free space keeps increasing while
	      we copy more and more files to the iPod */
	      ++count;
	      if (count == 1)  /* we need longer timeout */
		  prefs_set_statusbar_timeout (3*STATUSBAR_TIMEOUT);
	      if (count == n)  /* we need to reset timeout */
		  prefs_set_statusbar_timeout (0);
	      buf = g_strdup_printf (ngettext ("Copied %d of %d new track.",
					       "Copied %d of %d new tracks.", n),
				     count, n);
	      gtkpod_statusbar_message(buf);
	      g_free (buf);

              gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR (progress_bar),
					    (gdouble) count/n);

	      diff = time(NULL) - start;
	      fullsecs = (diff*n/count)-diff+5;
	      hrs  = fullsecs / 3600;
	      mins = (fullsecs % 3600) / 60;
	      secs = ((fullsecs % 60) / 5) * 5;
	      /* don't bounce up too quickly (>10% change only) */
/*	      left = ((mins < left) || (100*mins >= 110*left)) ? mins : left;*/
	      progtext = g_strdup_printf (
		  _("%d%% (%d:%02d:%02d left)"),
		  count*100/n, (int)hrs, (int)mins, (int)secs);
              gtk_progress_bar_set_text(GTK_PROGRESS_BAR (progress_bar),
					progtext);
	      g_free (progtext);
	  }
	  while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
      } /* while (gl_track) */
      if (abort)      result = FALSE;   /* negative result if user aborted */
      if (result == FALSE)
	  gtkpod_statusbar_message (_("Some tracks were not written to iPod. Export aborted!"));
  }
  prefs_set_statusbar_timeout (0);
  gtk_widget_destroy (dialog);
  while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
  return result;
}


/* used to handle export of database */
void handle_export (void)
{
  gchar *cft=NULL, *cfe=NULL, *cfgdir;
  gboolean success = TRUE;
  gchar *buf;

  cfgdir = prefs_get_cfgdir ();
  if (cfgdir)
  {
      cft = g_build_filename (cfgdir, "iTunesDB", NULL);
      cfe = g_build_filename (cfgdir, "iTunesDB.ext", NULL);
  }

  if (!file_itunesdb_read())
  {   /* No iTunesDB was read but user wants to export current
         data. If an iTunesDB is present on the iPod or in cfgdir,
	 this is most likely an error. We should tell the user */
      gboolean danger = FALSE;
      /* First check if we can find an existing iTunesDB. If yes, set
	 'danger' to TRUE */
      if (prefs_get_offline ())
      {
	  if (g_file_test (cft, G_FILE_TEST_EXISTS))  danger = TRUE;
      }
      else
      {  /* online */
	  gchar *ipod_path_as_filename = 
	      charset_from_utf8 (prefs_get_ipod_mount());
	  const gchar *itunes_components[] = {"iPod_Control", "iTunes", NULL};
	  gchar *itunes_filename = resolve_path(ipod_path_as_filename,
						itunes_components);
	  if (g_file_test (itunes_filename, G_FILE_TEST_EXISTS)) danger = TRUE;
	  g_free (ipod_path_as_filename);
	  g_free (itunes_filename);
      }
      if (danger)
      {
	GtkWidget *dialog = gtk_message_dialog_new (
	    GTK_WINDOW (gtkpod_window),
	    GTK_DIALOG_DESTROY_WITH_PARENT,
	    GTK_MESSAGE_WARNING,
	    GTK_BUTTONS_OK_CANCEL,
	    _("You did not import the existing iTunesDB. This is most likely incorrect and will result in the loss of the existing database.\n\nPress 'OK' if you want to proceed anyhow or 'Cancel' to abort. If you cancel, you can import the existing database before calling this function again.\n"));
	gint result = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	if (result == GTK_RESPONSE_CANCEL)
	{
	    g_free (cft);
	    g_free (cfe);
	    return;
	}
      }
  }

  block_widgets (); /* block user input */
  /* read offline playcounts -- in case we added some tracks we can
     now handle */
  parse_offline_playcount ();

  if(!prefs_get_offline ())
  {
      /* check if iPod directories are present */
      if (!ipod_dirs_present ())
      {   /* no -- create them */
	  ipod_directories_head ();
	  /* if still not present abort */
	  if (!ipod_dirs_present ())
	  {
	      gtkpod_warning (_("iPod directory structure must be present before synching to the iPod can be performed.\n"));
	      success = FALSE;
	  }
      }
      if (success)
      {
	  /* write tracks to iPod */
	  success = flush_tracks ();
      }
  }

  if (success && cfgdir)
  {
      gtkpod_statusbar_message (_("Now writing iTunesDB. Please wait..."));
      while (widgets_blocked && gtk_events_pending ())
	  gtk_main_iteration ();
      success = itunesdb_write_to_file (cft);
      if (success)
	  success = write_extended_info (cfe, cft);
  }

  /* now copy to iPod */
  if(success && !prefs_get_offline ())
  {
      gchar *ipt = NULL, *ipe = NULL;
      gchar *ipod_path_as_filename = 
	  charset_from_utf8 (prefs_get_ipod_mount());
      const gchar *itunes_components[] = {"iPod_Control", "iTunes", NULL};
      gchar *itunes_filename = resolve_path(ipod_path_as_filename,
					    itunes_components);
      ipt = g_build_filename (itunes_filename, "iTunesDB", NULL);
      ipe = g_build_filename (itunes_filename, "iTunesDB.ext", NULL);

      /* copy iTunesDB to iPod */
      while (widgets_blocked && gtk_events_pending ())
	  gtk_main_iteration ();
      success = itunesdb_cp (cft, ipt);
      if (!success)
      {
	  gtkpod_statusbar_message (_("Error writing iTunesDB to iPod. Export aborted!"));
      }
      /* else: copy extended info (PC filenames, md5 hash) to iPod */
      else if (prefs_get_write_extended_info ())
      {
	  success = itunesdb_cp (cfe, ipe);
	  if(!success)
	  {
	      gtkpod_statusbar_message (_("Extended information not written"));
	  }
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
      if (prefs_get_concal_autosync ())
      {
	  const gchar *str;
	  gtkpod_statusbar_message (_("Syncing contacts and calendar..."));
	  str = prefs_get_path (PATH_SYNC_CONTACTS);
	  if (str && *str)    tools_sync_contacts ();
	  str = prefs_get_path (PATH_SYNC_CALENDAR);
	  if (str && *str)    tools_sync_calendar ();
      }
      g_free (ipt);
      g_free (ipe);
      /* move old playcount file etc out of the way */
      if (success)
	  itunesdb_rename_files (ipod_path_as_filename);
      g_free (ipod_path_as_filename);
  }

  /* indicate that files and/or database is saved */
  if (success)
  {
      files_saved = TRUE;
      /* block menu item and button */
      display_disable_gtkpod_import_buttons();
      gtkpod_statusbar_message(_("iPod Database Saved"));
  }

  g_free (cfgdir);
  g_free (cft);
  g_free (cfe);

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
  space_data_update ();
}  
