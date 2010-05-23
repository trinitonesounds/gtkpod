/*
 |  Copyright (C) 2002 Corey Donohoe <atmos at atmos.org>
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

#include <glib.h>
#include "libgtkpod/itdb.h"

/*------------------------------------------------------------------

 Code to export tracks

 ------------------------------------------------------------------*/

/******************************************************************
 export_tracks_as_files - Export files off of your ipod to an arbitrary
 directory, specified by the file chooser dialog

 @tracks    - GList with data of type (Track*) we want to write
 @filenames - a pointer to a GList where to store the filenames used
 to write the tracks (or NULL)
 @display   - TRUE: display a list of tracks to be exported
 @message   - message to be displayed above the display of tracks

 ******************************************************************/
void export_tracks_as_files(GList *tracks, GList **filenames, gboolean display, gchar *message);

/******************************************************************
 export_playlist_file_init - Create a playlist file to a location
 specified by the file selection dialog.
 @tracks: GList with tracks to be in playlist file.
 ******************************************************************/
void export_tracks_to_playlist_file(GList *tracks);

/*------------------------------------------------------------------

 Code for DND: export when dragging from the iPod to the local
 database.

 ------------------------------------------------------------------*/

/**
 * If tracks are dragged from the iPod to the local database, the
 * tracks need to be copied from the iPod to the harddisk. This
 * function will ask where to copy them to, and add the tracks to the
 * MPL of @itdb_d.
 * A list of tracks that needs to be processed by the drag is
 * returned.
 *
 * If tracks are not dragged from the iPod to the local database, a
 * copy of @tracks is returned.
 *
 * The returned GList must be g_list_free()'ed after it is no longer
 * used.
 */
GList *transfer_track_glist_between_itdbs(iTunesDB *itdb_s, iTunesDB *itdb_d, GList *tracks);

/**
 * same as transfer_track_glist_between_itdbs() but the tracks are
 * represented as pointers in ASCII format. This function parses the
 * tracks in @data and calls transfer_track_glist_between_itdbs()
 */
GList *transfer_track_names_between_itdbs(iTunesDB *itdb_s, iTunesDB *itdb_d, gchar *data);
