/*
 |  Copyright (C) 2002-2011 Jorg Schuler <jcsjcs at users sourceforge net>
 |                                             Paul Richardson <phantom_sf at users.sourceforge.net>
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

#ifndef CLARITY_UTILS_H_
#define CLARITY_UTILS_H_

#include <gtk/gtk.h>
#include "libgtkpod/gp_itdb.h"

#define DEFAULT_COVER_ICON "clarity-default-cover"
#define DEFAULT_COVER_ICON_STOCK_ID "clarity-default-cover-icon"
#define DEFAULT_IMG_SIZE 140

/**
 * _get_default_track_image:
 *
 * Retrieve the artwork pixbuf from the default image file.
 *
 * Returns:
 * pixbuf of the default file for tracks with no cover art.
 */
GdkPixbuf *get_default_track_image(gint default_img_size);

/**
 * _get_track_image:
 *
 * Retrieve the artwork pixbuf from the given track.
 *
 * Returns:
 * pixbuf of the artwork of the track.
 */
GdkPixbuf *_get_track_image(Track *track);

#endif /* CLARITY_UTILS_H_ */
