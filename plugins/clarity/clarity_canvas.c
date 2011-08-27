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
#include "clarity_cover.h"
#include "clarity_canvas.h"
#include "clarity_preview.h"
#include "clarity_utils.h"

G_DEFINE_TYPE( ClarityCanvas, clarity_canvas, GTK_TYPE_BOX);

#define CLARITY_CANVAS_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), CLARITY_TYPE_CANVAS, ClarityCanvasPrivate))

#define MAX_ANGLE                       70
#define COVER_SPACE                    50
#define FRONT_COVER_SPACE     150
#define MAX_SCALE                          1.4
#define VISIBLE_ITEMS                     8

struct _ClarityCanvasPrivate {

    AlbumModel *model;

    // clutter embed widget
    GtkWidget *embed;

    // clutter items
    GList *covers;
    ClutterActor *container;
    ClutterTimeline *timeline;
    ClutterAlpha *alpha;
    ClutterActor *title_text;
    ClutterActor *artist_text;

    gint curr_index;

    gulong preview_signal;

    gboolean loading_complete;
};

enum DIRECTION {
    MOVE_LEFT = -1,
    MOVE_RIGHT = 1
};

static void clarity_canvas_finalize(GObject *gobject) {
    ClarityCanvasPrivate *priv = CLARITY_CANVAS(gobject)->priv;

    //FIXME
//    g_list_free_full(priv->covers, clarity_cover_destroy);

    if (G_IS_OBJECT(priv->alpha))
        g_object_unref(priv->alpha);

    if (G_IS_OBJECT(priv->timeline))
        g_object_unref(priv->timeline);

    if (GTK_IS_WIDGET(priv->embed))
        gtk_widget_destroy(priv->embed);

    /* call the parent class' finalize() method */
    G_OBJECT_CLASS(clarity_canvas_parent_class)->finalize(gobject);
}

static void clarity_canvas_class_init(ClarityCanvasClass *klass) {
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->finalize = clarity_canvas_finalize;

    g_type_class_add_private(klass, sizeof(ClarityCanvasPrivate));
}

static gboolean _preview_cover_cb(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
    ClarityCanvas *ccanvas = CLARITY_CANVAS(widget);
    ClarityCanvasPrivate *priv = ccanvas->priv;

    if (!priv->model)
        return TRUE;

    AlbumItem *item = album_model_get_item(priv->model, priv->curr_index);

    GtkWidget *dialog = clarity_preview_new(item);

    /* Display the dialog */
    gtk_widget_show_all(dialog);

    return TRUE;
}

/**
 * embed_widget_size_allocated_cb
 *
 * Ensures that when the embed gtk widget is resized or moved
 * around the clutter animations are centred correctly.
 *
 * This finds the new dimensions of the stage each time and centres
 * the group container accordingly.
 *
 */
void _embed_widget_size_allocated_cb(GtkWidget *widget,
                      GtkAllocation *allocation,
                      gpointer data) {
    ClarityCanvasPrivate *priv = (ClarityCanvasPrivate *) data;
    ClutterActor *stage = gtk_clutter_embed_get_stage(GTK_CLUTTER_EMBED(widget));

    gint centreX = clutter_actor_get_width(stage) / 2;
    gint centreY = clutter_actor_get_height(stage) / 2;
    clutter_actor_set_position(priv->container, centreX, centreY);
}

static void clarity_canvas_init(ClarityCanvas *self) {
    ClarityCanvasPrivate *priv;

    self->priv = CLARITY_CANVAS_GET_PRIVATE (self);

    priv = self->priv;

    priv->title_text = clutter_text_new();
    clutter_text_set_font_name(CLUTTER_TEXT(priv->title_text), "Sans");

    priv->artist_text = clutter_text_new();
    clutter_text_set_font_name(CLUTTER_TEXT(priv->title_text), "Sans");

    priv->container = clutter_group_new();
    clutter_actor_set_reactive(priv->container, TRUE);
    priv->preview_signal = g_signal_connect (self,
                                "button-press-event",
                                G_CALLBACK (_preview_cover_cb),
                                priv);
    clutter_container_add(CLUTTER_CONTAINER(priv->container), priv->title_text, priv->artist_text, NULL);

    priv->embed = gtk_clutter_embed_new();
    /*
     * Minimum size before the scrollbars of the parent window
     * are displayed.
     */
    gtk_widget_set_size_request(GTK_WIDGET(priv->embed), DEFAULT_IMG_SIZE * 4, DEFAULT_IMG_SIZE * 2.5);
    /*
     * Ensure that things are always centred when the embed
     * widget is resized.
     */
    g_signal_connect(priv->embed, "size-allocate",
                  G_CALLBACK(_embed_widget_size_allocated_cb), priv);

    ClutterActor *stage = gtk_clutter_embed_get_stage(GTK_CLUTTER_EMBED(priv->embed));
    clutter_container_add_actor(CLUTTER_CONTAINER(stage), priv->container);

    gtk_widget_show(priv->embed);

    gtk_box_pack_start(GTK_BOX(self), priv->embed, TRUE, TRUE, 0);

    priv->covers = NULL;
    priv->timeline = clutter_timeline_new(1600);
    priv->alpha = clutter_alpha_new_full(priv->timeline, CLUTTER_EASE_OUT_EXPO);
    priv->curr_index = 0;
    priv->loading_complete = FALSE;

}

GtkWidget *clarity_canvas_new() {
    return g_object_new(CLARITY_TYPE_CANVAS, NULL);
}

/**
 * clarity_canvas_get_background_display_color:
 *
 * Returns the background color of the clarity canvas.
 *
 * The return value is a GdkRGBA
 *
 */
GdkRGBA *clarity_canvas_get_background_color(ClarityCanvas *self) {
    g_return_val_if_fail(CLARITY_IS_CANVAS(self), NULL);

    ClarityCanvasPrivate *priv = CLARITY_CANVAS_GET_PRIVATE(self);

    ClutterActor *stage = gtk_clutter_embed_get_stage(GTK_CLUTTER_EMBED(priv->embed));

    ClutterColor *ccolor;
    ccolor = g_malloc(sizeof(ClutterColor));

    clutter_stage_get_color(CLUTTER_STAGE(stage), ccolor);
    g_return_val_if_fail(ccolor, NULL);

    GdkRGBA *rgba;
    rgba = g_malloc(sizeof(GdkRGBA));
    rgba->red = ((gdouble) ccolor->red) / 255;
    rgba->green = ((gdouble) ccolor->green) / 255;
    rgba->blue = ((gdouble) ccolor->blue) / 255;
    rgba->alpha = ((gdouble) ccolor->alpha) / 255;

    return rgba;
}

/**
 * clarity_canvas_get_text_color:
 *
 * Returns the text color of the clarity text.
 *
 * The return value is a GdkRGBA
 *
 */
GdkRGBA *clarity_canvas_get_text_color(ClarityCanvas *self) {
    g_return_val_if_fail(CLARITY_IS_CANVAS(self), NULL);

    ClarityCanvasPrivate *priv = CLARITY_CANVAS_GET_PRIVATE(self);

    ClutterColor *ccolor;
    ccolor = g_malloc(sizeof(ClutterColor));

    clutter_text_get_color(CLUTTER_TEXT(priv->title_text), ccolor);
    g_return_val_if_fail(ccolor, NULL);

    GdkRGBA *rgba;
    rgba = g_malloc(sizeof(GdkRGBA));
    rgba->red = ((gdouble) ccolor->red) / 255;
    rgba->green = ((gdouble) ccolor->green) / 255;
    rgba->blue = ((gdouble) ccolor->blue) / 255;
    rgba->alpha = ((gdouble) ccolor->alpha) / 255;

    return rgba;
}

void clarity_canvas_set_background_color(ClarityCanvas *self, const gchar *color_string) {
    g_return_if_fail(self);
    g_return_if_fail(color_string);

    ClarityCanvasPrivate *priv = CLARITY_CANVAS_GET_PRIVATE(self);

    ClutterActor *stage = gtk_clutter_embed_get_stage(GTK_CLUTTER_EMBED(priv->embed));

    ClutterColor *ccolor;
    ccolor = g_malloc(sizeof(ClutterColor));

    clutter_color_from_string(ccolor, color_string);
    clutter_stage_set_color(CLUTTER_STAGE(stage), ccolor);
}

void clarity_canvas_set_text_color(ClarityCanvas *self, const gchar *color_string) {
    g_return_if_fail(self);
    g_return_if_fail(color_string);

    ClarityCanvasPrivate *priv = CLARITY_CANVAS_GET_PRIVATE(self);

    ClutterColor *ccolor;
    ccolor = g_malloc(sizeof(ClutterColor));

    clutter_color_from_string(ccolor, color_string);

    clutter_text_set_color(CLUTTER_TEXT(priv->title_text), ccolor);
    clutter_text_set_color(CLUTTER_TEXT(priv->artist_text), ccolor);
}

void clarity_canvas_clear(ClarityCanvas *self) {
    g_return_if_fail(self);
    ClarityCanvasPrivate *priv = CLARITY_CANVAS_GET_PRIVATE(self);

    if (CLUTTER_IS_ACTOR(priv->container)) {
        GList *iter = priv->covers;
        while(iter) {
            ClarityCover *ccover = iter->data;
            // cover is not referenced anywhere else so it should be destroyed too
            clutter_container_remove(CLUTTER_CONTAINER(priv->container), CLUTTER_ACTOR(ccover), NULL);
            iter = iter->next;
        }
    }

    priv->covers = NULL;
    priv->model = NULL;
    priv->curr_index = 0;
}

static void _calculate_index_angle_and_dir (gint dist_from_front, enum DIRECTION dir, gint *angle, ClutterRotateDirection *rotation_dir) {
    /* The front item direction depends on the direction we came from */
    if (dist_from_front == 0) {
        *rotation_dir =  (dir == MOVE_RIGHT ? CLUTTER_ROTATE_CCW : CLUTTER_ROTATE_CW);
        *angle = 0;
    }

    /* Item on the right */
    else if (dist_from_front > 0) {
        *rotation_dir = CLUTTER_ROTATE_CCW;
        *angle = 360 - MAX_ANGLE;
    }

    /* Item on the left */
    else if (dist_from_front < 0) {
        *rotation_dir = CLUTTER_ROTATE_CW;
        *angle = MAX_ANGLE;
    }
}

static gint _calculate_index_distance (gint dist_from_front) {
    gint dist = ((ABS(dist_from_front) - 1) * COVER_SPACE) + FRONT_COVER_SPACE;

    if (dist_from_front == 0)
        return 0;

    return (dist_from_front > 0 ? dist : 0 - dist);
}

static float _calculate_index_scale(gint dist_from_front) {
    if (dist_from_front == 0)
        return MAX_SCALE;
    else
        return 1;
}

static gint _calculate_index_opacity (gint dist_from_front) {
    return CLAMP ( 255 * (VISIBLE_ITEMS - ABS(dist_from_front)) / VISIBLE_ITEMS, 0, 255);
}

static void _update_text(ClarityCanvasPrivate *priv) {
    g_return_if_fail(priv);

    if (g_list_length(priv->covers) == 0)
            return;

    ClarityCover *ccover = g_list_nth_data(priv->covers, priv->curr_index);

    gchar *title = clarity_cover_get_title(ccover);
    gchar *artist = clarity_cover_get_artist(ccover);

    clutter_text_set_text(CLUTTER_TEXT(priv->title_text), title);
    clutter_text_set_text(CLUTTER_TEXT(priv->artist_text), artist);

    g_warning("%s", clutter_text_get_font_name(CLUTTER_TEXT(priv->title_text)));

    g_free(title);
    g_free(artist);

    clutter_actor_raise_top(priv->title_text);
    clutter_actor_raise_top(priv->artist_text);

    gfloat artistx = (clutter_actor_get_width(priv->artist_text) / 2) * -1;
    gfloat artisty = ((clutter_actor_get_height(CLUTTER_ACTOR(ccover)) / 2) - 25) * -1;
    clutter_actor_set_position(priv->artist_text, artistx, artisty);

    gfloat titlex = (clutter_actor_get_width(priv->title_text) / 2) * -1;
    gfloat titley = artisty - clutter_actor_get_height(priv->artist_text) - 2;
    clutter_actor_set_position(priv->title_text, titlex, titley);
}

static void _display_clarity_cover(ClarityCover *ccover, gint index) {
    ClutterTimeline  *timeline = clutter_timeline_new(1600 * 5);
    ClutterAlpha *alpha = clutter_alpha_new_full (timeline, CLUTTER_EASE_OUT_EXPO);

    gint opacity = _calculate_index_opacity(index);
    clutter_actor_animate_with_alpha(CLUTTER_ACTOR(ccover), alpha, "opacity", opacity, NULL);
    clutter_timeline_start (timeline);
}

static gboolean _set_loading_complete(gpointer data) {
    ClarityCanvasPrivate *priv = (ClarityCanvasPrivate *) data;
    priv->loading_complete = TRUE;
    _update_text(priv);
    return FALSE;
}

static gboolean _create_cover_idle(gpointer data) {

    AlbumItem *album_item = (AlbumItem *) data;
    GObject *gobject = album_item->data;

    g_return_val_if_fail(CLARITY_IS_CANVAS(gobject), FALSE);
    ClarityCanvas *ccanvas = CLARITY_CANVAS(gobject);
    ClarityCanvasPrivate *priv = CLARITY_CANVAS_GET_PRIVATE(ccanvas);

    gint index = g_list_length(priv->covers);
    ClarityCover *ccover = clarity_cover_new();
    clutter_actor_set_opacity(CLUTTER_ACTOR(ccover), 0);
    priv->covers = g_list_append(priv->covers, ccover);

    clutter_container_add_actor(
                            CLUTTER_CONTAINER(priv->container),
                            CLUTTER_ACTOR(ccover));

    clarity_cover_set_album_item(ccover, album_item);

    //TEXT
    //FIXME
//    temp.filename = filename;
//    temp.filetype = filetype;

    //FIXME
    // Confirm whether this does improve performance
    if(index > 20)
        return FALSE;

    gint pos = _calculate_index_distance(index);
    float scale = _calculate_index_scale(index);

    gint angle;
    ClutterRotateDirection rotation_dir;
    _calculate_index_angle_and_dir(index, MOVE_LEFT, &angle, &rotation_dir);

    clutter_actor_set_rotation(
            CLUTTER_ACTOR(ccover),
            CLUTTER_Y_AXIS,
            angle,
            clutter_actor_get_width(CLUTTER_ACTOR(ccover)) / 2,
            0, 0);

    clutter_actor_set_position(
            CLUTTER_ACTOR(ccover),
            pos - clutter_actor_get_width(CLUTTER_ACTOR(ccover)) / 2,
            110 - clutter_actor_get_height(CLUTTER_ACTOR(ccover)));

    clutter_actor_set_scale_full(
            CLUTTER_ACTOR(ccover),
            scale,
            scale,
            clutter_actor_get_width(CLUTTER_ACTOR(ccover)) / 2,
            clutter_actor_get_height(CLUTTER_ACTOR(ccover)) / 2);

    clutter_actor_lower_bottom(CLUTTER_ACTOR(ccover));

    _display_clarity_cover(ccover, index);

    return FALSE;
}

void _init_album_item(gpointer data, gpointer user_data) {
    AlbumItem *item = (AlbumItem *) data;
    ClarityCanvas *cc = CLARITY_CANVAS(user_data);

    gdk_threads_enter();

    Track *track = g_list_nth_data(item->tracks, 0);
    item->albumart = _get_track_image(track);
    item->data = cc;

    g_idle_add_full(G_PRIORITY_LOW, _create_cover_idle, item, NULL);

    gdk_threads_leave();
}

static gpointer _init_album_model_threaded(gpointer data) {
    g_return_val_if_fail(CLARITY_IS_CANVAS(data), NULL);

    ClarityCanvas *cc = CLARITY_CANVAS(data);
    ClarityCanvasPrivate *priv = CLARITY_CANVAS_GET_PRIVATE(cc);
    AlbumModel *model = priv->model;

    album_model_foreach(model, _init_album_item, cc);

    g_idle_add_full(G_PRIORITY_LOW, _set_loading_complete, priv, NULL);

    return NULL;
}

void clarity_canvas_init_album_model(ClarityCanvas *self, AlbumModel *model) {
    g_return_if_fail(self);
    g_return_if_fail(model);

    if (album_model_get_size(model) == 0)
        return;

    ClarityCanvasPrivate *priv = CLARITY_CANVAS_GET_PRIVATE(self);
    priv->model = model;
    priv->loading_complete = FALSE;

    g_thread_create_full(_init_album_model_threaded, self, /* user data  */
    0, /* stack size */
    FALSE, /* joinable   */
    TRUE, /* bound      */
    G_THREAD_PRIORITY_LOW, NULL);

}

static void _clear_rotation_behaviours(GList *covers) {
    //Clear rotation behaviours
    GList *iter = covers;
    while (iter) {
        ClarityCover *ccover = iter->data;
        clarity_cover_clear_rotation_behaviour(ccover);
        iter = iter->next;
    }
}

static void _animate_indices(ClarityCanvasPrivate *priv, enum DIRECTION direction, gint increment) {

    for (gint i = 0; i < g_list_length(priv->covers); ++i) {

        ClarityCover *ccover = g_list_nth_data(priv->covers, i);

        gint dist = i - priv->curr_index + (direction * increment);
        gfloat depth = 1;
        gint pos = 0;
        gint opacity = 0;
        gint angle = 0;
        ClutterRotateDirection rotation_dir;

        opacity = _calculate_index_opacity(dist);
        depth = _calculate_index_scale(dist);
        pos = _calculate_index_distance(dist);
        _calculate_index_angle_and_dir(dist, direction, &angle, &rotation_dir);

        /*Rotation*/
        clarity_cover_set_rotation_behaviour(ccover, priv->alpha, angle, rotation_dir);

        /* Opacity */
        clutter_actor_animate_with_alpha (CLUTTER_ACTOR(ccover), priv->alpha,
                        "opacity", opacity,
                        NULL);

        /* Position and scale */
        clutter_actor_animate_with_alpha (CLUTTER_ACTOR(ccover), priv->alpha,
                        "scale-x",          depth,
                        "scale-y",          depth,
                        "scale-center-x" ,  clutter_actor_get_width(CLUTTER_ACTOR(ccover)) / 2,
                        "scale-center-y" ,  clutter_actor_get_height(CLUTTER_ACTOR(ccover)) / 2,
                        "x", pos - clutter_actor_get_width(CLUTTER_ACTOR(ccover)) / 2 ,
                        NULL);
     }
}

static void _restore_z_order(ClarityCanvasPrivate *priv) {
    g_return_if_fail(priv);

    if (g_list_length(priv->covers) == 0)
        return;

    GList *main_cover = g_list_nth(priv->covers, priv->curr_index);
    g_return_if_fail(main_cover);

    GList *iter = main_cover ->prev;
    while(iter) {
        ClarityCover *ccover = iter->data;
        clutter_actor_lower_bottom(CLUTTER_ACTOR(ccover));
        iter = iter->prev;
    }

    iter = main_cover->next;
    while(iter) {
        ClarityCover *ccover = iter->data;
        clutter_actor_lower_bottom(CLUTTER_ACTOR(ccover));
        iter = iter->next;
    }
}

static void _move(ClarityCanvasPrivate *priv, enum DIRECTION direction, gint increment) {
    /* Stop any animation */
    clutter_timeline_stop(priv->timeline);

    /* Clear all current rotation behaviours */
    _clear_rotation_behaviours(priv->covers);

    /* Animate to move left */
    _animate_indices (priv, direction, increment);

    /* Begin the animation */
    clutter_timeline_start(priv->timeline);

    priv->curr_index += ((direction * -1) * increment);

    _update_text(priv);
    _restore_z_order(priv);
}

void clarity_canvas_move_left(ClarityCanvas *self, gint increment) {
    g_return_if_fail(self);
    ClarityCanvasPrivate *priv = CLARITY_CANVAS_GET_PRIVATE(self);

    if(priv->curr_index == g_list_length(priv->covers) - 1)
        return;

    priv->loading_complete = FALSE;

    _move(priv, MOVE_LEFT, increment);
}

void clarity_canvas_move_right(ClarityCanvas *self, gint increment) {
    g_return_if_fail(self);
    ClarityCanvasPrivate *priv = CLARITY_CANVAS_GET_PRIVATE(self);

    if(priv->curr_index == 0)
        return;

    priv->loading_complete = FALSE;

    _move(priv, MOVE_RIGHT, increment);
}

gint clarity_canvas_get_current_index(ClarityCanvas *self) {
    g_return_val_if_fail(self, 0);
    ClarityCanvasPrivate *priv = CLARITY_CANVAS_GET_PRIVATE(self);

    return priv->curr_index;
}

gboolean clarity_canvas_is_loading(ClarityCanvas *self) {
    g_return_val_if_fail(self, FALSE);
    ClarityCanvasPrivate *priv = CLARITY_CANVAS_GET_PRIVATE(self);
    return !priv->loading_complete;
}

