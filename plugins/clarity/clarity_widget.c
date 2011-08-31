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
#include "libgtkpod/gp_private.h"
#include "libgtkpod/misc.h"
#include "libgtkpod/prefs.h"
#include "clarity_canvas.h"
#include "clarity_widget.h"
#include "album_model.h"

G_DEFINE_TYPE( ClarityWidget, clarity_widget, GTK_TYPE_BOX);

#define CLARITY_WIDGET_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), CLARITY_TYPE_WIDGET, ClarityWidgetPrivate))

#define LEFT_BUTTON "LEFT"
#define RIGHT_BUTTON "RIGHT"

struct _ClarityWidgetPrivate {

    AlbumModel *album_model;

    /* Gtk widgets */
    GtkWidget *contentpanel;

    /* Drawing area related widget */
    GtkWidget *draw_area;

    GtkWidget *controlbox;
    GtkWidget *leftbutton;
    GtkWidget *cdslider;
    GtkWidget *rightbutton;

    gulong slider_signal_id;
};

enum {
    PROP_0
};

/*
 * TODO
 *
 * popup menu
 * drag n drop
 * track added
 * track removed
 * track updated
 * set cover from file
 */

static void clarity_widget_dispose(GObject *gobject) {
    ClarityWidget *cw = CLARITY_WIDGET(gobject);
    cw->current_playlist = NULL;

    ClarityWidgetPrivate *priv = cw->priv;
    if (priv) {

        if (GTK_IS_WIDGET(priv->contentpanel))
            gtk_widget_destroy(priv->contentpanel);

        priv->contentpanel = NULL;
        priv->draw_area = NULL;
        priv->controlbox = NULL;
        priv->leftbutton = NULL;
        priv->cdslider = NULL;
        priv->rightbutton = NULL;

        album_model_destroy(priv->album_model);
    }

    /* call the parent class' finalize() method */
    G_OBJECT_CLASS(clarity_widget_parent_class)->dispose(gobject);
}

static gboolean _on_scrolling_covers_cb(GtkWidget *widget, GdkEventScroll *event, gpointer user_data) {
    ClarityWidgetPrivate *priv = (ClarityWidgetPrivate *) user_data;

    gint index = gtk_range_get_value(GTK_RANGE(priv->cdslider));

    if (event->direction == GDK_SCROLL_DOWN)
        index--;
    else {
        index++;
    }

    gtk_range_set_value(GTK_RANGE (priv->cdslider), index);

    return TRUE;
}

/**
 * on_clarity_button_clicked:
 *
 * Call handler for the left and right buttons. Cause the images to
 * be cycled in the direction indicated by the button.
 *
 * @widget: button which emitted the signal
 * @data: any data needed by the function (not required)
 *
 */
static void _on_clarity_button_clicked(GtkWidget *widget, gpointer data) {
    GtkButton *button;
    const gchar *name;
    ClarityWidgetPrivate *priv;

    priv = (ClarityWidgetPrivate *) data;
    button = GTK_BUTTON(widget);
    name = gtk_widget_get_name(GTK_WIDGET(button));

    gint index = gtk_range_get_value(GTK_RANGE(priv->cdslider));

    /*
     * This looks wrong but the scrolling is reversed, ie.
     * clicking the right button (--->) makes the animation
     * cycle towards the left but shows new covers from the
     * right.
     */
    if (g_str_equal(name, LEFT_BUTTON))
        index--;
    else {
        index++;
    }

    gtk_range_set_value(GTK_RANGE (priv->cdslider), index);
}

/**
 * on_clarity_slider_value_changed:
 *
 * Call handler used for cycling the cover images with the slider.
 *
 * @range: GtkHScale object used as the slider
 * @user_data: any data needed by the function (not required)
 *
 */
static void _on_clarity_slider_value_changed(GtkRange *range, gpointer data) {
    gint old_index, index;

    ClarityWidgetPrivate *priv;
    priv = (ClarityWidgetPrivate *) data;

    if (album_model_get_size(priv->album_model) == 0)
        return;

    index = gtk_range_get_value(range);

    ClarityCanvas *ccanvas = CLARITY_CANVAS(priv->draw_area);
    old_index = clarity_canvas_get_current_index(ccanvas);

    if (index > old_index)
        clarity_canvas_move_left(CLARITY_CANVAS(priv->draw_area), (index - old_index));
    else if (old_index > index)
        clarity_canvas_move_right(CLARITY_CANVAS(priv->draw_area), (old_index - index));
}

/**
 *
 * set_scale_range:
 *
 * Set the scale range - maximum value should be display
 * track list length - (8 NULL images + 1 as index value),
 * ie. the ones either end of the list.
 *
 */
static void _init_slider_range(ClarityWidgetPrivate *priv) {
    g_signal_handler_block(G_OBJECT(priv->cdslider), priv->slider_signal_id);

    gint slider_ubound = album_model_get_size(priv->album_model) - 1;
    if (slider_ubound < 1) {
        /* If only one album cover is displayed then slider_ubbound returns
         * 0 and causes a slider assertion error. Avoid this by disabling the
         * slider, which makes sense because only one cover is displayed.
         */
        slider_ubound = 1;
        gtk_widget_set_sensitive(GTK_WIDGET(priv->cdslider), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(priv->leftbutton), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(priv->rightbutton), FALSE);
    }
    else {
        gtk_widget_set_sensitive(GTK_WIDGET(priv->cdslider), TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(priv->leftbutton), TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(priv->rightbutton), TRUE);
    }

    gtk_range_set_range(GTK_RANGE (priv->cdslider), 0, slider_ubound);

    ClarityCanvas *ccanvas = CLARITY_CANVAS(priv->draw_area);
    gint index = clarity_canvas_get_current_index(ccanvas);
    if (index >= 0 && index <= slider_ubound)
        gtk_range_set_value(GTK_RANGE (priv->cdslider), index);
    else
        gtk_range_set_value(GTK_RANGE (priv->cdslider), 0);

    g_signal_handler_unblock(G_OBJECT(priv->cdslider), priv->slider_signal_id);
}

static void _set_background_color(ClarityWidget *self) {
    gchar *hex_string;

    if (!prefs_get_string_value("clarity_bg_color", NULL))
        hex_string = "#000000";
    else
        prefs_get_string_value("clarity_bg_color", &hex_string);

    ClarityWidgetPrivate *priv = CLARITY_WIDGET_GET_PRIVATE(self);

    clarity_canvas_set_background_color(CLARITY_CANVAS(priv->draw_area), hex_string);
}

static void _set_text_color(ClarityWidget *self) {
    gchar *hex_string;

    if (!prefs_get_string_value("clarity_fg_color", NULL))
        hex_string = "#FFFFFF";
    else
        prefs_get_string_value("clarity_fg_color", &hex_string);

    ClarityWidgetPrivate *priv = CLARITY_WIDGET_GET_PRIVATE(self);

    clarity_canvas_set_text_color(CLARITY_CANVAS(priv->draw_area), hex_string);
}

static void clarity_widget_class_init (ClarityWidgetClass *klass) {
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->dispose = clarity_widget_dispose;

    g_type_class_add_private (klass, sizeof (ClarityWidgetPrivate));
}

static void clarity_widget_init (ClarityWidget *self) {
    ClarityWidgetPrivate *priv;

    priv = CLARITY_WIDGET_GET_PRIVATE (self);

    priv->album_model = album_model_new();

    priv->draw_area = clarity_canvas_new();
    g_signal_connect (G_OBJECT(priv->draw_area),
                                    "scroll-event",
                                    G_CALLBACK(_on_scrolling_covers_cb),
                                    priv);

    _set_background_color(self);
    _set_text_color(self);

    priv->leftbutton = gtk_button_new_with_label("<");
    gtk_widget_set_name(priv->leftbutton, LEFT_BUTTON);
    gtk_button_set_relief(GTK_BUTTON(priv->leftbutton), GTK_RELIEF_NONE);
    gtk_widget_set_can_focus(priv->leftbutton, TRUE);
    g_signal_connect (G_OBJECT(priv->leftbutton), "clicked",
                G_CALLBACK(_on_clarity_button_clicked), priv);

    priv->cdslider = gtk_hscale_new_with_range(0, 1, 1);
    gtk_scale_set_digits(GTK_SCALE(priv->cdslider), 0);
    gtk_scale_set_draw_value(GTK_SCALE(priv->cdslider), FALSE);
    gtk_widget_set_can_focus(priv->cdslider, TRUE);
    gtk_range_set_increments(GTK_RANGE(priv->cdslider), 1, 2);
    priv->slider_signal_id = g_signal_connect (G_OBJECT(priv->cdslider), "value-changed",
                G_CALLBACK(_on_clarity_slider_value_changed), priv);

    priv->rightbutton = gtk_button_new_with_label(">");
    gtk_widget_set_name(priv->rightbutton, RIGHT_BUTTON);
    gtk_button_set_relief(GTK_BUTTON(priv->rightbutton), GTK_RELIEF_NONE);
    gtk_widget_set_can_focus(priv->rightbutton, TRUE);
    g_signal_connect (G_OBJECT(priv->rightbutton), "clicked",
                G_CALLBACK(_on_clarity_button_clicked), priv);

    _init_slider_range(priv);

    priv->controlbox = gtk_hbox_new(FALSE, 2);
    gtk_box_pack_start(GTK_BOX(priv->controlbox), priv->leftbutton, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(priv->controlbox), priv->cdslider, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(priv->controlbox), priv->rightbutton, FALSE, FALSE, 0);

    priv->contentpanel = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(priv->contentpanel), priv->draw_area, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(priv->contentpanel), priv->controlbox, FALSE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX (self), priv->contentpanel, TRUE, TRUE, 0);
    gtk_widget_show_all(GTK_WIDGET(self));
}

GtkWidget *clarity_widget_new() {

    ClarityWidget *cw = g_object_new(CLARITY_TYPE_WIDGET, NULL);
    return GTK_WIDGET(cw);
}

static void clarity_widget_clear(ClarityWidget *self) {
    ClarityWidgetPrivate *priv = CLARITY_WIDGET_GET_PRIVATE(self);
    clarity_canvas_clear(CLARITY_CANVAS(priv->draw_area));
    album_model_clear(priv->album_model);
}

static void _init_tracks(ClarityWidget *cw, GList *tracks) {
    g_return_if_fail(CLARITY_IS_WIDGET(cw));

    if (!tracks)
        return;

    ClarityWidgetPrivate *priv = CLARITY_WIDGET_GET_PRIVATE(cw);

    album_model_add_tracks(priv->album_model, tracks);

    clarity_canvas_init_album_model(CLARITY_CANVAS(priv->draw_area), priv->album_model);

    _init_slider_range(priv);
}

GdkRGBA *clarity_widget_get_background_display_color(ClarityWidget *self) {
    g_return_val_if_fail(CLARITY_IS_WIDGET(self), NULL);

    ClarityWidgetPrivate *priv = CLARITY_WIDGET_GET_PRIVATE(self);

    return clarity_canvas_get_background_color(CLARITY_CANVAS(priv->draw_area));
}

GdkRGBA *clarity_widget_get_text_display_color(ClarityWidget *self) {
    g_return_val_if_fail(CLARITY_IS_WIDGET(self), NULL);

    ClarityWidgetPrivate *priv = CLARITY_WIDGET_GET_PRIVATE(self);

    return clarity_canvas_get_text_color(CLARITY_CANVAS(priv->draw_area));
}

static void _resort_albums(ClarityWidget *self) {
    g_return_if_fail(CLARITY_IS_WIDGET(self));
    ClarityWidgetPrivate *priv = CLARITY_WIDGET_GET_PRIVATE(self);

    // Need to clear away the graphics and start again
    clarity_canvas_clear(CLARITY_CANVAS(priv->draw_area));

    // Resort the albums
    Playlist *playlist = self->current_playlist;
    if (playlist) {
        album_model_resort(priv->album_model, playlist->members);

        // Re-init the graphics
        clarity_canvas_init_album_model(CLARITY_CANVAS(priv->draw_area), priv->album_model);

        // Reset the slider and buttons
        _init_slider_range(priv);
    }
}

void clarity_widget_preference_changed_cb(GtkPodApp *app, gpointer pfname, gpointer value, gpointer data) {
    g_return_if_fail(CLARITY_IS_WIDGET(data));

    ClarityWidget *cw = CLARITY_WIDGET(data);

    gchar *pref_name = pfname;
    if (g_str_equal(pref_name, "clarity_bg_color"))
        _set_background_color(cw);
    else if (g_str_equal(pref_name, "clarity_fg_color"))
        _set_text_color(cw);
    else if (g_str_equal(pref_name, "clarity_sort"))
        _resort_albums(cw);
}

void clarity_widget_playlist_selected_cb(GtkPodApp *app, gpointer pl, gpointer data) {
    g_return_if_fail(CLARITY_IS_WIDGET(data));

    ClarityWidget *cw = CLARITY_WIDGET(data);

    Playlist *playlist = (Playlist *) pl;
    if (!playlist)
        return;

    if (cw->current_playlist == playlist)
        // Should already have all these tracks displayed
        return;

    clarity_widget_clear(cw);

    cw->current_playlist = playlist;
    GList *tracks = playlist->members;
    if (!tracks)
        return;

    _init_tracks(cw, tracks);
}

void clarity_widget_playlist_removed_cb(GtkPodApp *app, gpointer pl, gpointer data) {
    g_return_if_fail(CLARITY_IS_WIDGET(data));

    ClarityWidget *cw = CLARITY_WIDGET(data);
    Playlist *playlist = (Playlist *) pl;
    if (!playlist)
        return;

    if (cw->current_playlist == playlist)
        clarity_widget_clear(cw);
}

void clarity_widget_tracks_selected_cb(GtkPodApp *app, gpointer tks, gpointer data) {
    g_return_if_fail(CLARITY_IS_WIDGET(data));

    ClarityWidget *cw = CLARITY_WIDGET(data);
    ClarityWidgetPrivate *priv = CLARITY_WIDGET_GET_PRIVATE(cw);
    GList *tracks = tks;

    if (!tracks)
        return;

    ClarityCanvas *ccanvas = CLARITY_CANVAS(priv->draw_area);

    if (clarity_canvas_is_loading(ccanvas))
        return;

    gint album_index = album_model_get_index(priv->album_model, tracks->data);
    gtk_range_set_value(GTK_RANGE (priv->cdslider), album_index);
}

void clarity_widget_track_removed_cb(GtkPodApp *app, gpointer tk, gpointer data) {}
void clarity_widget_track_updated_cb(GtkPodApp *app, gpointer tk, gpointer data) {}
void clarity_widget_track_added_cb(GtkPodApp *app, gpointer tk, gpointer data) {}


