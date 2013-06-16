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
#include "libgtkpod/fileselection.h"
#include "libgtkpod/misc.h"
#include "plugin.h"
#include "clarity_cover.h"
#include "clarity_canvas.h"
#include "clarity_preview.h"
#include "clarity_utils.h"
#include "clarity_context_menu.h"

G_DEFINE_TYPE( ClarityCanvas, clarity_canvas, GTK_TYPE_BOX);

#define CLARITY_CANVAS_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), CLARITY_TYPE_CANVAS, ClarityCanvasPrivate))

#define ANGLE_CCW                        70
#define ANGLE_CW                           ANGLE_CCW - 360
#define MIRROR_ANGLE_CW           360 - ANGLE_CCW
#define MIRROR_ANGLE_CCW         ANGLE_CCW * -1
#define TEXT_SPACE                         2
#define COVER_SPACE                    50
#define FRONT_COVER_SPACE     150
#define MAX_SCALE                          1.4
#define VISIBLE_ITEMS                     8
#define FLOOR                              110

struct _ClarityCanvasPrivate {

    AlbumModel *model;

    // clutter embed widget
    GtkWidget *embed;

    // clutter items
    GList *covers;
    ClutterActor *container;
    ClutterActor *title_text;
    ClutterActor *artist_text;

    gint curr_index;

    gulong preview_signal;

    gboolean blocked;
};

enum DIRECTION {
    MOVE_LEFT = -1,
    MOVE_RIGHT = 1
};

static void clarity_canvas_finalize(GObject *gobject) {
    ClarityCanvasPrivate *priv = CLARITY_CANVAS(gobject)->priv;

    //FIXME
//    g_list_free_full(priv->covers, clarity_cover_destroy);

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

static void _update_text(ClarityCanvasPrivate *priv) {
    g_return_if_fail(priv);

    if (g_list_length(priv->covers) == 0)
            return;

    ClarityCover *ccover = g_list_nth_data(priv->covers, priv->curr_index);

    gchar *title = clarity_cover_get_title(ccover);
    gchar *artist = clarity_cover_get_artist(ccover);

    clutter_text_set_text(CLUTTER_TEXT(priv->title_text), title);
    clutter_text_set_text(CLUTTER_TEXT(priv->artist_text), artist);

    g_free(title);
    g_free(artist);

    // Ensure the text is above the artwork container
    ClutterActor* stage = clutter_actor_get_stage(priv->container);
    clutter_actor_set_child_above_sibling(stage, priv->title_text, priv->container);
    clutter_actor_set_child_above_sibling(stage, priv->artist_text, priv->container);

    /*
     * Find the position of the artwork container. This co-ordinate will be at the centre
     * of the central (front) artwork cover.
     */
    gfloat contx, conty;
    clutter_actor_get_position(priv->container, &contx, &conty);

    /* artist text x co-ordinate is the centre of the container minus half the width of the text */
    gfloat artistx = contx - (clutter_actor_get_width(priv->artist_text) / 2);

    /*
     * artist text y co-ordinate is the centre of the container minus the total of
     * half the centre cover's height, the height of the artist text, the space
     * between the text and the height of the title text.
     */
    gfloat artisty = conty - (clarity_cover_get_artwork_height(ccover) / 2)
                                    - clutter_actor_get_height(priv->artist_text)
                                    - TEXT_SPACE
                                    - clutter_actor_get_height(priv->title_text);

    clutter_actor_set_position(priv->artist_text, artistx, artisty);

    /* title text x co-ordinate is the centre of the container minus half the width of the text */
    gfloat titlex = contx - (clutter_actor_get_width(priv->title_text) / 2);

    /* title text y co-ordinate is above the height of the artist text and TEXT_SPACE */
    gfloat titley = artisty - clutter_actor_get_height(priv->artist_text) - TEXT_SPACE;

    clutter_actor_set_position(priv->title_text, titlex, titley);
}

void clarity_canvas_block_change(ClarityCanvas *self, gboolean value) {
    g_return_if_fail(self);

    ClarityCanvasPrivate *priv = CLARITY_CANVAS_GET_PRIVATE(self);
    priv->blocked = value;

    if (!value) {
        _update_text(priv);
    }
}

gboolean clarity_canvas_is_blocked(ClarityCanvas *self) {
    g_return_val_if_fail(self, TRUE);
    ClarityCanvasPrivate *priv = CLARITY_CANVAS_GET_PRIVATE(self);
    return priv->blocked;
}

static void _preview_cover(ClarityCanvas *self) {
    ClarityCanvasPrivate *priv = self->priv;
    if (!priv->model)
        return;

    AlbumItem *item = album_model_get_item_with_index(priv->model, priv->curr_index);

    GtkWidget *dialog = clarity_preview_new(item);

    /* Display the dialog */
    gtk_widget_show_all(dialog);
}

/**
 * on_main_cover_image_clicked_cb:
 *
 * Call handler used for displaying the tracks associated with
 * the main displayed album cover.
 *
 * @ClarityCanvas
 * @event: event object used to determine the event type
 * @data: any data needed by the function (not required)
 *
 */
static gint _on_main_cover_image_clicked_cb(GtkWidget *widget, GdkEvent *event, gpointer data) {
    ClarityCanvas *self = CLARITY_CANVAS(widget);
    ClarityCanvasPrivate *priv = self->priv;
    guint mbutton;

    if (event->type != GDK_BUTTON_PRESS)
            return FALSE;

    mbutton = event->button.button;

    if ((mbutton == 1) && (event->button.state & GDK_SHIFT_MASK)) {
        clarity_canvas_block_change(self, TRUE);

        AlbumItem *item = album_model_get_item_with_index(priv->model, priv->curr_index);
        if (item) {
            gtkpod_set_displayed_tracks(item->tracks);
        }

        clarity_canvas_block_change(self, FALSE);
    }
    else if (mbutton == 1) {
        _preview_cover(self);
    }
    else if ((mbutton == 3) && (event->button.state & GDK_SHIFT_MASK)) {
        /* Right mouse button clicked and shift pressed.
         * Go straight to edit details window
         */
        AlbumItem *item = album_model_get_item_with_index(priv->model, priv->curr_index);
        GList *tracks = item->tracks;
        gtkpod_edit_details(tracks);
    }
    else if (mbutton == 3) {
        /* Right mouse button clicked on its own so display
         * popup menu
         */
        clarity_context_menu_init(self);
    }

    return FALSE;
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

    priv->container = clutter_actor_new();
    clutter_actor_set_reactive(priv->container, TRUE);
    priv->preview_signal = g_signal_connect (self,
                                "button-press-event",
                                G_CALLBACK (_on_main_cover_image_clicked_cb),
                                priv);

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
    clutter_actor_add_child(stage, priv->container);
    clutter_actor_add_child(stage, priv->title_text);
    clutter_actor_add_child(stage, priv->artist_text);

    gtk_widget_show(priv->embed);

    gtk_box_pack_start(GTK_BOX(self), priv->embed, TRUE, TRUE, 0);

    priv->covers = NULL;
    priv->curr_index = 0;
    priv->blocked = FALSE;

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

    clutter_actor_get_background_color(stage, ccolor);
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
    clutter_actor_set_background_color(stage, ccolor);
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
            clutter_actor_remove_child(priv->container, CLUTTER_ACTOR(ccover));
            iter = iter->next;
        }

        if (CLUTTER_IS_ACTOR(priv->artist_text))
            clutter_text_set_text(CLUTTER_TEXT(priv->artist_text), "");

        if (CLUTTER_IS_ACTOR(priv->title_text))
            clutter_text_set_text(CLUTTER_TEXT(priv->title_text), "");
    }

    priv->covers = NULL;
    priv->model = NULL;
    priv->curr_index = 0;
}

/**
 * A positive rotation angle results in a CCW spin. So depending on the starting angle
 * either a positive or negative value must be calculated in order to result in the smallest
 * rotation, as opposed to a full spin.
 */
static gint _calculate_index_angle (gint new_position, enum DIRECTION dir, double current_angle) {
    gint angle = -1;

    if (new_position >= 2) {

        if (current_angle == 0 || current_angle == 360)
            // Need to go CCW to reach the correct angle and avoid a full spin
            angle = MIRROR_ANGLE_CCW;
        else {
            // Nothing to do, leave the angle alone
            angle = current_angle;
        }

    } else if (new_position <= -2) {

        if (current_angle == 0 || current_angle == 360)
            // Need to go CCW to reach the correct angle and avoid a full spin
            angle = ANGLE_CCW;
        else {
            // Nothing to do, leave the angle alone
            angle = current_angle;
        }

    } else if (new_position == 1) {

        if (dir == MOVE_RIGHT) {
            // Moving from left to right so from 0 / 360 to -70 / 290
            if (current_angle == 0 || current_angle == ANGLE_CCW)
                // From 0 or 70, the smallest rotation is to -70, which ensures we
                // are going CW hence no full spin.
                angle = MIRROR_ANGLE_CCW;
            else {
                // An angle of 360 will move CW to 290
                angle = MIRROR_ANGLE_CW;
            }

        } else {
            // Moving from right to left then this needs no change
            // since the old position was 2
            angle = current_angle;
        }

    } else if (new_position == 0) {

        if (abs(current_angle) > 180)
            // Whether the angle was -290 or 290, the smallest rotation
            // is to 360 and avoids a full spin
            angle = 360;
        else {
            // Angles of 70 and -70 are closest to 0 hence avoiding a full spin
            angle = 0;
        }

    } else if (new_position == -1) {

        if (dir == MOVE_LEFT) {
            // Moving from right to left so from 0 / 360 to 70 / -290
            if (current_angle == 0) {
                // Since the spin goes CCW, a simple rotation of the smaller
                // positive angle, ie. 70, is sufficient.
                angle = ANGLE_CCW;
            } else if (current_angle == MIRROR_ANGLE_CW) {
                // Odd case that to get from 290 to 70 but going CCW, need to
                // actually end up at (360 + 70).
                angle = 360 + ANGLE_CCW;
            } else {
                // An angle of 360 will move CW to -290 and avoid a full spin
                angle = ANGLE_CW;
            }

        } else {
            // Moving from left to right then this needs no change
            // since the old position was -2
            angle = current_angle;
        }
    }

    return angle;
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

static void _display_clarity_cover(ClarityCover *ccover, gint index) {
    clutter_actor_save_easing_state (CLUTTER_ACTOR(ccover));
    clutter_actor_set_easing_mode (CLUTTER_ACTOR(ccover), CLUTTER_EASE_OUT_EXPO);
    clutter_actor_set_easing_duration (CLUTTER_ACTOR(ccover), 1600);

    gint opacity = _calculate_index_opacity(index);
    clutter_actor_set_opacity (CLUTTER_ACTOR(ccover), opacity);
}

static void _set_cover_position(ClarityCover *ccover, gint index) {
    gint pos = _calculate_index_distance(index);
    clutter_actor_set_position(
                    CLUTTER_ACTOR(ccover),
                    pos - clarity_cover_get_artwork_width(ccover) / 2,
                    FLOOR - clarity_cover_get_artwork_height(ccover));
}

static gboolean _create_cover_actors(ClarityCanvasPrivate *priv, AlbumItem *album_item, gint index) {
    g_return_val_if_fail(priv, FALSE);

    ClarityCover *ccover = clarity_cover_new();

    clutter_actor_set_opacity(CLUTTER_ACTOR(ccover), 0);
    priv->covers = g_list_insert(priv->covers, ccover, index);

    clutter_actor_add_child(
                            priv->container,
                            CLUTTER_ACTOR(ccover));

    clarity_cover_set_album_item(ccover, album_item);

    _set_cover_position(ccover, index);

    if((priv->curr_index + VISIBLE_ITEMS < index) ||
            (priv->curr_index - VISIBLE_ITEMS > index)) {
        return FALSE;
    }

    float scale = _calculate_index_scale(index);

    gint angle;
    if (index > 0)
        angle = MIRROR_ANGLE_CW;
    else if (index < 0)
        angle = ANGLE_CCW;
    else
        angle = 0;

    angle = _calculate_index_angle(index, MOVE_LEFT, angle);

    clutter_actor_set_pivot_point(CLUTTER_ACTOR(ccover), 0.5f, 0.5f);
    clutter_actor_set_rotation_angle(CLUTTER_ACTOR(ccover), CLUTTER_Y_AXIS, angle);
    clutter_actor_set_scale(CLUTTER_ACTOR(ccover), scale, scale);

    clutter_actor_set_child_below_sibling(priv->container, CLUTTER_ACTOR(ccover), NULL);

    _display_clarity_cover(ccover, index);

    return FALSE;
}

void _init_album_item(gpointer value, gint index, gpointer user_data) {
    AlbumItem *item = (AlbumItem *) value;
    ClarityCanvas *self = CLARITY_CANVAS(user_data);
    ClarityCanvasPrivate *priv = CLARITY_CANVAS_GET_PRIVATE(self);

    album_model_init_coverart(priv->model, item);

    clarity_canvas_block_change(self, TRUE);
    _create_cover_actors(priv, item, index);
    clarity_canvas_block_change(self, FALSE);
}

void _destroy_cover(ClarityCanvas *cc, gint index) {
    ClarityCanvasPrivate *priv = CLARITY_CANVAS_GET_PRIVATE(cc);

    ClarityCover *ccover = (ClarityCover *) g_list_nth_data(priv->covers, index);
    if (!ccover)
        return;

    priv->covers = g_list_remove(priv->covers, ccover);

    clutter_actor_remove_child(
                               priv->container,
                               CLUTTER_ACTOR(ccover));

    clarity_cover_destroy(CLUTTER_ACTOR(ccover));

    return;
}

static gpointer _init_album_model(gpointer data) {
    g_return_val_if_fail(CLARITY_IS_CANVAS(data), NULL);

    ClarityCanvas *cc = CLARITY_CANVAS(data);
    ClarityCanvasPrivate *priv = CLARITY_CANVAS_GET_PRIVATE(cc);
    AlbumModel *model = priv->model;

    album_model_foreach(model, _init_album_item, cc);

    return NULL;
}

static gboolean _init_album_model_idle(gpointer data) {
    g_return_val_if_fail(CLARITY_IS_CANVAS(data), FALSE);

    ClarityCanvas *self = CLARITY_CANVAS(data);

    _init_album_model(self);

    return FALSE;
}

void clarity_canvas_init_album_model(ClarityCanvas *self, AlbumModel *model) {
    g_return_if_fail(self);
    g_return_if_fail(model);

    if (album_model_get_size(model) == 0)
        return;

    ClarityCanvasPrivate *priv = CLARITY_CANVAS_GET_PRIVATE(self);
    priv->model = model;

    /*
     * Necessary to avoid generating cogl errors in the following use case:
     * 1) Load gtkpod with clarity plugin window docked in the gui but
     *      obscured by another plugin window.
     * 2) Select a playlist.
     * 3) Select the relevant toggle button to bring the clarity window to
     *      the front and visible.
     *
     * This function gets called during the realized signal callback so using
     * g_idle_add lets the realized callback finish and the window display
     * before loading the cogl textures.
     */
    g_idle_add(_init_album_model_idle, self);
}

static void _clear_cover_transitions(GList *covers) {
    GList *iter = covers;
    while (iter) {
        ClarityCover *ccover = iter->data;
        clutter_actor_remove_all_transitions(CLUTTER_ACTOR(ccover));
        iter = iter->next;
    }
}

static void _clear_rotation_behaviours(GList *covers, gint current_index) {
    //Clear rotation behaviours
    for (gint i = 0; i < g_list_length(covers); ++i) {
        ClarityCover *ccover = g_list_nth_data(covers, i);
        clutter_actor_set_easing_duration (CLUTTER_ACTOR(ccover), 0);
        /*
         * Reset the rotation angle since it is the only property that has to be calculated
         * based on its current property value.
         */
        gint index = i - current_index;
        if (index >= 1)
            clutter_actor_set_rotation_angle(CLUTTER_ACTOR(ccover), CLUTTER_Y_AXIS, MIRROR_ANGLE_CW);
        else if (index == 0)
            clutter_actor_set_rotation_angle(CLUTTER_ACTOR(ccover), CLUTTER_Y_AXIS, 0);
        else
            clutter_actor_set_rotation_angle(CLUTTER_ACTOR(ccover), CLUTTER_Y_AXIS, ANGLE_CCW);
    }
}

static void _animate_indices(ClarityCanvasPrivate *priv, gint direction, gint increment) {

    /* Stop any animations already in progress */
    _clear_cover_transitions(priv->covers);

    /* Clear all current rotation behaviours */
    _clear_rotation_behaviours(priv->covers, priv->curr_index);

    for (gint i = 0; i < g_list_length(priv->covers); ++i) {
        ClarityCover *ccover = g_list_nth_data(priv->covers, i);

        gint dist = i - priv->curr_index + (direction * increment);
        gfloat scale = 1;
        gint pos = 0;
        gint opacity = 0;
        gint angle = 0;
        double current_rotation;

        opacity = _calculate_index_opacity(dist);
        scale = _calculate_index_scale(dist);
        pos = _calculate_index_distance(dist);
        gfloat w = clarity_cover_get_artwork_width(ccover);
        gfloat h = clarity_cover_get_artwork_height(ccover);

        clutter_actor_save_easing_state (CLUTTER_ACTOR(ccover));
        clutter_actor_set_easing_mode (CLUTTER_ACTOR(ccover), CLUTTER_EASE_OUT_EXPO);
        clutter_actor_set_easing_duration (CLUTTER_ACTOR(ccover), 1600);

        /* Position and scale */
        clutter_actor_set_scale(CLUTTER_ACTOR(ccover), scale, scale);
        clutter_actor_set_position(CLUTTER_ACTOR(ccover), pos - (w / 2), FLOOR - h);
        clutter_actor_set_pivot_point(CLUTTER_ACTOR(ccover), 0.5f, 0.5f);

        /*Rotation*/
        current_rotation = clutter_actor_get_rotation_angle(CLUTTER_ACTOR(ccover), CLUTTER_Y_AXIS);
        angle = _calculate_index_angle(dist, direction, current_rotation);

        if(current_rotation != angle) {
            clutter_actor_set_rotation_angle(CLUTTER_ACTOR(ccover), CLUTTER_Y_AXIS, angle);
        }

        /* Opacity */
        clutter_actor_set_opacity (CLUTTER_ACTOR(ccover), opacity);
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
        clutter_actor_set_child_below_sibling(priv->container, CLUTTER_ACTOR(ccover), NULL);
        iter = iter->prev;
    }

    iter = main_cover->next;
    while(iter) {
        ClarityCover *ccover = iter->data;
        clutter_actor_set_child_below_sibling(priv->container, CLUTTER_ACTOR(ccover), NULL);
        iter = iter->next;
    }
}

static void _move(ClarityCanvasPrivate *priv, enum DIRECTION direction, gint increment) {

    /* Animate to move left */
    _animate_indices (priv, direction, increment);

    priv->curr_index += ((direction * -1) * increment);

    _restore_z_order(priv);
}

void clarity_canvas_move_left(ClarityCanvas *self, gint increment) {
    g_return_if_fail(self);
    ClarityCanvasPrivate *priv = CLARITY_CANVAS_GET_PRIVATE(self);

    if(priv->curr_index == g_list_length(priv->covers) - 1)
        return;

    clarity_canvas_block_change(self, TRUE);
    _move(priv, MOVE_LEFT, increment);
    clarity_canvas_block_change(self, FALSE);
}

void clarity_canvas_move_right(ClarityCanvas *self, gint increment) {
    g_return_if_fail(self);
    ClarityCanvasPrivate *priv = CLARITY_CANVAS_GET_PRIVATE(self);

    if(priv->curr_index == 0)
        return;

    clarity_canvas_block_change(self, TRUE);
    _move(priv, MOVE_RIGHT, increment);
    clarity_canvas_block_change(self, FALSE);
}

gint clarity_canvas_get_current_index(ClarityCanvas *self) {
    g_return_val_if_fail(self, 0);
    ClarityCanvasPrivate *priv = CLARITY_CANVAS_GET_PRIVATE(self);

    return priv->curr_index;
}

AlbumItem *clarity_canvas_get_current_album_item(ClarityCanvas *self) {
    g_return_val_if_fail(self, NULL);
    ClarityCanvasPrivate *priv = CLARITY_CANVAS_GET_PRIVATE(self);

    if (!priv->model)
        return NULL;

    return album_model_get_item_with_index(priv->model, priv->curr_index);
}

void clarity_canvas_add_album_item(ClarityCanvas *self, AlbumItem *item) {
    g_return_if_fail(self);
    g_return_if_fail(item);

    ClarityCanvasPrivate *priv = CLARITY_CANVAS_GET_PRIVATE(self);
    gint index = album_model_get_index_with_album_item(priv->model, item);

    clarity_canvas_block_change(self, TRUE);

    _init_album_item(item, index, self);

    _animate_indices(priv, 0, 0);

    clarity_canvas_block_change(self, FALSE);
}

void clarity_canvas_remove_album_item(ClarityCanvas *self, AlbumItem *item) {
    g_return_if_fail(self);
    g_return_if_fail(item);

    ClarityCanvasPrivate *priv = CLARITY_CANVAS_GET_PRIVATE(self);
    gint index = album_model_get_index_with_album_item(priv->model, item);

    clarity_canvas_block_change(self, TRUE);

    _destroy_cover(self, index);

    _animate_indices(priv, 0, 0);

    clarity_canvas_block_change(self, FALSE);
}

void clarity_canvas_update(ClarityCanvas *self, AlbumItem *item) {
    g_return_if_fail(self);

    ClarityCanvasPrivate *priv = CLARITY_CANVAS_GET_PRIVATE(self);

    gint index = album_model_get_index_with_album_item(priv->model, item);

    clarity_canvas_block_change(self, TRUE);

    album_model_init_coverart(priv->model, item);

    ClarityCover *ccover = (ClarityCover *) g_list_nth_data(priv->covers, index);
    if (!ccover)
        return;

    clarity_cover_set_album_item(ccover, item);

    _set_cover_position(ccover, index);

    _animate_indices(priv, 0, 0);

    clarity_canvas_block_change(self, FALSE);
}

static void _set_cover_from_file(ClarityCanvas *self) {
    g_return_if_fail(self);

    ClarityCanvasPrivate *priv = CLARITY_CANVAS_GET_PRIVATE(self);

    gchar *filename = fileselection_get_cover_filename();

    if (filename) {
        AlbumItem *item = album_model_get_item_with_index(priv->model, priv->curr_index);
        clarity_util_update_coverart(item->tracks, filename);
    }

    g_free(filename);
}

void on_clarity_set_cover_menuitem_activate(GtkMenuItem *mi, gpointer data) {
    g_return_if_fail(CLARITY_IS_CANVAS(data));

    _set_cover_from_file(CLARITY_CANVAS(data));
}
