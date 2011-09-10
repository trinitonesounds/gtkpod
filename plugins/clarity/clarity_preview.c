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
#include <clutter-gtk/clutter-gtk.h>
#include "libgtkpod/gp_itdb.h"
#include "plugin.h"
#include "clarity_preview.h"
#include "clarity_utils.h"

G_DEFINE_TYPE( ClarityPreview, clarity_preview, GTK_TYPE_DIALOG);

#define CLARITY_PREVIEW_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), CLARITY_TYPE_PREVIEW, ClarityPreviewPrivate))

struct _ClarityPreviewPrivate {

    // clutter embed widget
    GtkWidget *embed;
    ClutterActor *container;

    AlbumItem *item;

};

static void clarity_preview_class_init(ClarityPreviewClass *klass) {
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private(klass, sizeof(ClarityPreviewPrivate));
}

static gboolean _close_dialog_cb(GtkWidget *widget, GdkEvent  *event, gpointer user_data) {
    gtk_widget_hide(widget);
    gtk_widget_destroy(widget);
    return TRUE;
}


static void clarity_preview_init(ClarityPreview *self) {
    ClarityPreviewPrivate *priv;

    priv = self->priv = CLARITY_PREVIEW_GET_PRIVATE (self);

    priv->container = clutter_group_new();
    clutter_actor_set_opacity(CLUTTER_ACTOR(priv->container), 0);

    priv->embed = gtk_clutter_embed_new();
        ClutterActor *stage = gtk_clutter_embed_get_stage(GTK_CLUTTER_EMBED(priv->embed));
    clutter_container_add_actor(CLUTTER_CONTAINER(stage), priv->container);

    gtk_window_set_decorated(GTK_WINDOW(self), FALSE);
    gtk_window_set_resizable (GTK_WINDOW (self), FALSE);
    gtk_window_set_title (GTK_WINDOW (self), "");
    gtk_window_set_skip_taskbar_hint (GTK_WINDOW (self), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW (self), GTK_WINDOW (gtkpod_app));
    gtk_window_set_destroy_with_parent(GTK_WINDOW(self), TRUE);
    gtk_window_set_modal(GTK_WINDOW(self), TRUE);
    gtk_widget_set_size_request(GTK_WIDGET(self), 300, 300);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(self));
    gtk_box_pack_start (GTK_BOX (content_area),
                          priv->embed,
                          TRUE, TRUE, 0);

    gtk_widget_show_all(priv->embed);

    g_signal_connect (self,
                                    "button-press-event",
                                    G_CALLBACK (_close_dialog_cb),
                                    priv->embed);
}

static GdkPixbuf *_get_album_artwork(AlbumItem *item) {
    ExtraTrackData *etd;
    GdkPixbuf *imgbuf = NULL;

    GList *iter = item->tracks;
    while(iter && !imgbuf) {
        Track *track = iter->data;
        etd = track->userdata;
        if (etd && etd->thumb_path_locale) {
            GError *error = NULL;
            imgbuf = gdk_pixbuf_new_from_file(etd->thumb_path_locale, &error);
            if (error != NULL) {
                g_warning("Loading file failed: %s", error->message);
                g_error_free(error);
            }
        }
        iter = iter->next;
    }

    /* Either thumb was null or the attempt at getting a pixbuf failed
     * due to invalid file. For example, some nut (like me) decided to
     * apply an mp3 file to the track as its cover file
     */
    if (!imgbuf) {
        /* Could not get a viable thumbnail so get default pixbuf */
        imgbuf = clarity_util_get_default_track_image(400);
    }

    return imgbuf;
}

GtkWidget *clarity_preview_new(AlbumItem *item) {
    GError *error = NULL;
    ClutterActor *texture;
    GdkPixbuf *image;
    GdkPixbuf *scaled;

    ClarityPreview *preview = g_object_new(CLARITY_TYPE_PREVIEW, NULL);
    ClarityPreviewPrivate *priv = CLARITY_PREVIEW_GET_PRIVATE (preview);

    priv->item = item;
    image = _get_album_artwork(item);

    gint pixheight = gdk_pixbuf_get_height(image);
    gint pixwidth = gdk_pixbuf_get_width(image);
    gint scrheight = gdk_screen_height() - 100;
    gint scrwidth = gdk_screen_width() - 100;

    gdouble ratio = (gdouble) pixwidth / (gdouble) pixheight;
    if (pixwidth > scrwidth) {
        pixwidth = scrwidth;
        pixheight = pixwidth / ratio;
    }

    if (pixheight > scrheight) {
        pixheight = scrheight;
        pixwidth = pixheight * ratio;
    }

    /* Set the preview's minimum size */
    gtk_widget_set_size_request(GTK_WIDGET(preview), pixwidth, pixheight);

    /* Scale the original image */
    scaled = gdk_pixbuf_scale_simple(image, pixwidth, pixheight, GDK_INTERP_BILINEAR);

    texture = gtk_clutter_texture_new();
    gtk_clutter_texture_set_from_pixbuf (GTK_CLUTTER_TEXTURE(texture), scaled, &error);
    if (!error) {
        clutter_container_add_actor(
                CLUTTER_CONTAINER(priv->container),
                CLUTTER_ACTOR(texture));
    }
    else {
        g_warning("Failed to load cover art preview: %s", error->message);
        g_error_free(error);
    }

    g_object_unref(image);

    ClutterTimeline *timeline = clutter_timeline_new(1600);
    clutter_actor_animate_with_timeline(
            CLUTTER_ACTOR(priv->container),
            CLUTTER_EASE_OUT_CUBIC,
            timeline,
            "opacity", 255,
            NULL);
    clutter_timeline_start(timeline);

    return GTK_WIDGET(preview);
}




