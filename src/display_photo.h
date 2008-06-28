/*
|  Copyright (C) 2007 P.G. Richardson <phantom_sf at users.sourceforge.net>
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
*/

#ifndef DISPLAY_PHOTO_H_
#define DISPLAY_PHOTO_H_

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include "display_itdb.h"

/* Gtkpod value for the photo playlists on the ipod.
 * This means a different value from both the mpl and podcast playlist values defined in libgpod.
 * Dont want to change latter as this is not a real playlist defined on the ipod but used for gtkpod
 * purposes only.
 */
#define GP_PL_TYPE_PHOTO 2
/* The values set in itdb_artwork.c for the width and height of ITDB_THUMB_PHOTO_FULL_SCREEN */
#define PHOTO_FULL_SCREEN_WIDTH 220
#define PHOTO_FULL_SCREEN_HEIGHT 176

/* Drag n Drop */
#define DND_GTKPOD_PHOTOIMAGELIST_TYPE "application/gtkpod-photoimagelist"
#define DND_GTKPOD_PHOTOIMAGELIST 1

#define GPHOTO_ALBUM_VIEW 0
#define GPHOTO_ICON_VIEW 1
#define GPHOTO_PREVIEW 2

void gphoto_load_photodb (iTunesDB *itdb);
gboolean gphoto_is_photo_playlist (Playlist *pl);
void gphoto_display_photo_window (iTunesDB *itdb);
void gphoto_change_to_photo_window (gboolean showflag);
gint gphoto_get_selected_photo_count ();
void gphoto_remove_selected_photos_from_album (gboolean show_dialogs);
void gphoto_remove_album_from_database ();
void gphoto_rename_selected_album ();

#endif /*DISPLAY_PHOTO_H_*/
