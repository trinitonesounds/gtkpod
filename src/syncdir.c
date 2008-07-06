/* Time-stamp: <2008-07-06 10:38:05 jcs>
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

/* This file provides functions for syncing a directory or directories
 * with a playlist */

#include <libintl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "display_itdb.h"
#include "file.h"
#include "info.h"
#include "misc.h"
#include "misc_track.h"
#include "prefs.h"
#include "syncdir.h"


struct add_files_data
{
    Playlist *playlist;
    GList    **tracks_updated;
    GHashTable *filepath_hash;
};

/* Used in the callback after adding a new track to 
 * to add to the filehash */
struct added_file_data
{
    GHashTable *filepath_hash;
    gchar *filepath;
};


/**
 * confirm_sync_dirs:
 *
 * @dirs_hash: hash table containing the directory names
 * @key_sync_confirm_dirs: preference key to specify whether or not
 *            the list of directories should be confirmed. The
 *            confirmation dialog may change the value of this prefs
 *            entry. If NULL confirmation takes place.
 *
 * Have the user confirm which directories should be included into the
 * confirmation process.
 *
 * Return value: FALSE: user aborted. TRUE: otherwise. @dirs_hash will
 * be adjusted to reflect the selected directories.
 */
static gboolean confirm_sync_dirs (GHashTable *dirs_hash,
				   const gchar *key_sync_confirm_dirs)
{
    g_return_val_if_fail (dirs_hash, FALSE);

    if (key_sync_confirm_dirs && !prefs_get_int (key_sync_confirm_dirs))
	return TRUE;

    /* FIXME: implement confirmation (doesn't strike me as a major
     * feature -- feel free to contribute)
     *
     * The idea would be to have the user check/uncheck each of the
     * individual directories. @dirs_hash will be adjusted to reflect
     * the selected directories.
     */
    return TRUE;
}


/**
 * confirm_delete_tracks:
 *
 * @tracks: GList with tracks that are supposed to be removed from the
 *          iPod or the local repository.
 * @key_sync_confirm_delete: preference key to specify whether or not
 *          the removal of tracks should be confirmed. The
 *          confirmation dialog may change the value of this prefs
 *          entry. If NULL confirmation takes place.
 *
 * Return value: TRUE: it's OK to remove the tracks. FALSE: it's not
 *               OK to remove the tracks. TRUE is also given if no
 *               tracks are present, @key_sync_confirm_delete is NULL
 *               or it's setting is 'FALSE' (0).
 */

static gboolean confirm_delete_tracks (GList *tracks,
				       const gchar *key_sync_confirm_delete)
{
    GtkResponseType response;
    struct DeleteData dd;
    gchar *label, *title;
    GString *string;
    iTunesDB *itdb;
    Track *tr;

    if (tracks == NULL)
	return TRUE;

    if (key_sync_confirm_delete &&
	!prefs_get_int (key_sync_confirm_delete))
	return TRUE;

    tr = g_list_nth_data (tracks, 0);
    g_return_val_if_fail (tr, FALSE);
    itdb = tr->itdb;
    g_return_val_if_fail (itdb, FALSE);

    dd.itdb = itdb;
    dd.pl = NULL;
    dd.tracks = tracks;
    if (itdb->usertype & GP_ITDB_TYPE_IPOD)
	dd.deleteaction = DELETE_ACTION_IPOD;
    if (itdb->usertype & GP_ITDB_TYPE_LOCAL)
	dd.deleteaction = DELETE_ACTION_DATABASE;

    delete_populate_settings (&dd,
			      &label, &title,
			      NULL, NULL,
			      &string);

    response = gtkpod_confirmation (
	-1,                       /* gint id, */
	TRUE,                     /* gboolean modal, */
	title,                    /* title */
	label,                    /* label */
	string->str,              /* scrolled text */
	NULL, 0, NULL,            /* option 1 */
	NULL, 0, NULL,            /* option 2 */
	TRUE,                     /* gboolean confirm_again, */
	key_sync_confirm_delete,  /* ConfHandlerOpt confirm_again_key,*/
	CONF_NULL_HANDLER,        /* ConfHandler ok_handler,*/
	NULL,                     /* don't show "Apply" button */
	CONF_NULL_HANDLER,        /* cancel_handler,*/
	NULL,                     /* gpointer user_data1,*/
	NULL);                    /* gpointer user_data2,*/


    g_free (label);
    g_free (title);
    g_string_free (string, TRUE);

    if (response == GTK_RESPONSE_OK)
    {
	/* it's OK to remove the tracks */
	return TRUE;
    }
    else
    {
	/* better not delete the tracks */
	return FALSE;
    }
}


/* Used by sync_show_summary() */
static void sync_add_tracks (GString *str,
			     GList *tracks, const gchar *title)
{
    GList *gl;

    g_return_if_fail (str);
    g_return_if_fail (title);

    if (tracks)
    {
	g_string_append (str, title);

	for (gl=tracks; gl; gl=gl->next)
	{
	    gchar *buf;
	    Track *tr = gl->data;
	    g_return_if_fail (tr);

	    buf = get_track_info (tr, FALSE);
	    g_string_append_printf (str, "%s\n", buf);
	    g_free (buf);
	}
	g_string_append_printf (str, "\n\n");
    }
}



/**
 * sync_show_summary:
 *
 * @key_sync_show_summary: preference key to specify whether or not a
 *          summary should be shown or not. If NULL, the summary is
 *          shown. This key may be changed by the confirmation dialog.
 * @playlist: playlist where are syncing with.
 * @tracks_to_delete_from_ipod: GList with tracks to be deleted from
 *          the iPod or local repository.
 * @tracks_to_delete_from_playlist: GList with tracks to be deleted
 *          from @playlist.
 * @tracks_updated: GList with tracks that have been updated.
 */
static void show_sync_summary (const gchar *key_sync_show_summary,
			       Playlist *playlist,
			       GList *tracks_to_delete_from_ipod,
			       GList *tracks_to_delete_from_playlist,
			       GList *tracks_updated)
{
    GString *summary;
    Playlist *mpl;
    gint no_length;

    g_return_if_fail (playlist);
    g_return_if_fail (playlist->itdb);

    if (key_sync_show_summary && !prefs_get_int (key_sync_show_summary))
	return;

    summary = g_string_sized_new (2000);

    /* mpl->name is the repository's name */
    mpl = itdb_playlist_mpl (playlist->itdb);
    g_return_if_fail (mpl);
    g_string_append_printf (summary,
			    _("Sync summary for %s/%s\n"),
			    mpl->name, playlist->name);

    /* used to check whether data was added or not */
    no_length = strlen (summary->str);

    sync_add_tracks (
	summary,
	tracks_updated,
	ngettext ("The following track has been added or updated:\n",
		  "The following tracks have been added or updated:\n",
		  g_list_length (tracks_updated)));

    if (playlist->itdb->usertype & GP_ITDB_TYPE_IPOD)
    {
	sync_add_tracks (
	    summary,
	    tracks_to_delete_from_ipod,
	    ngettext ("The following track has been completely removed from the iPod:\n",
		      "The following tracks have been completely removed from the iPod:\n",
		      g_list_length (tracks_to_delete_from_ipod)));
    }
    else
    {
	sync_add_tracks (
	    summary,
	    tracks_to_delete_from_ipod,
	    ngettext ("The following track has been removed from the repository:\n",
		      "The following tracks have been removed from the repository:\n",
		      g_list_length (tracks_to_delete_from_ipod)));
    }

    sync_add_tracks (summary,
		     tracks_to_delete_from_playlist,
		     ngettext ("The following track has been removed from the playlist:\n",
			       "The following tracks have been removed from the playlist:\n",
			       g_list_length (tracks_to_delete_from_playlist)));

    if (strlen (summary->str) == no_length)
    {
	g_string_append (summary, _("Nothing was changed.\n"));
    }

    gtkpod_confirmation (CONF_ID_SYNC_SUMMARY,
			 FALSE,
			 _("Sync summary"),
			 NULL,
			 summary->str,
			 NULL, 0, NULL,
			 NULL, 0, NULL,
			 TRUE,
			 key_sync_show_summary,
			 CONF_NULL_HANDLER, NULL, NULL,
			 NULL, NULL);

    g_string_free (summary, TRUE);
}




/* Callback for adding tracks (makes sure track isn't added to playlist
 * again if it already exists */
static void sync_addtrackfunc (Playlist *plitem, Track *track, gpointer data)
{
	struct added_file_data *afd = data;

    g_return_if_fail (plitem);
    g_return_if_fail (track);

    g_return_if_fail (afd->filepath_hash);
    g_return_if_fail (afd->filepath);

    /* add the new entry to the filepath */
    g_hash_table_insert (afd->filepath_hash, g_strdup (afd->filepath), track);

    /* only add if @track isn't already a member of the current playlist */
    if (!itdb_playlist_contains_track (plitem, track))
	gp_playlist_add_track (plitem, track, TRUE);
}


/* Builds a hash of all the tracks in the playlists db,
 * hashed by the file path */
static GHashTable *get_itdb_filepath_hash (Playlist *pl)
{
    GHashTable* filepath_hash;
    iTunesDB *itdb = pl->itdb;

    filepath_hash = g_hash_table_new_full (
	g_str_hash, g_str_equal, g_free, NULL);

    GList *gl;
    for (gl=itdb->tracks; gl; gl=gl->next)
    {
	ExtraTrackData *etr;
	Track *track = gl->data;
	g_return_val_if_fail (track, NULL);

	etr = track->userdata;
	g_return_val_if_fail (etr, NULL);

	/* track has filename info */
	if (etr->pc_path_locale && *etr->pc_path_locale)
	{
	    g_hash_table_insert (filepath_hash, g_strdup (etr->pc_path_locale), track);
	}
    }
	
    return filepath_hash;
}


/**
 * add_files:
 *
 * add all music/video files to the playlist @userdata->playlist. 
 * updated/newly added tracks are appended to @userdata->tracks_updated.
 */
static void add_files (gpointer key, gpointer value, gpointer user_data)
{
    struct add_files_data *afd = user_data;
    Playlist *pl;
    gchar *dirname;

    g_return_if_fail (key);
    g_return_if_fail (afd);
    g_return_if_fail (afd->playlist);
    g_return_if_fail (afd->tracks_updated);
    g_return_if_fail (afd->filepath_hash);

    dirname = key;
    pl = afd->playlist;

    if (g_file_test (dirname, G_FILE_TEST_IS_DIR))
    {
	GDir *dir = g_dir_open (dirname, 0, NULL);
	if (dir != NULL)
	{
	    G_CONST_RETURN gchar *next;
	    while ((next = g_dir_read_name (dir)))
	    {
		gchar *filename = g_build_filename (dirname, next, NULL);
		FileType filetype = determine_file_type (filename);
		gboolean updated = FALSE;
		Track *tr=NULL;

		switch (filetype)
		{
		case FILE_TYPE_UNKNOWN:
		case FILE_TYPE_DIRECTORY:
		case FILE_TYPE_IMAGE:
		case FILE_TYPE_M3U:
		case FILE_TYPE_PLS:
		    /* ignore non-music/video files */
		    break;
		case FILE_TYPE_MP3:
		case FILE_TYPE_M4A:
		case FILE_TYPE_M4P:
		case FILE_TYPE_M4B:
		case FILE_TYPE_WAV:
		case FILE_TYPE_M4V:
		case FILE_TYPE_MP4:
		case FILE_TYPE_MOV:
		case FILE_TYPE_MPG:
                case FILE_TYPE_OGG:
                case FILE_TYPE_FLAC:
		    tr = g_hash_table_lookup (afd->filepath_hash, filename);
		    if (tr)
		    {   /* track is already present in playlist.
			   Update if date stamp is different. */
			struct stat filestat;
			ExtraTrackData *etr = tr->userdata;
			g_return_if_fail (etr);

			stat (filename, &filestat);
/*
printf ("%ld %ld (%s)\n, %ld %d\n",
	filestat.st_mtime, etr->mtime,
	filename,
	filestat.st_size, tr->size);
*/
			if ((filestat.st_mtime != etr->mtime) ||
			    (filestat.st_size != tr->size))
			{
			    update_track_from_file (pl->itdb, tr);
			    updated = TRUE;
			}
		    }
		    else
		    {   /* track is not known -- at least not by it's
			 * filename -> add to playlist using the
			 * standard function. Duplicate adding is
			 * avoided by an addtrack function checking
			 * for duplication */
			struct added_file_data data;
			data.filepath = filename;
			data.filepath_hash = afd->filepath_hash;

			add_track_by_filename (pl->itdb, filename,
					       pl, FALSE,
					       sync_addtrackfunc, &data);

			tr = g_hash_table_lookup (afd->filepath_hash, filename);
			updated = TRUE;
		    }
		    break;
		}
		if (tr && updated)
		{
		    *afd->tracks_updated =
			g_list_append (*afd->tracks_updated, tr);
		}
		g_free (filename);
	    }
	} 
	g_dir_close (dir);
    }
}




/**
 * sync_playlist:
 *
 * @playlist: playlist to sync with contents on hard disk
 * @syncdir:  directory to sync with. If @syncdir is NULL, a list of
 *            directories is created from all the filenames of the
 *            member tracks
 * @key_sync_confirm_dirs: preference key to specify whether or not
 *            the list of directories should be confirmed. The
 *            confirmation dialog may change the value of this prefs
 *            entry. If NULL, @sync_confirm_dirs decides whether
 *            confirmation takes place or not.
 *            FIXME: not implemented at present.
 * @sync_confirm_dirs: see under @key_sync_confirm_dirs.
 * @key_sync_delete_tracks: preference key to specify whether or not
 *            tracks no longer present in the directory list should be
 *            removed from the iPod/database or not. Normally tracks
 *            are only removed from the current playlist. If this key's
 *            value is set to TRUE (1), they will be removed from the
 *            iPod /database completely, if they are not a member of
 *            other playlists. Note: to remove tracks from the MPL,
 *            this has to be TRUE.
 *            If NULL, @sync_delete_tracks will determine whether
 *            tracks are removed or not. Also, if @playlist is the
 *            MPL, tracks will be removed irrespective of this key's
 *            value. 
 * @sync_delete_tracks: see under @key_sync_delete_tracks.
 * @key_sync_confirm_delete: preference key to specify whether or not
 *            the removal of tracks should be confirmed. The
 *            confirmation dialog may change the value of this prefs
 *            entry. If NULL, @sync_confirm_delete will determine
 *            whether or not confirmation takes place.
 * @sync_confirm_delete: see under @key_sync_confirm_delete
 * @key_sync_show_summary: preference key to specify whether or not a
 *            summary of removed and newly added or updated tracks
 *            should be displayed. If NULL, @sync_show_shummary will
 *            determine whether or not a summary is displayed.
 * @sync_show_shummary: see under @key_sync_show_shummary
 *
 * Return value: none, but will give status information via the
 * statusbar and information windows.
 **/
void sync_playlist (Playlist *playlist,
		    const gchar *syncdir,
		    const gchar *key_sync_confirm_dirs,
		    gboolean sync_confirm_dirs,
		    const gchar *key_sync_delete_tracks,
		    gboolean sync_delete_tracks,
		    const gchar *key_sync_confirm_delete,
		    gboolean sync_confirm_delete,
		    const gchar *key_sync_show_summary,
		    gboolean sync_show_summary)
{
    GHashTable *dirs_hash, *filepath_hash;
    gboolean delete_tracks, is_mpl;
    time_t current_time;
    GList *tracks_to_delete_from_ipod = NULL;
    GList *tracks_to_delete_from_playlist = NULL;
    GList *tracks_updated = NULL;
    struct add_files_data afd;
    GList *gl;

    g_return_if_fail (playlist);

    /* Create a hash to keep the directory names ("key", and "value"
       to be freed with g_free). key is dirname in local encoding,
       value is dirname in utf8, if available */
    dirs_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

    /* If @syncdir is not NULL, put @syndir into the hash
       table. Otherwise put the dirs of all tracks in @playlist into
       the table. */

    if (syncdir)
    {
	/* make sure the directory name does not end in '/' -- the
	   code below does not seem to like this */
	gint len = strlen (syncdir);
	gchar *dir = g_strdup (syncdir);
	if (len > 1)
	{
	    if (G_IS_DIR_SEPARATOR (dir[len-1]))
	    {
		dir[len-1] = 0;
	    }
	}
	g_hash_table_insert (dirs_hash, dir, NULL);
    }
    else
    {
	for (gl = playlist->members; gl; gl=gl->next)
	{
	    ExtraTrackData *etr;
	    Track *track = gl->data;
	    g_return_if_fail (track);
	    etr = track->userdata;
	    g_return_if_fail (etr);

	    if (etr->pc_path_locale && *etr->pc_path_locale)
	    {
		gchar *dirname_local;

		dirname_local = g_path_get_dirname (etr->pc_path_locale);
		if (etr->pc_path_utf8 && *etr->pc_path_utf8)
		{
		    g_hash_table_insert (
			dirs_hash,
			dirname_local,
			g_path_get_dirname (etr->pc_path_utf8));
		}
		else
		{   /* no utf8 -- make sure we don't replace a dir
		     * entry that had the utf8 data set */
		    if (!g_hash_table_lookup (dirs_hash, dirname_local))
		    {
			g_hash_table_insert (dirs_hash,
					     dirname_local, NULL);
		    }
		    else
		    {
			g_free (dirname_local);
		    }
		}
	    }
	}
    }

    /* Confirm directories */
    if (key_sync_confirm_dirs || sync_confirm_dirs)
    {
	if (!confirm_sync_dirs (dirs_hash, key_sync_confirm_dirs))
	{   /* aborted */
	    g_hash_table_destroy (dirs_hash);
	    return;
	}
    }

    /* current_time can be used to recognize newly added/updated
       tracks */
    current_time = time (NULL);

    /* craete a hash with all files in the current playlist for faster
     * comparison with files in the directory */
    filepath_hash = get_itdb_filepath_hash (playlist);

    afd.playlist = playlist;
    afd.tracks_updated = &tracks_updated;
    afd.filepath_hash = filepath_hash;
    /* Add all files in all directories present in dirs_hash */
    g_hash_table_foreach (dirs_hash, add_files, &afd);

    /* we won't need this hash any more */
    g_hash_table_destroy (filepath_hash);
    filepath_hash = NULL;

    /* Remove updated and duplicate list so it won't pop up at a later
       time */
    display_updated ((void *)-1, NULL);
    display_non_updated ((void *)-1, NULL);
    gp_duplicate_remove (NULL, (void *)-1);

    /* Should tracks be deleted that were not present in the
     * directories? */
    if (key_sync_delete_tracks == NULL)
    {
	delete_tracks = TRUE;
    }
    else
    {
	delete_tracks = prefs_get_int (key_sync_delete_tracks);
    }
    /* Is playlist the MPL? */
    is_mpl = itdb_playlist_is_mpl (playlist);

    /* Identify all tracks in playlist not being located in one of the
       specified dirs, or no longer existing. */
    for (gl=playlist->members; gl; gl=gl->next)
    {
	ExtraTrackData *etr;
	gboolean remove;

	Track *tr = gl->data;
	g_return_if_fail (tr);
	etr = tr->userdata;
	g_return_if_fail (etr);

	remove = FALSE;
	if (etr->pc_path_locale && *etr->pc_path_locale)
	{
	    gchar *dirname_local;

	    dirname_local = g_path_get_dirname (etr->pc_path_locale);
	    if (!g_hash_table_lookup_extended (dirs_hash, dirname_local,
					       NULL, NULL))
	    {   /* file is not in one of the specified directories */
		remove = TRUE;
	    }
	    else
	    {   /* check if file exists */
		if (g_file_test (etr->pc_path_locale,
				 G_FILE_TEST_EXISTS) == FALSE)
		{   /* no -- remove */
		    remove = TRUE;
		}
	    }
	    g_free (dirname_local);
	}

	if (remove)
	{   /* decide whether track needs to be removed from the iPod
	     * (only member of this playlist) or only from this
	     * playlist (if delete_tracks is not set, no tracks are
	     * removed from the MPL) */
	    if (delete_tracks &&
		(is_mpl || (itdb_playlist_contain_track_number (tr)==1)))
	    {
		tracks_to_delete_from_ipod =
		    g_list_append (tracks_to_delete_from_ipod, tr);
	    }
	    else
	    {
		if (!is_mpl)
		{
		    tracks_to_delete_from_playlist =
			g_list_append (tracks_to_delete_from_playlist, tr);
		}
	    }
	}
    }


    if (tracks_to_delete_from_ipod &&
	(key_sync_confirm_delete || sync_confirm_delete) &&
	(confirm_delete_tracks (tracks_to_delete_from_ipod,
				key_sync_confirm_delete) == FALSE))
    {   /* User doesn't want us to remove those tracks from the
	 * iPod. We'll therefore just remove them from the playlist
	 * (if playlist is the MPL, don't remove at all) */
	if (!is_mpl)
	{
	    tracks_to_delete_from_playlist = g_list_concat (
		tracks_to_delete_from_playlist,
		tracks_to_delete_from_ipod);
	}
	else
	{
	    g_list_free (tracks_to_delete_from_ipod);
	}
	tracks_to_delete_from_ipod = NULL;
    }


    if (key_sync_show_summary || sync_show_summary)
    {
	show_sync_summary (key_sync_show_summary,
			   playlist,
			   tracks_to_delete_from_ipod,
			   tracks_to_delete_from_playlist,
			   tracks_updated);
    }

    /* Remove completely */
    for (gl=tracks_to_delete_from_ipod; gl; gl=gl->next)
    {
	Track *tr = gl->data;
	g_return_if_fail (tr);

	if (tr->itdb->usertype & GP_ITDB_TYPE_IPOD)
	    gp_playlist_remove_track (NULL, tr, DELETE_ACTION_IPOD);
	else if (tr->itdb->usertype & GP_ITDB_TYPE_LOCAL)
	    gp_playlist_remove_track (NULL, tr, DELETE_ACTION_DATABASE);
    }

    /* Remove from playlist */
    for (gl=tracks_to_delete_from_playlist; gl; gl=gl->next)
    {
	Track *tr = gl->data;
	g_return_if_fail (tr);

	gp_playlist_remove_track (playlist, tr, DELETE_ACTION_PLAYLIST);
    }

    /* Was any data changed? */
    if (tracks_to_delete_from_ipod ||
	tracks_to_delete_from_playlist ||
	tracks_updated)
    {
	data_changed (playlist->itdb);
	gtkpod_tracks_statusbar_update ();
    }

    g_list_free (tracks_to_delete_from_ipod);
    g_list_free (tracks_to_delete_from_playlist);
    g_list_free (tracks_updated);
}


/**
 * sync_all_playlists:
 *
 * @itdb: repository whose playlists are to be updated
 * 
 * Will update all playlists in @itdb according to options set. The
 * following pref subkeys are relevant:
 * 
 * sync_confirm_dirs
 * sync_delete_tracks
 * sync_confirm_delete
 * sync_show_summary
 */

void sync_all_playlists (iTunesDB *itdb)
{
    gint index;
    GList *gl;

    g_return_if_fail (itdb);

    index = get_itdb_index (itdb);

    for (gl=itdb->playlists; gl; gl=gl->next)
    {
	gint syncmode;
	Playlist *pl = gl->data;
	g_return_if_fail (pl);

	syncmode = get_playlist_prefs_int (pl, KEY_SYNCMODE);
	if (syncmode != SYNC_PLAYLIST_MODE_NONE)
	{
	    gchar *key_sync_confirm_dirs =
		get_playlist_prefs_key (index, pl, KEY_SYNC_CONFIRM_DIRS);
	    gchar *key_sync_delete_tracks =
		get_playlist_prefs_key (index, pl, KEY_SYNC_DELETE_TRACKS);
	    gchar *key_sync_confirm_delete =
		get_playlist_prefs_key (index, pl, KEY_SYNC_CONFIRM_DELETE);
	    gchar *key_sync_show_summary =
		get_playlist_prefs_key (index, pl, KEY_SYNC_SHOW_SUMMARY);
	    gchar *syncdir = NULL;

	    if (syncmode == SYNC_PLAYLIST_MODE_MANUAL)
	    {
		syncdir = get_playlist_prefs_string (pl,
						     KEY_MANUAL_SYNCDIR);
	    }

	    sync_playlist (pl, syncdir,
			   key_sync_confirm_dirs, 0,
			   key_sync_delete_tracks, 0,
			   key_sync_confirm_delete, 0,
			   key_sync_show_summary, 0);

	    g_free (key_sync_confirm_dirs);
	    g_free (key_sync_delete_tracks);
	    g_free (key_sync_confirm_delete);
	    g_free (key_sync_show_summary);
	    g_free (syncdir);
	}
    }
}
