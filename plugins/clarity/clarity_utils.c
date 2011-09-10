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

#ifndef CLARITY_UTILS_C_
#define CLARITY_UTILS_C_

#include "clarity_utils.h"
#include "libgtkpod/file.h"

/**
 * _get_default_track_image:
 *
 * Retrieve the artwork pixbuf from the default image file.
 *
 * Returns:
 * pixbuf of the default file for tracks with no cover art.
 */
GdkPixbuf *clarity_util_get_default_track_image(gint default_img_size) {
    GdkPixbuf *pixbuf = NULL;
    GdkPixbuf *scaled = NULL;
    GError *error = NULL;

    pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), DEFAULT_COVER_ICON, DEFAULT_IMG_SIZE, 0, &error);
    if (error != NULL) {
        g_warning("Error occurred loading the default file - \nCode: %d\nMessage: %s\n", error->code, error->message);
        g_return_val_if_fail(pixbuf, NULL);
    }

    scaled = gdk_pixbuf_scale_simple(pixbuf, default_img_size, default_img_size, GDK_INTERP_BILINEAR);
    g_object_unref(pixbuf);

    return scaled;
}

GdkPixbuf *clarity_util_get_track_image(Track *track) {
    GdkPixbuf *pixbuf = NULL;
    ExtraTrackData *etd;

    etd = track->userdata;
    g_return_val_if_fail(etd, NULL);

    if (itdb_track_has_thumbnails(track)) {
        pixbuf = itdb_track_get_thumbnail(track, DEFAULT_IMG_SIZE, DEFAULT_IMG_SIZE);
    }

    if (!pixbuf) {
        /* Could not get a viable thumbnail so get default pixbuf */
        pixbuf = clarity_util_get_default_track_image(DEFAULT_IMG_SIZE);
    }

    return pixbuf;
}

void clarity_util_update_coverart(GList *tracks, const gchar *filename) {
    g_return_if_fail(filename);

    if (!tracks)
        return;

    GList *tks = g_list_copy(tracks);

    while (tks) {
        Track *track = tks->data;

        if (gp_track_set_thumbnails(track, filename)) {
            ExtraTrackData *etd = track->userdata;
            etd->tartwork_changed = TRUE;

            gtkpod_track_updated(track);
            data_changed(track->itdb);

            etd->tartwork_changed = FALSE;
        }

        tks = tks->next;
    }
}

#endif /* CLARITY_UTILS_C_ */
