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

#ifndef ALBUM_MODEL_H_
#define ALBUM_MODEL_H_

#include <gtk/gtk.h>
#include "libgtkpod/itdb.h"


G_BEGIN_DECLS

typedef struct {
    GList *tracks;
    gchar *albumname;
    gchar *artist;
    GdkPixbuf *albumart;

    gpointer data;

} AlbumItem;

typedef void (*AMFunc) (gpointer value, gint index, gpointer user_data);

GType album_model_get_type (void);

#define ALBUM_TYPE_MODEL            (album_model_get_type ())

#define ALBUM_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ALBUM_TYPE_MODEL, AlbumModel))

#define ALBUM_IS_MODEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ALBUM_TYPE_MODEL))

#define ALBUM_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ALBUM_TYPE_MODEL, AlbumModelClass))

#define ALBUM_IS_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ALBUM_TYPE_MODEL))

#define ALBUM_MODEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ALBUM_TYPE_MODEL, AlbumModelClass))

typedef struct _AlbumModelPrivate AlbumModelPrivate;
typedef struct _AlbumModel        AlbumModel;
typedef struct _AlbumModelClass   AlbumModelClass;

struct _AlbumModel {
    /*<private>*/
    GObject parent;

    /* structure containing private members */
     /*<private>*/
     AlbumModelPrivate *priv;
};

struct _AlbumModelClass {
  GObjectClass parent_class;

};

AlbumModel *album_model_new();

void album_model_destroy(AlbumModel *model);

void album_model_clear(AlbumModel *model);

void album_model_foreach(AlbumModel *model, AMFunc func, gpointer user_data);

void album_model_resort(AlbumModel *model, GList *tracks);

void album_model_add_tracks(AlbumModel *model, GList *tracks);

/**
 * Add a single track to the album model.
 *
 * @return gboolean: whether a new album model was
 *                                  created.
 */
gboolean album_model_add_track(AlbumModel *model, Track *track);

AlbumItem *album_model_get_item_with_index(AlbumModel *model, gint index);

AlbumItem *album_model_get_item_with_track(AlbumModel *model, Track *track);

gint album_model_get_index_with_album_item(AlbumModel *model, AlbumItem *item);

gint album_model_get_index_with_track(AlbumModel *model, Track *track);

gint album_model_get_size(AlbumModel *model);

gint compare_tracks(Track *a, Track *b);


#endif /* ALBUM_MODEL_H_ */
