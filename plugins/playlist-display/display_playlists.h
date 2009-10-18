/* Time-stamp: <2007-03-19 23:11:13 jcs>
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

#ifndef __DISPLAY_PLAYLIST_H__
#define __DISPLAY_PLAYLIST_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gpod/itdb.h>

/* "Column numbers" in playlist model */
typedef enum  {
  PM_COLUMN_ITDB = 0,
  PM_COLUMN_TYPE,
  PM_COLUMN_PLAYLIST,
  PM_COLUMN_PHOTOS,
  PM_NUM_COLUMNS
} PM_column_type;

/* Drag and drop types */
enum {
    DND_GTKPOD_TRACKLIST = 1000,
    DND_GTKPOD_TM_PATHLIST,
    DND_GTKPOD_PLAYLISTLIST,
    DND_TEXT_URI_LIST,
    DND_TEXT_PLAIN,
    DND_IMAGE_JPEG
};

void pm_create_treeview (void);
void pm_set_selected_playlist(Itdb_Playlist *pl);
void pm_remove_all_playlists (gboolean clear_sort);
void pm_add_all_itdbs (void);


#endif /* __DISPLAY_PLAYLIST_H__ */
