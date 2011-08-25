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

#ifndef ALBUM_MODEL_C_
#define ALBUM_MODEL_C_

#include "album_model.h"
#include "libgtkpod/prefs.h"
#include "libgtkpod/misc.h"

G_DEFINE_TYPE( AlbumModel, album_model, G_TYPE_OBJECT);

#define ALBUM_MODEL_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), ALBUM_TYPE_MODEL, AlbumModelPrivate))

struct _AlbumModelPrivate {

    GHashTable *album_hash;
    GList *album_key_list;

    gint loaded;
};

/**
 *
 * clarity_widget_free_album:
 *
 * Destroy an album struct once no longer needed.
 *
 */
static void album_model_free_album(AlbumItem *album) {
    if (album != NULL) {
        if (album->tracks) {
            g_list_free(album->tracks);
        }

        g_free(album->albumname);
        g_free(album->artist);

        if (album->albumart)
            g_object_unref(album->albumart);
    }
}

static void album_model_finalize(GObject *gobject) {
    AlbumModelPrivate *priv = ALBUM_MODEL_GET_PRIVATE(gobject);

    priv->album_key_list = g_list_remove_all(priv->album_key_list, NULL);
    g_hash_table_foreach_remove(priv->album_hash, (GHRFunc) gtk_true, NULL);
    g_hash_table_destroy(priv->album_hash);
    g_list_free(priv->album_key_list);

    /* call the parent class' finalize() method */
    G_OBJECT_CLASS(album_model_parent_class)->finalize(gobject);
}

static void album_model_class_init (AlbumModelClass *klass) {
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->finalize = album_model_finalize;

    g_type_class_add_private (klass, sizeof (AlbumModelPrivate));
}

static void album_model_init (AlbumModel *self) {
    AlbumModelPrivate *priv;

    priv = ALBUM_MODEL_GET_PRIVATE (self);
    priv->album_hash = g_hash_table_new_full(g_str_hash, g_str_equal, (GDestroyNotify) g_free, (GDestroyNotify) album_model_free_album);
    priv->album_key_list = NULL;
    priv->loaded = 0;

}

AlbumModel *album_model_new() {
    return g_object_new(ALBUM_TYPE_MODEL, NULL);
}

void album_model_destroy(AlbumModel *model) {
    album_model_finalize(G_OBJECT(model));
}

void album_model_clear(AlbumModel *model) {
    AlbumModelPrivate *priv;

    priv = ALBUM_MODEL_GET_PRIVATE (model);
    g_hash_table_remove_all (priv->album_hash);

    g_list_free(priv->album_key_list);
    priv->album_key_list = NULL;
}

static gchar *_create_key(Track *track) {
    g_return_val_if_fail(track, "");

    return g_strconcat(track->artist, "_", track->album, NULL);
}

void album_model_add_track(AlbumModel *model, Track *track) {
    AlbumItem *album;
    gchar *album_key;
    AlbumModelPrivate *priv;

    priv = ALBUM_MODEL_GET_PRIVATE (model);
    album_key = _create_key(track);
    /* Check whether an album item has already been created in connection
     * with the track's artist and album
     */
    album = g_hash_table_lookup(priv->album_hash, album_key);
    if (album == NULL) {
        /* Album item not found so create a new one and populate */
        album = g_new0 (AlbumItem, 1);
        album->albumart = NULL;
        album->albumname = g_strdup(track->album);
        album->artist = g_strdup(track->artist);
        album->tracks = NULL;
        album->tracks = g_list_prepend(album->tracks, track);

        /* Insert the new Album Item into the hash */
        g_hash_table_insert(priv->album_hash, album_key, album);
        /* Add the key to the list for sorting and other functions */
        priv->album_key_list = g_list_prepend(priv->album_key_list, album_key);
    }
    else {
        /* Album Item found in the album hash so
         * append the track to the end of the
         * track list */
        g_free(album_key);
        album->tracks = g_list_prepend(album->tracks, track);
    }
}

void album_model_reset_loaded_index(AlbumModel *model) {
    g_return_if_fail(model);

    AlbumModelPrivate *priv;
    priv = ALBUM_MODEL_GET_PRIVATE (model);
    priv->loaded = 0;
}

void album_model_foreach (AlbumModel *model, GFunc func, gpointer user_data) {
    g_return_if_fail(model);
    g_return_if_fail(func);

    AlbumModelPrivate *priv;
    priv = ALBUM_MODEL_GET_PRIVATE (model);
    GList *iter = priv->album_key_list;

    while(iter) {
        gchar *key = iter->data;
        AlbumItem *item = g_hash_table_lookup(priv->album_hash, key);

        (* func) (item, user_data);

        iter = iter->next;
    }
}


gint album_model_get_size(AlbumModel *model) {
    g_return_val_if_fail(model, 0);

    AlbumModelPrivate *priv;
    priv = ALBUM_MODEL_GET_PRIVATE (model);

    return g_list_length(priv->album_key_list);
}

AlbumItem *album_model_get_item(AlbumModel *model, gint index) {
    g_return_val_if_fail(model, NULL);

    AlbumModelPrivate *priv;
    priv = ALBUM_MODEL_GET_PRIVATE (model);

    gchar *key = g_list_nth_data(priv->album_key_list, index);
    return g_hash_table_lookup(priv->album_hash, key);
}

/**
 * compare_album_keys:
 *
 * Comparison function for comparing keys in
 * the key list to sort them into alphabetical order.
 * Could use g_ascii_strcasecmp directly but the NULL
 * strings cause assertion errors.
 *
 * @a: first album key to compare
 * @b: second album key to compare
 *
 */
static gint _compare_album_keys(gchar *a, gchar *b) {
    if (a == NULL)
        return -1;
    if (b == NULL)
        return -1;

    return compare_string(a, b, prefs_get_int("cad_case_sensitive"));
}

gint album_model_get_index(AlbumModel *model, Track *track) {
    g_return_val_if_fail(model, -1);

    AlbumModelPrivate *priv;
    priv = ALBUM_MODEL_GET_PRIVATE (model);

    gchar *trk_key = _create_key(track);
    GList *key = g_list_find_custom(priv->album_key_list, trk_key, (GCompareFunc) _compare_album_keys);
    g_return_val_if_fail (key, -1);

    gint index = g_list_position(priv->album_key_list, key);
    g_free(trk_key);

    return index;
}

#endif /* ALBUM_MODEL_C_ */
