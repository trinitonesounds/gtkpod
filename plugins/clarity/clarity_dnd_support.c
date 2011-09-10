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

#ifndef CLARITY_DND_SUPPORT_C_
#define CLARITY_DND_SUPPORT_C_

#include "libgtkpod/gtkpod_app_iface.h"
#include "clarity_dnd_support.h"
#include "fetchcover.h"
#include "clarity_canvas.h"
#include "clarity_utils.h"

/**
 * dnd_clarity_drag_drop:
 *
 * Used by the drag and drop of a jpg. When a drop is
 * made, this determines whether the drop is valid
 * then requests the data from the source widget.
 *
 */
gboolean dnd_clarity_drag_drop(GtkWidget *widget, GdkDragContext *drag_context, gint x, gint y, guint time, gpointer user_data) {
    GdkAtom target;
    target = gtk_drag_dest_find_target(widget, drag_context, NULL);

    if (target != GDK_NONE) {
        gtk_drag_get_data(widget, drag_context, target, time);
        return TRUE;
    }

    return FALSE;
}

/**
 * dnd_clarity_drag_motion:
 *
 * Used by the drag and drop of a jpg. While the jpg is being
 * dragged, this reports to the source widget whether it is an
 * acceptable location to allow a drop.
 *
 */
gboolean dnd_clarity_drag_motion(GtkWidget *widget, GdkDragContext *dc, gint x, gint y, guint time, gpointer user_data) {
    GdkAtom target;
    iTunesDB *itdb;
    ExtraiTunesDBData *eitdb;

    itdb = gp_get_selected_itdb();
    /* no drop is possible if no playlist/repository is selected */
    if (itdb == NULL) {
        gdk_drag_status(dc, 0, time);
        return FALSE;
    }

    eitdb = itdb->userdata;
    g_return_val_if_fail (eitdb, FALSE);
    /* no drop is possible if no repository is loaded */
    if (!eitdb->itdb_imported) {
        gdk_drag_status(dc, 0, time);
        return FALSE;
    }

    target = gtk_drag_dest_find_target(widget, dc, NULL);
    /* no drop possible if no valid target can be found */
    if (target == GDK_NONE) {
        gdk_drag_status(dc, 0, time);
        return FALSE;
    }

    gdk_drag_status(dc, gdk_drag_context_get_suggested_action(dc), time);

    return TRUE;
}

/**
 * dnd_clarity_drag_data_received:
 *
 * Used by the drag and drop of a jpg. When the drop is performed, this
 * acts on the receipt of the data from the source widget and applies
 * the jpg to the track.
 *
 */
void dnd_clarity_drag_data_received(GtkWidget *widget, GdkDragContext *dc, gint x, gint y, GtkSelectionData *data, guint info, guint time, gpointer user_data) {
    g_return_if_fail(CLARITY_IS_CANVAS(widget));
    g_return_if_fail (dc);
    g_return_if_fail (data);
    g_return_if_fail (gtk_selection_data_get_data(data));
    g_return_if_fail (gtk_selection_data_get_length(data) > 0);

    /* mozilla bug 402394 */

#if DEBUG
    printf ("data length = %d\n", gtk_selection_data_get_length(data->length));
    printf ("data->data = %s\n", gtk_selection_data_get_data(data));
#endif

    AlbumItem *item;
    GList *tracks;
    gchar *url = NULL;
    Fetch_Cover *fcover;
    gchar *filename = NULL;
    gboolean image_status = FALSE;
    gchar *image_error = NULL;
    /* For use with DND_IMAGE_JPEG */
    GdkPixbuf *pixbuf;
    GError *error = NULL;

    ClarityCanvas *ccanvas = CLARITY_CANVAS(widget);

    /* Find the display cover item in the cover display */
    item = clarity_canvas_get_current_album_item(ccanvas);
    if (!item) {
        /* looks like there are no covers yet something got dragged into it */
        gtk_drag_finish(dc, FALSE, FALSE, time);
        return;
    }

    tracks = item->tracks;

    switch (info) {
    case DND_IMAGE_JPEG:
#if DEBUG
        printf ("Using DND_IMAGE_JPEG\n");
#endif
        pixbuf = gtk_selection_data_get_pixbuf(data);
        if (pixbuf != NULL) {
            /* initialise the url string with a safe value as not used if already have image */
            url = "local image";
            /* Initialise a fetchcover object */
            fcover = fetchcover_new(url, tracks);
            clarity_canvas_block_change(ccanvas, TRUE);

            /* find the filename with which to save the pixbuf to */
            if (fetchcover_select_filename(fcover)) {
                filename = g_build_filename(fcover->dir, fcover->filename, NULL);
                if (!gdk_pixbuf_save(pixbuf, filename, "jpeg", &error, NULL)) {
                    /* Save failed for some reason */
                    if (error->message)
                        fcover->err_msg = g_strdup(error->message);
                    else
                        fcover->err_msg = "Saving image to file failed. No internal error message was returned.";

                    g_error_free(error);
                }
                else {
                    /* Image successfully saved */
                    image_status = TRUE;
                }
            }
            /* record any errors and free the fetchcover */
            if (fcover->err_msg != NULL)
                image_error = g_strdup(fcover->err_msg);

            free_fetchcover(fcover);
            g_object_unref(pixbuf);
            clarity_canvas_block_change(ccanvas, FALSE);
        }
        else {
            /* despite the data being of type image/jpeg, the pixbuf is NULL */
            image_error = "jpeg data flavour was used but the data did not contain a GdkPixbuf object";
        }
        break;
    case DND_TEXT_PLAIN:
#if DEBUG
        printf ("Defaulting to using DND_TEXT_PLAIN\n");
#endif

#ifdef HAVE_CURL
        /* initialise the url string with the data from the dnd */
        url = g_strdup ((gchar *) gtk_selection_data_get_data(data));
        /* Initialise a fetchcover object */
        fcover = fetchcover_new (url, tracks);
        clarity_canvas_block_change(ccanvas, TRUE);

        if (fetchcover_net_retrieve_image (fcover))
        {
#if DEBUG
            printf ("Successfully retrieved\n");
            printf ("Url of fetch cover: %s\n", fcover->url->str);
            printf ("filename of fetch cover: %s\n", fcover->filename);
#endif

            filename = g_build_filename(fcover->dir, fcover->filename, NULL);
            image_status = TRUE;
        }

        /* record any errors and free the fetchcover */
        if (fcover->err_msg != NULL)
        image_error = g_strdup(fcover->err_msg);

        free_fetchcover (fcover);
        clarity_canvas_block_change(ccanvas, FALSE);
#else
        image_error = g_strdup(_("Item had to be downloaded but gtkpod was not compiled with curl."));
        image_status = FALSE;
#endif
    }

    if (!image_status || filename == NULL) {
        gtkpod_warning(_("Error occurred dropping an image onto the clarity display: %s\n"), image_error);

        if (image_error)
            g_free(image_error);
        if (filename)
            g_free(filename);

        gtk_drag_finish(dc, FALSE, FALSE, time);
        return;
    }

    clarity_util_update_coverart(tracks, filename);

    if (image_error)
        g_free(image_error);

    g_free(filename);

    gtkpod_statusbar_message(_("Successfully set new cover art for selected tracks"));
    gtk_drag_finish(dc, FALSE, FALSE, time);
    return;
}

#endif /* CLARITY_DND_SUPPORT_C_ */
