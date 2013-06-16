/*
 |  Copyright (C) 2002-2013 Jorg Schuler <jcsjcs at users sourceforge net>
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
 */

/**
 * A clutter effect that flips an image upside down and fades it from fully opaque
 * to completely transparent.
 */
#include <math.h>
#include "clarity_flip_effect.h"

G_DEFINE_TYPE(ClarityFlipEffect, clarity_flip_effect, CLUTTER_TYPE_EFFECT);

#define CLARITY_FLIP_EFFECT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), CLARITY_TYPE_FLIP_EFFECT, ClarityFlipEffectPrivate))

struct _ClarityFlipEffectPrivate {
    ClutterActor *source;
    ClutterActor *target;

    CoglMaterial *highlight;
};

static void clarity_flip_paint(ClutterEffect *effect, ClutterEffectPaintFlags  flags) {
    gfloat width, height;
    guint8 opacity;
    CoglColor color_1, color_2;
    CoglTextureVertex vertices[4];

    ClarityFlipEffect *self = CLARITY_FLIP_EFFECT(effect);
    ClarityFlipEffectPrivate *priv = CLARITY_FLIP_EFFECT_GET_PRIVATE(self);

    /* if we don't have a source actor, don't paint */
    if (priv->source == NULL)
        return;

    /* if we don't have a target actor, don't paint */
    if (priv->target == NULL)
        return;

    /* get the size of the reflection */
    clutter_actor_get_size (priv->source, &width, &height);

    /* get the composite opacity of the actor */
    opacity = clutter_actor_get_paint_opacity (priv->source);

    /* figure out the two colors for the reflection: the first is
     * full color and the second is the same, but at 0 opacity
     */
    ClutterColor *bgcolor;
    bgcolor = g_malloc(sizeof(ClutterColor));
    ClutterActor *stage = clutter_actor_get_stage(priv->source);
    clutter_actor_get_background_color(stage, bgcolor);

    float r = ((float) bgcolor->red) / 255;
    float g = ((float) bgcolor->green) / 255;
    float b = ((float) bgcolor->blue) / 255;

    cogl_color_init_from_4f (&color_1, r, g, b, opacity / 255.0);
    cogl_color_premultiply (&color_1);
    cogl_color_init_from_4f (&color_2, r, g, b, 0.0);
    cogl_color_premultiply (&color_2);

    g_free(bgcolor);

    /* now describe the four vertices of the quad */
    vertices[0].x = 0;
    vertices[0].y = 0;
    vertices[0].z = 0;
    vertices[0].tx = 0.0;
    vertices[0].ty = 0.0;
    vertices[0].color = color_2;

    vertices[1].x = width;
    vertices[1].y = 0;
    vertices[1].z = 0;
    vertices[1].tx = 1.0;
    vertices[1].ty = 0.0;
    vertices[1].color = color_2;

    vertices[2].x = width;
    vertices[2].y = height;
    vertices[2].z = 0;
    vertices[2].tx = 1.0;
    vertices[2].ty = 1.0;
    vertices[2].color = color_1;

    vertices[3].x = 0;
    vertices[3].y = height;
    vertices[3].z = 0;
    vertices[3].tx = 0.0;
    vertices[3].ty = 1.0;
    vertices[3].color = color_1;

    /* Continue to the rest of the paint sequence */
    clutter_actor_continue_paint (CLUTTER_ACTOR(priv->target));

    priv->highlight = cogl_material_new ();
    cogl_set_source (priv->highlight);
    cogl_polygon (vertices, 4, TRUE);
}

/* GObject class and instance init */
static void clarity_flip_effect_class_init(ClarityFlipEffectClass *klass) {
    ClutterEffectClass *effect_class = CLUTTER_EFFECT_CLASS(klass);
    effect_class->paint = clarity_flip_paint;

    g_type_class_add_private (klass, sizeof (ClarityFlipEffectPrivate));
}

static void clarity_flip_effect_init(ClarityFlipEffect *self) {
    self->priv = CLARITY_FLIP_EFFECT_GET_PRIVATE(self);
    self->priv->source = NULL;
    self->priv->target = NULL;
    self->priv->highlight = NULL;
}

ClutterEffect *clarity_flip_effect_new(ClutterActor *source, ClutterActor *target) {
    ClutterEffect *effect = g_object_new(CLARITY_TYPE_FLIP_EFFECT, NULL);
    ClarityFlipEffectPrivate *priv = CLARITY_FLIP_EFFECT_GET_PRIVATE(effect);

    priv->source = source;
    priv->target = target;

    return effect;
}
