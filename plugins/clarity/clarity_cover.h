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

/* inclusion guard */
#ifndef CLARITY_COVER_H_
#define CLARITY_COVER_H_

/* include any dependencies */
#include <gtk/gtk.h>
#include <clutter-gtk/clutter-gtk.h>
#include "libgtkpod/itdb.h"
#include "album_model.h"

/* GObject implementation */

/* declare this function signature to remove compilation errors with -Wall;
 * the clarity_cover_get_type() function is actually added via the
 * G_DEFINE_TYPE macro in the .c file
 */
GType clarity_cover_get_type (void);

/* GObject type macros */
/* returns the class type identifier (GType) for ClarityCover */
#define CLARITY_TYPE_COVER            (clarity_cover_get_type ())

/* cast obj to a ClarityCover object structure*/
#define CLARITY_COVER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLARITY_TYPE_COVER, ClarityCover))

/* check whether obj is a ClarityCover */
#define CLARITY_IS_COVER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLARITY_TYPE_COVER))

/* cast klass to ClarityCoverClass class structure */
#define CLARITY_COVER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLARITY_TYPE_COVER, ClarityCoverClass))

/* check whether klass is a member of the ClarityCoverClass */
#define CLARITY_IS_COVER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLARITY_TYPE_COVER))

/* get the ClarityCoverClass structure for a ClarityCover obj */
#define CLARITY_COVER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLARITY_TYPE_COVER, ClarityCoverClass))

/*
 * Private instance fields; see
 * http://www.gotw.ca/gotw/024.htm for the rationale
 */
typedef struct _ClarityCoverPrivate ClarityCoverPrivate;
typedef struct _ClarityCover        ClarityCover;
typedef struct _ClarityCoverClass   ClarityCoverClass;

/* object structure */
struct _ClarityCover
{
  /*<private>*/
  ClutterGroup parent_instance;

  /* structure containing private members */
  /*<private>*/
  ClarityCoverPrivate *priv;
};

/* class structure */
struct _ClarityCoverClass
{
  /*<private>*/
  ClutterGroupClass parent_class;
};

/* public API */

void clarity_cover_set_album_item (ClarityCover *self, AlbumItem *item);

gint clarity_cover_get_index(ClarityCover *self);

void clarity_cover_set_index(ClarityCover *self, gint index);

void clarity_cover_clear_rotation_behaviour(ClarityCover *self);

void clarity_cover_set_rotation_behaviour(ClarityCover *self, ClutterAlpha *alpha, int final_angle, ClutterRotateDirection direction);

gchar *clarity_cover_get_title(ClarityCover *self);

gchar *clarity_cover_get_artist(ClarityCover *self);

/* constructor - note this returns a ClutterActor instance */
ClarityCover *clarity_cover_new (void);

#endif /* CLARITY_COVER_H_ */
