/* Time-stamp: <2005-05-26 23:29:54 jcs>
|
|  Copyright (C) 2002-2004 Jorg Schuler <jcsjcs at users.sourceforge.net>
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

#include <math.h>
#include <string.h>

#include "display_itdb.h"
#include "display.h"
#include "md5.h"
#include "file.h"
#include "misc.h"
#include "misc_track.h"
#include "info.h"
#include "prefs.h"


/* A struct containing a list with available iTunesDBs. A pointer to
   this struct is stored in gtkpod_window as itdbs_head */
static struct itdbs_head *itdbs_head = NULL;

void gp_itdb_extra_destroy (ExtraiTunesDBData *eitdb)
{
    if (eitdb)
    {
	md5_free_eitdb (eitdb);
	g_free (eitdb);
    }
}

ExtraiTunesDBData *gp_itdb_extra_duplicate (ExtraiTunesDBData *eitdb)
{
    ExtraiTunesDBData *eitdb_dup = NULL;
    if (eitdb)
    {
	/* FIXME: not yet implemented */
	g_return_val_if_reached (NULL);
    }
    return eitdb_dup;
}

void gp_playlist_extra_destroy (ExtraPlaylistData *epl)
{
    if (epl)
    {
	g_free (epl);
    }
}

ExtraPlaylistData *gp_playlist_extra_duplicate (ExtraPlaylistData *epl)
{
    ExtraPlaylistData *epl_dup = NULL;

    if (epl)
    {
	epl_dup = g_new (ExtraPlaylistData, 1);
	memcpy (epl_dup, epl, sizeof (ExtraPlaylistData));
    }
    return epl_dup;
}

void gp_track_extra_destroy (ExtraTrackData *etrack)
{
    if (etrack)
    {
	g_free (etrack->year_str);
	g_free (etrack->pc_path_utf8);
	g_free (etrack->hostname);
	g_free (etrack->md5_hash);
	g_free (etrack->charset);
	g_free (etrack);
    }
}

ExtraTrackData *gp_track_extra_duplicate (ExtraTrackData *etr)
{
    ExtraTrackData *etr_dup = NULL;

    if (etr)
    {
	etr_dup = g_new (ExtraTrackData, 1);
	memcpy (etr_dup, etr, sizeof (ExtraTrackData));
	/* copy strings */
	etr_dup->year_str = g_strdup (etr->year_str);
	etr_dup->pc_path_locale = g_strdup (etr->pc_path_locale);
	etr_dup->pc_path_utf8 = g_strdup (etr->pc_path_utf8);
	etr_dup->hostname = g_strdup (etr->hostname);
	etr_dup->md5_hash = g_strdup (etr->md5_hash);
	etr_dup->charset = g_strdup (etr->charset);
    }
    return etr_dup;
}


/* Create a new itdb struct including ExtraiTunesDBData */
iTunesDB *gp_itdb_new (void)
{
    iTunesDB *itdb = itdb_new ();
    gp_itdb_add_extra (itdb);
    return itdb;
}


    /* Add and initialize ExtraiTunesDBData if missing */
void gp_itdb_add_extra (iTunesDB *itdb)
{
    g_return_if_fail (itdb);

    if (!itdb->userdata)
    {
	ExtraiTunesDBData *eitdb = g_new0 (ExtraiTunesDBData, 1);
	itdb->userdata = eitdb;
	itdb->userdata_destroy =
	    (ItdbUserDataDestroyFunc)gp_itdb_extra_destroy;
	itdb->userdata_duplicate =
	    (ItdbUserDataDuplicateFunc)gp_itdb_extra_duplicate;
	eitdb->data_changed = FALSE;
	eitdb->itdb_imported = FALSE;
    }
}


/* Validate a complete @itdb (including tracks and playlists),
 * i.e. add the Extra*Data and validate the track entries */
void gp_itdb_add_extra_full (iTunesDB *itdb)
{
    GList *gl;

    g_return_if_fail (itdb);

    /* Add and initialize ExtraiTunesDBData if missing */
    gp_itdb_add_extra (itdb);

    /* validate tracks */
    for (gl=itdb->tracks; gl; gl=gl->next)
    {
	Track *track = gl->data;
	g_return_if_fail (track);
	gp_track_add_extra (track);
    }

    /* validate playlists */
    for (gl=itdb->playlists; gl; gl=gl->next)
    {
	Playlist *pl = gl->data;
	g_return_if_fail (pl);
	gp_playlist_add_extra (pl);
    }
}

Playlist *gp_playlist_new (const gchar *title, gboolean spl)
{
    Playlist *pl = itdb_playlist_new (title, spl);
    pl->userdata = g_new0 (ExtraPlaylistData, 1);
    pl->userdata_destroy =
	(ItdbUserDataDestroyFunc)gp_playlist_extra_destroy;
    pl->userdata_duplicate =
	(ItdbUserDataDuplicateFunc)gp_playlist_extra_duplicate;
    return pl;
}

/* Add and initialize the ExtraPlaylistData if missing */
void gp_playlist_add_extra (Playlist *pl)
{
    g_return_if_fail (pl);

    if (!pl->userdata)
    {
	ExtraPlaylistData *epl = g_new0 (ExtraPlaylistData, 1);
	pl->userdata = epl;
	pl->userdata_destroy =
	    (ItdbUserDataDestroyFunc)gp_playlist_extra_destroy;
	pl->userdata_duplicate =
	    (ItdbUserDataDuplicateFunc)gp_playlist_extra_duplicate;
    }
}

Track *gp_track_new (void)
{
    Track *track = itdb_track_new ();
    /* Add ExtraTrackData */
    gp_track_add_extra (track);
    return track;
}

/* Add and initialize the ExtraTrackData if missing */
void gp_track_add_extra (Track *track)
{
    g_return_if_fail (track);

    if (!track->userdata);
    {
	ExtraTrackData *etr = g_new0 (ExtraTrackData, 1);
	track->userdata = etr;
	track->userdata_destroy =
	    (ItdbUserDataDestroyFunc)gp_track_extra_destroy;
	track->userdata_duplicate =
	    (ItdbUserDataDuplicateFunc)gp_track_extra_duplicate;
    }
}


/* Append track to the track list of @itdb */
/* Note: the track will also have to be added to the playlists */
/* Returns: pointer to the added track -- may be different in the case
   of duplicates. In that case a pointer to the already existing track
   is returned. */
Track *gp_track_add (iTunesDB *itdb, Track *track)
{
    Track *oldtrack, *result=NULL;

    if((oldtrack = md5_track_exists_insert (itdb, track)))
    {
	gp_duplicate_remove (oldtrack, track);
	itdb_track_free (track);
	result = oldtrack;
    }
    else
    {
	/* Make sure all strings are initialised -- that way we don't
	   have to worry about it when we are handling the strings */
	/* exception: md5_hash, hostname, charset: these may be NULL. */
	gp_track_validate_entries (track);
	itdb_track_add (itdb, track, -1);
	result = track;
    }
    data_changed (itdb);
    return result;
}

/* add itdb to itdbs */
void gp_itdb_add (iTunesDB *itdb, gint pos)
{
    ExtraiTunesDBData *eitdb;

    g_return_if_fail (itdbs_head);
    g_return_if_fail (itdb);
    eitdb = itdb->userdata;
    g_return_if_fail (eitdb);

    eitdb->itdbs_head = itdbs_head;
    itdbs_head->itdbs = g_list_insert (itdbs_head->itdbs, itdb, pos);
    pm_add_itdb (itdb, pos);
}

/* Also replaces @old_itdb in the itdbs GList and take care that the
 * displayed itdb gets replaced as well */
void gp_replace_itdb (iTunesDB *old_itdb, iTunesDB *new_itdb)
{
    ExtraiTunesDBData *new_eitdb;
    Playlist *old_pl, *mpl;
    GList *old_link;
    gchar *old_pl_name = NULL;
    GtkTreePath *path;
    gint pos = -1; /* default: add to the end */

    g_return_if_fail (old_itdb);
    g_return_if_fail (new_itdb);
    g_return_if_fail (itdbs_head);

    new_eitdb = new_itdb->userdata;
    g_return_if_fail (new_eitdb);

    old_link = g_list_find (itdbs_head->itdbs, old_itdb);
    g_return_if_fail (old_link);

    /* remember old selection */
    old_pl = pm_get_selected_playlist ();
    if (old_pl)
    {   /* remember name of formerly selected playlist if it's in the
	   same itdb */
	if (old_pl->itdb == old_itdb)
	    old_pl_name = g_strdup (old_pl->name);
    }

    /* get position of @old_itdb */
    mpl = itdb_playlist_mpl (old_itdb);
    g_return_if_fail (mpl);
    path = pm_get_path (mpl);
    if (path)
    {
	gint *indices = gtk_tree_path_get_indices (path);
	if (indices)
	    pos = indices[0];
	gtk_tree_path_free (path);
    }

    /* remove @old_itdb (all playlists are removed if the MPL is
       removed and add @new_itdb at its place */
    pm_remove_playlist (mpl, FALSE);

    /* replace old_itdb with new_itdb */
    new_eitdb->itdbs_head = itdbs_head;
    old_link->data = new_itdb;
    /* free old_itdb */
    itdb_free (old_itdb);

    /* display replacement */
    pm_add_itdb (new_itdb, pos);

    /* reselect old playlist if still available */
    if (old_pl_name)
    {
	Playlist *pl = itdb_playlist_by_name (new_itdb, old_pl_name);
	if (pl) pm_select_playlist (pl);
    }
}    




/* add playlist to itdb and to display */
void gp_playlist_add (iTunesDB *itdb, Playlist *pl, gint32 pos)
{
    g_return_if_fail (itdb);
    g_return_if_fail (pl);

    itdb_playlist_add (itdb, pl, pos);
    pm_add_playlist (pl, pos);
    data_changed (itdb);
}

/* create new playlist with title @name and add to @itdb and to
 * display at position @pos */
Playlist *gp_playlist_add_new (iTunesDB *itdb, gchar *name,
			       gboolean spl, gint32 pos)
{
    Playlist *pl;

    g_return_val_if_fail (itdb, NULL);
    g_return_val_if_fail (name, NULL);

    pl = gp_playlist_new (name, spl);
    itdb_playlist_add (itdb, pl, pos);
    pm_add_playlist (pl, pos);
    data_changed (itdb);
    return pl;
}

/** If playlist @pl_name doesn't exist, then it will be created
 * and added to the tail of playlists, otherwise pointer to an existing
 * playlist will be returned
 */
Playlist *gp_playlist_by_name_or_add (iTunesDB *itdb, gchar *pl_name,
				      gboolean spl)
{
    Playlist *pl = NULL;

    g_return_val_if_fail (itdb, pl);
    g_return_val_if_fail (pl_name, pl);
    pl = itdb_playlist_by_name (itdb, pl_name);
    if (pl)
    {   /* check if the it's the same type (spl or normal) */
	if (pl->is_spl == spl) return pl;
    }
    /* Create a new playlist */
    pl = gp_playlist_add_new (itdb, pl_name, spl, -1);
    return pl;
}


/* Remove a playlist from the itdb and from the display */
void gp_playlist_remove (Playlist *pl)
{
    g_return_if_fail (pl);
    g_return_if_fail (pl->itdb);
    pm_remove_playlist (pl, TRUE);
    data_changed (pl->itdb);
    itdb_playlist_remove (pl);
}



/* FIXME: this is a bit dangerous. . . we delete all
 * playlists with titles @pl_name and return how many
 * pl have been removed.
 ***/
guint gp_playlist_remove_by_name (iTunesDB *itdb, gchar *pl_name)
{
    guint i;
    guint pl_removed=0;

    g_return_val_if_fail (itdb, pl_removed);

    for(i=1; i < itdb_playlists_number(itdb); i++)
    {
	Playlist *pl = itdb_playlist_by_nr (itdb, i);
	g_return_val_if_fail (pl, pl_removed);
	g_return_val_if_fail (pl->name, pl_removed);
        if(strcmp (pl->name, pl_name) == 0)
        {
            gp_playlist_remove (pl);
	    /* we just deleted the ith element of playlists, so
	     * we must examine the new ith element. */
            pl_removed++;
            i--;
        }
    }
    return pl_removed;
}


/* This function removes the track "track" from the
   playlist "plitem" and also adjusts the display.
   No action is taken if "track" is not in the playlist.
   If "plitem" == NULL, remove from master playlist.
   If the track is removed from the MPL, it's also removed
   from memory completely (i.e. from the tracklist and md5 hash).
   Depending on @deleteaction, the track is either marked for deletion
   on the ipod/hard disk or just removed from the database
 */
void gp_playlist_remove_track (Playlist *plitem, Track *track,
			       DeleteAction deleteaction)
{
    iTunesDB *itdb;

    g_return_if_fail (track);
    itdb = track->itdb;
    g_return_if_fail (itdb);

    switch (deleteaction)
    {
    case DELETE_ACTION_IPOD:
    case DELETE_ACTION_LOCAL:
    case DELETE_ACTION_DATABASE:
	/* remove from MPL in these cases */
	plitem = NULL;
	break;
    case DELETE_ACTION_PLAYLIST:
	/* cannot remove from MPL */
	g_return_if_fail (plitem);
	break;
    }

    if (plitem == NULL)  plitem = itdb_playlist_mpl (track->itdb);
    
    g_return_if_fail (plitem);

    /* remove track from display */
    pm_remove_track (plitem, track);

    /* remove track from playlist */
    itdb_playlist_remove_track (plitem, track);

    if (plitem->type == ITDB_PL_TYPE_MPL)
    { /* if it's the MPL, we remove the track permanently */
	GList *gl = g_list_nth (itdb->playlists, 1);
	ExtraiTunesDBData *eitdb = itdb->userdata;
	g_return_if_fail (eitdb);

	while (gl)
	{  /* first we remove the track from all other playlists (i=1) */
	    Playlist *pl = gl->data;
	    g_return_if_fail (pl);
	    pm_remove_track (pl, track);
	    itdb_playlist_remove_track (pl, track);
	    gl=gl->next;
	}
	md5_track_remove (track);

	if (itdb->usertype & GP_ITDB_TYPE_IPOD)
	{
	    switch (deleteaction)
	    {
	    case DELETE_ACTION_DATABASE:
		/* ATTENTION: this might create a dangling file on the
		   iPod! */
		itdb_track_remove (track);
		break;
	    case DELETE_ACTION_IPOD:
		if (track->transferred)
		{
		    itdb_track_unlink (track);
		    mark_track_for_deletion (itdb, track);
		}
		else
		{
		    itdb_track_remove (track);
		}
		break;
	    case DELETE_ACTION_PLAYLIST:
	    case DELETE_ACTION_LOCAL:
		break;
		/* not allowed -- programming error */
		g_return_if_reached ();
		break;
	    }
	}
	if (itdb->usertype & GP_ITDB_TYPE_LOCAL)
	{
	    switch (deleteaction)
	    {
	    case DELETE_ACTION_LOCAL:
		itdb_track_unlink (track);
		mark_track_for_deletion (itdb, track);
		break;
	    case DELETE_ACTION_DATABASE:
		itdb_track_remove (track);
		break;
	    case DELETE_ACTION_PLAYLIST:
	    case DELETE_ACTION_IPOD:
		/* not allowed -- programming error */
		g_return_if_reached ();
		break;
	    }
	}
    }

    data_changed (itdb);
}

/* This function appends the track "track" to the
   playlist @pl. It then lets the display model know.
   @display: if TRUE, track is added the display.  Otherwise it's only
   added to memory */
void gp_playlist_add_track (Playlist *pl, Track *track, gboolean display)
{
    iTunesDB *itdb;

    g_return_if_fail (track);
    g_return_if_fail (pl);
    itdb = pl->itdb;
    g_return_if_fail (itdb);

    pl->members = g_list_append (pl->members, track);
    if (display)  pm_add_track (pl, track, TRUE);

    data_changed (itdb);
}


/* Make sure all strings are initialised -- that way we don't
   have to worry about it when we are handling the strings.
   exception: md5_hash, hostname and charset: these may be NULL. */
void gp_track_validate_entries (Track *track)
{
    ExtraTrackData *etr;

    g_return_if_fail (track);
    etr = track->userdata;
    g_return_if_fail (etr);

    if (!track->album)           track->album = g_strdup ("");
    if (!track->artist)          track->artist = g_strdup ("");
    if (!track->title)           track->title = g_strdup ("");
    if (!track->genre)           track->genre = g_strdup ("");
    if (!track->comment)         track->comment = g_strdup ("");
    if (!track->composer)        track->composer = g_strdup ("");
    if (!track->fdesc)           track->fdesc = g_strdup ("");
    if (!track->grouping)        track->grouping = g_strdup ("");
    if (!etr->pc_path_utf8)      etr->pc_path_utf8 = g_strdup ("");
    if (!etr->pc_path_locale)    etr->pc_path_locale = g_strdup ("");
    if (!track->ipod_path)       track->ipod_path = g_strdup ("");
    /* Make sure year_str is identical to year */
    g_free (etr->year_str);
    etr->year_str = g_strdup_printf ("%d", track->year);
}



/* Initialize the itdb data
 *
 * If itdb_n_type/filename/mountpoint... exist in the prefs, that data
 * is being used and local databases are read in directly if they
 * exist. Otherwise a simple two-itdb list is constructed consisting
 * of one local database and one ipod database.
 *
 */
void init_data (GtkWidget *window)
{
    gchar *cfgdir;
    gint type;

    g_return_if_fail (window);
    g_return_if_fail (itdbs_head == NULL);

    cfgdir = prefs_get_cfgdir ();

    itdbs_head = g_new0 (struct itdbs_head, 1);

    g_object_set_data (G_OBJECT (window), "itdbs_head", itdbs_head);

    if (prefs_get_int_value ("itdb_0_type", &type))
    {   /* databases have been set up previously -- use what is stored
	 * in the prefs */
	gint i;
	for (i=0;;++i)
	{
	    gchar *property = g_strdup_printf ("itdb_%d_type", i);
	    gboolean valid = prefs_get_int_value (property, &type);
	    g_free (property);
	    if (valid)
	    {
		iTunesDB *itdb = NULL;
		Playlist *pl = NULL;
		ExtraiTunesDBData *eitdb;
		gchar *filename = NULL;
		gchar *mountpoint = NULL;
		gchar *offline_filename = NULL;

		if (type & GP_ITDB_TYPE_LOCAL)
		{
		    gchar *fn = g_strdup_printf ("itdb_%d_filename", i);

		    filename = prefs_get_string (fn);

		    if (!filename)
		    {
			gchar *local = g_strdup_printf ("local%d.itdb",i);
			filename = g_build_filename (cfgdir, local, NULL);
			g_free (local);
		    }
		    g_free (fn);
		    if (g_file_test (filename, G_FILE_TEST_EXISTS))
			itdb = gp_import_itdb (NULL, type,
					       NULL, NULL, filename);
		}
		else if (type & GP_ITDB_TYPE_IPOD)
		{
		    gchar *mp = g_strdup_printf ("itdb_%d_mountpoint", i);
		    gchar *fn = g_strdup_printf ("itdb_%d_filename", i);
		    

		    offline_filename = prefs_get_string (fn);
		    mountpoint = prefs_get_string (mp);

		    if (!offline_filename)
		    {
			gchar *local = g_strdup_printf ("gtkpod_%d.itdb",i);
			offline_filename = g_build_filename (
			    cfgdir, local, NULL);
			g_free (local);
		    }
		    if (!mountpoint)
			mountpoint = g_strdup (prefs_get_ipod_mount ());
		    g_free (fn);
		    g_free (mp);
		}
		else
		{
		    g_return_if_reached ();
		}


		if (!itdb)
		{
		    gchar *nm, *name;

		    itdb = gp_itdb_new ();
		    eitdb = itdb->userdata;
		    g_return_if_fail (eitdb);
		    itdb->usertype = type;
		    itdb->filename = filename;
		    itdb->mountpoint = mountpoint;
		    eitdb->offline_filename = offline_filename;

		    nm = g_strdup_printf ("itdb_%d_name", i);
		    if (!prefs_get_string_value (nm, &name))
		    {
			if (type & GP_ITDB_TYPE_LOCAL)
			    name = g_strdup (_("Local"));
			else
			    name = g_strdup ("gtkpod");
		    }
		    pl = gp_playlist_new (name, FALSE);
		    g_free (name);
		    g_free (nm);
		    pl->type = ITDB_PL_TYPE_MPL;
		    itdb_playlist_add (itdb, pl, -1);

		    eitdb->data_changed = FALSE;
		    eitdb->itdb_imported = FALSE;
		}
		else
		{
		    g_free (filename);
		    g_free (mountpoint);
		    g_free (offline_filename);
		}

		/* add to the display */
		gp_itdb_add (itdb, -1);
	    }
	    else
	    {
		break; /* for (i=0;;++i) */
	    }
	}
    }
    else
    {   /* first run -- set up a local database and an iPod database */
	iTunesDB *itdb;
	ExtraiTunesDBData *eitdb;
	gchar *fn;
	Playlist *pl;

	/* iPod database */
	itdb = gp_itdb_new ();
	eitdb = itdb->userdata;
	g_return_if_fail (eitdb);
	itdb->usertype = GP_ITDB_TYPE_IPOD;
	itdb->mountpoint = g_strdup (prefs_get_ipod_mount ());
	eitdb->offline_filename = g_build_filename (
	    cfgdir, "iTunesDB", NULL);
	gp_itdb_add (itdb, -1);
	pl = gp_playlist_new ("gtkpod", FALSE);
	pl->type = ITDB_PL_TYPE_MPL;  /* MPL! */
	gp_playlist_add (itdb, pl, -1);
	g_return_if_fail (itdb->userdata);
	eitdb->data_changed = FALSE;

	/* local database. First check if a database file already
	   exists -- if yes load it */
	fn = g_build_filename (cfgdir, "local_0.itdb", NULL);
	if (g_file_test (fn, G_FILE_TEST_EXISTS))
	{
	    itdb = gp_import_itdb (NULL, GP_ITDB_TYPE_LOCAL,
				   NULL, NULL, fn);
	}
	if (!itdb)
	{   /* local database does not exist or cannot be loaded */
	    itdb = gp_itdb_new ();
	    eitdb = itdb->userdata;
	    g_return_if_fail (eitdb);
	    itdb->usertype = GP_ITDB_TYPE_LOCAL;
	    itdb->filename = g_strdup (fn);
	    pl = gp_playlist_new (_("Local"), FALSE);
	    pl->type = ITDB_PL_TYPE_MPL;  /* MPL! */
	    itdb_playlist_add (itdb, pl, -1);
	}
	gp_itdb_add (itdb, -1);
	g_free (fn);
    }

    /* set md5 */
    prefs_set_md5tracks (prefs_get_md5tracks ());

    /* update prefs with new information in case it's needed */
    gp_update_itdb_prefs ();

    g_free (cfgdir);
}


/* Update the information about the itdbs in the prefs structure */
void gp_update_itdb_prefs (void)
{
    struct itdbs_head *itdbs_head;
    GList *gl;
    gint i=0;

    g_return_if_fail (gtkpod_window);
    itdbs_head = g_object_get_data (G_OBJECT (gtkpod_window),
				    "itdbs_head");
    g_return_if_fail (itdbs_head);
    for (gl=itdbs_head->itdbs; gl; gl=gl->next)
    {
	gchar *prop, *prop2;
	Playlist *mpl;
	ExtraiTunesDBData *eitdb;
	iTunesDB *itdb = gl->data;

	g_return_if_fail (itdb);
	eitdb = itdb->userdata;
	g_return_if_fail (eitdb);

	prop = g_strdup_printf ("itdb_%d_type", i);
	prefs_set_int_value (prop, itdb->usertype);
	g_free (prop);

	mpl = itdb_playlist_mpl (itdb);
	g_return_if_fail (mpl);
	prop = g_strdup_printf ("itdb_%d_name", i);
	prefs_set_string_value (prop, mpl->name);
	g_free (prop);

	prop = g_strdup_printf ("itdb_%d_filename", i);
	prop2 = g_strdup_printf ("itdb_%d_mountpoint", i);
	if (itdb->usertype & GP_ITDB_TYPE_LOCAL)
	{
	    prefs_set_string_value (prop, itdb->filename);
	    prefs_set_string_value (prop2, NULL);
	}
	else if (itdb->usertype & GP_ITDB_TYPE_IPOD)
	{
	    prefs_set_string_value (prop, eitdb->offline_filename);
	    prefs_set_string_value (prop2, itdb->mountpoint);
	}
	else
	{
	    g_return_if_reached ();
	}
	g_free (prop);
	g_free (prop2);

	++i;
    }
}


/* Increase playcount of filename <file> by <num>. If md5 is activated,
   use md5 to find the track. Otherwise use the filename. If @md5 is
   set, this value is used directly to look up the track in the
   database (instead of calculating it from the file).

   Return value:
   TRUE: OK (playcount increased in GP_ITDB_TYPE_IPOD)
   FALSE: file could not be found in the GP_ITDB_TYPE_IPOD */
gboolean gp_increase_playcount (gchar *md5, gchar *file, gint num)
{
    gboolean result = FALSE;
    Track *track = NULL;
    GList *gl;

    g_return_val_if_fail (itdbs_head, FALSE);

    for (gl=itdbs_head->itdbs; gl; gl=gl->next)
    {
	iTunesDB *itdb = gl->data;
	g_return_val_if_fail (itdb, FALSE);

	if (md5) track = md5_md5_exists (itdb, md5);
	else     track = md5_file_exists (itdb, file, TRUE);
	if (!track)	  track = gp_track_by_filename (itdb, file);
	if (track)
	{
	    gchar *buf1, *buf;
	    track->playcount += num;
	    data_changed (itdb);
	    pm_track_changed (track);
	    buf1 = get_track_info (track);
	    buf = g_strdup_printf (_("Increased playcount for '%s'"), buf1);
	    gtkpod_statusbar_message (buf);
	    g_free (buf);
	    g_free (buf1);
	    if (itdb->usertype & GP_ITDB_TYPE_IPOD)    result = TRUE;
	}
    }
    return result;
}


/* determine "active" itdb -- it's either the itdb of the playlist
 * currently selected, or the first itdb if none is selected */
iTunesDB *gp_get_active_itdb (void)
{
    Playlist *pl = pm_get_selected_playlist ();
    struct itdbs_head *itdbs_head;

    /* If playlist is selected, use the itdb of the playlist as the
       active itdb */
    if (pl)
    {
	g_return_val_if_fail (pl->itdb, NULL);
	return pl->itdb;
    }

    /* Otherwise choose the first itdb */
    g_return_val_if_fail (gtkpod_window, NULL);
    itdbs_head = g_object_get_data (G_OBJECT (gtkpod_window),
				    "itdbs_head");
    g_return_val_if_fail (itdbs_head, NULL);
    g_return_val_if_fail (itdbs_head->itdbs, NULL);
    g_return_val_if_fail (itdbs_head->itdbs->data, NULL);
    return itdbs_head->itdbs->data;
}


/* get the "ipod" itdb, that's the first itdb with
   type==GP_ITDB_TYPE_IPOD. Returns NULL and prints error when no
   matching itdb can be found */
iTunesDB *gp_get_ipod_itdb (void)
{
    struct itdbs_head *itdbs_head;
    GList *gl;

    g_return_val_if_fail (gtkpod_window, NULL);
    itdbs_head = g_object_get_data (G_OBJECT (gtkpod_window),
				    "itdbs_head");
    g_return_val_if_fail (itdbs_head, NULL);
    for (gl=itdbs_head->itdbs; gl; gl=gl->next)
    {
	iTunesDB *itdb = gl->data;
	g_return_val_if_fail (itdb, NULL);
	if (itdb->usertype & GP_ITDB_TYPE_IPOD)  return itdb;
    }
    g_return_val_if_reached (NULL);
}
