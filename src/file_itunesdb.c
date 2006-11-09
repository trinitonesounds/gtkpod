/* Time-stamp: <2006-06-25 15:57:03 jcs>
|
|  Copyright (C) 2002-2005 Jorg Schuler <jcsjcs at users sourceforge net>
|  Part of the gtkpod project.
| 
|  URL: http://www.gtkpod.org/
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
#include "syncdir.h"
#include "tools.h"
#include "ipod_init.h"

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
    time_t mtime;
    gchar *thumb_path_locale;
    gchar *thumb_path_utf8;
    gchar *md5_hash;
    gchar *charset;
    gchar *hostname;
    gchar *ipod_path;
    gint32 oldsize;
    guint32 playcount;
    guint32 rating;        /* still read but never written */
    gboolean transferred;
};

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
static GList *extendeddeletion = NULL;
static float extendedinfoversion = 0.0;


/* Some declarations */
static gboolean gp_write_itdb (iTunesDB *itdb);


/* fills in extended info if available */
/* num/total are used to give updates in case the md5 checksums have
   to be matched against the files which is very time consuming */
void fill_in_extended_info (Track *track, gint32 total, gint32 num)
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
      gtkpod_statusbar_message (
	  _("Matching MD5 checksum for file %d/%d"),
	  num, total);
      while (widgets_blocked && gtk_events_pending ())
	  gtk_main_iteration ();

      if (!etr->md5_hash)
      {
	  gchar *filename = get_file_name_from_source (track, SOURCE_IPOD);
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
      {
	  etr->pc_path_locale = g_strdup (sei->pc_path_locale);
	  etr->mtime = sei->mtime;
      }
      if (sei->pc_path_utf8 && !etr->pc_path_utf8)
	  etr->pc_path_utf8 = g_strdup (sei->pc_path_utf8);
      if (sei->thumb_path_locale && !etr->thumb_path_locale)
	  etr->thumb_path_locale = g_strdup (sei->thumb_path_locale);
      if (sei->thumb_path_utf8 && !etr->thumb_path_utf8)
	  etr->thumb_path_utf8 = g_strdup (sei->thumb_path_utf8);
      if (sei->md5_hash && !etr->md5_hash)
	  etr->md5_hash = g_strdup (sei->md5_hash);
      if (sei->charset && !etr->charset)
	  etr->charset = g_strdup (sei->charset);
      if (sei->hostname && !etr->hostname)
	  etr->hostname = g_strdup (sei->hostname);
      etr->oldsize = sei->oldsize;
      track->playcount += sei->playcount;
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
	g_free (sei->pc_path_locale);
	g_free (sei->pc_path_utf8);
	g_free (sei->thumb_path_locale);
	g_free (sei->thumb_path_utf8);
	g_free (sei->md5_hash);
	g_free (sei->charset);
	g_free (sei->hostname);
	g_free (sei->ipod_path);
	g_free (sei);
    }
}

static void destroy_extendedinfohash (void)
{
    if (extendedinfohash)
    {
	g_hash_table_destroy (extendedinfohash);
	extendedinfohash = NULL;
    }
    if (extendedinfohash_md5)
    {
	g_hash_table_destroy (extendedinfohash_md5);
	extendedinfohash_md5 = NULL;
    }
    if (extendeddeletion)
    {
	g_list_foreach (extendeddeletion, (GFunc)itdb_track_free, NULL);
	g_list_free (extendeddeletion);
	extendeddeletion = NULL;
    }
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
		    while (widgets_blocked && gtk_events_pending ())
			gtk_main_iteration ();
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
			extendeddeletion = g_list_append (extendeddeletion,
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
	    else if (g_ascii_strcasecmp (line, "thumbnail_locale") == 0)
		sei->thumb_path_locale = g_strdup (arg);
	    else if (g_ascii_strcasecmp (line, "thumbnail_utf8") == 0)
		sei->thumb_path_utf8 = g_strdup (arg);
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
	    else if (g_ascii_strcasecmp (line, "pc_mtime") == 0)
	    {
		sei->mtime = (time_t)g_ascii_strtoull (arg, NULL, 10);
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
 * @type: GP_ITDB_TYPE_LOCAL/IPOD (bitwise flags!)
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
    gint32 total, num;
    gboolean offline;


    g_return_val_if_fail (!(type & GP_ITDB_TYPE_LOCAL) || name_loc, NULL);
    g_return_val_if_fail (!(type & GP_ITDB_TYPE_IPOD) || 
			  (mp && name_off), NULL);
    g_return_val_if_fail (cfgdir, NULL);

    if (old_itdb)
	offline = get_offline (old_itdb);
    else
	offline = FALSE;

    block_widgets ();
    if (offline || (type & GP_ITDB_TYPE_LOCAL))
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
	    if (prefs_get_int("write_extended_info"))
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
	gchar *name_ext=NULL, *name_db=NULL;

	gchar *itunes_dir = itdb_get_itunes_dir (mp);
	if (itunes_dir)
	{
	    name_ext = itdb_get_path (itunes_dir, "iTunesDB.ext");
	    name_db  = itdb_get_path (itunes_dir, "iTunesDB");
	}
	if (name_db)
	{
	    if (prefs_get_int("write_extended_info"))
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
	    gtkpod_warning (_("'%s' (or similar) does not exist. Import aborted.\n\n"),
			    name);
	    g_free (name);
	}
	g_free (name_ext);
	g_free (name_db);
	g_free (itunes_dir);
    }

    if (!itdb)
    {
	release_widgets ();
	return NULL;
    }

    /* add Extra*Data */
    gp_itdb_add_extra_full (itdb);

    eitdb = itdb->userdata;
    g_return_val_if_fail (eitdb, (release_widgets(), NULL));

    eitdb->offline = offline;

    /* fill in additional info */
    itdb->usertype = type;
    if (type & GP_ITDB_TYPE_IPOD)
    {
	if (offline)
	{
	    itdb_set_mountpoint (itdb, mp);
	    g_free (itdb->filename);
	    itdb->filename = NULL;
	}
	eitdb->offline_filename = g_strdup (name_off);
    }

    total = g_list_length (itdb->tracks);
    num = 1;
    /* validate all tracks and fill in extended info */
    for (gl=itdb->tracks; gl; gl=gl->next)
    {
	Track *track = gl->data;
	ExtraTrackData *etr;
	g_return_val_if_fail (track, (release_widgets(), NULL));
	etr = track->userdata;
	g_return_val_if_fail (etr, (release_widgets(), NULL));
	fill_in_extended_info (track, total, num);
	gp_track_validate_entries (track);
	/* properly set value for has_artwork */
	if ((track->has_artwork == 0x00) ||
	    ((track->has_artwork == 0x02) &&
	     (extendedinfoversion > 0.0) && (extendedinfoversion <= 0.99)))
	{   /* if has_artwork is not set (0x00), or it has been
	       (potentially wrongly) set to 0x02 by gtkpod V0.99 or
	       smaller, determine the correct(?) value */
	    if (track->artwork->thumbnails)
		track->has_artwork = 0x01;
	    else
		track->has_artwork = 0x02;
	}

	/* set mediatype to audio if unset (important only for iPod Video) */
	if (track->mediatype == 0)
	    track->mediatype = 0x00000001;
	/* restore deleted thumbnails */
	if ((track->artwork->thumbnails == NULL) &&
	    (strlen (etr->thumb_path_locale) != 0))
	{
	    /* !! gp_track_set_thumbnails() writes on
	       etr->thumb_path_locale, so we need to g_strdup()
	       first !! */
	    gchar *filename = g_strdup (etr->thumb_path_locale);
	    gp_track_set_thumbnails (track, filename);
	    g_free (filename);
	}
	++num;
    }
    /* take over the pending deletion information */
    while (extendeddeletion)
    {
	Track *track = extendeddeletion->data;
	g_return_val_if_fail (track, (release_widgets(), NULL));
	mark_track_for_deletion (itdb, track);
	extendeddeletion = g_list_delete_link (extendeddeletion,
					       extendeddeletion);
    }

    /* delete hash information (if present) */
    destroy_extendedinfohash ();
    /* find duplicates */
    gp_md5_hash_tracks_itdb (itdb);

    /* mark the data as unchanged */
    data_unchanged (itdb);
    /* set mark that this itdb struct contains an imported iTunesDB */
    eitdb->itdb_imported = TRUE;

    if (old_itdb)
    {
	/* this table holds pairs of old_itdb-tracks/new_itdb/tracks */
	ExtraiTunesDBData *old_eitdb = old_itdb->userdata;
	GHashTable *track_hash = g_hash_table_new (g_direct_hash,
						   g_direct_equal);
	Playlist *mpl = itdb_playlist_mpl (itdb);
	g_return_val_if_fail (mpl, (release_widgets(), NULL));
	g_return_val_if_fail (old_eitdb, (release_widgets(), NULL));

	/* add tracks from @old_itdb to new itdb */
	for (gl=old_itdb->tracks; gl; gl=gl->next)
	{
	    Track *duptr, *addtr;
	    Track *track = gl->data;
	    g_return_val_if_fail (track, (release_widgets(), NULL));
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
	    g_return_val_if_fail (pl, (release_widgets(), NULL));
	    duppl = itdb_playlist_duplicate (pl);
	    /* switch members */
	    for (glm=duppl->members; glm; glm=glm->next)
	    {
		Track *newtr;
		g_return_val_if_fail (glm->data, (release_widgets(), NULL));
		newtr = g_hash_table_lookup (track_hash, glm->data);
		g_return_val_if_fail (newtr, (release_widgets(), NULL));
		glm->data = newtr;
	    }
	    /* if it's the podcasts list, don't add the list again if
	       it already exists, but only the members. */
	    if (itdb_playlist_is_podcasts (duppl) &&
		itdb_playlist_podcasts (itdb))
	    {
		Playlist *podcasts = itdb_playlist_podcasts (itdb);
		for (glm=duppl->members; glm; glm=glm->next)
		{
		    g_return_val_if_fail (glm->data, (release_widgets(), NULL));
		    itdb_playlist_add_track (podcasts, glm->data, -1);
		}
		itdb_playlist_free (duppl);
	    }
	    else
	    {
		itdb_playlist_add (itdb, duppl, -1);
	    }
	    gl = gl->next;
	}
	g_hash_table_destroy (track_hash);
	/* copy data_changed flag */
	eitdb->data_changed = old_eitdb->data_changed;
    }

    release_widgets();

    return itdb;
}

/* attempts to import all iPod databases */
void gp_load_ipods (void)
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
	    gp_load_ipod (itdb);
	}
    }
}


/* 
 * Merges @old_itdb with a newly imported one, then replaces @old_itdb
 * in the itdbs-list and the display.
 *
 * old_itdb->usertype, ->mountpoint, ->filename,
 * ->eitdb->offline_filename must be set according to usertype and
 * will be used to read the new itdb
 *
 * Return value: pointer to the new repository
 */
iTunesDB *gp_merge_itdb (iTunesDB *old_itdb)
{
    ExtraiTunesDBData *old_eitdb;
    iTunesDB *new_itdb;

    g_return_val_if_fail (old_itdb, NULL);
    old_eitdb = old_itdb->userdata;
    g_return_val_if_fail (old_eitdb, NULL);

    if (old_itdb->usertype & GP_ITDB_TYPE_LOCAL)
    {
	g_return_val_if_fail (old_itdb->filename, NULL);

	new_itdb = gp_import_itdb (old_itdb, old_itdb->usertype,
				   NULL, NULL, old_itdb->filename);
    }
    else if (old_itdb->usertype & GP_ITDB_TYPE_IPOD)
    {
	const gchar *mountpoint = itdb_get_mountpoint (old_itdb);
	g_return_val_if_fail (mountpoint, NULL);
	g_return_val_if_fail (old_eitdb->offline_filename, NULL);

	new_itdb = gp_import_itdb (old_itdb, old_itdb->usertype,
				   mountpoint,
				   old_eitdb->offline_filename,
				   NULL);
    }
    else
	g_return_val_if_reached (NULL);

    if (new_itdb)
    {
	gp_replace_itdb (old_itdb, new_itdb);

	/* take care of autosync... */
	sync_all_playlists (new_itdb);

	/* update all live SPLs */
	itdb_spl_update_live (new_itdb);
    }

    gtkpod_tracks_statusbar_update ();

    return new_itdb;
}


/**
 * gp_load_ipod: loads the contents of an iPod into @itdb. If data
 * already exists in @itdb, data is merged.
 *
 * @itdb: repository to load iPod contents into. mountpoint must be
 * set, and the iPod must not be loaded already
 * (eitdb->itdb_imported).
 *
 * Return value: the new repository holding the contents of the iPod.
 */
iTunesDB *gp_load_ipod (iTunesDB *itdb)
{
    ExtraiTunesDBData *eitdb;
    iTunesDB *new_itdb = NULL;
    gchar *mountpoint;
    gchar *itunesdb;
    gboolean load = TRUE;

    g_return_val_if_fail (itdb, NULL);
    g_return_val_if_fail (itdb->usertype & GP_ITDB_TYPE_IPOD, NULL);
    eitdb = itdb->userdata;
    g_return_val_if_fail (eitdb, NULL);
    g_return_val_if_fail (eitdb->itdb_imported == FALSE, NULL);

    mountpoint = get_itdb_prefs_string (itdb, KEY_MOUNTPOINT);
    call_script ("gtkpod.load", mountpoint, NULL);

    itdb_device_set_mountpoint (itdb->device, mountpoint);

    itunesdb = itdb_get_itunesdb_path (mountpoint);
    if (!itunesdb)
    {
	gchar *str = g_strdup_printf (_("Could not find iPod directory structure at '%s'.\nIf you are sure that the iPod is properly mounted at '%s', gtkpod can create the directory structure for you.\n\nDo you want to create the directory structure now?\n"), mountpoint, mountpoint);
	GtkWidget *dialog = gtk_message_dialog_new (
	    GTK_WINDOW (gtkpod_window),
	    GTK_DIALOG_DESTROY_WITH_PARENT,
	    GTK_MESSAGE_WARNING,
	    GTK_BUTTONS_YES_NO,
	    str);
	gint result = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	g_free (str);

	if (result == GTK_RESPONSE_YES)
	{
	    load = gp_ipod_init (itdb);
	}
	else
	{
	    load = FALSE;
	}
    }
    g_free (itunesdb);
    g_free (mountpoint);

    if (load)
    {
	new_itdb = gp_merge_itdb (itdb);
	if (new_itdb)
	{
	    gchar *old_model = get_itdb_prefs_string (new_itdb,
						      KEY_IPOD_MODEL);
	    gchar *new_model = itdb_device_get_sysinfo (new_itdb->device,
							"ModelNumStr");

	    if (!old_model && new_model)
	    {
		set_itdb_prefs_string (new_itdb, KEY_IPOD_MODEL, new_model);
	    }
	    else if (old_model && !new_model)
	    {
		gp_ipod_init_set_model (new_itdb, old_model);
	    }
	    else if (!old_model && !new_model)
	    {
		gp_ipod_init_set_model (new_itdb, NULL);
	    }
	    else
	    {   /* old_model && new_model are set */
#if 0
		const gchar *old_ptr = old_model;
		const gchar *new_ptr = new_model;
		/* Normalize model number */
		if (isalpha (old_model[0]))   ++old_ptr;
		if (isalpha (new_model[0]))   ++new_ptr;
		if (strcmp (old_ptr, new_ptr) != 0)
		{   /* Model number has changed -- confirm */
                }
#endif		
		set_itdb_prefs_string (new_itdb, KEY_IPOD_MODEL, new_model);
	    }
	}
    }
    return new_itdb;
}


/**
 * gp_eject_ipod: store @itdb and call ~/.gtkpod/gtkpod.eject with the
 * mountpoint as parameter. Then @itdb is deleted and replaced with an
 * empty version. eitdb->ejected is set.
 *
 * @itdb: must be an iPod itdb (eject does not make sense otherwise)
 *
 * Return value: TRUE if saving was successful, FALSE otherwise.
 */
gboolean gp_eject_ipod (iTunesDB *itdb)
{

    g_return_val_if_fail (itdb, FALSE);
    g_return_val_if_fail (itdb->usertype & GP_ITDB_TYPE_IPOD, FALSE);

    if (gp_save_itdb (itdb))
    {
	gint index;
	gchar *mountpoint;
	iTunesDB *new_itdb;

	mountpoint = get_itdb_prefs_string (itdb, KEY_MOUNTPOINT);
	call_script ("gtkpod.eject", mountpoint, FALSE);
	g_free (mountpoint);

	index = get_itdb_index (itdb);
	new_itdb = setup_itdb_n (index);
	if (new_itdb)
	{
	    ExtraiTunesDBData *new_eitdb;

	    new_eitdb = new_itdb->userdata;
	    g_return_val_if_fail (new_eitdb, TRUE);

	    gp_replace_itdb (itdb, new_itdb);

	    new_eitdb->ipod_ejected = TRUE;
    	}
	return TRUE;
    }
    return FALSE;
}


/**
 * gp_save_itdb: Save a repository after updating smart playlists. If
 * the repository is an iPod, contacts, notes and calendar are also
 * updated.
 *
 * @itdb: repository to save
 *
 * return value: TRUE on succes, FALSE when an error occured.
 */
gboolean gp_save_itdb (iTunesDB *itdb)
{
    Playlist *pl;
    gboolean success;
    g_return_val_if_fail (itdb, FALSE);

    /* update smart playlists before writing */
    itdb_spl_update_live (itdb);
    pl = pm_get_selected_playlist ();
    if (pl && (pl->itdb == itdb) &&
	pl->is_spl && pl->splpref.liveupdate)
    {   /* Update display if necessary */
	st_redisplay (0);
    }

    success = gp_write_itdb (itdb);

    if (itdb->usertype & GP_ITDB_TYPE_IPOD)
    {
	if (get_itdb_prefs_int (itdb, "concal_autosync"))
	{
	    tools_sync_all (itdb);
	}
    }

    return success;
}



/*------------------------------------------------------------------*\
 *                                                                  *
 *      Functions concerning deletion of tracks                      *
 *                                                                  *
\*------------------------------------------------------------------*/

/* Fills in @size with the size in bytes taken on the iPod or the
 * local harddisk with files to be deleted, @num with the number of
 * tracks to be deleted. */ 
void gp_info_deleted_tracks (iTunesDB *itdb,
			     gdouble *size, guint32 *num)
{
    ExtraiTunesDBData *eitdb;
    GList *gl;

    if (size) *size = 0;
    if (num)  *num = 0;

    g_return_if_fail (itdb);
    eitdb = itdb->userdata;
    g_return_if_fail (eitdb);

    for (gl=eitdb->pending_deletion; gl; gl=gl->next)
    {
	ExtraTrackData *etr;
	Track *tr = gl->data;
	g_return_if_fail (tr);
	etr = tr->userdata;
	g_return_if_fail (tr);

	if (itdb->usertype & GP_ITDB_TYPE_IPOD)
	{
	    if (tr->transferred)
	    {   /* I don't understand why "- etr->oldsize" (JCS) --
		 * supposedly related to update_track_from_file() but
		 I can't see how. */
		if (size)  *size += tr->size - etr->oldsize;
	    }
	}
	if (itdb->usertype & GP_ITDB_TYPE_LOCAL)
	{   /* total size on hard disk */
	    if (size) *size += tr->size;
	}

	if (num)   *num += 1;
    }
}

void mark_track_for_deletion (iTunesDB *itdb, Track *track)
{
    ExtraiTunesDBData *eitdb;
    g_return_if_fail (itdb);
    g_return_if_fail (track);
    g_return_if_fail (track->itdb == NULL);
    eitdb = itdb->userdata;
    g_return_if_fail (eitdb);

    eitdb->pending_deletion = g_list_append (eitdb->pending_deletion,
					     track);
}

/* It might be necessary to unmark for deletion like in case of
   dangling tracks with no real files on ipod */
void unmark_track_for_deletion (iTunesDB *itdb, Track *track)
{
    ExtraiTunesDBData *eitdb;
    g_return_if_fail (itdb);
    g_return_if_fail (track);
    eitdb = itdb->userdata;
    g_return_if_fail (eitdb);

    if (track != NULL)
        eitdb->pending_deletion = g_list_remove (
	    eitdb->pending_deletion, track);
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
  ExtraiTunesDBData *eitdb;
  FILE *fp;
  gchar *md5;
  GList *gl;
  gchar *name;

  g_return_val_if_fail (itdb, FALSE);
  g_return_val_if_fail (itdb->filename, FALSE);
  eitdb = itdb->userdata;
  g_return_val_if_fail (eitdb, FALSE);

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
      if (etr->pc_path_locale && *etr->pc_path_locale)
	  fprintf (fp, "filename_locale=%s\n", etr->pc_path_locale);
      if (etr->pc_path_utf8 && *etr->pc_path_utf8)
	  fprintf (fp, "filename_utf8=%s\n", etr->pc_path_utf8);
      if (etr->thumb_path_locale && *etr->thumb_path_locale)
	  fprintf (fp, "thumbnail_locale=%s\n", etr->thumb_path_locale);
      if (etr->thumb_path_utf8 && *etr->thumb_path_utf8)
	  fprintf (fp, "thumbnail_utf8=%s\n", etr->thumb_path_utf8);
      /* this is just for convenience for people looking for a track
	 on the ipod away from gktpod/itunes etc. */
      if (strlen (track->ipod_path) != 0)
	  fprintf (fp, "filename_ipod=%s\n", track->ipod_path);
      if (etr->md5_hash && *etr->md5_hash)
	  fprintf (fp, "md5_hash=%s\n", etr->md5_hash);
      if (etr->charset && *etr->charset)
	  fprintf (fp, "charset=%s\n", etr->charset);
      if (!track->transferred && etr->oldsize)
	  fprintf (fp, "oldsize=%d\n", etr->oldsize);
      if (etr->mtime)
	  fprintf (fp, "pc_mtime=%llu\n", (unsigned long long)etr->mtime);
      fprintf (fp, "transferred=%d\n", track->transferred);
      while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
  }
  if (get_offline(itdb))
  { /* we are offline and also need to export the list of tracks that
       are to be deleted */
      for(gl = eitdb->pending_deletion; gl; gl = gl->next)
      {
	  Track *track = gl->data;
	  g_return_val_if_fail (track, (fclose (fp), FALSE));
	  
	  fprintf (fp, "id=000\n");  /* our sign for tracks pending
					deletion */
	  fprintf (fp, "filename_ipod=%s\n", track->ipod_path);
	  while (widgets_blocked && gtk_events_pending ())
	      gtk_main_iteration ();
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
    guint result;

    result = remove ((gchar *)filename);
    g_mutex_lock (mutex);
    mutex_data = TRUE; /* signal that thread will end */
    g_cond_signal (cond);
    g_mutex_unlock (mutex);
    return GUINT_TO_POINTER(result);
}
#endif


/* Threaded copy of ipod track */
/* Returns: GError *error */
static gpointer th_copy (gpointer data)
{
    Track *track = data;
    ExtraTrackData *etr;
    GError *error = NULL;
    g_return_val_if_fail (track, NULL);
    etr = track->userdata;
    g_return_val_if_fail (etr, NULL);

    itdb_cp_track_to_ipod (track, etr->pc_path_locale, &error);

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
static void file_dialog_abort (gboolean *abort_flag)
{
    *abort_flag = TRUE;
}


/* check if iPod directory stucture is present */
static gboolean ipod_dirs_present (const gchar *mountpoint)
{
    gchar *file;
    gchar *dir;
    gboolean result = TRUE;

    g_return_val_if_fail (mountpoint, FALSE);

    dir = itdb_get_music_dir (mountpoint);
    if (!dir)
	return FALSE;

    file = itdb_get_path (dir, "F00");
    if (!file || !g_file_test(file, G_FILE_TEST_IS_DIR))
	result = FALSE;
    g_free (file);
    g_free (dir);

    dir = itdb_get_itunes_dir (mountpoint);
    if(!dir || !g_file_test(dir, G_FILE_TEST_IS_DIR))
      result = FALSE;
    g_free(dir);

    return result;
}



static GtkWidget *create_file_dialog (GtkWidget **progress_bar,
				      gboolean *abort_flag)
{
  GtkWidget *dialog, *label, *image, *hbox;

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
  if (progress_bar)
      *progress_bar = gtk_progress_bar_new ();

  /* Indicate that user wants to abort */
  g_signal_connect_swapped (GTK_OBJECT (dialog), "response",
			    G_CALLBACK (file_dialog_abort),
			    abort_flag);

  /* Add the image/label + progress bar to dialog */
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
                      hbox, FALSE, FALSE, 0);
  if (progress_bar)
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
			  *progress_bar, FALSE, FALSE, 0);
  gtk_widget_show_all (dialog);

  return dialog;
}


/* Removes all tracks that were marked for deletion from the iPod or
   the local harddisk (for itdb->usertype == GP_ITDB_TYPE_LOCAL) */
/* Returns TRUE on success, FALSE if some error occured and not all
   files were removed */
static gboolean delete_files (iTunesDB *itdb)
{
  GtkWidget *dialog, *progress_bar;
  gchar *progtext = NULL;
  gboolean result = TRUE;
  static gboolean abort_flag;
  ExtraiTunesDBData *eitdb;
#ifdef G_THREADS_ENABLED
  GThread *thread = NULL;
  GTimeVal gtime;
  if (!mutex) mutex = g_mutex_new ();
  if (!cond) cond = g_cond_new ();
#endif



  g_return_val_if_fail (itdb, FALSE);
  eitdb = itdb->userdata;
  g_return_val_if_fail (eitdb, FALSE);

  if (!eitdb->pending_deletion)
  {
      return TRUE;
  }

  if (itdb->usertype & GP_ITDB_TYPE_IPOD)
  {
      g_return_val_if_fail (itdb_get_mountpoint (itdb), FALSE);
  }

  abort_flag = FALSE;

  dialog = create_file_dialog (&progress_bar, &abort_flag);
  progtext = g_strdup (_("deleting..."));
  gtk_progress_bar_set_text(GTK_PROGRESS_BAR (progress_bar), progtext);
  while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
  g_free (progtext);

  /* lets clean up those pending deletions */
  while (!abort_flag && eitdb->pending_deletion)
  {
      gchar *filename = NULL;
      Track *track = eitdb->pending_deletion->data;
      g_return_val_if_fail (track, FALSE);

      if (itdb->usertype & GP_ITDB_TYPE_IPOD)
      {
	  track->itdb = itdb;
	  filename = get_file_name_from_source (track, SOURCE_IPOD);
	  track->itdb = NULL;
      }
      if (itdb->usertype & GP_ITDB_TYPE_LOCAL)
      {
	  filename = get_file_name_from_source (track, SOURCE_LOCAL);
      }

      if(filename)
      {
	  guint rmres;
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
	      rmres = GPOINTER_TO_UINT(g_thread_join (thread));
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
	  g_free(filename);
      }
      itdb_track_free (track);
      eitdb->pending_deletion = g_list_delete_link (
	  eitdb->pending_deletion, eitdb->pending_deletion);
      while (widgets_blocked && gtk_events_pending ())
	  gtk_main_iteration ();
  }
  gtk_widget_destroy (dialog);
  if (abort_flag) result = FALSE;
  return result;
}




/* Flushes all non-transferred tracks to the iPod filesystem
   Returns TRUE on success, FALSE if some error occured or not all
   tracks were written. */
static gboolean flush_tracks (iTunesDB *itdb)
{
  GList *gl;
  gint count, n;
  gboolean result = TRUE;
  static gboolean abort_flag;
  GtkWidget *dialog, *progress_bar;
  time_t start;
  gchar *progtext = NULL;
  ExtraiTunesDBData *eitdb;
#ifdef G_THREADS_ENABLED
  GThread *thread = NULL;
  GTimeVal gtime;
  if (!mutex) mutex = g_mutex_new ();
  if (!cond) cond = g_cond_new ();
#endif

  g_return_val_if_fail (itdb, FALSE);
  eitdb = itdb->userdata;
  g_return_val_if_fail (eitdb, FALSE);

  n = itdb_tracks_number_nontransferred (itdb);

  if (n==0) return TRUE;

  abort_flag = FALSE;
  /* create the dialog window */
  dialog = create_file_dialog (&progress_bar, &abort_flag);
  progtext = g_strdup (_("preparing to copy..."));
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (progress_bar), progtext);
  while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
  g_free (progtext);

  /* count number of tracks to be transferred */
  count = 0; /* tracks transferred */
  start = time (NULL);

  for (gl=itdb->tracks; gl && !abort_flag; gl=gl->next)
  {
      time_t diff, fullsecs, hrs, mins, secs;
      Track *track = gl->data;
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
		  /* wait a maximum of 20 ms */
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
				  iPod is never read and free space
				  keeps increasing while we copy more
				  and more files to the iPod */
	  ++count;
	  if (count == 1)  /* we need longer timeout */
	  {
	      gtkpod_statusbar_timeout (3*STATUSBAR_TIMEOUT);
	  }
	  if (count == n)  /* we need to reset timeout */
	  {
	      gtkpod_statusbar_timeout (0);
          }
	  gtkpod_statusbar_message (
	      ngettext ("Copied %d of %d new track.",
			"Copied %d of %d new tracks.", n),
	      count, n);

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
  } /* for (.;.;.) */

  if (abort_flag)      result = FALSE;   /* negative result if user aborted */
  if (result == FALSE)
      gtkpod_statusbar_message (_("Some tracks were not written to iPod. Export aborted!"));
  gtkpod_statusbar_timeout (0);
  gtk_widget_destroy (dialog);
  while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
  return result;
}


static gboolean gp_write_itdb (iTunesDB *itdb)
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
	  if (get_offline (itdb))
	  {
	      tunes = g_strdup (eitdb->offline_filename);
	  }
	  else
	  {
	      const gchar *mountpoint = itdb_get_mountpoint (itdb);
	      g_return_val_if_fail (mountpoint, FALSE);
	      tunes = itdb_get_itunesdb_path (mountpoint);
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

  if((itdb->usertype & GP_ITDB_TYPE_IPOD) && !get_offline (itdb))
  {
      const gchar *mountpoint = itdb_get_mountpoint (itdb);
      g_return_val_if_fail (mountpoint, FALSE);
      /* check if iPod directories are present */
      if (!ipod_dirs_present (mountpoint))
      {   /* no -- create them */
	  gp_ipod_init (itdb);
	  /* if still not present abort */
	  if (!ipod_dirs_present (mountpoint))
	  {
	      gtkpod_warning (_("iPod directory structure must be present before synching to the iPod can be performed.\n"));
	      success = FALSE;
	  }
      }
      if (success)
      {   /* remove deleted files */
	  success = delete_files (itdb);
	  if (!success)
	  {
	      gtkpod_statusbar_message (_("Some tracks could not be deleted from the iPod. Export aborted!"));
	  }
      }
      if (success)
      {
	  /* write tracks to iPod */
	  success = flush_tracks (itdb);
      }
  }

  if (itdb->usertype & GP_ITDB_TYPE_LOCAL)
  {
      success = delete_files (itdb);
  }

  if (success)
      gtkpod_statusbar_message (_("Now writing database. Please wait..."));
  while (widgets_blocked && gtk_events_pending ())
      gtk_main_iteration ();


  if (success && !get_offline (itdb) &&
      (itdb->usertype & GP_ITDB_TYPE_IPOD))
  {   /* write to the iPod */
      GError *error = NULL;
      if (!itdb_write (itdb, &error))
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
      {   /* write shuffle data */
	  if (!itdb_shuffle_write (itdb, &error))
	  {   /* an error occured */
	      success = FALSE;
	      if (error && error->message)
		  gtkpod_warning ("%s\n\n", error->message);
	      else
		  g_warning ("error->message == NULL!\n");
	      g_error_free (error);
	      error = NULL;
	  }
      }
      if (success)
      {
	  if (prefs_get_int("write_extended_info"))
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
		      gtkpod_statusbar_message (_("Extended information file not deleted: '%s\'"), ext);
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
	  if (prefs_get_int("write_extended_info"))
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

  if (success && get_offline (itdb) &&
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
      if (success && prefs_get_int("write_extended_info"))
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
      Playlist *mpl = itdb_playlist_mpl (itdb);
      g_return_val_if_fail (mpl, success);
      data_unchanged (itdb);
      if (itdb->usertype & GP_ITDB_TYPE_IPOD)
      {
	  gtkpod_statusbar_message(_("%s: Database saved"), mpl->name);
      }
      else
      {
	  gtkpod_statusbar_message(_("%s: Changes saved"), mpl->name);
      }
  }

  g_free (cfgdir);

  release_widgets (); /* Allow input again */

  return success;
}



/* used to handle export of database */
/* ATTENTION: directly used as callback in gtkpod.glade -- if you
   change the arguments of this function make sure you define a
   separate callback for gtkpod.glade */
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
	ExtraiTunesDBData *eitdb;
	iTunesDB *itdb = gl->data;
	g_return_if_fail (itdb);
	eitdb = itdb->userdata;
	g_return_if_fail (eitdb);

	if (eitdb->data_changed)
	{
	    success &= gp_save_itdb (itdb);
	}
    }

    release_widgets ();
}




/* indicate that data was changed and update the free space indicator,
 * as well as the changed indicator in the playlist view */
void data_changed (iTunesDB *itdb)
{
    ExtraiTunesDBData *eitdb;
    g_return_if_fail (itdb);
    eitdb = itdb->userdata;
    g_return_if_fail (eitdb);

    eitdb->data_changed = TRUE;
    pm_name_changed (itdb_playlist_mpl (itdb));
    space_data_update ();
}


/* indicate that data was changed and update the free space indicator,
 * as well as the changed indicator in the playlist view */
void data_unchanged (iTunesDB *itdb)
{
    ExtraiTunesDBData *eitdb;
    g_return_if_fail (itdb);
    eitdb = itdb->userdata;
    g_return_if_fail (eitdb);

    eitdb->data_changed = FALSE;
    pm_name_changed (itdb_playlist_mpl (itdb));
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
/* printf ("itdb: %p, changed: %d, imported: %d\n",
   itdb, eitdb->data_changed, eitdb->itdb_imported);*/
	changed |= eitdb->data_changed;
    }
    return !changed;
}
