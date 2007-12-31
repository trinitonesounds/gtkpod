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
#include <cairo/cairo.h>

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
  /* Gtk widgets */
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


GList *coverart_get_displayed_tracks (void);
GdkPixbuf *coverart_get_default_track_thumb (gint default_img_size);
void coverart_init (gchar *progpath);
void coverart_display_big_artwork ();
void coverart_select_cover (Itdb_Track *track);
void coverart_display_update (gboolean clear_track_list);
void coverart_track_changed (Track *track, gint signal);
void coverart_clear_images ();
void coverart_block_change (gboolean val);
void coverart_init_display ();
GdkPixbuf *coverart_get_track_thumb (Track *track, Itdb_Device *device, gint default_img_size);
void coverart_set_cover_from_file ();
GdkColor *coverart_get_background_display_color ();
GdkColor *coverart_get_foreground_display_color ();
#endif
