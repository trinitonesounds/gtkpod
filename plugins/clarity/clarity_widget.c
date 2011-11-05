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
#include "clarity_dnd_support.h"

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

/**
 * Sort the given list of tracks based on the clarity_sort preference
 */
GList *_sort_track_list(GList *tracks) {
    enum GtkPodSortTypes value = prefs_get_int("clarity_sort");

    switch(value) {
        case SORT_ASCENDING:
            tracks = g_list_sort(tracks, (GCompareFunc) compare_tracks);
            break;
        case SORT_DESCENDING:
            tracks = g_list_sort(tracks, (GCompareFunc) compare_tracks);
            tracks = g_list_reverse(tracks);
            break;
        default:
            // Do Nothing
            break;
    }

    return tracks;
}

/**
 * Clear the clarity canvas of all tracks and album covers
 */
static void _clarity_widget_clear(ClarityWidget *self) {
    ClarityWidgetPrivate *priv = CLARITY_WIDGET_GET_PRIVATE(self);
    clarity_canvas_clear(CLARITY_CANVAS(priv->draw_area));
    album_model_clear(priv->album_model);
}

/**
 * Reload the clarity canvas with the given playlist.
 */
static void _init_clarity_with_playlist(ClarityWidget *cw, Playlist *playlist) {
    if (! gtk_widget_get_realized(GTK_WIDGET(cw)))
        return;

    if (cw->current_playlist == playlist)
        // Should already have all these tracks displayed
        return;

    _clarity_widget_clear(cw);

    cw->current_playlist = playlist;
    GList *tracks = playlist->members;
    if (!tracks)
        return;

    ClarityWidgetPrivate *priv = CLARITY_WIDGET_GET_PRIVATE(cw);

    album_model_add_tracks(priv->album_model, tracks);

    clarity_canvas_init_album_model(CLARITY_CANVAS(priv->draw_area), priv->album_model);

    _init_slider_range(priv);
}

/**
 * Select the given tracks in the clarity widget
 */
static void _clarity_widget_select_tracks(ClarityWidget *self, GList *tracks) {
    if (! gtk_widget_get_realized(GTK_WIDGET(self)))
            return;

    ClarityWidgetPrivate *priv = CLARITY_WIDGET_GET_PRIVATE(self);

    tracks = _sort_track_list(tracks);
    ClarityCanvas *ccanvas = CLARITY_CANVAS(priv->draw_area);

    if (clarity_canvas_is_blocked(ccanvas))
        return;

    gint album_index = album_model_get_index_with_track(priv->album_model, tracks->data);
    gtk_range_set_value(GTK_RANGE (priv->cdslider), album_index);
}

/**
 * Select gtkpod's currently selected tracks in the clarity window.
 *
 * Shoudl be called from a g_idle thread.
 */
static gboolean _clarity_widget_select_tracks_idle(gpointer data) {
    if (! CLARITY_IS_WIDGET(data))
        return FALSE;

    ClarityWidget *cw = CLARITY_WIDGET(data);
    GList *tracks = gtkpod_get_selected_tracks();
    if (!tracks)
        return FALSE;

    _clarity_widget_select_tracks(cw, tracks);

    return FALSE;
}

/**
 * Necessary callback for following use case:
 *
 * 1) Load gtkpod with clarity plugin window docked in the gui but
 *      obscured by another plugin window.
 * 2) Select a playlist.
 * 3) Select the relevant toggle button to bring the clarity window to
 *      the front and visible.
 *
 * Without this callback the window remains blank.
 */
static void _clarity_widget_realized_cb(GtkWidget *widget, gpointer data) {
    if (! CLARITY_IS_WIDGET(widget))
        return;

    ClarityWidget *cw = CLARITY_WIDGET(widget);
    Playlist *playlist = gtkpod_get_current_playlist();
    if (!playlist)
        return;

    _init_clarity_with_playlist(cw, playlist);

    /*
     * Needs to be an idle function that will be called
     * after the idle cover addition functions called by
     * _init_clarity_with_playlist.
     */
    g_idle_add(_clarity_widget_select_tracks_idle, cw);
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

    priv->cdslider = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 1, 1);
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

    /* Dnd destinaton for foreign image files */
    gtk_drag_dest_set(priv->draw_area, 0, clarity_drop_types, TGNR(clarity_drop_types), GDK_ACTION_COPY
            | GDK_ACTION_MOVE);

    g_signal_connect ((gpointer) priv->draw_area, "drag-drop",
            G_CALLBACK (dnd_clarity_drag_drop),
            NULL);

    g_signal_connect ((gpointer) priv->draw_area, "drag-data-received",
            G_CALLBACK (dnd_clarity_drag_data_received),
            NULL);

    g_signal_connect ((gpointer) priv->draw_area, "drag-motion",
            G_CALLBACK (dnd_clarity_drag_motion),
            NULL);

    /*
     * Ensure everything is inited correctly if gtkpod is loaded with
     * the clarity window is not initially visible.
     */
    g_signal_connect_after(GTK_WIDGET(self), "realize",
            G_CALLBACK(_clarity_widget_realized_cb),
            NULL);

    _init_slider_range(priv);

    priv->controlbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_start(GTK_BOX(priv->controlbox), priv->leftbutton, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(priv->controlbox), priv->cdslider, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(priv->controlbox), priv->rightbutton, FALSE, FALSE, 0);

    priv->contentpanel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(priv->contentpanel), priv->draw_area, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(priv->contentpanel), priv->controlbox, FALSE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX (self), priv->contentpanel, TRUE, TRUE, 0);
    gtk_widget_show_all(GTK_WIDGET(self));
}

GtkWidget *clarity_widget_new() {

    ClarityWidget *cw = g_object_new(CLARITY_TYPE_WIDGET, NULL);
    return GTK_WIDGET(cw);
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

    _init_clarity_with_playlist(cw, playlist);
}

void clarity_widget_playlist_removed_cb(GtkPodApp *app, gpointer pl, gpointer data) {
    g_return_if_fail(CLARITY_IS_WIDGET(data));

    ClarityWidget *cw = CLARITY_WIDGET(data);
    Playlist *playlist = (Playlist *) pl;
    if (!playlist)
        return;

    if (! gtk_widget_get_realized(GTK_WIDGET(cw)))
        return;

    if (cw->current_playlist == playlist)
        _clarity_widget_clear(cw);
}

void clarity_widget_tracks_selected_cb(GtkPodApp *app, gpointer tks, gpointer data) {
    g_return_if_fail(CLARITY_IS_WIDGET(data));

    ClarityWidget *cw = CLARITY_WIDGET(data);
    GList *tracks = g_list_copy((GList *) tks);

    if (!tracks)
        return;

    _clarity_widget_select_tracks(cw, tracks);
}

static void _add_track(ClarityWidgetPrivate *priv, Track *track) {
    ClarityCanvas *ccanvas = CLARITY_CANVAS(priv->draw_area);

    if (clarity_canvas_is_blocked(ccanvas))
        return;

    if (album_model_add_track(priv->album_model, track)) {
        AlbumItem *item = album_model_get_item_with_track(priv->album_model, track);
        clarity_canvas_add_album_item(CLARITY_CANVAS(priv->draw_area), item);
        _init_slider_range(priv);
    }
}

void clarity_widget_track_added_cb(GtkPodApp *app, gpointer tk, gpointer data) {
    g_return_if_fail(CLARITY_IS_WIDGET(data));

    ClarityWidget *cw = CLARITY_WIDGET(data);
    ClarityWidgetPrivate *priv = CLARITY_WIDGET_GET_PRIVATE(cw);
    Track *track = tk;

    if (!track)
        return;

    if (! gtk_widget_get_realized(GTK_WIDGET(cw)))
        return;

    GList *current_tracks = cw->current_playlist->members;
    if (!g_list_find(current_tracks, track)) {
        // Track not added to this playlist
        return;
    }

    _add_track(priv, track);
}

static void _remove_track(ClarityWidgetPrivate *priv, AlbumItem *item, Track *track) {
    g_return_if_fail(priv);

    ClarityCanvas *ccanvas = CLARITY_CANVAS(priv->draw_area);

    if (clarity_canvas_is_blocked(ccanvas))
        return;

    if(!item)
        return;

    if(g_list_length(item->tracks) == 1) {
        // Last track in album item so remove canvas cover first
        clarity_canvas_remove_album_item(CLARITY_CANVAS(priv->draw_area), item);
    }

    // Remove the track from the model
    album_model_remove_track(priv->album_model, item, track);

    _init_slider_range(priv);
}

void clarity_widget_track_removed_cb(GtkPodApp *app, gpointer tk, gpointer data) {
    g_return_if_fail(CLARITY_IS_WIDGET(data));

    ClarityWidget *cw = CLARITY_WIDGET(data);
    ClarityWidgetPrivate *priv = CLARITY_WIDGET_GET_PRIVATE(cw);
    Track *track = tk;

    if (!track)
        return;

    if (! gtk_widget_get_realized(GTK_WIDGET(cw)))
        return;

    AlbumItem *item = album_model_get_item_with_track(priv->album_model, track);

    _remove_track(priv, item, track);
}

void clarity_widget_track_updated_cb(GtkPodApp *app, gpointer tk, gpointer data) {
    g_return_if_fail(CLARITY_IS_WIDGET(data));

    ClarityWidget *cw = CLARITY_WIDGET(data);
    ClarityWidgetPrivate *priv = CLARITY_WIDGET_GET_PRIVATE(cw);
    Track *track = tk;

    if (!track)
        return;

    if (! gtk_widget_get_realized(GTK_WIDGET(cw)))
        return;

    ClarityCanvas *ccanvas = CLARITY_CANVAS(priv->draw_area);

    if (clarity_canvas_is_blocked(ccanvas))
        return;

    AlbumItem *item = NULL;

    gint index = album_model_get_index_with_track(priv->album_model, track);
    if (index > -1) {
        /*
         * Track has a valid key so can get the album back.
         * Either has happened:
         * a) Artist/Album key has been changed so the track is being moved
         *     to another existing album
         * b) Artwork has been updated
         * c) Some other change has occurred that is irrelevant to this code.
         *
         * To determine if a) is the case need to determine whether track exists
         * in the album items track list. If it does then b) or c) is true.
         */
        item = album_model_get_item_with_track(priv->album_model, track);
        g_return_if_fail (item);

        index = g_list_index(item->tracks, track);
        if (index != -1) {
            /*
             * Track exists in the album list so determine whether
             * its artwork is up to date
             */
            ExtraTrackData *etd;
            etd = track->userdata;
            if (etd->tartwork_changed) {
                clarity_canvas_update(ccanvas, item);
                return;
            }
            else {
                /*
                 *  Artwork is up to date so nothing changed relevant
                 *  to the display.
                 */
                return;
            }
        }
        else {
            /*
             * Track does not exist in the album list so the artist/album
             * key has definitely changed so find the old album item the long
             * way.
             */
            item = album_model_search_for_track(priv->album_model, track);
        }
    }

    /* item represents the old album item containing the track */
    if (item) {
        /*
         * The track is in this album so remove it in preparation for
         * readding it back either under the same album item but with
         * a different cover or under a different album item due to a
         * different album key.
         */
        _remove_track(priv, item, track);
    }

    /*
     * Create a new album item or find existing album to house the
     * "brand new" track
     */
    _add_track(priv, track);
}


