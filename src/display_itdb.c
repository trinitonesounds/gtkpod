/* Time-stamp: <2005-01-07 00:07:54 jcs>
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

#include "display_itdb.h"
#include "display.h"
#include "md5.h"
#include "support.h"
#include "file.h"
#include "misc.h"
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

void gp_playlist_extra_destroy (ExtraPlaylistData *epl)
{
    if (epl)
    {
	g_free (epl);
    }
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

iTunesDB *gp_itdb_new (void)
{
    iTunesDB *itdb = itdb_new ();
    ExtraiTunesDBData *eitdb = g_new0 (ExtraiTunesDBData, 1);
    itdb->userdata = eitdb;
    itdb->userdata_destroy =
	(ItdbUserDataDestroyFunc)gp_itdb_extra_destroy;
    eitdb->data_changed = FALSE;
    return itdb;
}

Playlist *gp_playlist_new (const gchar *title, gboolean spl)
{
    Playlist *pl = itdb_playlist_new (title, spl);
    pl->userdata = g_new0 (ExtraPlaylistData, 1);
    pl->userdata_destroy =
	(ItdbUserDataDestroyFunc)gp_playlist_extra_destroy;
    return pl;
}

Track *gp_track_new (void)
{
    Track *track = itdb_track_new ();
    track->userdata = g_new0 (ExtraTrackData, 1);
    track->userdata_destroy =
	(ItdbUserDataDestroyFunc)gp_track_extra_destroy;
    return track;
}

/* add itdb to itdbs */
void gp_itdb_add (iTunesDB *itdb)
{
    ExtraiTunesDBData *eitdb;

    g_assert (itdbs_head);
    g_return_if_fail (itdb);
    eitdb = itdb->userdata;
    g_assert (eitdb);

    eitdb->itdbs_head = itdbs_head;
    itdbs_head->itdbs = g_list_append (itdbs_head->itdbs, itdb);
}

/* add playlist to itdb and to display */
void gp_playlist_add (iTunesDB *itdb, Playlist *pl, gint32 pos)
{
    g_return_if_fail (itdb);
    g_return_if_fail (pl);

    itdb_playlist_add (itdb, pl, pos);
    pm_add_playlist (pl, pos);
}

/* This function removes the track "track" from the
   playlist "plitem". It then lets the display model know.
   No action is taken if "track" is not in the playlist.
   If "plitem" == NULL, remove from master playlist.
   If the track is removed from the MPL, it's also removed
   from memory completely */
void gp_playlist_remove_track (Playlist *plitem, Track *track)
{
    g_return_if_fail (track);
    g_return_if_fail (track->itdb);

    if (plitem == NULL)  plitem = itdb_playlist_mpl (track->itdb);
    g_return_if_fail (plitem);

    pm_remove_track (plitem, track);

    itdb_playlist_remove_track (plitem, track);

    if (plitem->type == ITDB_PL_TYPE_MPL)
    { /* if it's the MPL, we remove the track permanently */
	GList *gl = g_list_nth (track->itdb->playlists, 1);
	while (gl)
	{  /* first we remove the track from all other playlists (i=1) */
	    Playlist *pl = gl->data;
	    g_return_if_fail (pl);
	    pm_remove_track (pl, track);
	    itdb_playlist_remove_track (pl, track);
	}
	itdb_track_remove (track);
    }
}

/* This function appends the track "track" to the
   playlist @pl. It then lets the display model know.
   If @pl == NULL, add to master playlist
   @display: if TRUE, track is added the display.  Otherwise it's only
   added to memory */
void gp_playlist_add_track (Playlist *pl, Track *track, gboolean display)
{
    iTunesDB *itdb;

    g_return_if_fail (track);
    itdb = pl->itdb;
    g_return_if_fail (itdb);

    if (pl == NULL)  pl = itdb_playlist_mpl (itdb);
    g_return_if_fail (pl);

    pl->members = g_list_append (pl->members, track);
    if (display)  pm_add_track (pl, track, TRUE);
}


void init_data (GtkWidget *window)
{
    iTunesDB *itdb;
    Playlist *pl;

    g_assert (window);
    g_assert (itdbs_head == NULL);

    itdbs_head = g_new0 (struct itdbs_head, 1);

    g_object_set_data (G_OBJECT (window), "itdbs_head", itdbs_head);

    itdb = gp_itdb_new ();
    itdb->usertype = GP_ITDB_TYPE_IPOD;
    gp_itdb_add (itdb);
    pl = gp_playlist_new (_("gtkpod"), FALSE);
    pl->type = ITDB_PL_TYPE_MPL;  /* MPL! */
    gp_playlist_add (itdb, pl, -1);

    itdb = gp_itdb_new ();
    itdb->usertype = GP_ITDB_TYPE_LOCAL;
    gp_itdb_add (itdb);
    pl = gp_playlist_new (_("Local"), FALSE);
    pl->type = ITDB_PL_TYPE_MPL;  /* MPL! */
    gp_playlist_add (itdb, pl, -1);
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
	if (!track)	  track = itdb_track_by_filename (itdb, file);
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
	    if (itdb->usertype == GP_ITDB_TYPE_IPOD)    result = TRUE;
	}
    }
    return result;
}


