/*
 |  Copyright (C) 2007 P.G. Richardson <phantom_sf at users.sourceforge.net>
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
 |  $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gdk/gdk.h>
#include <pango/pangocairo.h>
#include <math.h>
#include "libgtkpod/gp_private.h"
#include "libgtkpod/misc.h"
#include "libgtkpod/gtkpod_app_iface.h"
#include "libgtkpod/prefs.h"
#include "libgtkpod/fileselection.h"
#include "display_coverart.h"
#include "plugin.h"
#include "fetchcover.h"
#include "cover_display_context_menu.h"

#ifndef HAVE_GSEALED_GDK
/* Compatibility macros for previous GDK versions */
#define gdk_drag_context_get_selected_action(x) ((x)->action)
#define gdk_drag_context_get_suggested_action(x) ((x)->suggested_action)
#endif

#define DEBUG 0

/* Declarations */
static void free_album(Album_Item *album);
static gint compare_album_keys(gchar *a, gchar *b);
static void set_display_window_dimensions();
static void set_highlight(Cover_Item *cover, gint index, cairo_t *cr);
static void set_shadow_reflection(Cover_Item *cover, cairo_t *cr);
static void remove_track_from_album(Album_Item *album, Track *track, gchar *key, gint index, GList *keylistitem);
static GdkPixbuf *coverart_get_default_track_thumb(gint default_img_size);
static GdkPixbuf *coverart_get_track_thumb(Track *track, Itdb_Device *device, gint default_img_size);

/* callback declarations */
static void on_cover_display_button_clicked(GtkWidget *widget, gpointer data);
static gboolean on_contentpanel_scroll_wheel_turned(GtkWidget *widget, GdkEventScroll *event, gpointer user_data);
static gint on_main_cover_image_clicked(GtkWidget *widget, GdkEvent *event, gpointer data);
static void on_cover_display_slider_value_changed(GtkRange *range, gpointer user_data);
static gboolean on_parent_delete_event(GtkWidget *widget, GdkEvent *event, gpointer data);

/* dnd declarations */
static gboolean
dnd_coverart_drag_drop(GtkWidget *widget, GdkDragContext *drag_context, gint x, gint y, guint time, gpointer user_data);
static void
        dnd_coverart_drag_data_received(GtkWidget *widget, GdkDragContext *dc, gint x, gint y, GtkSelectionData *data, guint info, guint time, gpointer user_data);
static gboolean
dnd_coverart_drag_motion(GtkWidget *widget, GdkDragContext *dc, gint x, gint y, guint time, gpointer user_data);

static void set_cover_dimensions(Cover_Item *cover, int cover_index, gdouble img_width, gdouble img_height);
static void coverart_sort_images(GtkSortType order);
static void set_slider_range(gint index);
static GdkColor *convert_hexstring_to_gdk_color(gchar *hexstring);
static gboolean on_drawing_area_exposed(GtkWidget *draw_area, GdkEventExpose *event);
static void draw(cairo_t *cairo_context);
static void redraw(gboolean force_pixbuf_update);

/* Prefs keys */
const gchar *KEY_DISPLAY_COVERART = "display_coverart";

/* The structure that holds values used throughout all the functions */
static CD_Widget *cdwidget = NULL;
/* The backing hash for the albums and its associated key list */
static GHashTable *album_hash;
static GList *album_key_list;
/* Dimensions used for the canvas */
static gint MIN_WIDTH;
static gint MIN_HEIGHT;
/* Flag set to force an update of covers if a modification has been made */
static gboolean force_pixbuf_covers = FALSE;
/* signal handler id for the components */
static gulong slide_signal_id;
static gulong rbutton_signal_id;
static gulong lbutton_signal_id;
//static gulong window_signal_id;
static gulong contentpanel_signal_id;

static GtkTargetEntry coverart_drop_types[] =
    {
    /* Konqueror supported flavours */
        { "image/jpeg", 0, DND_IMAGE_JPEG },

    /* Fallback flavours */
        { "text/plain", 0, DND_TEXT_PLAIN },
        { "STRING", 0, DND_TEXT_PLAIN } };

#if 0
static void debug_albums ()
{
    gint i;
    Cover_Item *cover;
    Album_Item *album;
    gchar *key;

    printf("Album list\n");
    for(i = 0; i < g_list_length(album_key_list); ++i)
    {
        key = g_list_nth_data(album_key_list, i);
        if (key == NULL)
        printf("Album key is null\n");
        else
        {
            album = g_hash_table_lookup (album_hash, key);
            printf("Index = %d -> Album Details: Artist = %s, Album = %s, No. Tracks = %d\n", i, album->artist, album->albumname, g_list_length (album->tracks));
        }
    }

    printf("Cover List\n");
    for(i = 0; i < IMG_TOTAL; ++i)
    {
        cover = g_ptr_array_index(cdwidget->cdcovers, i);
        if (cover->album == NULL)
        printf("Cover album is null\n");
        else
        printf("Cover Details: Artist = %s, Album = %s\n", cover->album->artist, cover->album->albumname);
    }
}
#endif

static gboolean coverart_window_valid() {
    if (!cdwidget)
        return FALSE;

    if (!cdwidget->draw_area)
        return FALSE;

    if (!gtk_widget_get_window(GTK_WIDGET(cdwidget->draw_area)))
        return FALSE;

    return TRUE;
}

/**
 * coverart_init_display:
 *
 * Initialise the boxes and canvases of the coverart_display.
 *
 */
void coverart_init_display(GtkWidget *parent, gchar *gladepath) {
    GtkWidget *cover_temp_window;
    GtkBuilder *xml;

    cdwidget = g_new0(CD_Widget, 1);

    cdwidget->parent = parent;
    cdwidget->gladepath = gladepath;
    xml = gtkpod_builder_xml_new(cdwidget->gladepath);
    cover_temp_window = gtkpod_builder_xml_get_widget(xml, "cover_display_window");
    cdwidget->contentpanel = gtkpod_builder_xml_get_widget(xml, "cover_display_panel");
    cdwidget->canvasbox = gtkpod_builder_xml_get_widget(xml, "cover_display_canvasbox");
    cdwidget->controlbox = gtkpod_builder_xml_get_widget(xml, "cover_display_controlbox");
    cdwidget->leftbutton = GTK_BUTTON (gtkpod_builder_xml_get_widget (xml, "cover_display_leftbutton"));
    cdwidget->rightbutton = GTK_BUTTON (gtkpod_builder_xml_get_widget (xml, "cover_display_rightbutton"));
    cdwidget->cdslider = GTK_HSCALE (gtkpod_builder_xml_get_widget (xml, "cover_display_scaler"));
    /* create a new drawing area */
    cdwidget->draw_area = gtk_drawing_area_new();
    cdwidget->cdcovers = g_ptr_array_sized_new(IMG_TOTAL);

    g_return_if_fail (cdwidget->contentpanel);
    g_return_if_fail (cdwidget->canvasbox);
    g_return_if_fail (cdwidget->controlbox);
    g_return_if_fail (cdwidget->leftbutton);
    g_return_if_fail (cdwidget->rightbutton);
    g_return_if_fail (cdwidget->cdslider);
    g_return_if_fail (cdwidget->draw_area);
    /* according to GTK FAQ: move a widget to a new parent */
    g_object_ref(cdwidget->contentpanel);
    gtk_container_remove(GTK_CONTAINER (cover_temp_window), cdwidget->contentpanel);
    gtk_widget_destroy(cover_temp_window);

    /* Initialise the album hash backing store */
    album_hash = g_hash_table_new_full(g_str_hash, g_str_equal, (GDestroyNotify) g_free, (GDestroyNotify) free_album);
    album_key_list = NULL;
    set_display_window_dimensions();

    gint i;
    Cover_Item *cover;
    for (i = 0; i < IMG_TOTAL; ++i) {
        cover = g_new0(Cover_Item, 1);
        g_ptr_array_add(cdwidget->cdcovers, cover);
        cover = NULL;
    }
    gtk_box_pack_start(GTK_BOX(cdwidget->canvasbox), GTK_WIDGET(cdwidget->draw_area), TRUE, TRUE, 0);

    /* create the expose event for the drawing area */
    g_signal_connect (G_OBJECT (cdwidget->draw_area), "expose_event", G_CALLBACK (on_drawing_area_exposed), NULL);
    gtk_widget_add_events(cdwidget->draw_area, GDK_BUTTON_PRESS_MASK);
    /* set up some callback events on the main scaled image */
    g_signal_connect(GTK_OBJECT(cdwidget->draw_area), "button-press-event", G_CALLBACK(on_main_cover_image_clicked), NULL);

    /* Dnd destinaton for foreign image files */
    gtk_drag_dest_set(cdwidget->canvasbox, 0, coverart_drop_types, TGNR(coverart_drop_types), GDK_ACTION_COPY
            | GDK_ACTION_MOVE);

    g_signal_connect ((gpointer) cdwidget->canvasbox, "drag-drop",
            G_CALLBACK (dnd_coverart_drag_drop),
            NULL);

    g_signal_connect ((gpointer) cdwidget->canvasbox, "drag-data-received",
            G_CALLBACK (dnd_coverart_drag_data_received),
            NULL);

    g_signal_connect ((gpointer) cdwidget->canvasbox, "drag-motion",
            G_CALLBACK (dnd_coverart_drag_motion),
            NULL);

    contentpanel_signal_id = g_signal_connect (G_OBJECT(cdwidget->contentpanel), "scroll-event",
            G_CALLBACK(on_contentpanel_scroll_wheel_turned), NULL);

    lbutton_signal_id = g_signal_connect (G_OBJECT(cdwidget->leftbutton), "clicked",
            G_CALLBACK(on_cover_display_button_clicked), NULL);

    rbutton_signal_id = g_signal_connect (G_OBJECT(cdwidget->rightbutton), "clicked",
            G_CALLBACK(on_cover_display_button_clicked), NULL);

    slide_signal_id = g_signal_connect (G_OBJECT(cdwidget->cdslider), "value-changed",
            G_CALLBACK(on_cover_display_slider_value_changed), NULL);

    if (GTK_IS_SCROLLED_WINDOW(parent))
        gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(parent), cdwidget->contentpanel);
    else
        gtk_container_add(GTK_CONTAINER (parent), cdwidget->contentpanel);

    g_signal_connect(G_OBJECT(parent), "delete-event", G_CALLBACK(on_parent_delete_event), NULL);
    gtk_widget_show_all(parent);

    coverart_block_change(FALSE);
}

/**
 * set_display_window_dimensions:
 *
 * Initialises the display component width and height.
 * Sets the podpane's paned position value too.
 */
static void set_display_window_dimensions() {
    MIN_WIDTH = (DEFAULT_IMG_SIZE * 2) + (BORDER * 2);
    MIN_HEIGHT = MIN_WIDTH;

    gtk_widget_set_size_request(GTK_WIDGET(cdwidget->canvasbox), MIN_WIDTH, MIN_HEIGHT);
    /* set the size of the drawing area */
    gtk_widget_set_size_request(GTK_WIDGET(cdwidget->draw_area), MIN_WIDTH, MIN_HEIGHT);
}

/**
 * coverart_block_change:
 *
 * Select covers events can be switched off when automatic
 * selections of tracks are taking place.
 *
 * @val: indicating whether to block or unblock select cover events
 *
 */
void coverart_block_change(gboolean val) {
    if (gtk_widget_get_realized(GTK_WIDGET(gtkpod_app))) {
        if (val) {
            GdkCursor *cursor = gdk_cursor_new(GDK_WATCH);
            gdk_window_set_cursor(gtk_widget_get_window(GTK_WIDGET(gtkpod_app)), cursor);
            gdk_cursor_unref(cursor);
        }
        else
            gdk_window_set_cursor(gtk_widget_get_window(GTK_WIDGET(gtkpod_app)), NULL);
    }

    if (cdwidget != NULL)
        cdwidget->block_display_change = val;
}

/**
 * redraw:
 *
 * Draw the artist and album text strings.
 *
 * @cairo_context: the context of the artwork display
 * @text: the text to be added to the artwork display
 * @x: the x coordinate of its location
 * @y: the y coordinate of its location
 *
 */
static void draw_string(cairo_t *cairo_context, const gchar *text, gdouble x, gdouble y) {
    static PangoFontDescription *desc = NULL;
    GdkColor *color = coverart_get_foreground_display_color();
    PangoLayout *layout;
    PangoRectangle extents;

    gdk_cairo_set_source_color(cairo_context, color);
    g_free(color);

    if (!desc) {
        desc = pango_font_description_from_string("Sans Bold 9");
    }

    layout = pango_cairo_create_layout(cairo_context);
    pango_layout_set_text(layout, text, -1);
    pango_layout_set_font_description(layout, desc);
    pango_layout_get_pixel_extents(layout, NULL, &extents);

    cairo_move_to(cairo_context, x + extents.x - (extents.width / 2), y + extents.y - (extents.height / 2));

    pango_cairo_show_layout(cairo_context, layout);

    g_object_unref(layout);
}

/**
 * redraw:
 *
 * Utility function for set all the x, y, width and height
 * dimensions applicable to a single cover widget
 *
 * @force_pixbuf_update: flag indicating whether to force an update of the pixbuf covers
 */
static void redraw(gboolean force_pixbuf_update) {
    g_return_if_fail(cdwidget);
    g_return_if_fail(cdwidget->draw_area);
    g_return_if_fail(gtk_widget_get_window(GTK_WIDGET(cdwidget->draw_area)));

    force_pixbuf_covers = force_pixbuf_update;
    GdkRegion *region = gdk_drawable_get_clip_region(gtk_widget_get_window(GTK_WIDGET(cdwidget->draw_area)));
    /* redraw the cairo canvas completely by exposing it */
    gdk_window_invalidate_region(gtk_widget_get_window(GTK_WIDGET(cdwidget->draw_area)), region, TRUE);
    gdk_window_process_updates(gtk_widget_get_window(GTK_WIDGET(cdwidget->draw_area)), TRUE);
    gdk_region_destroy(region);

    if (g_list_length(album_key_list) <= 1) {
        gtk_widget_set_sensitive(GTK_WIDGET(cdwidget->cdslider), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(cdwidget->leftbutton), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(cdwidget->rightbutton), FALSE);
    }
    else {
        gtk_widget_set_sensitive(GTK_WIDGET(cdwidget->cdslider), TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(cdwidget->leftbutton), TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(cdwidget->rightbutton), TRUE);
    }
}

/**
 * draw:
 *
 * Paint the coverart display using cairo.
 *
 * @cairo_context: the coverart display context
 */
static void draw(cairo_t *cairo_context) {
    gint cover_index[] =
        { 0, 8, 1, 7, 2, 6, 3, 5, 4 };
    /* Draw the background */
    GdkColor *color = coverart_get_background_display_color();

    cairo_save(cairo_context);
    gdk_cairo_set_source_color(cairo_context, color);
    cairo_set_operator(cairo_context, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cairo_context);
    cairo_restore(cairo_context);

    Album_Item *album;
    gint i, album_index;
    Cover_Item *cover;
    gchar *key;

    for (i = 0; i < IMG_TOTAL; ++i) {
        cover = g_ptr_array_index(cdwidget->cdcovers, cover_index[i]);
        album_index = cdwidget->first_imgindex + cover_index[i];

        /* Get the key from the key list appropriate to the index
         * provided by the first image index property
         */
        key = g_list_nth_data(album_key_list, album_index);

        if (key == NULL)
            continue;

        /* Find the Album Item appropriate to the key */
        album = g_hash_table_lookup(album_hash, key);
        cover->album = album;

        if (force_pixbuf_covers) {
            g_object_unref(album->albumart);
            album->albumart = NULL;
            if (album->scaled_art != NULL) {
                g_object_unref(album->scaled_art);
                album->scaled_art = NULL;
            }
        }

        Track *track;
        if (album->albumart == NULL) {
            track = g_list_nth_data(album->tracks, 0);
            album->albumart = coverart_get_track_thumb(track, track->itdb->device, DEFAULT_IMG_SIZE);
        }

        /* Set the x, y, height and width of the CD cover */
        set_cover_dimensions(cover, cover_index[i], gdk_pixbuf_get_width(album->albumart), gdk_pixbuf_get_height(album->albumart));

        /* Set the Cover */
        GdkPixbuf *scaled;
        if (album->scaled_art == NULL)
            scaled = gdk_pixbuf_scale_simple(album->albumart, cover->img_width, cover->img_height, GDK_INTERP_BILINEAR);
        else
            scaled = album->scaled_art;

        gdk_cairo_set_source_pixbuf(cairo_context, scaled, cover->img_x, cover->img_y);
        cairo_paint(cairo_context);

        /* Draw a black line around the cd cover */
        cairo_save(cairo_context);
        cairo_set_line_width(cairo_context, 1);
        cairo_set_source_rgb(cairo_context, 0, 0, 0);
        cairo_rectangle(cairo_context, cover->img_x, cover->img_y, cover->img_width, cover->img_height);
        cairo_stroke(cairo_context);
        cairo_restore(cairo_context);

        /* Display the highlight */
        set_highlight(cover, cover_index[i], cairo_context);

        /* flip image vertically to create reflection */
        GdkPixbuf *reflection;
        reflection = gdk_pixbuf_flip(scaled, FALSE);

        cairo_save(cairo_context);
        gdk_cairo_set_source_pixbuf(cairo_context, reflection, cover->img_x, cover->img_y + cover->img_height + 2);
        cairo_paint(cairo_context);
        cairo_restore(cairo_context);

        g_object_unref(reflection);
        g_object_unref(scaled);

        /* Set the reflection shadow */
        set_shadow_reflection(cover, cairo_context);

        cairo_save(cairo_context);
        /* Set the text if the index is the central image cover */
        if (cover_index[i] == IMG_MAIN) {
            draw_string(cairo_context, album->artist, cover->img_x + (cover->img_width / 2), cover->img_y + cover->img_height + 15);
            draw_string(cairo_context, album->albumname, cover->img_x + (cover->img_width / 2), cover->img_y + cover->img_height + 30);
        }
        cairo_restore(cairo_context);
    }

    force_pixbuf_covers = FALSE;

    /* free the scaled pixbufs from the non visible covers either side of the current display.
     * Experimental feature that should save on memory.
     */
    key = g_list_nth_data(album_key_list, cdwidget->first_imgindex - 1);
    if (key != NULL) {
        album = g_hash_table_lookup(album_hash, key);
        if (album->scaled_art) {
            g_object_unref(album->scaled_art);
            album->scaled_art = NULL;
        }
    }

    key = g_list_nth_data(album_key_list, cdwidget->first_imgindex + IMG_TOTAL + 1);
    if (key != NULL) {
        album = g_hash_table_lookup(album_hash, key);
        if (album->scaled_art) {
            g_object_unref(album->scaled_art);
            album->scaled_art = NULL;
        }
    }
    g_free(color);
}

/**
 * coverart_display_update:
 *
 * Refreshes the coverart display depending on the playlist selection. Using the
 * clear_track_list, the refresh can be quicker is set to FALSE. However, the track
 * list is not updated in this case. Using TRUE, the display is completely cleared and
 * redrawn.
 *
 * @clear_track_list: flag indicating whether to clear the displaytracks list or not
 *
 */
void coverart_display_update(gboolean clear_track_list) {
    gint i;
    GList *tracks;
    Track *track;
    Album_Item *album;

    if (!coverart_window_valid())
        return;

    /* Ensure that the setting of images hasnt been turned off
     * due to being in the middle of a selection operation
     */
    if (cdwidget->block_display_change)
        return;

    if (clear_track_list) {
        /* Free up the hash table and the key list */
        g_hash_table_foreach_remove(album_hash, (GHRFunc) gtk_true, NULL);
        g_list_free(album_key_list);
        album_key_list = NULL;

        /* Find the selected playlist */
        Playlist *pl = gtkpod_get_current_playlist();
        if (!pl) {
            return;
        }

        tracks = pl->members;
        if (!tracks) {
            return;
        }

        while (tracks) {
            gchar *album_key;
            track = tracks->data;

            album_key = g_strconcat(track->artist, "_", track->album, NULL);
            /* Check whether an album item has already been created in connection
             * with the track's artist and album
             */
            album = g_hash_table_lookup(album_hash, album_key);
            if (album == NULL) {
                /* Album item not found so create a new one and populate */
                album = g_new0 (Album_Item, 1);
                album->albumart = NULL;
                album->scaled_art = NULL;
                album->albumname = g_strdup(track->album);
                album->artist = g_strdup(track->artist);
                album->tracks = NULL;
                album->tracks = g_list_prepend(album->tracks, track);

                /* Insert the new Album Item into the hash */
                g_hash_table_insert(album_hash, album_key, album);
                /* Add the key to the list for sorting and other functions */
                album_key_list = g_list_prepend(album_key_list, album_key);
            }
            else {
                /* Album Item found in the album hash so
                 * append the track to the end of the
                 * track list */
                g_free(album_key);
                album->tracks = g_list_prepend(album->tracks, track);
            }

            tracks = tracks->next;
        }

        cdwidget->first_imgindex = 0;
    }

    /* Remove all null tracks before any sorting should take place */
    album_key_list = g_list_remove_all(album_key_list, NULL);

    /* Sort the tracks to the order set in the preference */
    coverart_sort_images(prefs_get_int("cad_sort"));

    /* Add 4 null tracks to the end of the track list for padding */
    for (i = 0; i < IMG_MAIN; ++i)
        album_key_list = g_list_append(album_key_list, NULL);

    /* Add 4 null tracks to the start of the track list for padding */
    for (i = 0; i < IMG_MAIN; ++i)
        album_key_list = g_list_prepend(album_key_list, NULL);

    if (clear_track_list)
        set_slider_range(0);
    else
        set_slider_range(cdwidget->first_imgindex);
}

/**
 * Sort the coverart display according to the given
 * sort order.
 *
 */
void coverart_display_sort(gint order) {
    prefs_set_int("cad_sort", order);
    coverart_display_update(TRUE);
    gtkpod_broadcast_preference_change("cad_sort", order);
}

/**
 * set_cover_dimensions:
 *
 * Utility function for set all the x, y, width and height
 * dimensions applicable to a single cover widget
 *
 * @cover: cover widget for which dimensions are to be set
 * @cover_index: index of the widget. Used to determine whether
 * 												cover is the main cover or not
 */
static void set_cover_dimensions(Cover_Item *cover, int cover_index, gdouble img_width, gdouble img_height) {
    gdouble x = 0, y = 0;
    gdouble small_img_width, small_img_height;
    gdouble display_width = 0, display_height = 0;
    gdouble display_diff = 0, display_ratio = 0;
    gint temp_index = 0;
    gint PANEL_WIDTH = 0, PANEL_HEIGHT = 0;
    gint CONTBOX_WIDTH = 0, CONTBOX_HEIGHT = 0;

    gdk_drawable_get_size(GDK_DRAWABLE(gtk_widget_get_window(cdwidget->canvasbox)), &PANEL_WIDTH, &PANEL_HEIGHT);
    gtk_widget_get_size_request(cdwidget->controlbox, &CONTBOX_WIDTH, &CONTBOX_HEIGHT);

    // If panel width not been rendered default to minimum
    if (PANEL_WIDTH < MIN_WIDTH)
        PANEL_WIDTH = MIN_WIDTH;

    // If panel height not been rendered default to minimum
    if (PANEL_HEIGHT < MIN_HEIGHT)
        PANEL_HEIGHT = MIN_HEIGHT;

    // Ensure that the img width is less than half of the total
    // width of the panel.
    display_ratio = img_width / img_height;
    if (img_width > (PANEL_WIDTH / 2)) {
        img_width = (PANEL_WIDTH / 2);
        img_height = img_width / display_ratio;
    }

    // Small image size is 0.9 the size of image size
    small_img_width = img_width * 0.9;
    small_img_height = img_height * 0.9;

    /*
     * PANEL_WIDTH is the width of the panel
     * BORDER is the 10 pixel frame around the images
     */
    // Half of the display area
    display_width = (PANEL_WIDTH / 2) - (BORDER * 2);
    // width that caters for the width of a small image
    display_diff = display_width - small_img_width;

    /* Set the x location of the cover image */
    switch (cover_index) {
    case 0:
    case 1:
    case 2:
    case 3:
        // This breaks the display_diff up into 4 equal parts
        // which each cover then occupies
        display_ratio = ((gdouble) cover_index) / 4;
        x = BORDER + (display_ratio * display_diff);
        break;
    case IMG_MAIN:
        /* The Main Image CD Cover Image */
        x = (PANEL_WIDTH - img_width) / 2;
        break;
    case 5:
    case 6:
    case 7:
    case 8:
        // As calculating from right -> left below, need to
        // covert the indices to 0, 1, 2, 3.
        temp_index = cover_index - 8;
        if (temp_index < 0)
            temp_index = temp_index * -1;

        // This breaks the display_diff up into 4 equal parts
        // which each cover then occupies. However, as going
        // from right, x needs to be subtracted from the overall
        // panel width
        display_ratio = ((gdouble) temp_index) / 4;
        x = PANEL_WIDTH - (BORDER + small_img_width + (display_ratio * display_diff));
        break;
    }

    /*
     * Set the y location of the cover image.
     * The y location must be determined by
     * height of the cover image so that the
     * highlight and shadow fit in correctly.
     */
    display_height = PANEL_HEIGHT - (BORDER * 2) - CONTBOX_HEIGHT;

    switch (cover_index) {
    case 0:
    case 8:
        y = display_height - (small_img_height + (BORDER * 15));
        break;
    case 1:
    case 7:
        y = display_height - (small_img_height + (BORDER * 12));
        break;
    case 2:
    case 6:
        y = display_height - (small_img_height + (BORDER * 9));
        break;
    case 3:
    case 5:
        y = display_height - (small_img_height + (BORDER * 6));
        break;
    case IMG_MAIN:
        /* The Main Image CD Cover Image */
        y = display_height - (img_height + (BORDER * 3));
    }

    cover->img_x = x;
    cover->img_y = y;

    if (cover_index == IMG_MAIN) {
        cover->img_height = img_height;
        cover->img_width = img_width;
    }
    else {
        cover->img_height = small_img_height;
        cover->img_width = small_img_width;
    }
}

/**
 * set_highlight:
 *
 * Sets the highlighted image to the cover to give shine
 * and a reflection.
 *
 * @cover: A Cover_Item object which the higlighted is added to.
 *
 */
static void set_highlight(Cover_Item *cover, gint index, cairo_t * cr) {
    if (index == IMG_MAIN)
        return;

    cairo_save(cr);
    cairo_pattern_t *pat;
    pat = cairo_pattern_create_linear(cover->img_x, cover->img_y, cover->img_x, cover->img_y
            + (((gdouble) cover->img_height) / 2.5));
    cairo_pattern_add_color_stop_rgba(pat, 0.0, 1, 1, 1, 0);
    cairo_pattern_add_color_stop_rgba(pat, 0.4, 1, 1, 1, 0.6);
    cairo_pattern_add_color_stop_rgba(pat, 0.9, 1, 1, 1, 0);
    cairo_pattern_set_extend(pat, CAIRO_EXTEND_REPEAT);

    cairo_rectangle(cr, cover->img_x, cover->img_y, cover->img_width, cover->img_height);
    cairo_rotate(cr, M_PI / 4);
    cairo_set_source(cr, pat);
    cairo_fill(cr);
    cairo_pattern_destroy(pat);
    cairo_restore(cr);
}

/**
 * set_shadow_reflection:
 *
 * Sets the shadow reflection to the same as the
 * background of the display.
 *
 * @cover: A Cover_Item object which the higlighted is added to.
 *
 */
static void set_shadow_reflection(Cover_Item *cover, cairo_t *cr) {
    GdkColor *color = coverart_get_background_display_color();
    gdouble r = ((gdouble) (color->red >> 8)) / 255;
    gdouble g = ((gdouble) (color->green >> 8)) / 255;
    gdouble b = ((gdouble) (color->blue >> 8)) / 255;
    g_free(color);

    cairo_save(cr);
    cairo_pattern_t *pat;
    pat = cairo_pattern_create_linear(cover->img_x, cover->img_y + cover->img_height + 2, cover->img_x, cover->img_y
            + cover->img_height + 2 + cover->img_height);
    cairo_pattern_add_color_stop_rgba(pat, 0, r, g, b, 0.3);
    cairo_pattern_add_color_stop_rgba(pat, 0.5, r, g, b, 1);
    cairo_rectangle(cr, cover->img_x, cover->img_y + cover->img_height + 2, cover->img_width + 10, cover->img_height);
    cairo_set_source(cr, pat);
    cairo_fill(cr);
    cairo_pattern_destroy(pat);
    cairo_restore(cr);
}

/**
 *
 * Function to cause a refresh on the given track.
 * The signal will be one of:
 *
 *    COVERART_REMOVE_SIGNAL - track deleted
 *    COVERART_CREATE_SIGNAL - track created
 *    COVERART_CHANGE_SIGNAL - track modified
 *
 * If the track was in the current display of artwork then the
 * artwork will be updated. If it was not then a refresh is unnecessary
 * and the function will return accordingly.
 *
 * @track: affected track
 * @signal: flag indicating the type of track change that has occurred.
 */
void coverart_track_changed(Track *track, gint signal) {
    GList *keypos;
    gchar *trk_key;
    Album_Item *album;
    gint index;
    gboolean findremove;

    if (!coverart_window_valid())
        return;
    /*
     * Scenarios:
     * a) A track is being deleted that is not in the display
     * b) A track is being deleted that is in the display
     * c) A track has changed is some way so maybe the coverart
     * d) A track has been created and its artist and album are already in the displaylist
     * e) A track has been created and its artist and album are not in the displaylist
     */

    trk_key = g_strconcat(track->artist, "_", track->album, NULL);
    /* Determine the index of the found album */
    keypos = g_list_find_custom(album_key_list, trk_key, (GCompareFunc) compare_album_keys);

    switch (signal) {
    case COVERART_REMOVE_SIGNAL:
        g_free(trk_key);
        if (! keypos)
            return;

        /* Reassign trk_key to the key from the list */
        trk_key = keypos->data;
        index = g_list_position(album_key_list, keypos);

        album = g_hash_table_lookup(album_hash, trk_key);

        /* Remove the track from the album item */
        remove_track_from_album(album, track, trk_key, index, keypos);

        /* Size of key list may have changed so reset the slider
         * to appropriate range and index.
         */
        set_slider_range(index - IMG_MAIN);
        break;
    case COVERART_CREATE_SIGNAL:
        /* Check whether an album item has already been created in connection
         * with the track's artist and album
         */
        album = g_hash_table_lookup(album_hash, trk_key);
        if (album == NULL) {
            /* Album item not found so create a new one and populate */
            album = g_new0 (Album_Item, 1);
            album->albumart = NULL;
            album->scaled_art = NULL;
            album->albumname = g_strdup(track->album);
            album->artist = g_strdup(track->artist);
            album->tracks = NULL;
            album->tracks = g_list_append(album->tracks, track);

            /* Insert the new Album Item into the hash */
            g_hash_table_insert(album_hash, trk_key, album);

            /* Add the key to the list for sorting and other functions */
            /* But first ... */
            /* Remove all null tracks before any sorting should take place */
            album_key_list = g_list_remove_all(album_key_list, NULL);

            if (prefs_get_int("cad_sort") == SORT_ASCENDING) {
                album_key_list = g_list_insert_sorted(album_key_list, trk_key, (GCompareFunc) compare_album_keys);
            }
            else if (prefs_get_int("cad_sort") == SORT_DESCENDING) {
                /* Already in descending order so reverse into ascending order */
                album_key_list = g_list_reverse(album_key_list);
                /* Insert the track */
                album_key_list = g_list_insert_sorted(album_key_list, trk_key, (GCompareFunc) compare_album_keys);
                /* Reverse again */
                album_key_list = g_list_reverse(album_key_list);
            }
            else {
                /* NO SORT */
                album_key_list = g_list_append(album_key_list, trk_key);
            }

            /* Readd in the null tracks */
            /* Add 4 null tracks to the end of the track list for padding */
            gint i;
            for (i = 0; i < IMG_MAIN; ++i)
                album_key_list = g_list_append(album_key_list, NULL);

            /* Add 4 null tracks to the start of the track list for padding */
            for (i = 0; i < IMG_MAIN; ++i)
                album_key_list = g_list_prepend(album_key_list, NULL);

            redraw(FALSE);
        }
        else {
            /* Album Item found in the album hash so append the track to
             * the end of the track list
             */
            album->tracks = g_list_append(album->tracks, track);
        }

        /* Set the slider to the newly inserted track.
         * In fact sets image_index to 4 albums previous
         * to newly inserted album to ensure this album is
         * the main middle one.
         */
        keypos = g_list_find_custom(album_key_list, trk_key, (GCompareFunc) compare_album_keys);
        index = g_list_position(album_key_list, keypos);
        set_slider_range(index - IMG_MAIN);

        break;
    case COVERART_CHANGE_SIGNAL:
        /* A track is declaring itself as changed so what to do? */
        findremove = FALSE;
        if (!keypos) {
            /* The track could not be found according to the key!
             * The ONLY way this could happen is if the user changed the
             * artist or album of the track. Well it should be rare but the only
             * way to remove it from its "old" album item is to search each one
             */
            findremove = TRUE;
        }
        else {
            /* Track has a valid key so can get the album back.
             * Either has happened:
             * a) Artist/Album key has been changed so the track is being moved to another existing album
             * b) Some other change has occurred that is irrelevant to this code.
             */

            /* To determine if a) is the case need to determine whether track exists in the
             * album items track list. If it does then b) is true and nothing more is required.
             */
            album = g_hash_table_lookup(album_hash, trk_key);
            g_return_if_fail (album);

            index = g_list_index(album->tracks, track);
            if (index != -1) {
                /* Track exists in the album list so ignore the change and return */
                ExtraTrackData *etd;
                etd = track->userdata;
                if (etd->tartwork_changed == TRUE) {
                    etd->tartwork_changed = FALSE;
                    redraw(TRUE);
                }

                return;
            }
            else {
                /* Track does not exist in the album list so the artist/album key has definitely changed */
                findremove = TRUE;
            }
        }

        if (findremove) {
            /* It has been determined that the track has had its key changed
             * and thus a search must be performed to find the "original" album
             * that the track belonged to, remove it then add the track to the new
             * album.
             */
            GList *klist;
            gchar *key;
            klist = g_list_first(album_key_list);
            while (klist != NULL) {
                key = (gchar *) klist->data;
                index = g_list_index(album_key_list, key);
                if (key != NULL) {
                    album = g_hash_table_lookup(album_hash, key);

                    gint album_trk_index;
                    album_trk_index = g_list_index(album->tracks, track);
                    if (album_trk_index != -1) {
                        /* The track is in this album so remove it in preparation for readding
                         * under the new album key
                         */
                        remove_track_from_album(album, track, key, index, klist);
                        /* Found the album and removed so no need to continue the loop */
                        break;
                    }
                }
                klist = klist->next;
            }

            /* Create a new album item or find existing album to house the "brand new" track */
            coverart_track_changed(track, COVERART_CREATE_SIGNAL);
        }
    }
}

/**
 * on_drawing_area_exposed:
 *
 * Callback for the drawing area. When the drwaing area is covered,
 * resized, changed etc.. This will be called the draw() function is then
 * called from this and the cairo redrawing takes place.
 *
 * @draw_area: drawing area where al the cairo drawing takes place
 * @event: gdk expose event
 *
 * Returns:
 * boolean indicating whether other handlers should be run.
 */
static gboolean on_drawing_area_exposed(GtkWidget *draw_area, GdkEventExpose *event) {
    if (!draw_area || !gtk_widget_get_window(draw_area))
        return FALSE;

    cairo_t *cairo_draw_context;

    /* get a cairo_t */
    cairo_draw_context = gdk_cairo_create(gtk_widget_get_window(draw_area));

    /* set a clip region for the expose event */
    cairo_rectangle(cairo_draw_context, event->area.x, event->area.y, event->area.width, event->area.height);
    cairo_clip(cairo_draw_context);
    draw(cairo_draw_context);
    cairo_destroy(cairo_draw_context);

    return FALSE;
}

/**
 * on_contentpanel_scroll_wheel_turned:
 *
 * Call handler for the scroll wheel. Cause the images to
 * be cycled in the direction indicated by the scroll wheel.
 *
 * @widget: CoverArt Display
 * @event: scroll wheel event
 * @data: any data needed by the function (not required)
 *
 */
static gboolean on_contentpanel_scroll_wheel_turned(GtkWidget *widget, GdkEventScroll *event, gpointer user_data) {
    gint displaytotal;

    if (event->direction == GDK_SCROLL_DOWN)
        cdwidget->first_imgindex++;
    else
        cdwidget->first_imgindex--;

    displaytotal = g_list_length(album_key_list) - 8;

    if (displaytotal <= 0)
        return TRUE;

    /* Use the index value from the slider for the main image index */
    if (cdwidget->first_imgindex < 0)
        cdwidget->first_imgindex = 0;
    else if (cdwidget->first_imgindex > (displaytotal - 1))
        cdwidget->first_imgindex = displaytotal - 1;

    /* Change the value of the slider to do the work of scrolling the covers */
    gtk_range_set_value(GTK_RANGE (cdwidget->cdslider), cdwidget->first_imgindex);

    return TRUE;
}

/**
 * on_cover_display_button_clicked:
 *
 * Call handler for the left and right buttons. Cause the images to
 * be cycled in the direction indicated by the button.
 *
 * @widget: button which emitted the signal
 * @data: any data needed by the function (not required)
 *
 */
static void on_cover_display_button_clicked(GtkWidget *widget, gpointer data) {
    GtkButton *button;
    const gchar *label;
    gint displaytotal;

    button = GTK_BUTTON(widget);
    label = gtk_button_get_label(button);

    if (g_str_equal(label, (gchar *) ">"))
        cdwidget->first_imgindex++;
    else
        cdwidget->first_imgindex--;

    displaytotal = g_list_length(album_key_list) - 8;

    if (displaytotal <= 0)
        return;

    /* Use the index value from the slider for the main image index */
    if (cdwidget->first_imgindex < 0)
        cdwidget->first_imgindex = 0;
    else if (cdwidget->first_imgindex > (displaytotal - 1))
        cdwidget->first_imgindex = displaytotal - 1;

    /* Change the value of the slider to do the work of scrolling the covers */
    gtk_range_set_value(GTK_RANGE (cdwidget->cdslider), cdwidget->first_imgindex);

    /* debug_albums(); */
}

/**
 * on_cover_display_slider_value_changed:
 *
 * Call handler used for cycling the cover images with the slider.
 *
 * @range: GTKHScale object used as the slider
 * @user_data: any data needed by the function (not required)
 *
 */
static void on_cover_display_slider_value_changed(GtkRange *range, gpointer user_data) {
    gint index, displaytotal;

    if (!coverart_window_valid())
        return;

    if (cdwidget->block_display_change)
        return;

    index = gtk_range_get_value(range);
    displaytotal = g_list_length(album_key_list);

    if (displaytotal <= 0)
        return;

    /* Use the index value from the slider for the main image index */
    cdwidget->first_imgindex = index;

    if (cdwidget->first_imgindex > (displaytotal - IMG_MAIN))
        cdwidget->first_imgindex = displaytotal - IMG_MAIN;

    redraw(FALSE);
}

/**
 * on_main_cover_image_clicked:
 *
 * Call handler used for displaying the tracks associated with
 * the main displayed album cover.
 *
 * @GnomeCanvas: main cd cover image canvas
 * @event: event object used to determine the event type
 * @data: any data needed by the function (not required)
 *
 */
static gint on_main_cover_image_clicked(GtkWidget *widget, GdkEvent *event, gpointer data) {
    Cover_Item *cover;
    guint mbutton;

    if (event->type != GDK_BUTTON_PRESS)
        return FALSE;

    mbutton = event->button.button;

    if (mbutton == 1) {
        Album_Item *album;
        /* Left mouse button clicked so find all tracks with displayed cover */
        cover = g_ptr_array_index(cdwidget->cdcovers, IMG_MAIN);
        /* Stop redisplay of the artwork as its already
         * in the correct location
         */
        coverart_block_change(TRUE);

        /* Select the correct track in the sorttabs */
        album = cover->album;
        g_return_val_if_fail (album, FALSE);

        /* Clear the tracks listed in the display */
        gtkpod_set_displayed_tracks(album->tracks);

        /* Turn the display change back on */
        coverart_block_change(FALSE);
    }
    else if ((mbutton == 3) && (event->button.state & GDK_SHIFT_MASK)) {
        /* Right mouse button clicked and shift pressed.
         * Go straight to edit details window
         */
        GList *tracks = coverart_get_displayed_tracks();
        gtkpod_edit_details(tracks);
    }
    else if (mbutton == 3) {
        /* Right mouse button clicked on its own so display
         * popup menu
         */
        /*int i;
         GList *tracks = coverart_get_displayed_tracks();
         for (i = 0; i < g_list_length(tracks); ++i)
         {
         Track *track;
         track = g_list_nth_data (tracks, i);
         printf ("display_coverart-main_image_clicked - Artist:%s  Album:%s  Title:%s\n", track->artist, track->album, track->title);
         }*/
        cad_context_menu_init();
    }

    return FALSE;
}

/**
 * coverart_get_track_thumb:
 *
 * Retrieve the artwork pixbuf from the given track.
 *
 * @track: Track from where the pixbuf is obtained.
 * @device: Reference to the device upon which the track is located
 * @default_img_size: If the default image must be used then this may contain a default value
 * 		for its size.
 *
 * Returns:
 * pixbuf referenced by the provided track or the pixbuf of the
 * default file if track has no cover art.
 */
static GdkPixbuf *coverart_get_track_thumb(Track *track, Itdb_Device *device, gint default_size) {
    GdkPixbuf *pixbuf = NULL;
    GdkPixbuf *image = NULL;
    ExtraTrackData *etd;
    gint w, h;
    float ratio;

    etd = track->userdata;
    g_return_val_if_fail (etd, NULL);

    image = itdb_track_get_thumbnail(track, 200, 200);
    if (image) {
        w = gdk_pixbuf_get_width(image);
        h = gdk_pixbuf_get_height(image);

        if (default_size > 0) {
            if (w > default_size || h > default_size) {
                /* need to scale the image back down to size */
                if (w == h) {
                    w = default_size;
                    h = default_size;
                }
                else if (h > w) {
                    ratio = h / w;
                    h = default_size;
                    w = (gint) (default_size / ratio);
                }
                else {
                    ratio = w / h;
                    w = default_size;
                    h = (gint) (default_size / ratio);
                }
                pixbuf = gdk_pixbuf_scale_simple(image, w, h, GDK_INTERP_BILINEAR);
            }
            else
                pixbuf = gdk_pixbuf_copy(image);

            g_object_unref(image);
        }
        else {
            pixbuf = gdk_pixbuf_scale_simple(image, DEFAULT_IMG_SIZE, DEFAULT_IMG_SIZE, GDK_INTERP_BILINEAR);
            g_object_unref(image);
        }
    }

    if (pixbuf == NULL) {
        /* Could not get a viable thumbnail so get default pixbuf */
        pixbuf = coverart_get_default_track_thumb(default_size);
    }

    return pixbuf;
}

/**
 * coverart_get_displayed_tracks:
 *
 * Get all tracks suggested by the displayed album cover.
 *
 * Returns:
 * GList containing references to all the displayed covered tracks
 */
GList *coverart_get_displayed_tracks(void) {
    Cover_Item *cover;

    cover = g_ptr_array_index(cdwidget->cdcovers, IMG_MAIN);
    g_return_val_if_fail (cover->album, NULL);
    return cover->album->tracks;
}

/**
 * coverart_get_default_track_thumb:
 *
 * Retrieve the artwork pixbuf from the default image file.
 *
 * Returns:
 * pixbuf of the default file for tracks with no cover art.
 */
static GdkPixbuf *coverart_get_default_track_thumb(gint default_img_size) {
    GdkPixbuf *pixbuf = NULL;
    GdkPixbuf *scaled = NULL;
    gdouble default_size = 140;
    GError *error = NULL;

    if (default_img_size != 0)
        default_size = (gdouble) default_img_size;

    pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), DEFAULT_COVER_ICON, 240, 0, &error);
    if (error != NULL) {
        g_warning("Error occurred loading the default file - \nCode: %d\nMessage: %s\n", error->code, error->message);

        g_return_val_if_fail(pixbuf, NULL);
    }

    scaled = gdk_pixbuf_scale_simple(pixbuf, default_size, default_size, GDK_INTERP_BILINEAR);
    g_object_unref(pixbuf);

    return scaled;
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
static void set_slider_range(gint index) {
    g_signal_handler_block(G_OBJECT(cdwidget->cdslider), slide_signal_id);

    gint slider_ubound = g_list_length(album_key_list) - IMG_TOTAL;
    if (slider_ubound < 1) {
        /* If only one album cover is displayed then slider_ubbound returns
         * 0 and causes a slider assertion error. Avoid this by disabling the
         * slider, which makes sense because only one cover is displayed.
         */
        slider_ubound = 1;
        gtk_widget_set_sensitive(GTK_WIDGET(cdwidget->cdslider), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(cdwidget->leftbutton), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(cdwidget->rightbutton), FALSE);
    }
    else {
        gtk_widget_set_sensitive(GTK_WIDGET(cdwidget->cdslider), TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(cdwidget->leftbutton), TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(cdwidget->rightbutton), TRUE);
    }

    gtk_range_set_range(GTK_RANGE (cdwidget->cdslider), 0, slider_ubound);
    if (index >= 0 && index <= slider_ubound)
        gtk_range_set_value(GTK_RANGE (cdwidget->cdslider), index);
    else
        gtk_range_set_value(GTK_RANGE (cdwidget->cdslider), 0);

    g_signal_handler_unblock(G_OBJECT(cdwidget->cdslider), slide_signal_id);
}

/**
 * coverart_select_cover
 *
 * When a track / album is selected, the artwork cover
 * is selected in the display
 *
 * @track: chosen track
 *
 */
void coverart_select_cover(Track *track) {
    gint displaytotal, index;

    /* Only select covers if the display is visible */
    if (!coverart_window_valid())
        return;

    /* Only select covers if fire display change is enabled */
    if (cdwidget->block_display_change)
        return;

    displaytotal = g_list_length(album_key_list);
    if (displaytotal <= 0)
        return;

    gchar *trk_key;
    trk_key = g_strconcat(track->artist, "_", track->album, NULL);

    /* Determine the index of the found track */
    GList *key = g_list_find_custom(album_key_list, trk_key, (GCompareFunc) compare_album_keys);
    g_return_if_fail (key);
    index = g_list_position(album_key_list, key);
    g_free(trk_key);

    /* Use the index value for the main image index */
    cdwidget->first_imgindex = index - IMG_MAIN;
    if (cdwidget->first_imgindex < 0)
        cdwidget->first_imgindex = 0;
    else if ((cdwidget->first_imgindex + IMG_TOTAL) >= displaytotal)
        cdwidget->first_imgindex = displaytotal - IMG_TOTAL;

    /* Set the index value of the slider but avoid causing an infinite
     * cover selection by blocking the event
     */
    g_signal_handler_block(cdwidget->cdslider, slide_signal_id);
    gtk_range_set_value(GTK_RANGE (cdwidget->cdslider), index);
    g_signal_handler_unblock(cdwidget->cdslider, slide_signal_id);
}

/**
 * on_drawing_area_exposed:
 *
 * Callback for the drawing area. When the drawing area is covered,
 * resized, changed etc.. This will be called the draw() function is then
 * called from this and the cairo redrawing takes place.
 *
 * @draw_area: drawing area where al the cairo drawing takes place
 * @event: gdk expose event
 *
 * Returns:
 * boolean indicating whether other handlers should be run.
 */
static gboolean on_coverart_preview_dialog_exposed(GtkWidget *drawarea, GdkEventExpose *event, gpointer data) {
    GdkPixbuf *image = data;

    /* Draw the image using cairo */
    cairo_t *cairo_context;

    /* get a cairo_t */
    cairo_context = gdk_cairo_create(gtk_widget_get_window(drawarea));
    /* set a clip region for the expose event */
    cairo_rectangle(cairo_context, event->area.x, event->area.y, event->area.width, event->area.height);
    cairo_clip(cairo_context);

    gdk_cairo_set_source_pixbuf(cairo_context, image, 0, 0);
    cairo_paint(cairo_context);

    cairo_destroy(cairo_context);
    return FALSE;
}

/**
 * display_coverart_image_dialog
 *
 * @GdkPixbuf: image
 *
 * function to load a transient dialog displaying the provided image at either
 * it maximum size or the size of the screen (whichever is smallest).
 *
 */
static void display_coverart_image_dialog(GdkPixbuf *image) {
    g_return_if_fail (image);

    GtkWidget *dialog;
    GtkWidget *drawarea;
    GtkWidget *res_label;
    GdkPixbuf *scaled = NULL;
    gchar *text;
    GtkBuilder *xml;

    xml = gtkpod_builder_xml_new(cdwidget->gladepath);
    dialog = gtkpod_builder_xml_get_widget(xml, "coverart_preview_dialog");
    drawarea = gtkpod_builder_xml_get_widget(xml, "coverart_preview_dialog_drawarea");
    res_label = gtkpod_builder_xml_get_widget(xml, "coverart_preview_dialog_res_lbl");
    g_return_if_fail (dialog);
    g_return_if_fail (drawarea);
    g_return_if_fail (res_label);

    /* Set the dialog parent */
    gtk_window_set_transient_for(GTK_WINDOW (dialog), GTK_WINDOW (gtkpod_app));

    gint pixheight = gdk_pixbuf_get_height(image);
    gint pixwidth = gdk_pixbuf_get_width(image);

    /* Set the resolution in the label */
    text = g_markup_printf_escaped(_("<b>Image Dimensions: %d x %d</b>"), pixwidth, pixheight);
    gtk_label_set_markup(GTK_LABEL (res_label), text);
    g_free(text);

    gint scrheight = gdk_screen_height() - 100;
    gint scrwidth = gdk_screen_width() - 100;

    gdouble ratio = (gdouble) pixwidth / (gdouble) pixheight;
    if (pixwidth > scrwidth) {
        pixwidth = scrwidth;
        pixheight = pixwidth / ratio;
    }

    if (pixheight > scrheight) {
        pixheight = scrheight;
        pixwidth = pixheight * ratio;
    }

    scaled = gdk_pixbuf_scale_simple(image, pixwidth, pixheight, GDK_INTERP_BILINEAR);

    /* Set the draw area's minimum size */
    gtk_widget_set_size_request(drawarea, pixwidth, pixheight);
    g_signal_connect (G_OBJECT (drawarea), "expose_event", G_CALLBACK (on_coverart_preview_dialog_exposed), scaled);

    /* Display the dialog and block everything else until the
     * dialog is closed.
     */
    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));

    /* Destroy the dialog as no longer required */

    g_object_unref(scaled);
    gtk_widget_destroy(GTK_WIDGET (dialog));
}

/**
 * coverart_display_big_artwork:
 *
 * Display a big version of the artwork in a dialog
 *
 */
void coverart_display_big_artwork() {
    Cover_Item *cover;
    ExtraTrackData *etd;
    GdkPixbuf *imgbuf = NULL;

    cover = g_ptr_array_index(cdwidget->cdcovers, IMG_MAIN);
    g_return_if_fail (cover);

    if (cover->album == NULL)
        return;

    Track *track;
    track = g_list_nth_data(cover->album->tracks, 0);
    etd = track->userdata;
    if (etd && etd->thumb_path_locale) {
        GError *error = NULL;
        imgbuf = gdk_pixbuf_new_from_file(etd->thumb_path_locale, &error);
        if (error != NULL) {
            g_error_free(error);
        }
    }

    /* Either thumb was null or the attempt at getting a pixbuf failed
     * due to invalid file. For example, some nut (like me) decided to
     * apply an mp3 file to the track as its cover file
     */
    if (imgbuf == NULL) {
        /* Could not get a viable thumbnail so get default pixbuf */
        imgbuf = coverart_get_default_track_thumb(256);
    }

    display_coverart_image_dialog(imgbuf);

    /* Unreference pixbuf if it is not pointing to
     * the album's artwork
     */
    if (cover->album->albumart == NULL)
        g_object_unref(imgbuf);
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
static gint compare_album_keys(gchar *a, gchar *b) {
    if (a == NULL)
        return -1;
    if (b == NULL)
        return -1;

    return compare_string(a, b, prefs_get_int("cad_case_sensitive"));
}

/**
 * coverart_sort_images:
 *
 * When the alphabetize function is initiated this will
 * sort the covers in the same way. Used at any point to
 * sort the covers BUT must be called after an initial coverart_display_update
 * as the latter initialises the album_key_list list
 *
 * @order: order type
 *
 */
static void coverart_sort_images(GtkSortType order) {
    if (order == SORT_NONE) {
        /* No sorting means original order so this should have been called after a coverart_display_update (TRUE)
         * when the TRUE means the tracks were freshly established from the playlist and the hash and key_list
         * recreated.
         */
        return;
    }
    else {
        album_key_list = g_list_sort(album_key_list, (GCompareFunc) compare_album_keys);
    }

    if (order == GTK_SORT_DESCENDING) {
        album_key_list = g_list_reverse(album_key_list);
    }
}

/**
 *
 * remove_track_from_album:
 *
 * Removes track from an album item and removes the latter
 * if it no longer has any tracks in it.
 *
 * @album: album to be checked for removal.
 * @track: track to be removed from the Album_Item
 * @key: string concatentation of the artist_album of track. Key for hash
 * @index: position of the key in the album key list
 * @keylistitem: the actual GList item in the album key list
 */
static void remove_track_from_album(Album_Item *album, Track *track, gchar *key, gint index, GList *keylistitem) {
    album->tracks = g_list_remove(album->tracks, track);
    if (g_list_length(album->tracks) == 0) {
        /* No more tracks related to this album item so delete it */
        gboolean delstatus = g_hash_table_remove(album_hash, key);
        if (!delstatus)
            gtkpod_warning(_("Failed to remove the album from the album hash store."));
        /*else
         printf("Successfully removed album\n");
         */
        album_key_list = g_list_remove_link(album_key_list, keylistitem);

        if (index < (cdwidget->first_imgindex + IMG_MAIN) && index > IMG_MAIN) {
            /* index of track is less than visible cover's indexes so subtract 1 from
             * first img index. Will mean that when deleteing album item then
             * set covers will be called at the correct position.
             *
             * However, index must be greater than IMG_MAIN else a NULL track will
             * become the IMG_MAIN tracks displayed.
             */
            cdwidget->first_imgindex--;
        }
    }
}

/**
 * coverart_set_cover_from_file:
 *
 * Add a cover to the displayed track by setting it from a
 * picture file.
 *
 */
void coverart_set_cover_from_file() {
    gchar *filename;
    Track *track;
    Cover_Item *cover;
    GList *tracks;

    if (!coverart_window_valid())
        return;

    filename = fileselection_get_cover_filename();

    if (filename) {
        cover = g_ptr_array_index(cdwidget->cdcovers, IMG_MAIN);
        tracks = cover->album->tracks;

        while (tracks) {
            track = tracks->data;

            if (gp_track_set_thumbnails(track, filename))
                data_changed(track->itdb);

            tracks = tracks->next;
        }
        /* Nullify so that the album art is picked up from the tracks again */
        g_object_unref(cover->album->albumart);
        cover->album->albumart = NULL;
        if (cover->album->scaled_art != NULL) {
            g_object_unref(cover->album->scaled_art);
            cover->album->scaled_art = NULL;
        }
    }

    g_free(filename);

    redraw(FALSE);
}

/**
 * coverart_get_background_display_color:
 *
 * Used by coverart draw functions to determine the background color
 * of the coverart display, which is selected from the preferences.
 *
 */
GdkColor *coverart_get_background_display_color() {
    gchar *hex_string;
    GdkColor *color;

    if (album_key_list == NULL)
        hex_string = "#FFFFFF";
    else if (!prefs_get_string_value("coverart_display_bg_color", NULL))
        hex_string = "#000000";
    else
        prefs_get_string_value("coverart_display_bg_color", &hex_string);

    color = convert_hexstring_to_gdk_color(hex_string);
    return color;
}

/**
 * coverart_get_foreground_display_color:
 *
 * Used by coverart draw functions to determine the foreground color
 * of the coverart display, which is selected from the preferences. The
 * foreground color refers to the color used by the artist and album text.
 *
 */
GdkColor *coverart_get_foreground_display_color() {
    gchar *hex_string;
    GdkColor *color;

    if (album_key_list == NULL)
        hex_string = "#000000";
    else if (!prefs_get_string_value("coverart_display_bg_color", NULL))
        hex_string = "#FFFFFF";
    else
        prefs_get_string_value("coverart_display_fg_color", &hex_string);

    color = convert_hexstring_to_gdk_color(hex_string);
    return color;
}

/**
 *
 * free_album:
 *
 * Destroy an album struct once no longer needed.
 *
 */
static void free_album(Album_Item *album) {
    if (album != NULL) {
        if (album->tracks) {
            g_list_free(album->tracks);
        }

        g_free(album->albumname);
        g_free(album->artist);

        if (album->albumart)
            g_object_unref(album->albumart);

        if (album->scaled_art)
            g_object_unref(album->scaled_art);
    }
}

static gboolean on_parent_delete_event(GtkWidget *widget, GdkEvent *event, gpointer data) {
    destroy_coverart_display();
    return FALSE;
}

/**
 *
 * destroy_coverart_display
 *
 * destroy the CD Widget and free everything currently
 * in memory.
 */
void destroy_coverart_display() {
    gint i;
    g_signal_handler_disconnect(cdwidget->leftbutton, lbutton_signal_id);
    g_signal_handler_disconnect(cdwidget->rightbutton, rbutton_signal_id);
    g_signal_handler_disconnect(cdwidget->cdslider, slide_signal_id);
    g_signal_handler_disconnect(cdwidget->contentpanel, contentpanel_signal_id);
    //    g_signal_handler_disconnect(gtkpod_window, window_signal_id);

    /* Components not freed as they are part of the glade xml file */
    cdwidget->leftbutton = NULL;
    cdwidget->rightbutton = NULL;
    cdwidget->cdslider = NULL;
    cdwidget->contentpanel = NULL;
    cdwidget->canvasbox = NULL;
    cdwidget->controlbox = NULL;
    cdwidget->parent = NULL;

    /* native variables rather than references so should be destroyed when freed */
    cdwidget->first_imgindex = 0;
    cdwidget->block_display_change = FALSE;

    Cover_Item *cover;
    for (i = 0; i < IMG_TOTAL; ++i) {
        cover = g_ptr_array_index(cdwidget->cdcovers, i);
        /* Nullify pointer to album reference. Will be freed below */
        cover->album = NULL;
    }

    g_ptr_array_free(cdwidget->cdcovers, TRUE);

    /* Destroying canvas should destroy the background and cvrtext */
    gtk_widget_destroy(GTK_WIDGET(cdwidget->draw_area));

    /* Remove all null tracks before using it to destroy the hash table */
    album_key_list = g_list_remove_all(album_key_list, NULL);
    g_hash_table_foreach_remove(album_hash, (GHRFunc) gtk_true, NULL);
    g_hash_table_destroy(album_hash);
    g_list_free(album_key_list);

    g_free(cdwidget);
    cdwidget = NULL;
}

/**
 * dnd_coverart_drag_drop:
 *
 * Used by the drag and drop of a jpg. When a drop is
 * made, this determines whether the drop is valid
 * then requests the data from the source widget.
 *
 */
static gboolean dnd_coverart_drag_drop(GtkWidget *widget, GdkDragContext *drag_context, gint x, gint y, guint time, gpointer user_data) {
    GdkAtom target;
    target = gtk_drag_dest_find_target(widget, drag_context, NULL);

    if (target != GDK_NONE) {
        gtk_drag_get_data(widget, drag_context, target, time);
        return TRUE;
    }

    return FALSE;
}

/**
 * dnd_coverart_drag_motion:
 *
 * Used by the drag and drop of a jpg. While the jpg is being
 * dragged, this reports to the source widget whether it is an
 * acceptable location to allow a drop.
 *
 */
static gboolean dnd_coverart_drag_motion(GtkWidget *widget, GdkDragContext *dc, gint x, gint y, guint time, gpointer user_data) {
    GdkAtom target;
    iTunesDB *itdb;
    ExtraiTunesDBData *eitdb;

    itdb = gp_get_selected_itdb();
    /* no drop is possible if no playlist/repository is selected */
    if (itdb == NULL) {
        gdk_drag_status(dc, 0, time);
        return FALSE;
    }

    eitdb = itdb->userdata;
    g_return_val_if_fail (eitdb, FALSE);
    /* no drop is possible if no repository is loaded */
    if (!eitdb->itdb_imported) {
        gdk_drag_status(dc, 0, time);
        return FALSE;
    }

    target = gtk_drag_dest_find_target(widget, dc, NULL);
    /* no drop possible if no valid target can be found */
    if (target == GDK_NONE) {
        gdk_drag_status(dc, 0, time);
        return FALSE;
    }

    gdk_drag_status(dc, gdk_drag_context_get_suggested_action(dc), time);

    return TRUE;
}

/**
 * dnd_coverart_drag_data_received:
 *
 * Used by the drag and drop of a jpg. When the drop is performed, this
 * acts on the receipt of the data from the source widget and applies
 * the jpg to the track.
 *
 */
static void dnd_coverart_drag_data_received(GtkWidget *widget, GdkDragContext *dc, gint x, gint y, GtkSelectionData *data, guint info, guint time, gpointer user_data) {
    g_return_if_fail (widget);
    g_return_if_fail (dc);
    g_return_if_fail (data);
    g_return_if_fail (gtk_selection_data_get_data(data));
    g_return_if_fail (gtk_selection_data_get_length(data) > 0);

    /* mozilla bug 402394 */

#if DEBUG
    printf ("data length = %d\n", gtk_selection_data_get_length(data->length));
    printf ("data->data = %s\n", gtk_selection_data_get_data(data));
#endif

    Cover_Item *cover;
    GList *tracks;
    gchar *url = NULL;
    Fetch_Cover *fcover;
    Track *track;
    gchar *filename = NULL;
    gboolean image_status = FALSE;
    gchar *image_error = NULL;
    /* For use with DND_IMAGE_JPEG */
    GdkPixbuf *pixbuf;
    GError *error = NULL;

    /* Find the display cover item in the cover display */
    cover = g_ptr_array_index(cdwidget->cdcovers, IMG_MAIN);
    if (!cover) {
        /* looks like there are no covers yet something got dragged into it */
        gtk_drag_finish(dc, FALSE, FALSE, time);
        return;
    }

    tracks = cover->album->tracks;

    switch (info) {
    case DND_IMAGE_JPEG:
#if DEBUG
        printf ("Using DND_IMAGE_JPEG\n");
#endif
        pixbuf = gtk_selection_data_get_pixbuf(data);
        if (pixbuf != NULL) {
            /* initialise the url string with a safe value as not used if already have image */
            url = "local image";
            /* Initialise a fetchcover object */
            fcover = fetchcover_new(url, tracks);
            coverart_block_change(TRUE);

            /* find the filename with which to save the pixbuf to */
            if (fetchcover_select_filename(fcover)) {
                filename = g_build_filename(fcover->dir, fcover->filename, NULL);
                if (!gdk_pixbuf_save(pixbuf, filename, "jpeg", &error, NULL)) {
                    /* Save failed for some reason */
                    if (error->message)
                        fcover->err_msg = g_strdup(error->message);
                    else
                        fcover->err_msg = "Saving image to file failed. No internal error message was returned.";

                    g_error_free(error);
                }
                else {
                    /* Image successfully saved */
                    image_status = TRUE;
                }
            }
            /* record any errors and free the fetchcover */
            if (fcover->err_msg != NULL)
                image_error = g_strdup(fcover->err_msg);

            free_fetchcover(fcover);
            g_object_unref(pixbuf);
            coverart_block_change(FALSE);
        }
        else {
            /* despite the data being of type image/jpeg, the pixbuf is NULL */
            image_error = "jpeg data flavour was used but the data did not contain a GdkPixbuf object";
        }
        break;
    case DND_TEXT_PLAIN:
#if DEBUG
        printf ("Defaulting to using DND_TEXT_PLAIN\n");
#endif

#ifdef HAVE_CURL
        /* initialise the url string with the data from the dnd */
        url = g_strdup ((gchar *) gtk_selection_data_get_data(data));
        /* Initialise a fetchcover object */
        fcover = fetchcover_new (url, tracks);
        coverart_block_change (TRUE);

        if (fetchcover_net_retrieve_image (fcover))
        {
#if DEBUG
            printf ("Successfully retrieved\n");
            printf ("Url of fetch cover: %s\n", fcover->url->str);
            printf ("filename of fetch cover: %s\n", fcover->filename);
#endif

            filename = g_build_filename(fcover->dir, fcover->filename, NULL);
            image_status = TRUE;
        }

        /* record any errors and free the fetchcover */
        if (fcover->err_msg != NULL)
        image_error = g_strdup(fcover->err_msg);

        free_fetchcover (fcover);
        coverart_block_change (FALSE);
#else
        image_error = g_strdup(_("Item had to be downloaded but gtkpod was not compiled with curl."));
        image_status = FALSE;
#endif
    }

    if (!image_status || filename == NULL) {
        gtkpod_warning(_("Error occurred dropping an image onto the coverart display: %s\n"), image_error);

        if (image_error)
            g_free(image_error);
        if (filename)
            g_free(filename);

        gtk_drag_finish(dc, FALSE, FALSE, time);
        return;
    }

    while (tracks) {
        track = tracks->data;

        if (gp_track_set_thumbnails(track, filename))
            data_changed(track->itdb);

        tracks = tracks->next;
    }
    /* Nullify so that the album art is picked up from the tracks again */
    cover->album->albumart = NULL;
    if (cover->album->scaled_art != NULL) {
        g_object_unref(cover->album->scaled_art);
        cover->album->scaled_art = NULL;
    }

    redraw(FALSE);

    if (image_error)
        g_free(image_error);

    g_free(filename);

    gtkpod_statusbar_message(_("Successfully set new coverart for selected tracks"));
    gtk_drag_finish(dc, FALSE, FALSE, time);
    return;
}

/**
 * convert_hexstring_to_gdk_color:
 *
 * Convert a #FFEEFF string to a GdkColor.
 * Returns a freshly allocated GdkColor that should be freed.
 *
 * @GdkColor
 */
static GdkColor *convert_hexstring_to_gdk_color(gchar *hexstring) {
    GdkColor *colour;
    GdkColormap *map;
    map = gdk_colormap_get_system();

    colour = g_malloc(sizeof(GdkColor));

    if (!gdk_color_parse(hexstring, colour))
        return NULL;

    if (!gdk_colormap_alloc_color(map, colour, FALSE, TRUE))
        return NULL;

    return colour;
}

void coverart_display_update_cb(GtkPodApp *app, gpointer pl, gpointer data) {
    if (!coverart_window_valid())
        return;

    coverart_display_update(TRUE);
}

void coverart_display_track_removed_cb(GtkPodApp *app, gpointer tk, gpointer data) {
    Track *old_track = tk;
    if (!coverart_window_valid())
        return;

    coverart_track_changed(old_track, COVERART_REMOVE_SIGNAL);
    redraw(FALSE);
}

void coverart_display_set_tracks_cb(GtkPodApp *app, gpointer tks, gpointer data) {
    GList *tracks = tks;
    if (!coverart_window_valid())
        return;

    if (tracks)
        coverart_select_cover(tracks->data);

    redraw(FALSE);
}

void coverart_display_track_updated_cb(GtkPodApp *app, gpointer tk, gpointer data) {
    Track *track = tk;
    if (!coverart_window_valid())
        return;

    coverart_track_changed(track, COVERART_CHANGE_SIGNAL);
    redraw(FALSE);
}

void coverart_display_track_added_cb(GtkPodApp *app, gpointer tk, gpointer data) {
    Track *track = tk;
    if (!cdwidget || !cdwidget->draw_area || !gtk_widget_get_window(GTK_WIDGET(cdwidget->draw_area)))
        return;

    coverart_track_changed(track, COVERART_CREATE_SIGNAL);
    redraw(FALSE);
}

