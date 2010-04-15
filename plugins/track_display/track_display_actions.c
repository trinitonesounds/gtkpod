/*
 |  Copyright (C) 2002-2010 Jorg Schuler <jcsjcs at users sourceforge net>
 |                                          Paul Richardson <phantom_sf at users.sourceforge.net>
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

#include "track_display_actions.h"
#include "display_tracks.h"
#include "libgtkpod/misc.h"
#include "libgtkpod/misc_track.h"
#include "libgtkpod/file.h"

static void delete_selected_tracks(DeleteAction deleteaction) {
    GList *tracks = gtkpod_get_selected_tracks();

    if (tracks) {
        delete_track_head(deleteaction);
        g_list_free(tracks);
    }
    else {
        message_sb_no_tracks_selected();
    }
}

void on_delete_selected_tracks_from_playlist(GtkAction *action, TrackDisplayPlugin* plugin) {
    delete_selected_tracks(DELETE_ACTION_PLAYLIST);
}

void on_delete_selected_tracks_from_database(GtkAction *action, TrackDisplayPlugin* plugin) {
    delete_selected_tracks(DELETE_ACTION_DATABASE);
}

void on_delete_selected_tracks_from_harddisk(GtkAction *action, TrackDisplayPlugin* plugin) {
    delete_selected_tracks(DELETE_ACTION_LOCAL);
}

void on_delete_selected_tracks_from_ipod(GtkAction *action, TrackDisplayPlugin* plugin) {
    delete_selected_tracks(DELETE_ACTION_IPOD);
}

void on_delete_selected_tracks_from_device(GtkAction *action, TrackDisplayPlugin* plugin) {
    iTunesDB *itdb = gtkpod_get_current_itdb();
    if (!itdb)
        return;

    if (itdb->usertype & GP_ITDB_TYPE_IPOD) {
        on_delete_selected_tracks_from_ipod(action, plugin);
    }
    else if (itdb->usertype & GP_ITDB_TYPE_LOCAL) {
        on_delete_selected_tracks_from_harddisk(action, plugin);
    }
}

void on_update_selected_tracks (GtkAction *action, TrackDisplayPlugin* plugin) {
    GList *tracks = gtkpod_get_selected_tracks();

    if (tracks) {
      update_tracks(tracks);
    }
}

