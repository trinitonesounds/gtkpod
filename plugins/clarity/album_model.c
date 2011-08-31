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
#include "libgtkpod/gp_private.h"

G_DEFINE_TYPE( AlbumModel, album_model, G_TYPE_OBJECT);

#define ALBUM_MODEL_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), ALBUM_TYPE_MODEL, AlbumModelPrivate))

struct _AlbumModelPrivate {

    GHashTable *album_hash;
    GList *album_key_list;
};

static gchar *_create_key(gchar *artist, gchar *album) {
    return g_strconcat(artist, "_", album, NULL);
}

static gchar *_create_key_from_track(Track *track) {
    g_return_val_if_fail(track, "");

    return _create_key(track->artist, track->album);
}

static void _add_track_to_album_item(AlbumItem *item, Track *track) {
    item->tracks = g_list_prepend(item->tracks, track);
}

static AlbumItem *_create_album_item(Track *track) {
    AlbumItem *item = NULL;

    /* Album item not found so create a new one and populate */
    item = g_new0 (AlbumItem, 1);
    item->albumart = NULL;
    item->albumname = g_strdup(track->album);
    item->artist = g_strdup(track->artist);
    item->tracks = NULL;
    _add_track_to_album_item(item, track);

    return item;
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
static gint _compare_album_item_keys(gchar *a, gchar *b) {
    if (a == NULL)
        return -1;
    if (b == NULL)
        return -1;

    return compare_string(a, b, prefs_get_int("clarity_case_sensitive"));
}

void _index_album_item(AlbumModelPrivate *priv, gchar *album_key, AlbumItem *item) {
    enum GtkPodSortTypes value = prefs_get_int("clarity_sort");

    g_hash_table_insert(priv->album_hash, album_key, item);

    switch(value) {
    case SORT_ASCENDING:
        priv->album_key_list = g_list_insert_sorted(priv->album_key_list, album_key, (GCompareFunc) _compare_album_item_keys);
        break;
    case SORT_DESCENDING:
        /* Already in descending order so reverse into ascending order */
        priv->album_key_list = g_list_reverse(priv->album_key_list);
        /* Insert the track */
        priv->album_key_list = g_list_insert_sorted(priv->album_key_list, album_key, (GCompareFunc) _compare_album_item_keys);
        /* Reverse again */
        priv->album_key_list = g_list_reverse(priv->album_key_list);
        break;
    default:
        /* NO SORT */

        // Quicker to reverse, prepend then reverse back
        priv->album_key_list = g_list_reverse(priv->album_key_list);
        priv->album_key_list = g_list_prepend(priv->album_key_list, album_key);
        priv->album_key_list = g_list_reverse(priv->album_key_list);
        break;
    }
}

/**
 * Return true if a new album item was created. Otherwise false.
 */
static gboolean _insert_track(AlbumModelPrivate *priv, Track *track) {
    AlbumItem *item;
    gchar *album_key;

    album_key = _create_key_from_track(track);
    /* Check whether an album item has already been created in connection
     * with the track's artist and album
     */
    item = g_hash_table_lookup(priv->album_hash, album_key);
    if (!item) {
        // Create new album item
        item = _create_album_item(track);
        _index_album_item(priv, album_key, item);
        return TRUE;
    }

    /* Album Item found in the album hash so prepend the
     * track to the start of the track list */
    g_free(album_key);
    _add_track_to_album_item(item, track);
    return FALSE;
}

/**
 *
 * clarity_widget_free_album:
 *
 * Destroy an album struct once no longer needed.
 *
 */
static void album_model_free_album_item(AlbumItem *item) {
    if (item) {
        if (item->tracks) {
            g_list_free(item->tracks);
        }
        item->tracks = NULL;

        g_free(item->albumname);
        g_free(item->artist);

        if (item->albumart)
            g_object_unref(item->albumart);

        item->data = NULL;
    }
}

static void album_model_finalize(GObject *gobject) {
    AlbumModelPrivate *priv = ALBUM_MODEL_GET_PRIVATE(gobject);

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
    priv->album_hash = g_hash_table_new_full(g_str_hash, g_str_equal, (GDestroyNotify) g_free, (GDestroyNotify) album_model_free_album_item);
    priv->album_key_list = NULL;

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

    if (priv->album_key_list) {
        g_list_free(priv->album_key_list);
        priv->album_key_list = NULL;
    }
}

void album_model_resort(AlbumModel *model, GList *tracks) {
    AlbumModelPrivate *priv = ALBUM_MODEL_GET_PRIVATE (model);
    enum GtkPodSortTypes value = prefs_get_int("clarity_sort");

    switch (value) {
    case SORT_ASCENDING:
        priv->album_key_list = g_list_sort(priv->album_key_list, (GCompareFunc) _compare_album_item_keys);
        break;
    case SORT_DESCENDING:
        priv->album_key_list = g_list_sort(priv->album_key_list, (GCompareFunc) _compare_album_item_keys);
        priv->album_key_list = g_list_reverse(priv->album_key_list);
        break;
    default:
        // No sorting needs to re-initialise the model from scratch
        album_model_clear(model);
        album_model_add_tracks(model, tracks);
        break;
    }
}

void album_model_add_tracks(AlbumModel *model, GList *tracks) {
    g_return_if_fail(model);

    AlbumModelPrivate *priv = ALBUM_MODEL_GET_PRIVATE(model);
    GList *trks = tracks;
    while(trks) {
        Track *track = trks->data;
        _insert_track(priv, track);
        trks = trks->next;
    }
}

gboolean album_model_add_track(AlbumModel *model, Track *track) {
    g_return_val_if_fail(model, -1);
    g_return_val_if_fail(track, -1);

    AlbumModelPrivate *priv = ALBUM_MODEL_GET_PRIVATE(model);
    return _insert_track(priv, track);
}

void album_model_foreach (AlbumModel *model, AMFunc func, gpointer user_data) {
    g_return_if_fail(model);
    g_return_if_fail(func);

    AlbumModelPrivate *priv = ALBUM_MODEL_GET_PRIVATE (model);
    GList *iter = priv->album_key_list;

    gint i = 0;
    while(iter) {
        gchar *key = iter->data;
        AlbumItem *item = g_hash_table_lookup(priv->album_hash, key);

        (* func) (item, i, user_data);

        iter = iter->next;
        i++;
    }
}

AlbumItem *album_model_get_item_with_index(AlbumModel *model, gint index) {
    g_return_val_if_fail(model, NULL);

    AlbumModelPrivate *priv = ALBUM_MODEL_GET_PRIVATE (model);

    gchar *key = g_list_nth_data(priv->album_key_list, index);
    return g_hash_table_lookup(priv->album_hash, key);
}

AlbumItem *album_model_get_item_with_track(AlbumModel *model, Track *track) {
    g_return_val_if_fail(model, NULL);

    AlbumModelPrivate *priv = ALBUM_MODEL_GET_PRIVATE (model);

    gchar *album_key = _create_key_from_track(track);
    return g_hash_table_lookup(priv->album_hash, album_key);
}

static gint _get_index(AlbumModelPrivate *priv, gchar *trk_key) {
    GList *key_list = priv->album_key_list;

    GList *key = g_list_find_custom(key_list, trk_key, (GCompareFunc) _compare_album_item_keys);
    g_return_val_if_fail (key, -1);

    gint index = g_list_position(key_list, key);

    return index;
}

gint album_model_get_index_with_album_item(AlbumModel *model, AlbumItem *item) {
    g_return_val_if_fail(model, -1);
    AlbumModelPrivate *priv = ALBUM_MODEL_GET_PRIVATE (model);

    gchar *trk_key = _create_key(item->artist, item->albumname);
    gint index = _get_index(priv, trk_key);
    g_free(trk_key);

    return index;
}

gint album_model_get_index_with_track(AlbumModel *model, Track *track) {
    g_return_val_if_fail(model, -1);

    AlbumModelPrivate *priv = ALBUM_MODEL_GET_PRIVATE (model);

    gchar *trk_key = _create_key_from_track(track);
    gint index = _get_index(priv, trk_key);
    g_free(trk_key);

    return index;
}

gint album_model_get_size(AlbumModel *model) {
    g_return_val_if_fail(model, 0);

    AlbumModelPrivate *priv;
    priv = ALBUM_MODEL_GET_PRIVATE (model);

    return g_list_length(priv->album_key_list);
}

#endif /* ALBUM_MODEL_C_ */
