/* Time-stamp: <2005-05-07 23:58:53 jcs>
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
#include "itdb.h"
#include "info.h"
#include "md5.h"
#include "misc.h"
#include "misc_track.h"
#include "prefs.h"
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


/* fills in extended info if available */
void fill_in_extended_info (Track *track)
{
  gint ipod_id=0;
  ExtraTrackData *etr;
  struct track_extended_info *sei=NULL;

  g_return_if_fail (track);
  etr = track->userdata;
  g_return_if_fail (etr);

  if (extendedinfohash && track->id)
  {
      /* copy id to gint value -- needed for the hash table functions */
      ipod_id = track->id;
      sei = g_hash_table_lookup (extendedinfohash, &ipod_id);
  }
  if (!sei && extendedinfohash_md5)
  {
      if (!etr->md5_hash)
      {
	  gchar *filename = get_track_name_on_ipod (track);
	  etr->md5_hash = md5_hash_on_filename (filename, FALSE);
	  g_free (filename);
      }
      if (etr->md5_hash)
      {
	  sei = g_hash_table_lookup (extendedinfohash_md5, etr->md5_hash);
      }
  }
  if (sei) /* found info for this id! */
  {
      if (sei->pc_path_locale && !etr->pc_path_locale)
	  etr->pc_path_locale = g_strdup (sei->pc_path_locale);
      if (sei->pc_path_utf8 && !etr->pc_path_utf8)
	  etr->pc_path_utf8 = g_strdup (sei->pc_path_utf8);
      if (sei->md5_hash && !etr->md5_hash)
	  etr->md5_hash = g_strdup (sei->md5_hash);
      if (sei->charset && !etr->charset)
	  etr->charset = g_strdup (sei->charset);
      if (sei->hostname && !etr->hostname)
	  etr->hostname = g_strdup (sei->hostname);
      etr->oldsize = sei->oldsize;
      track->playcount += sei->playcount;
      if (sei->peak_signal_set)
      {
	  etr->peak_signal_set = sei->peak_signal_set;
	  etr->peak_signal = sei->peak_signal;
      }
      if (extendedinfoversion > 0.81)
      {
	  /* before 0.82 we used gint instead of double, so re-reading
	     the tags is safer (0.81 was CVS only and was a bit messed
	     up for a while) */
	  if (sei->radio_gain_set)
	  {
	      etr->radio_gain_set = sei->radio_gain_set;
	      etr->radio_gain = sei->radio_gain;
	  }
	  if (sei->audiophile_gain_set)
	  {
	      etr->audiophile_gain_set = sei->audiophile_gain_set;
	      etr->audiophile_gain = sei->audiophile_gain;
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
   The data is stored in a hash table with the ipod_id as key.  This
   hash table is used by fill_in_extended_info() (called from
   gp_import_itdb()) to fill in missing information */
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
		    gtkpod_warning (_("iTunesDB '%s' does not match checksum in extended information file '%s'\ngtkpod will try to match the information using MD5 checksums. This may take a long time.\n\n"), itunes, name);
		    while (gtk_events_pending ())  gtk_main_iteration ();
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
			Track *track = gp_track_new ();
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


/* Import an iTunesDB and return an iTunesDB structure.
 * If @old_itdb is set, it will be merged into the newly imported
 * one. @old_itdb will not be changed.
 * @type: GP_ITDB_TYPE_LOCAL or GP_ITDB_TYPE_IPOD
 * @mp: mount point of iPod (if reading an iPod iTunesDB)
 * @name_off: name of the iTunesDB in offline mode
 * @name_loc: name of iTunesDB (if reading a local file browser) */
/* Return value: a new iTunesDB structure or NULL in case of an error */
iTunesDB *gp_import_itdb (iTunesDB *old_itdb, const gint type,
			  const gchar *mp, const gchar *name_off,
			  const gchar *name_loc)
{
    gchar *cfgdir = prefs_get_cfgdir ();
    GList *gl;
    ExtraiTunesDBData *eitdb;
    iTunesDB *itdb = NULL;
    GError *error = NULL;

    g_return_val_if_fail (!(type & GP_ITDB_TYPE_LOCAL) || name_loc, NULL);
    g_return_val_if_fail (!(type & GP_ITDB_TYPE_IPOD) || 
			  (mp && name_off), NULL);
    g_return_val_if_fail (cfgdir, NULL);

    if (prefs_get_offline() || (type & GP_ITDB_TYPE_LOCAL))
    { /* offline or local database - requires extended info */
	gchar *name_ext;
	gchar *name_db;

	if (type & GP_ITDB_TYPE_LOCAL)
	{
	    name_ext = g_strdup_printf ("%s.ext", name_loc);
	    name_db = g_strdup (name_loc);
	}
	else
	{
	    name_ext = g_strdup_printf ("%s.ext", name_off);
	    name_db = g_strdup (name_off);
	}

	if (g_file_test (name_db, G_FILE_TEST_EXISTS))
	{
	    if (prefs_get_write_extended_info ())
	    {
		if (!read_extended_info (name_ext, name_db))
		{
		    gtkpod_warning (_("Extended info will not be used. If you have non-transferred tracks,\nthese will be lost.\n"));
		}
	    }
	    itdb = itdb_parse_file (name_db, &error);
	    if (itdb && !error)
	    {
		if (type & GP_ITDB_TYPE_IPOD)
		    gtkpod_statusbar_message(
			_("Offline iPod database successfully imported"));
		else
		    gtkpod_statusbar_message(
			_("Local database successfully imported"));
	    }
	    else
	    {
		if (error)
		{
		    if (type & GP_ITDB_TYPE_IPOD)
			gtkpod_warning (
			    _("Offline iPod database import failed: '%s'\n\n"),
			    error->message);
		    else
			gtkpod_warning (
			    _("Local database import failed: '%s'\n\n"),
			    error->message);
		}
		else
		{
		    if (type & GP_ITDB_TYPE_IPOD)
			gtkpod_warning (
			    _("Offline iPod database import failed: \n\n"));
		    else
			gtkpod_warning (
			    _("Local database import failed: \n\n"));
		}
	    }
	}
	else
	{
	    gtkpod_warning (
		_("'%s' does not exist. Import aborted.\n\n"),
		name_db);
	}
	g_free (name_ext);
	g_free (name_db);
	g_free (cfgdir);
    }
    else
    { /* GP_ITDB_TYPE_IPOD _and_ iPod is connected */
	const gchar *ext_db[] = { "iPod_Control","iTunes","iTunesDB.ext",NULL};
	const gchar *db[] = {"iPod_Control","iTunes","iTunesDB",NULL};
	gchar *name_ext = resolve_path (mp, ext_db);
	gchar *name_db = resolve_path (mp, db);
	if (name_db)
	{
	    if (prefs_get_write_extended_info ())
	    {
		if (!read_extended_info (name_ext, name_db))
		{
		    gtkpod_warning (_("Extended info will not be used.\n"));
		}
	    }
	    itdb = itdb_parse (mp, &error);
	    if(itdb && !error)
	    {
		gtkpod_statusbar_message (
		    _("iPod Database Successfully Imported"));
	    }
	    else
	    {
		if (error)
		{
		    gtkpod_warning (
			_("iPod Database Import Failed: '%s'\n\n"),
			error->message);
		}
		else
		{
		    gtkpod_warning (
			_("iPod Database Import Failed.\n\n"));
		}
	    }
	}
	else
	{
	    gchar *name = g_build_filename (
		mp,
		"iPod_Control","iTunes","iTunesDB",NULL);
	    gtkpod_warning (_("'%s' does not exist. Import aborted.\n\n"),
			    name);
	    g_free (name);
	}
	g_free (name_ext);
	g_free (name_db);
    }

    if (!itdb) return NULL;

    /* add Extra*Data */
    gp_itdb_add_extra_full (itdb);
    /* validate all tracks and fill in extended info */
    for (gl=itdb->tracks; gl; gl=gl->next)
    {
	Track *track = gl->data;
	fill_in_extended_info (track);
	gp_track_validate_entries (track);
    }
    /* delete hash information (if present) */
    destroy_extendedinfohash ();
    /* find duplicates */
    gp_md5_hash_tracks_itdb (itdb);

    eitdb = itdb->userdata;
    g_return_val_if_fail (eitdb, NULL);

    /* fill in additional info */
    itdb->usertype = type;
    if (type & GP_ITDB_TYPE_IPOD)
    {
	if (prefs_get_offline ())
	{
	    itdb->mountpoint = g_strdup (mp);
	    g_free (itdb->filename);
	    itdb->filename = NULL;
	}
	eitdb->offline_filename = g_strdup (name_off);
    }	    

    eitdb->data_changed = FALSE;
    eitdb->itdb_imported = TRUE;

    if (old_itdb)
    {
	/* this table holds pairs of old_itdb-tracks/new_itdb/tracks */
	ExtraiTunesDBData *old_eitdb = old_itdb->userdata;
	GHashTable *track_hash = g_hash_table_new (g_direct_hash,
						   g_direct_equal);
	Playlist *mpl = itdb_playlist_mpl (itdb);
	g_return_val_if_fail (mpl, NULL);
	g_return_val_if_fail (old_eitdb, NULL);

	/* add tracks from @old_itdb to new itdb */
	for (gl=old_itdb->tracks; gl; gl=gl->next)
	{
	    Track *duptr, *addtr;
	    Track *track = gl->data;
	    g_return_val_if_fail (track, NULL);
	    duptr = itdb_track_duplicate (track);
	    /* add to database -- if duplicate detection is on and the
	       same track already exists in the database, the already
	       existing track is returned and @duptr is freed */
	    addtr = gp_track_add (itdb, duptr);
	    g_hash_table_insert (track_hash, track, addtr);
	    if (addtr == duptr)
	    {   /* Add to MPL */
		itdb_playlist_add_track (mpl, addtr, -1);
	    }
	}
	/* add playlists */
	gl = old_itdb->playlists;
	while (gl && gl->next)
	{
	    GList *glm;
	    Playlist *duppl;
	    Playlist *pl = gl->next->data; /* skip MPL */
	    g_return_val_if_fail (pl, NULL);
	    duppl = itdb_playlist_duplicate (pl);
	    /* switch members */
	    for (glm=duppl->members; glm; glm=glm->next)
	    {
		Track *newtr;
		g_return_val_if_fail (glm->data, NULL);
		newtr = g_hash_table_lookup (track_hash, glm->data);
		g_return_val_if_fail (newtr, NULL);
		glm->data = newtr;
	    }
	    itdb_playlist_add (itdb, duppl, -1);
	    gl = gl->next;
	}
	g_hash_table_destroy (track_hash);
	/* copy data_changed flag */
	eitdb->data_changed = old_eitdb->data_changed;
    }
    /* set mark that this itdb struct contains an imported iTunesDB */

    return itdb;
}

/* attempts to import all iPod databases */
void gp_merge_ipod_itdbs (void)
{
    struct itdbs_head *itdbs_head;
    GList *gl;

    g_return_if_fail (gtkpod_window);
    itdbs_head = g_object_get_data (G_OBJECT (gtkpod_window),
				    "itdbs_head");
    g_return_if_fail (itdbs_head);

    for (gl=itdbs_head->itdbs; gl; gl=gl->next)
    {
	iTunesDB *itdb = gl->data;
	ExtraiTunesDBData *eitdb;
	g_return_if_fail (itdb);
	eitdb = itdb->userdata;
	g_return_if_fail (eitdb);
	if ((itdb->usertype & GP_ITDB_TYPE_IPOD) && !eitdb->itdb_imported)
	{
	    gp_merge_itdb (itdb);
	}
    }
    display_disable_gtkpod_import_buttons ();
}


/* 
 * Merges @old_itdb with a newly imported one, then replaces @old_itdb
 * in the itdbs-list and the display.
 *
 * old_itdb->usertype, ->mountpoint, ->filename,
 * ->eitdb->offline_filename must be set according to usertype and
 * will be used to read the new itdb
 *
 */
void gp_merge_itdb (iTunesDB *old_itdb)
{
    ExtraiTunesDBData *old_eitdb;
    iTunesDB *new_itdb;

    g_return_if_fail (old_itdb);
    old_eitdb = old_itdb->userdata;
    g_return_if_fail (old_eitdb);

    if (old_itdb->usertype & GP_ITDB_TYPE_LOCAL)
    {
	g_return_if_fail (old_itdb->filename);

	new_itdb = gp_import_itdb (old_itdb, old_itdb->usertype,
				   NULL, NULL, old_itdb->filename);
    }
    else if (old_itdb->usertype & GP_ITDB_TYPE_IPOD)
    {
	g_return_if_fail (old_itdb->mountpoint);
	g_return_if_fail (old_eitdb->offline_filename);

	new_itdb = gp_import_itdb (old_itdb, old_itdb->usertype,
				   old_itdb->mountpoint,
				   old_eitdb->offline_filename,
				   NULL);
    }
    else
	g_return_if_reached ();

    if (new_itdb)
	gp_replace_itdb (old_itdb, new_itdb);

    gp_update_itdb_prefs ();
    gtkpod_tracks_statusbar_update ();
}


/* Like get_track_name_on_disk(), but verifies the track actually
   exists.
   Must g_free return value after use */
gchar *get_track_name_on_disk_verified (Track *track)
{
    gchar *name = NULL;
    ExtraTrackData *etr;

    g_return_val_if_fail (track, NULL);
    etr = track->userdata;
    g_return_val_if_fail (etr, NULL);

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
    if(!name && etr->pc_path_locale && (*etr->pc_path_locale))
    {
	if (g_file_test (etr->pc_path_locale, G_FILE_TEST_EXISTS))
	    name = g_strdup (etr->pc_path_locale);
    }
    return name;
}


/* Get track name from source @source */
gchar *get_track_name_from_source (Track *track, FileSource source)
{
    gchar *result = NULL;
    ExtraTrackData *etr;

    g_return_val_if_fail (track, NULL);
    etr = track->userdata;
    g_return_val_if_fail (etr, NULL);

    switch (source)
    {
    case SOURCE_PREFER_LOCAL:
	if (etr->pc_path_locale && (*etr->pc_path_locale))
	{
	    if (g_file_test (etr->pc_path_locale, G_FILE_TEST_EXISTS))
	    {
		result = g_strdup (etr->pc_path_locale);
	    }
	}
	if (!result) result = get_track_name_on_ipod (track);
	break;
    case SOURCE_LOCAL:
	if (etr->pc_path_locale && (*etr->pc_path_locale))
	{
	    if (g_file_test (etr->pc_path_locale, G_FILE_TEST_EXISTS))
	    {
		result = g_strdup (etr->pc_path_locale);
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


/* Fills @size with the size in bytes taken on the iPod and @num with
   the number of total deleted tracks.  @size and @num may be NULL */
/* FIXME: @itdb is not used yet */
void gp_info_deleted_tracks (iTunesDB *itdb,
			     gdouble *size, guint32 *num)
{
    GList *gl;

    if (size) *size = 0;
    if (num)  *num = 0;
    g_return_if_fail (itdb);

    for (gl=pending_deletion; gl; gl=gl->next)
    {
	ExtraTrackData *etr;
	Track *tr = gl->data;
	g_return_if_fail (tr);
	etr = tr->userdata;
	g_return_if_fail (tr);

	if (tr->transferred)
	{
	    if (size)  *size += tr->size - etr->oldsize;
	}
	if (num)   *num += 1;
    }
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


/* Writes extended info (md5 hash, PC-filename...) of @itdb into file
 * @itdb->filename+".ext". @itdb->filename will also be used to
 * calculate the md5 checksum of the corresponding iTunesDB */
static gboolean write_extended_info (iTunesDB *itdb)
{
  FILE *fp;
  gchar *md5;
  GList *gl;
  gchar *name;

  g_return_val_if_fail (itdb, FALSE);
  g_return_val_if_fail (itdb->filename, FALSE);

  space_data_update ();

  name = g_strdup_printf ("%s.ext", itdb->filename);
  fp = fopen (name, "w");
  if (!fp)
  {
      gtkpod_warning (_("Could not open \"%s\" for writing extended info.\n"),
		      name);
      g_free (name);
      return FALSE;
  }
  g_free (name);
  name = NULL;
  md5 = md5_hash_on_filename (itdb->filename, FALSE);
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
  for (gl=itdb->tracks; gl; gl=gl->next)
  {
      Track *track = gl->data;
      ExtraTrackData *etr;
      g_return_val_if_fail (track, (fclose (fp), FALSE));
      etr = track->userdata;
      g_return_val_if_fail (etr, (fclose (fp), FALSE));
      fprintf (fp, "id=%d\n", track->id);
      if (etr->hostname)
	  fprintf (fp, "hostname=%s\n", etr->hostname);
      if (strlen (etr->pc_path_locale) != 0)
	  fprintf (fp, "filename_locale=%s\n", etr->pc_path_locale);
      if (strlen (etr->pc_path_utf8) != 0)
	  fprintf (fp, "filename_utf8=%s\n", etr->pc_path_utf8);
      /* this is just for convenience for people looking for a track
	 on the ipod away from gktpod/itunes etc. */
      if (strlen (track->ipod_path) != 0)
	  fprintf (fp, "filename_ipod=%s\n", track->ipod_path);
      if (etr->md5_hash)
	  fprintf (fp, "md5_hash=%s\n", etr->md5_hash);
      if (etr->charset)
	  fprintf (fp, "charset=%s\n", etr->charset);
      if (!track->transferred && etr->oldsize)
	  fprintf (fp, "oldsize=%d\n", etr->oldsize);
      if (etr->peak_signal_set)
      {
	  gchar buf[20];
	  g_ascii_dtostr (buf, 20, (gdouble)etr->peak_signal);
	  fprintf (fp, "peak_signal=%s\n", buf);
      }
      if (etr->radio_gain_set)
      {
	  gchar buf[20];
	  g_ascii_dtostr (buf, 20, etr->radio_gain);
	  fprintf (fp, "radio_gain=%s\n", buf);
      }
      if (etr->audiophile_gain_set)
      {
	  gchar buf[20];
	  g_ascii_dtostr (buf, 20, etr->audiophile_gain);
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
	  Track *track = gl_track->data;
	  g_return_val_if_fail (track, (fclose (fp), FALSE));
	  
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
/* returns: int result (of remove()) */
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
#endif


/* Threaded copy of ipod track */
/* Returns: GError *error */
static gpointer th_copy (gpointer s)
{
    Track *track = s;
    ExtraTrackData *etr;
    gchar *mount = charset_from_utf8 (prefs_get_ipod_mount ());
    GError *error = NULL;
    g_return_val_if_fail (track, NULL);
    etr = track->userdata;
    g_return_val_if_fail (etr, NULL);

    itdb_cp_track_to_ipod (mount, track, etr->pc_path_locale, &error);
    g_free (mount);
    /* delete old size */
    if (track->transferred) etr->oldsize = 0;
#ifdef G_THREADS_ENABLED
    g_mutex_lock (mutex);
    mutex_data = TRUE;   /* signal that thread will end */
    g_cond_signal (cond);
    g_mutex_unlock (mutex);
#endif
    return error;
}

/* This function is called when the user presses the abort button
 * during flush_tracks() */
static void flush_tracks_abort (gboolean *abort)
{
    *abort = TRUE;
}


/* check if iPod directory stucture is present */
static gboolean ipod_dirs_present (gchar *mountpoint)
{
    const gchar *music[] = {"iPod_Control","Music",NULL},
      *itunes[] = {"iPod_Control","iTunes",NULL};
    gchar *file;
    gboolean result = TRUE;

    g_return_val_if_fail (mountpoint, FALSE);

    file = resolve_path(mountpoint, music);
    if(!file || !g_file_test(file,G_FILE_TEST_IS_DIR))
      result = FALSE;
    g_free(file);
    
    file = resolve_path(mountpoint, itunes);
    if(!file || !g_file_test(file,G_FILE_TEST_IS_DIR))
      result = FALSE;
    g_free(file);
    
    return result;
}


/* Flushes all non-transferred tracks to the iPod filesystem
   Returns TRUE on success, FALSE if some error occured or not all
   tracks were written. */
static gboolean flush_tracks (iTunesDB *itdb)
{
  gint count, n;
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

  g_return_val_if_fail (itdb, FALSE);

  n = itdb_tracks_number_nontransferred (itdb);

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
      g_return_val_if_fail (track, FALSE);
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
      itdb_track_free (track);
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
      /* we now have as much space as we're gonna have, copy files to
       * ipod */
      GList *gl;

      progtext = g_strdup (_("preparing to copy..."));
      gtk_progress_bar_set_text(GTK_PROGRESS_BAR (progress_bar), progtext);
      g_free (progtext);
      while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();

      /* count number of tracks to be transferred */
      if (n != 0)  display_disable_gtkpod_import_buttons();
      count = 0; /* tracks transferred */
      start = time (NULL);

      for (gl=itdb->tracks; gl && !abort; gl=gl->next)
      {
	  track = gl->data;
	  g_return_val_if_fail (track, FALSE); /* this will hang the
						  application :-( */
	  if (!track->transferred)             /* but this would crash
						  it otherwise... */
	  {
	      GError *error = NULL;
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
		  error = g_thread_join (thread);
	      }
	      else {
		  g_warning ("Thread creation failed, falling back to default.\n");
		  error = th_copy (track);
	      }
#else
	      error = th_copy (track);
#endif
	      if (error)
	      {   /* an error occured */
		  result = FALSE;
		  if (error->message)
		      gtkpod_warning ("%s\n\n", error->message);
		  else
		      g_warning ("error->message == NULL!\n");
		  g_error_free (error);
	      }
	      data_changed (itdb); /* otherwise new free space status from
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
      } /* for (;;) */
      if (abort)      result = FALSE;   /* negative result if user aborted */
      if (result == FALSE)
	  gtkpod_statusbar_message (_("Some tracks were not written to iPod. Export aborted!"));
  }
  prefs_set_statusbar_timeout (0);
  gtk_widget_destroy (dialog);
  while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
  return result;
}


gboolean gp_write_itdb (iTunesDB *itdb)
{
  gchar *cfgdir;
  gboolean success = TRUE;
  ExtraiTunesDBData *eitdb;

  g_return_val_if_fail (itdb, FALSE);
  eitdb = itdb->userdata;
  g_return_val_if_fail (eitdb, FALSE);

  cfgdir = prefs_get_cfgdir ();
  g_return_val_if_fail (cfgdir, FALSE);

  if (!eitdb->itdb_imported)
  {   /* No iTunesDB was read but user wants to export current
         data. If an iTunesDB is present on the iPod or in cfgdir,
	 this is most likely an error. We should tell the user */
      gchar *tunes = NULL;
      /* First check if we can find an existing iTunesDB. */
      if (itdb->usertype & GP_ITDB_TYPE_LOCAL)
      {
	  tunes = g_strdup (itdb->filename);
      }
      else if (itdb->usertype & GP_ITDB_TYPE_IPOD)
      {
	  if (prefs_get_offline ())
	  {
	      tunes = g_strdup (eitdb->offline_filename);
	  }
	  else
	  {
	      const gchar *itunes_components[] = {"iPod_Control",
						  "iTunes",
						  "iTunesDB", NULL};
	      tunes = resolve_path (itdb->mountpoint, itunes_components);
	  }
      }
      else
      {
	  g_free (cfgdir);
	  g_return_val_if_reached (FALSE);
      }
      if (g_file_test (tunes, G_FILE_TEST_EXISTS))
      {
	  gchar *str = g_strdup_printf (_("You did not import the existing iTunesDB ('%s'). This is most likely incorrect and will result in the loss of the existing database.\n\nPress 'OK' if you want to proceed anyhow or 'Cancel' to skip storing. If you cancel, you can import the existing database before calling this function again.\n"), tunes);
	  GtkWidget *dialog = gtk_message_dialog_new (
	      GTK_WINDOW (gtkpod_window),
	      GTK_DIALOG_DESTROY_WITH_PARENT,
	      GTK_MESSAGE_WARNING,
	      GTK_BUTTONS_OK_CANCEL,
	      str);
	  gint result = gtk_dialog_run (GTK_DIALOG (dialog));
	  gtk_widget_destroy (dialog);
	  g_free (str);
	  if (result == GTK_RESPONSE_CANCEL)
	  {
	      g_free (cfgdir);
	      return FALSE;
	  }
      }
  }

  block_widgets (); /* block user input */

  if((itdb->usertype & GP_ITDB_TYPE_IPOD) && !prefs_get_offline ())
  {
      /* check if iPod directories are present */
      if (!ipod_dirs_present (itdb->mountpoint))
      {   /* no -- create them */
	  ipod_directories_head (itdb->mountpoint);
	  /* if still not present abort */
	  if (!ipod_dirs_present (itdb->mountpoint))
	  {
	      gtkpod_warning (_("iPod directory structure must be present before synching to the iPod can be performed.\n"));
	      success = FALSE;
	  }
      }
      if (success)
      {
	  /* write tracks to iPod */
	  success = flush_tracks (itdb);
      }
  }

  if (success)
      gtkpod_statusbar_message (_("Now writing iTunesDB. Please wait..."));
  while (widgets_blocked && gtk_events_pending ())
      gtk_main_iteration ();

  if (success && !prefs_get_offline () &&
      (itdb->usertype & GP_ITDB_TYPE_IPOD))
  {   /* write to the iPod */
      GError *error = NULL;
      if (!itdb_write (itdb, NULL, &error))
      {   /* an error occured */
	  success = FALSE;
	  if (error && error->message)
	      gtkpod_warning ("%s\n\n", error->message);
	  else
	      g_warning ("error->message == NULL!\n");
	  g_error_free (error);
	  error = NULL;
      }
      if (success)
      {
	  if (prefs_get_write_extended_info ())
	  {   /* write extended information */
	      success = write_extended_info (itdb);
	  }
	  else
	  {   /* delete extended information if present */
	      gchar *ext = g_strdup_printf ("%s.ext", itdb->filename);
	      if (g_file_test (ext, G_FILE_TEST_EXISTS))
	      {
		  if (remove (ext) != 0)
		  {
		      gchar *buf = g_strdup_printf (_("Extended information file not deleted: '%s\'"), ext);
		      gtkpod_statusbar_message (buf);
		      g_free (buf);
		  }
	      }
	  }
      }
      if (success && cfgdir)
      {   /* copy to cfgdir */
	  GError *error = NULL;
	  if (!itdb_cp (itdb->filename, eitdb->offline_filename, &error))
	  {
	      success = FALSE;
	      if (error && error->message)
		  gtkpod_warning ("%s\n\n", error->message);
	      else
		  g_warning ("error->message == NULL!\n");
	      g_error_free (error);
	      error = NULL;
	  }
	  if (prefs_get_write_extended_info ())
	  {
	      gchar *from, *to;
	      from = g_strdup_printf ("%s.ext", itdb->filename);
	      to = g_strdup_printf ("%s.ext", eitdb->offline_filename);
	      if (!itdb_cp (from, to, &error))
	      {
		  success = FALSE;
		  if (error && error->message)
		      gtkpod_warning ("%s\n\n", error->message);
		  else
		      g_warning ("error->message == NULL!\n");
		  g_error_free (error);
	      }
	      g_free (from);
	      g_free (to);
	  }
      }
  }

  if (success && prefs_get_offline () &&
      (itdb->usertype & GP_ITDB_TYPE_IPOD))
  {   /* write to cfgdir */
      GError *error = NULL;
      if (!itdb_write_file (itdb, eitdb->offline_filename, &error))
      {   /* an error occured */
	  success = FALSE;
	  if (error && error->message)
	      gtkpod_warning ("%s\n\n", error->message);
	  else
	      g_warning ("error->message == NULL!\n");
	  g_error_free (error);
	  error = NULL;
      }
      if (success && prefs_get_write_extended_info ())
      {   /* write extended information */
	  success = write_extended_info (itdb);
      }
  }


  if (success && (itdb->usertype & GP_ITDB_TYPE_LOCAL))
  {   /* write to cfgdir */
      GError *error = NULL;
      if (!itdb_write_file (itdb, NULL, &error))
      {   /* an error occured */
	  success = FALSE;
	  if (error && error->message)
	      gtkpod_warning ("%s\n\n", error->message);
	  else
	      g_warning ("error->message == NULL!\n");
	  g_error_free (error);
	  error = NULL;
      }
      if (success)
      {   /* write extended information */
	  success = write_extended_info (itdb);
      }
  }

  /* indicate that files and/or database is saved */
  if (success)
  {
      eitdb->data_changed = FALSE;
      /* block menu item and button */
      display_disable_gtkpod_import_buttons();
      gtkpod_statusbar_message(_("iPod Database Saved"));
  }

  g_free (cfgdir);

  release_widgets (); /* Allow input again */

  return success;
}



/* used to handle export of database */
void handle_export (void)
{
    GList *gl;
    gboolean success = TRUE;
    struct itdbs_head *itdbs_head;

    g_return_if_fail (gtkpod_window);

    itdbs_head = g_object_get_data (G_OBJECT (gtkpod_window),
				    "itdbs_head");
    g_return_if_fail (itdbs_head);

    block_widgets (); /* block user input */

    /* read offline playcounts -- in case we added some tracks we can
       now handle */
    parse_offline_playcount ();

    for (gl=itdbs_head->itdbs; gl; gl=gl->next)
    {
	iTunesDB *itdb = gl->data;
	ExtraiTunesDBData *eitdb;
	g_return_if_fail (itdb);
	eitdb = itdb->userdata;
	g_return_if_fail (eitdb);
	if (eitdb->data_changed || eitdb->itdb_imported)
	    success &= gp_write_itdb (itdb);
    }

    if (prefs_get_concal_autosync ())
    {
	const gchar *str;
	gtkpod_statusbar_message (_("Syncing contacts, calendar and notes..."));
	str = prefs_get_path (PATH_SYNC_CONTACTS);
	if (str && *str)    tools_sync_contacts ();
	str = prefs_get_path (PATH_SYNC_CALENDAR);
	if (str && *str)    tools_sync_calendar ();
	str = prefs_get_path (PATH_SYNC_NOTES);
	if (str && *str)    tools_sync_notes ();
    }

    release_widgets ();
}




/* indicate that data was changed and update the free space indicator */
void data_changed (iTunesDB *itdb)
{
    ExtraiTunesDBData *eitdb;
    g_return_if_fail (itdb);
    eitdb = itdb->userdata;
    g_return_if_fail (eitdb);

    eitdb->data_changed = TRUE;
    space_data_update ();
}


/* Check if all files are saved (i.e. none of the itdbs has the
 * data_changed flag set */
gboolean files_are_saved (void)
{
    struct itdbs_head *itdbs_head;
    gboolean changed = FALSE;
    GList *gl;

    g_return_val_if_fail (gtkpod_window, TRUE);
    itdbs_head = g_object_get_data (G_OBJECT (gtkpod_window),
				    "itdbs_head");
    g_return_val_if_fail (itdbs_head, TRUE);
    for (gl=itdbs_head->itdbs; gl; gl=gl->next)
    {
	iTunesDB *itdb = gl->data;
	ExtraiTunesDBData *eitdb;
	g_return_val_if_fail (itdb, !changed);
	eitdb = itdb->userdata;
	g_return_val_if_fail (eitdb, !changed);
printf ("itdb: %p, changed: %d, imported: %d\n",
	itdb, eitdb->data_changed, eitdb->itdb_imported);
	changed |= eitdb->data_changed;
    }
    return !changed;
}


/* FIXME: transitional function to set the mountpoint of all
 * GP_ITDB_TYPE_IPOD itdbs. Should be replaced with a new system to
 * individually define the mountpoints */
void gp_itdb_set_mountpoint (const gchar *mp)
{
    struct itdbs_head *itdbs_head;

    GList *gl;

    g_return_if_fail (gtkpod_window);
    itdbs_head = g_object_get_data (G_OBJECT (gtkpod_window),
				    "itdbs_head");
    if (!itdbs_head) return;

    for (gl=itdbs_head->itdbs; gl; gl=gl->next)
    {
	iTunesDB *itdb = gl->data;
	g_return_if_fail (itdb);
	if (itdb->usertype & GP_ITDB_TYPE_IPOD)
	{
	    g_free (itdb->mountpoint);
	    itdb->mountpoint = g_strdup (mp);
	}
    }
}
