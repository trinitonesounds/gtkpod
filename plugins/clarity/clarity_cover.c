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

#include "clarity_utils.h"
#include "clarity_cover.h"
#include "clarity_flip_effect.h"

#define V_PADDING 4

/**
 * SECTION:clarity-cover
 * @short_description: Cover widget
 *
 * An artwork cover widget with support for a text label and background color.
 */

G_DEFINE_TYPE( ClarityCover, clarity_cover, CLUTTER_TYPE_ACTOR);

/* macro for accessing the object's private structure */
#define CLARITY_COVER_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), CLARITY_TYPE_COVER, ClarityCoverPrivate))

/* private structure - should only be accessed through the public API;
 * this is used to store member variables whose properties
 * need to be accessible from the implementation; for example, if we
 * intend to create wrapper functions which modify properties on the
 * actors composing an object, we should keep a reference to the actors
 * here
 *
 * this is also the place where other state variables go:
 * for example, you might record the current state of the button
 * (toggled on or off) or a background image
 */
struct _ClarityCoverPrivate {

    ClutterActor *texture;
    ClutterContent *artwork;
    int height;
    int width;
    ClutterActor *reflection;

    gchar *title;
    gchar *artist;
};

/* enumerates property identifiers for this class;
 * note that property identifiers should be non-zero integers,
 * so we add an unused PROP_0 to occupy the 0 position in the enum
 */
enum {
    PROP_0, PROP_TITLE, PROP_ARTIST
};

/*
 * The finalize method finishes releasing the remaining
 * resources just before the object itself will be freed from memory, and
 * therefore it will only be called once.
 */
static void clarity_cover_finalize(GObject *gobject) {
    ClarityCoverPrivate *priv = CLARITY_COVER(gobject)->priv;

    g_free(priv->title);
    g_free(priv->artist);

    /* call the parent class' finalize() method */
    G_OBJECT_CLASS(clarity_cover_parent_class)->finalize(gobject);
}

/* ClutterActor implementation
 *
 * we only implement destroy(), get_preferred_height(), get_preferred_width(),
 * allocate(), and paint(), as this is the minimum we can get away with
 */

/* composite actors should implement destroy(), and inside their
 * implementation destroy any actors they are composed from;
 * in this case, we just destroy the child ClutterBox
 */
void clarity_cover_destroy(ClutterActor *self) {
    ClarityCoverPrivate *priv = CLARITY_COVER_GET_PRIVATE(self);

    if (priv) {
        if (CLUTTER_IS_ACTOR(priv->texture)) {
            clutter_actor_destroy(priv->texture);
            priv->texture = NULL;
        }

        if (CLUTTER_IS_IMAGE(priv->artwork)) {
            g_object_unref(priv->artwork);
            priv->artwork = NULL;
        }

        if (CLUTTER_IS_ACTOR(priv->reflection)) {
            clutter_actor_destroy(priv->reflection);
            priv->reflection = NULL;
        }
    }

    /* chain up to destroy() on the parent ClutterActorClass;
     * note that we check the parent class has a destroy() implementation
     * before calling it
     */
    if (CLUTTER_ACTOR_CLASS(clarity_cover_parent_class)->destroy)
        CLUTTER_ACTOR_CLASS(clarity_cover_parent_class)->destroy(self);
}

/* GObject class and instance initialization functions; note that
 * these have been placed after the Clutter implementation, as
 * they refer to the static function implementations above
 */

/* class init: attach functions to superclasses, define properties
 * and signals
 */
static void clarity_cover_class_init(ClarityCoverClass *klass) {
    ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS(klass);
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->finalize = clarity_cover_finalize;

    actor_class->destroy = clarity_cover_destroy;

    g_type_class_add_private(klass, sizeof(ClarityCoverPrivate));
}

/* object init: create a private structure and pack
 * composed ClutterActors into it
 */
static void clarity_cover_init(ClarityCover *self) {
    ClarityCoverPrivate *priv;

    priv = self->priv = CLARITY_COVER_GET_PRIVATE (self);

    clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);

    priv->title = NULL;
    priv->artist = NULL;

    priv->texture = NULL;
    priv->artwork = NULL;
    priv->width = 0;
    priv->height = 0;
    priv->reflection = NULL;
}

//static void _clone_paint_cb (ClutterActor *actor) {
//    ClutterActor *source;
//    ClutterActorBox box;
//    CoglHandle material;
//    gfloat width, height;
//    guint8 opacity;
//    CoglColor color_1, color_2;
//    CoglTextureVertex vertices[4];
//
//    /* if we don't have a source actor, don't paint */
//    source = clutter_clone_get_source (CLUTTER_CLONE (actor));
//    if (source == NULL)
//        goto out;
//
//    /* if the source does not have any content, don't paint */
//    ClutterContent *content = clutter_actor_get_content(actor);
//    clutter_image_
//
//    material = clutter_texture_get_cogl_material (CLUTTER_TEXTURE (source));
//    if (material == NULL)
//        goto out;
//
//    /* get the size of the reflection */
//    clutter_actor_get_allocation_box (actor, &box);
//    clutter_actor_box_get_size (&box, &width, &height);
//
//    /* get the composite opacity of the actor */
//    opacity = clutter_actor_get_paint_opacity (actor);
//
//    /* figure out the two colors for the reflection: the first is
//     * full color and the second is the same, but at 0 opacity
//     */
//    cogl_color_init_from_4f (&color_1, 1.0, 1.0, 1.0, opacity / 255.0);
//    cogl_color_premultiply (&color_1);
//    cogl_color_init_from_4f (&color_2, 1.0, 1.0, 1.0, 0.0);
//    cogl_color_premultiply (&color_2);
//
//    /* now describe the four vertices of the quad; since it has
//     * to be a reflection, we need to invert it as well
//    */
//    vertices[0].x = 0; vertices[0].y = 0; vertices[0].z = 0;
//    vertices[0].tx = 0.0; vertices[0].ty = 1.0;
//    vertices[0].color = color_1;
//
//    vertices[1].x = width; vertices[1].y = 0; vertices[1].z = 0;
//    vertices[1].tx = 1.0; vertices[1].ty = 1.0;
//    vertices[1].color = color_1;
//
//    vertices[2].x = width; vertices[2].y = height; vertices[2].z = 0;
//    vertices[2].tx = 1.0; vertices[2].ty = 0.0;
//    vertices[2].color = color_2;
//
//    vertices[3].x = 0; vertices[3].y = height; vertices[3].z = 0;
//    vertices[3].tx = 0.0; vertices[3].ty = 0.0;
//    vertices[3].color = color_2;
//
//    /* paint the same texture but with a different geometry */
//    cogl_set_source (material);
//    cogl_polygon (vertices, 4, TRUE);
//
//    out:
//      /* prevent the default clone handler from running */
//      g_signal_stop_emission_by_name (actor, "paint");
//}

/**
 * Create the reflection for the given cover
 */
static void _create_reflection(ClarityCover *self, GdkPixbuf *albumart) {
    GError *error = NULL;
    gint y_offset;
    ClutterContent *reflectimg;

    g_return_if_fail(CLARITY_IS_COVER(self));

    ClarityCoverPrivate *priv = CLARITY_COVER_GET_PRIVATE (self);
    g_return_if_fail(priv);

    if (! priv->reflection) {
        priv->reflection = clutter_actor_new();
        y_offset = clutter_actor_get_height (priv->texture) + V_PADDING;
        clutter_actor_add_constraint (priv->reflection, clutter_bind_constraint_new (priv->texture, CLUTTER_BIND_X, 0.0));
        clutter_actor_add_constraint (priv->reflection, clutter_bind_constraint_new (priv->texture, CLUTTER_BIND_Y, y_offset));
        clutter_actor_add_constraint (priv->reflection, clutter_bind_constraint_new (priv->texture, CLUTTER_BIND_WIDTH, 0.0));
        clutter_actor_add_constraint (priv->reflection, clutter_bind_constraint_new (priv->texture, CLUTTER_BIND_HEIGHT, 0.0));

        ClutterEffect * flip_effect = clarity_flip_effect_new(priv->texture, priv->reflection);
        clutter_actor_add_effect(priv->reflection, flip_effect);
        clutter_actor_add_child(CLUTTER_ACTOR(self), priv->reflection);
    }

    /* flip image vertically to create reflection */
    GdkPixbuf *reflect = gdk_pixbuf_flip(albumart, FALSE);
    GdkPixbuf *scaled = gdk_pixbuf_scale_simple(reflect, priv->width, priv->height, GDK_INTERP_BILINEAR);

    // Set cover reflection
    reflectimg = clutter_image_new();
    clutter_image_set_data( CLUTTER_IMAGE (reflectimg),
                                                        gdk_pixbuf_get_pixels (scaled),
                                                        gdk_pixbuf_get_has_alpha (scaled)
                                                                        ? COGL_PIXEL_FORMAT_RGBA_8888
                                                                        : COGL_PIXEL_FORMAT_RGB_888,
                                                        priv->width,
                                                        priv->height,
                                                        gdk_pixbuf_get_rowstride (scaled),
                                                        &error);

    g_object_unref(reflect);
    g_object_unref(scaled);

    if (error) {
        g_warning("%s", error->message);
        g_error_free(error);
        return;
    }

    // Set the width and height
    clutter_actor_set_width(priv->reflection, priv->width);
    clutter_actor_set_height(priv->reflection, priv->height);
    clutter_actor_set_content (priv->reflection, reflectimg);
}


void clarity_cover_set_album_item (ClarityCover *self, AlbumItem *item) {
    g_return_if_fail(CLARITY_IS_COVER(self));

    ClarityCoverPrivate *priv = CLARITY_COVER_GET_PRIVATE (self);
    g_return_if_fail(priv);

    GError *error = NULL;

    if (!priv->texture) {
        priv->texture = clutter_actor_new();
        clutter_actor_add_child(CLUTTER_ACTOR(self), priv->texture);
    }

    if (!priv->artwork) {
            priv->artwork = clutter_image_new();
        }

    priv->width = gdk_pixbuf_get_width (item->albumart);
    priv->height = gdk_pixbuf_get_height (item->albumart);

    if( priv->height > DEFAULT_IMG_SIZE) {
        priv->width = priv->width * DEFAULT_IMG_SIZE / priv->height;
        priv->height = DEFAULT_IMG_SIZE;
    }

    // Set cover artwork
    clutter_image_set_data( CLUTTER_IMAGE (priv->artwork),
                                              gdk_pixbuf_get_pixels (item->albumart),
                                              gdk_pixbuf_get_has_alpha (item->albumart)
                                                      ? COGL_PIXEL_FORMAT_RGBA_8888
                                                      : COGL_PIXEL_FORMAT_RGB_888,
                                              priv->width,
                                              priv->height,
                                              gdk_pixbuf_get_rowstride (item->albumart),
                                              &error);
    if (error) {
        g_warning("%s", error->message);
        g_error_free(error);
        return;
    }

    // Set the width and height
    clutter_actor_set_width(priv->texture, priv->width);
    clutter_actor_set_height(priv->texture, priv->height);
    clutter_actor_set_content (priv->texture, priv->artwork);

    // Create the reflection
    _create_reflection(self, item->albumart);


    // Add title / artist data
    if (priv->title)
        g_free(priv->title);

    priv->title = g_strdup(item->albumname);

    if (priv->artist)
            g_free(priv->artist);

    priv->artist = g_strdup(item->artist);
}

gchar *clarity_cover_get_title(ClarityCover *self) {
    ClarityCoverPrivate *priv = self->priv;
    return g_strdup(priv->title);
}

gchar *clarity_cover_get_artist(ClarityCover *self) {
    ClarityCoverPrivate *priv = self->priv;
    return g_strdup(priv->artist);
}

gfloat clarity_cover_get_artwork_height(ClarityCover *self) {
    ClarityCoverPrivate *priv = self->priv;
    return priv->height;
}

gfloat clarity_cover_get_artwork_width(ClarityCover *self) {
    ClarityCoverPrivate *priv = self->priv;
    return priv->width;
}

/**
 * clarity_cover_new:
 *
 * Creates a new #ClarityCover instance
 *
 * Returns: a new #ClarityCover
 */
ClarityCover *clarity_cover_new(void) {
    return g_object_new(CLARITY_TYPE_COVER, NULL);
}

