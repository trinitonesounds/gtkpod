/* Time-stamp: <2005-01-03 22:40:46 jcs>
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

#include "display_itdb.h"
#include "display.h"
#include "md5.h"
#include "support.h"

/* list with available iTunesDBs. A pointer to this list is stored in
 * gtkpod_window as "itdbs" data (see . */
static GList *itdbs=NULL;


void gp_itdb_extra_destroy (ExtraiTunesDBData *eitdb)
{
    if (eitdb)
    {
	md5_free (eitdb->md5hash);
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
    itdb->userdata = g_new0 (ExtraiTunesDBData, 1);
    itdb->userdata_destroy =
	(ItdbUserDataDestroyFunc)gp_itdb_extra_destroy;
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
    g_return_if_fail (itdb);

    itdbs = g_list_append (itdbs, itdb);
}

/* add playlist to itdb and to display */
void gp_playlist_add (iTunesDB *itdb, Playlist *pl, gint32 pos)
{
    g_return_if_fail (itdb);
    g_return_if_fail (pl);

    itdb_playlist_add (itdb, pl, pos);
    pm_add_playlist (pl, pos);
}

void init_data (GtkWidget *window)
{
    iTunesDB *itdb;
    Playlist *pl;

    g_assert (window);
    g_return_if_fail (itdbs == NULL);

    g_object_set_data (G_OBJECT (window), "itdbs", &itdbs);

    itdb = gp_itdb_new ();
    itdb->usertype = GP_ITDB_TYPE_IPOD;
    gp_add_itdb (itdbs, itdb);
    pl = gp_playlist_new (_("gtkpod"), FALSE);
    pl->type = PL_TYPE_MPL;  /* MPL! */
    gp_playlist_add (itdb, pl, -1);

    itdb = gp_itdb_new ();
    itdb->usertype = GP_ITDB_TYPE_LOCAL;
    gp_itdb_add (itdb);
    pl = gp_playlist_new (_("Local"), FALSE);
    pl->type = PL_TYPE_MPL;  /* MPL! */
    gp_playlist_add (itdb, pl, -1);
}
