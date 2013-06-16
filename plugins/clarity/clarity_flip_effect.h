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

#ifndef CLARITY_FLIP_EFFECT_H_
#define CLARITY_FLIP_EFFECT_H_

#include <clutter/clutter.h>

GType clarity_flip_effect_get_type (void);

#define CLARITY_TYPE_FLIP_EFFECT        (clarity_flip_effect_get_type ())

#define CLARITY_FLIP_EFFECT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLARITY_TYPE_FLIP_EFFECT, ClarityFlipEffect))

#define CLARITY_IS_FLIP_EFFECT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLARITY_TYPE_FLIP_EFFECT))

#define CLARITY_FLIP_EFFECT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLARITY_TYPE_FLIP_EFFECT, ClarityFlipEffectClass))

#define CLARITY_IS_FLIP_EFFECT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLARITY_TYPE_FLIP_EFFECT))

#define CLARITY_FLIP_EFFECT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLARITY_TYPE_FLIP_EFFECT, ClarityFlipEffectClass))

typedef struct _ClarityFlipEffect        ClarityFlipEffect;
typedef struct _ClarityFlipEffectPrivate ClarityFlipEffectPrivate;
typedef struct _ClarityFlipEffectClass   ClarityFlipEffectClass;

struct _ClarityFlipEffect {
    ClutterEffect parent_instance;

    ClarityFlipEffectPrivate *priv;
 };

struct _ClarityFlipEffectClass {
    ClutterEffectClass parent_class;
};

ClutterEffect *clarity_flip_effect_new (ClutterActor *source, ClutterActor *target);

#endif /* CLARITY_FLIP_EFFECT_H_ */
