/*
|  Copyright (C) 2007 P.G. Richardson <phantom_sf at users.sourceforge.net>
|  Part of the gtkpod project.
|
|  URL: http://www.gtkpod.org/
|  URL: http://gtkpod.sourceforge.net/
|
|  Gtkpod is free software; you can redistribute it and/or modify
|  it under the terms of the GNU General Public License as published by
|  the Free Software Foundation; either version 2 of the License, or
|  (at your option) any later version.
|
|  Gtkpod is distributed in the hope that it will be useful,
|  but WITHOUT ANY WARRANTY; without even the implied warranty of
|  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
|  GNU General Public License for more details.
|
|  You should have received a copy of the GNU General Public License
|  along with gtkpod; if not, write to the Free Software
|  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
|
|  iTunes and iPod are trademarks of Apple
|
|  This product is not supported/written/published by Apple!
|
|  $Id$
*/

#ifndef __DISPLAY_COVERART_H__
#define __DISPLAY_COVERART_H__

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <cairo/cairo.h>
#include "libgtkpod/itdb.h"

#define IMG_MAIN 4
#define IMG_NEXT 1
#define IMG_PREV 2
#define IMG_TOTAL 9
#define BORDER 10
#define DEFAULT_IMG_SIZE 140

#define COVERART_REMOVE_SIGNAL 1
#define COVERART_CREATE_SIGNAL 2
#define COVERART_CHANGE_SIGNAL 3

typedef struct {
	GList *tracks;
	gchar *albumname;
	gchar *artist;
	GdkPixbuf *albumart;
	GdkPixbuf *scaled_art;
} Album_Item;

typedef struct {
	/* Data */
	Album_Item *album;
	gdouble img_x;
	gdouble img_y;
	gdouble img_width;
	gdouble img_height;
} Cover_Item;

typedef struct {
    GladeXML *xml;

  /* Gtk widgets */
    GtkWidget *parent;
	GtkWidget *contentpanel;
	GtkWidget *canvasbox;
	GtkWidget *controlbox;
	GtkButton *leftbutton;
	GtkHScale *cdslider;
	GtkButton *rightbutton;

	/* Utility data */
	GPtrArray *cdcovers;
	gint first_imgindex;
	gboolean block_display_change;

	/* Drawing area related widgets */
	GtkWidget *draw_area;
} CD_Widget;

extern const gchar *DISPLAY_COVER_SHOW;

/**
 * coverart_init_display:
 *
 * Initialise the boxes and canvases of the coverart_display.
 *
 */
void coverart_init_display(GtkWidget *parent);

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
void coverart_display_update (gboolean clear_track_list);

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
void coverart_track_changed (Track *track, gint signal);

/**
 * coverart_block_change:
 *
 * Select covers events can be switched off when automatic
 * selections of tracks are taking place.
 *
 * @val: indicating whether to block or unblock select cover events
 *
 */
void coverart_block_change (gboolean val);

/**
 * coverart_set_cover_from_file:
 *
 * Add a cover to the displayed track by setting it from a
 * picture file.
 *
 */
void coverart_set_cover_from_file ();

/**
 * coverart_get_displayed_tracks:
 *
 * Get all tracks suggested by the displayed album cover.
 *
 * Returns:
 * GList containing references to all the displayed covered tracks
 */
GList *coverart_get_displayed_tracks (void);

/**
 * coverart_display_big_artwork:
 *
 * Display a big version of the artwork in a dialog
 *
 */
void coverart_display_big_artwork ();

/**
 * coverart_select_cover
 *
 * When a track / album is selected, the artwork cover
 * is selected in the display
 *
 * @track: chosen track
 *
 */
void coverart_select_cover (Itdb_Track *track);

/**
 * coverart_get_background_display_color:
 *
 * Used by coverart draw functions to determine the background color
 * of the coverart display, which is selected from the preferences.
 *
 */
GdkColor *coverart_get_background_display_color ();

/**
 * coverart_get_foreground_display_color:
 *
 * Used by coverart draw functions to determine the foreground color
 * of the coverart display, which is selected from the preferences. The
 * foreground color refers to the color used by the artist and album text.
 *
 */
GdkColor *coverart_get_foreground_display_color ();

/**
 *
 * destroy_coverart_display
 *
 * destroy the CD Widget and free everything currently
 * in memory.
 */
void destroy_coverart_display();

void coverart_display_update_cb(GtkPodApp *app, gpointer pl, gpointer data);
void coverart_display_track_removed_cb(GtkPodApp *app, gpointer tk, gpointer data);
void coverart_display_set_tracks_cb(GtkPodApp *app, gpointer tks, gpointer data);
void coverart_display_track_updated_cb(GtkPodApp *app, gpointer tk, gpointer data);
void coverart_display_track_added_cb(GtkPodApp *app, gpointer tk, gpointer data);

#endif
