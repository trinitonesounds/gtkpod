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

#ifndef CLARITY_PREVIEW_H_
#define CLARITY_PREVIEW_H_

#include "album_model.h"

G_BEGIN_DECLS

GType clarity_preview_get_type (void);

#define CLARITY_TYPE_PREVIEW            (clarity_preview_get_type ())

#define CLARITY_PREVIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLARITY_TYPE_PREVIEW, ClarityPreview))

#define CLARITY_IS_PREVIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLARITY_TYPE_PREVIEW))

#define CLARITY_PREVIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLARITY_TYPE_PREVIEW, ClarityPreviewClass))

#define CLARITY_IS_PREVIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLARITY_TYPE_PREVIEW))

#define CLARITY_PREVIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLARITY_TYPE_PREVIEW, ClarityPreviewClass))

typedef struct _ClarityPreviewPrivate ClarityPreviewPrivate;
typedef struct _ClarityPreview        ClarityPreview;
typedef struct _ClarityPreviewClass   ClarityPreviewClass;

struct _ClarityPreview {
    /*<private>*/
    GtkDialog parent_instance;

    /* structure containing private members */
     /*<private>*/
     ClarityPreviewPrivate *priv;
};

struct _ClarityPreviewClass {
  GtkDialogClass parent_class;

};

GtkWidget *clarity_preview_new(AlbumItem *item);

G_END_DECLS

#endif /* CLARITY_PREVIEW_H_ */
