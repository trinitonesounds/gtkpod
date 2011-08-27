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

#include <gtk/gtk.h>
#include "album_model.h"

#ifndef CLARITY_CANVAS_H_
#define CLARITY_CANVAS_H_

G_BEGIN_DECLS

GType clarity_canvas_get_type (void);

#define CLARITY_TYPE_CANVAS            (clarity_canvas_get_type ())

#define CLARITY_CANVAS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLARITY_TYPE_CANVAS, ClarityCanvas))

#define CLARITY_IS_CANVAS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLARITY_TYPE_CANVAS))

#define CLARITY_CANVAS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLARITY_TYPE_CANVAS, ClarityCanvasClass))

#define CLARITY_IS_CANVAS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLARITY_TYPE_CANVAS))

#define CLARITY_CANVAS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLARITY_TYPE_CANVAS, ClarityCanvasClass))

typedef struct _ClarityCanvasPrivate ClarityCanvasPrivate;
typedef struct _ClarityCanvas        ClarityCanvas;
typedef struct _ClarityCanvasClass   ClarityCanvasClass;

struct _ClarityCanvas {
    /*<private>*/
    GtkBox parent_instance;

    /* structure containing private members */
     /*<private>*/
     ClarityCanvasPrivate *priv;
};

struct _ClarityCanvasClass {
  GtkBoxClass parent_class;

};

GtkWidget * clarity_canvas_new();

GdkRGBA *clarity_canvas_get_background_color(ClarityCanvas *self);

void clarity_canvas_set_background(ClarityCanvas *self, const gchar *color_string);

void clarity_canvas_clear(ClarityCanvas *self);

void clarity_canvas_init_album_model(ClarityCanvas *self, AlbumModel *model);

void clarity_canvas_move_left(ClarityCanvas *self, gint increment);

void clarity_canvas_move_right(ClarityCanvas *self, gint increment);

gint clarity_canvas_get_current_index(ClarityCanvas *self);

gboolean clarity_canvas_is_loading(ClarityCanvas *self);

G_END_DECLS

#endif /* CLARITY_CANVAS_H_ */
