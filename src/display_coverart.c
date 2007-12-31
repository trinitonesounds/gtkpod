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

#include "display_private.h"
#include "display.h"
#include "itdb.h"
#include "prefs.h"
#include "display_coverart.h"
#include "context_menus.h"
#include "details.h"
#include "fileselection.h"
#include "fetchcover.h"
#include "info.h"
#include "gdk/gdk.h"
#include <pango/pangocairo.h>
#include <math.h>

#define DEBUG 0

/* Declarations */
static void free_album (Album_Item *album);
static void free_CDWidget ();
static gint compare_album_keys (gchar *a, gchar *b);
static void set_display_window_dimensions ();
static void set_highlight (Cover_Item *cover, gint index, cairo_t *cr);
static void set_shadow_reflection (Cover_Item *cover, cairo_t *cr);
static void remove_track_from_album (Album_Item *album, Track *track, gchar *key, gint index, GList *keylistitem);

/* callback declarations */
static gboolean on_gtkpod_window_configure (GtkWidget *widget, GdkEventConfigure *event, gpointer data);
static void on_cover_display_button_clicked (GtkWidget *widget, gpointer data);
static gboolean on_contentpanel_scroll_wheel_turned (GtkWidget *widget, GdkEventScroll *event, gpointer user_data);
static gint on_main_cover_image_clicked (GtkWidget *widget, GdkEvent *event, gpointer data);
static void on_cover_display_slider_value_changed (GtkRange *range, gpointer user_data);

/* dnd declarations */
static gboolean dnd_coverart_drag_drop(GtkWidget *widget, GdkDragContext *drag_context, gint x, gint y, guint time, gpointer user_data);
static void dnd_coverart_drag_data_received(GtkWidget *widget, GdkDragContext *dc, gint x, gint y, GtkSelectionData *data, guint info, guint time, gpointer user_data);
static gboolean dnd_coverart_drag_motion (GtkWidget *widget, GdkDragContext *dc, gint x, gint y, guint time, gpointer user_data);

static void set_cover_dimensions (Cover_Item *cover, int cover_index, gdouble img_width, gdouble img_height);
static void coverart_sort_images (GtkSortType order);
static void set_slider_range (gint index);
static GdkColor *convert_hexstring_to_gdk_color (gchar *hexstring);
static gboolean on_drawing_area_exposed (GtkWidget *draw_area, GdkEventExpose *event);
static void draw (cairo_t *cairo_context);
static void redraw (gboolean force_pixbuf_update);

/* Prefs keys */
const gchar *KEY_DISPLAY_COVERART="display_coverart";

/* The structure that holds values used throughout all the functions */
static CD_Widget *cdwidget = NULL;
/* The backing hash for the albums and its associated key list */
static GHashTable *album_hash;
static GList *album_key_list;
/* Dimensions used for the canvas */
static gint WIDTH;
static gint HEIGHT;
static gint DEFAULT_WIDTH;
static gint DEFAULT_HEIGHT;
/* Path of the png file used for albums without cd covers */
static gchar *DEFAULT_FILE;
/* Flag set to force an update of covers if a modification has been made */
static gboolean force_pixbuf_covers = FALSE;
/* signal handler id for the components */
static gulong slide_signal_id;
static gulong rbutton_signal_id;
static gulong lbutton_signal_id;
static gulong window_signal_id;
static gulong contentpanel_signal_id;

static GtkTargetEntry coverart_drop_types [] = {
		/* Konqueror supported flavours */
		{ "image/jpeg", 0, DND_IMAGE_JPEG },
		
		/* Fallback flavours */
		{ "text/plain", 0, DND_TEXT_PLAIN },
		{ "STRING", 0, DND_TEXT_PLAIN }
};

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
			album  = g_hash_table_lookup (album_hash, key);
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

/**
 * coverart_init:
 *
 * Initialises the image file used if an album has no cover. This
 * needs to be loaded early as it uses the path of the binary
 * to determine where to load the file from, in the same way as
 * main() determines where to load the glade file from.
 *
 * @progpath: path of the gtkpod binary being loaded.
 *  
 */
void coverart_init (gchar *progpath)
{
	gchar *progname;
	
	progname = g_find_program_in_path (progpath);
	static const gchar *SEPsrcSEPgtkpod = G_DIR_SEPARATOR_S "src" G_DIR_SEPARATOR_S "gtkpod";

	if (!g_path_is_absolute (progname))
	{
		gchar *cur_dir = g_get_current_dir ();
		gchar *prog_absolute;

		if (g_str_has_prefix (progname, "." G_DIR_SEPARATOR_S))
			prog_absolute = g_build_filename (cur_dir, progname+2, NULL);
		else
			prog_absolute = g_build_filename (cur_dir, progname, NULL);
	  		
		g_free (progname);
		g_free (cur_dir);
		progname = prog_absolute;
	}

	if (g_str_has_suffix (progname, SEPsrcSEPgtkpod))
	{
		gchar *suffix = g_strrstr (progname, SEPsrcSEPgtkpod);
		
		if (suffix)
		{
			*suffix = 0;
			DEFAULT_FILE = g_build_filename (progname, "data", "default-cover.png", NULL);
		}
	}
    
	g_free (progname);
	if (DEFAULT_FILE && !g_file_test (DEFAULT_FILE, G_FILE_TEST_EXISTS))
	{
		g_free (DEFAULT_FILE);
		DEFAULT_FILE = NULL;
	}
  
  if (!DEFAULT_FILE)
  {
		DEFAULT_FILE = g_build_filename (PACKAGE_DATA_DIR, PACKAGE, "data", "default-cover.png", NULL);
  }
}

/**
 * coverart_init_display:
 *
 * Initialise the boxes and canvases of the coverart_display.
 *  
 */
void coverart_init_display ()
{
	/* Alway initialise these buttons whether the coverart is displayed or not as these are the
	 * up/down buttons on the display window and should be properly initialised
	 */
	GtkWidget *upbutton = gtkpod_xml_get_widget (main_window_xml, "cover_up_button");
	GtkWidget *downbutton = gtkpod_xml_get_widget (main_window_xml, "cover_down_button");
	GtkWidget *lbutton = gtkpod_xml_get_widget (main_window_xml, "cover_display_leftbutton");
	GtkWidget *rbutton = gtkpod_xml_get_widget (main_window_xml, "cover_display_rightbutton");
	GtkWidget *slider = gtkpod_xml_get_widget (main_window_xml, "cover_display_scaler");
		
	/* show/hide coverart display -- default to show */
	if (prefs_get_int (KEY_DISPLAY_COVERART))
	{
		gtk_widget_hide (upbutton);
		gtk_widget_show (downbutton);
		gtk_widget_show (lbutton);
		gtk_widget_show (rbutton);
		gtk_widget_show (slider);
		if (cdwidget != NULL)
			gtk_widget_show_all (cdwidget->contentpanel);
	}
	else
	{
		gtk_widget_show (upbutton);
		gtk_widget_hide (downbutton);
		gtk_widget_hide (lbutton);
		gtk_widget_hide (rbutton);
		gtk_widget_hide (slider);
		if (cdwidget != NULL)
			gtk_widget_hide_all (cdwidget->contentpanel);
		return;
	}
			
	cdwidget = g_new0(CD_Widget, 1);
	
	cdwidget->canvasbox = gtkpod_xml_get_widget (main_window_xml, "cover_display_canvasbox"); 
	cdwidget->contentpanel = gtkpod_xml_get_widget (main_window_xml, "cover_display_window");
	cdwidget->controlbox = gtkpod_xml_get_widget (main_window_xml, "cover_display_controlbox");
	cdwidget->leftbutton = GTK_BUTTON (gtkpod_xml_get_widget (main_window_xml, "cover_display_leftbutton"));
	cdwidget->rightbutton = GTK_BUTTON (gtkpod_xml_get_widget (main_window_xml, "cover_display_rightbutton"));
	cdwidget->cdslider = GTK_HSCALE (gtkpod_xml_get_widget (main_window_xml, "cover_display_scaler"));
	/* create a new drawing area */
	cdwidget->draw_area = gtk_drawing_area_new();
	cdwidget->cdcovers = g_ptr_array_sized_new (IMG_TOTAL);
			
	g_return_if_fail (cdwidget->contentpanel);
	g_return_if_fail (cdwidget->canvasbox);
	g_return_if_fail (cdwidget->controlbox);
	g_return_if_fail (cdwidget->leftbutton);
	g_return_if_fail (cdwidget->rightbutton);
	g_return_if_fail (cdwidget->cdslider);
	g_return_if_fail (cdwidget->draw_area);
		
  /* Initialise the album hash backing store */
  album_hash = g_hash_table_new_full ( g_str_hash,
  																																			g_str_equal,
  																																			(GDestroyNotify) g_free,
  																																			(GDestroyNotify) free_album);
	album_key_list = NULL;
	set_display_window_dimensions ();

	gint i;
	Cover_Item *cover;			
	for(i = 0; i < IMG_TOTAL; ++i)
	{
		cover = g_new0(Cover_Item, 1);	
		g_ptr_array_add(cdwidget->cdcovers, cover);
		cover = NULL;
	}
	gtk_box_pack_start_defaults (GTK_BOX(cdwidget->canvasbox), GTK_WIDGET(cdwidget->draw_area));
				
	/* create the expose event for the drawing area */
	g_signal_connect (G_OBJECT (cdwidget->draw_area), "expose_event",  G_CALLBACK (on_drawing_area_exposed), NULL);
	gtk_widget_add_events (cdwidget->draw_area, GDK_BUTTON_PRESS_MASK);
	/* set up some callback events on the main scaled image */
	g_signal_connect(GTK_OBJECT(cdwidget->draw_area), "button-press-event", GTK_SIGNAL_FUNC(on_main_cover_image_clicked), NULL);
	
	/* Dnd destinaton for foreign image files */
	gtk_drag_dest_set (
			cdwidget->canvasbox, 
			0, 
			coverart_drop_types, 
			TGNR (coverart_drop_types), 
			GDK_ACTION_COPY|GDK_ACTION_MOVE);

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
	
	window_signal_id = g_signal_connect (gtkpod_window, "configure_event", 
  		G_CALLBACK (on_gtkpod_window_configure), NULL);
  
  gtk_widget_show_all (cdwidget->contentpanel);
  		
	coverart_block_change (FALSE);
}

/**
 * set_display_window_dimensions:
 *
 * Initialises the display component width and height.
 * Sets the podpane's paned position value too.
 */
static void set_display_window_dimensions ()
{
	GtkWidget *podpane;
		  		
	g_object_get (gtkpod_window,
								"default_height", &HEIGHT,
								NULL);
		
	HEIGHT = HEIGHT / 2.5;
	DEFAULT_HEIGHT = HEIGHT;
	
	podpane = gtkpod_xml_get_widget (main_window_xml, "paned0");	
	g_return_if_fail (podpane);

	WIDTH = HEIGHT;	
	gtk_paned_set_position (GTK_PANED(podpane), WIDTH);
	DEFAULT_WIDTH = WIDTH;
	
	gtk_widget_set_size_request(GTK_WIDGET(cdwidget->canvasbox), WIDTH, HEIGHT);
	/* set the size of the drawing area */
	gtk_widget_set_size_request (GTK_WIDGET(cdwidget->draw_area), WIDTH, HEIGHT);	
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
void coverart_block_change (gboolean val)
{
  if (GTK_WIDGET_REALIZED(gtkpod_window))
  {
      if (val)
      {
	  GdkCursor *cursor = gdk_cursor_new (GDK_WATCH);
	  gdk_window_set_cursor (gtkpod_window->window, cursor);
	  gdk_cursor_unref (cursor);
      }
      else
	  gdk_window_set_cursor (gtkpod_window->window, NULL);
  }

  if (cdwidget != NULL)
      cdwidget->block_display_change = val;
}

static void draw_string (cairo_t *cairo_context,
						 const gchar *text,
						 gdouble x,
						 gdouble y)
{
	static PangoFontDescription *desc = NULL;
	GdkColor *color = coverart_get_foreground_display_color ();
	PangoLayout *layout;
	PangoRectangle extents;
	
	gdouble r = ((gdouble) (color->red >> 8)) / 255; 
	gdouble g = ((gdouble) (color->green >>8)) / 255; 
	gdouble b = ((gdouble) (color->blue >> 8)) / 255;
	g_free (color);
	
	if(!desc)
	{
		desc = pango_font_description_from_string ("Sans Bold 9");
	}

	layout = pango_cairo_create_layout (cairo_context);
	pango_layout_set_text (layout, text, -1);
	pango_layout_set_font_description (layout, desc);
	cairo_set_source_rgb (cairo_context, r, g, b);
	pango_layout_get_pixel_extents (layout, NULL, &extents);
	
	cairo_move_to (cairo_context,
				   x + extents.x - (extents.width / 2),
				   y + extents.y - (extents.height / 2));
	
	pango_cairo_show_layout (cairo_context, layout);
	
	g_object_unref (layout);
}

static void draw (cairo_t *cairo_context)
{
	gint cover_index[] = {0, 8, 1, 7, 2, 6, 3, 5, 4};
	/* Draw the background */
	GdkColor *color = coverart_get_background_display_color ();
	gdouble r = ((gdouble) (color->red >> 8)) / 255; 
	gdouble g = ((gdouble) (color->green >>8)) / 255; 
	gdouble b = ((gdouble) (color->blue >> 8)) / 255;
	cairo_save (cairo_context);
	cairo_set_source_rgb (cairo_context, r, 	g, b);
	cairo_set_operator (cairo_context, CAIRO_OPERATOR_SOURCE);
	cairo_paint (cairo_context);
	cairo_restore (cairo_context);
	g_free (color);
	
	Album_Item *album;
	gint i, album_index;
	Cover_Item *cover;
	gchar *key;
	
	for(i = 0; i < IMG_TOTAL; ++i)
	{
		cover = g_ptr_array_index(cdwidget->cdcovers, cover_index[i]);
		album_index = cdwidget->first_imgindex + cover_index[i];
		
		/* Get the key from the key list appropriate to the index
		 * provided by the first image index property
		 */
		key = g_list_nth_data (album_key_list, album_index);
	  
		if (key == NULL)
			continue;
		
		/* Find the Album Item appropriate to the key */
		album  = g_hash_table_lookup (album_hash, key);
		cover->album = album;
		
		if (force_pixbuf_covers)
		{
			gdk_pixbuf_unref (album->albumart);
			album->albumart = NULL;
		}
		
		Track *track;
		if (album->albumart == NULL)
		{
			track = g_list_nth_data (album->tracks, 0);
			album->albumart = coverart_get_track_thumb (track, track->itdb->device, DEFAULT_IMG_SIZE);				
		}
		
		/* Set the x, y, height and width of the CD cover */
		set_cover_dimensions (
					cover,
					cover_index[i],
					gdk_pixbuf_get_width (album->albumart),
					gdk_pixbuf_get_height (album->albumart));
		
		/* Set the Cover */
		GdkPixbuf *scaled;
		scaled = gdk_pixbuf_scale_simple (
				album->albumart, 
				cover->img_width, 
				cover->img_height, 
				GDK_INTERP_BILINEAR);
		cairo_rectangle (
								cairo_context, 
								cover->img_x,
								cover->img_y,
								cover->img_width,
								cover->img_height);
		gdk_cairo_set_source_pixbuf (
				cairo_context,
				scaled,
				cover->img_x,
				cover->img_y);
		cairo_fill (cairo_context);
		
		/* Draw a black line around the cd cover */
		cairo_move_to (cairo_context, cover->img_x, cover->img_y);
		cairo_rel_line_to (cairo_context, cover->img_width, 0);
		cairo_rel_line_to (cairo_context, 0, cover->img_height);
		cairo_rel_line_to (cairo_context, -(cover->img_width), 0);
		cairo_close_path (cairo_context);
		cairo_set_line_width (cairo_context, 1);
		cairo_set_source_rgb (cairo_context, 0, 0, 0);
		cairo_stroke (cairo_context);
		
		/* Display the highlight */
		set_highlight (cover, cover_index[i], cairo_context);
				
		 /* flip image vertically to create reflection */
		GdkPixbuf *reflection;
		reflection = gdk_pixbuf_flip (scaled, FALSE);
		cairo_rectangle (
								cairo_context, 
								cover->img_x,
								cover->img_y + cover->img_height + 2,
								cover->img_width,
								cover->img_height);
		gdk_cairo_set_source_pixbuf (
						cairo_context,
						reflection,
						cover->img_x,
						cover->img_y + cover->img_height + 2);
		cairo_fill (cairo_context);
			    	
		gdk_pixbuf_unref (reflection);
		gdk_pixbuf_unref (scaled);
		
		/* Set the reflection shadow */
		set_shadow_reflection (cover, cairo_context);
		
		cairo_save(cairo_context);
		/* Set the text if the index is the central image cover */
		if (cover_index[i] == IMG_MAIN)
		{
			draw_string (cairo_context, album->artist, WIDTH / 2,
						 cover->img_y + cover->img_height + 15);
					
			draw_string (cairo_context, album->albumname, WIDTH / 2,
						 cover->img_y + cover->img_height + 30);

			cairo_stroke (cairo_context);
		}
		cairo_restore(cairo_context);
	}
	
	force_pixbuf_covers = FALSE;
}

/**
 * coverart_display_update:
 *
 * Takes a list of tracks and sets the 9 image cover display.
 *
 * @clear_track_list: flag indicating whether to clear the displaytracks list or not
 *  
 */
void coverart_display_update (gboolean clear_track_list)
{
	gint i;
	GList *tracks;
	Track *track;
	Album_Item *album;
	Playlist *playlist;

	if ( ! prefs_get_int (KEY_DISPLAY_COVERART))
		return;
		
	/* initialize display if not already done */
	if (!cdwidget)
		coverart_init_display ();

	/* Ensure that the setting of images hasnt been turned off
	 * due to being in the middle of a selection operation
	 */
	if (cdwidget->block_display_change)
		return;
	
	if (clear_track_list)
	{
		/* Find the selected playlist */
		playlist = pm_get_selected_playlist ();
		if (playlist == NULL)
			return;
		
		tracks = playlist->members;
		/* Free up the hash table and the key list */
		g_hash_table_foreach_remove(album_hash, (GHRFunc) gtk_true, NULL);
				
		g_list_free (album_key_list);
    
    album_key_list = NULL;;
    
		if (tracks == NULL)
		{
			redraw (FALSE);
			return;
		}
		
		while (tracks)
		{
	    gchar *album_key;
			track = tracks->data;
			
			album_key = g_strconcat (track->artist, "_", track->album, NULL);
			/* Check whether an album item has already been created in connection
			 * with the track's artist and album
			 */
			album = g_hash_table_lookup (album_hash, album_key);
			if (album == NULL)
			{
				/* Album item not found so create a new one and populate */
				album = g_new0 (Album_Item, 1);
				album->albumart = NULL;
				album->albumname = g_strdup (track->album);
				album->artist = g_strdup (track->artist);
				album->tracks = NULL;
				album->tracks = g_list_append (album->tracks, track);
				
				/* Insert the new Album Item into the hash */
				g_hash_table_insert (album_hash, album_key, album);
				/* Add the key to the list for sorting and other functions */
				album_key_list = g_list_append (album_key_list, album_key);
			}
			else
			{
			    /* Album Item found in the album hash so
			     * append the track to the end of the
			     * track list */
			    g_free (album_key);
			    album->tracks = g_list_append (album->tracks, track);
			}
			
			tracks = tracks->next;
		}
		
		cdwidget->first_imgindex = 0;
	}
		
	/* Remove all null tracks before any sorting should take place */	
 	album_key_list = g_list_remove_all (album_key_list, NULL);
 		
	/* Sort the tracks to the order set in the preference */
	coverart_sort_images (prefs_get_int("st_sort"));
	
	/* Add 4 null tracks to the end of the track list for padding */
	for (i = 0; i < IMG_MAIN; ++i)
		album_key_list = g_list_append (album_key_list, NULL);
	
	/* Add 4 null tracks to the start of the track list for padding */
	for (i = 0; i < IMG_MAIN; ++i)
		album_key_list = g_list_prepend (album_key_list, NULL);

	redraw (FALSE);
	
	set_slider_range (cdwidget->first_imgindex);
	
	/*
	printf("######### ORIGINAL LINE UP ########\n");
	debug_albums ();
	printf("######### END OF ORIGINAL LINE UP #######\n");
	*/
}

static void redraw (gboolean force_pixbuf_update)
{
	force_pixbuf_covers = force_pixbuf_update;
	GdkRegion *region = gdk_drawable_get_clip_region (cdwidget->draw_area->window);
	/* redraw the cairo canvas completely by exposing it */
	gdk_window_invalidate_region (cdwidget->draw_area->window, region, TRUE);
	gdk_window_process_updates (cdwidget->draw_area->window, TRUE);
	gdk_region_destroy (region);
	
	if (g_list_length (album_key_list) <= 1)
	{
		gtk_widget_set_sensitive (GTK_WIDGET(cdwidget->cdslider), FALSE); 
		gtk_widget_set_sensitive (GTK_WIDGET(cdwidget->leftbutton), FALSE); 
		gtk_widget_set_sensitive (GTK_WIDGET(cdwidget->rightbutton), FALSE);
	}
	else
	{
		gtk_widget_set_sensitive (GTK_WIDGET(cdwidget->cdslider), TRUE); 
		gtk_widget_set_sensitive (GTK_WIDGET(cdwidget->leftbutton), TRUE); 
		gtk_widget_set_sensitive (GTK_WIDGET(cdwidget->rightbutton), TRUE);
	}
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
static void set_cover_dimensions (Cover_Item *cover, int cover_index, gdouble img_width, gdouble img_height)
{
	gdouble x = 0, y = 0;
	gdouble small_img_width, small_img_height;
	gdouble display_width = 0, display_height = 0;
	gdouble display_diff = 0, display_ratio = 0;
	gint temp_index = 0;
	
	if (img_width > (WIDTH / 2))
	{
		display_ratio = img_width / img_height;
		img_width = (WIDTH / 2);
		img_height = img_width / display_ratio;
	}
	
	small_img_width = img_width * 0.75;
	small_img_height = img_height * 0.75;
	
	/* WIDTH is the width of the display_coverart window
	 * BORDER is the 10 pixel frame around the images
	 */
	display_width = (WIDTH / 2) - (BORDER * 2);
	
	display_diff = display_width - small_img_width;	
	
	/* Set the x location of the cover image */
	switch(cover_index) {
		case 0:
		case 1:
		case 2:
		case 3:
			display_ratio = ((gdouble) cover_index) / 4;
			x = BORDER + (display_ratio * display_diff);
			break;
		case IMG_MAIN:
			/* The Main Image CD Cover Image */
			x = (WIDTH - img_width) / 2;
			break;
		case 5:
		case 6:
		case 7:
		case 8:			
			temp_index = cover_index - 8;
			if (temp_index < 0)
				temp_index = temp_index * -1; 
	
			display_ratio = ((gdouble) temp_index) / 4;
			x = WIDTH - (BORDER + small_img_width + (display_ratio * display_diff)); 
			break;
	}
		
	/* Set the y location of the cover image. The y location must be determined by
	 * height of the cover image so that the hightlight and shadow fit in correctly.
	 */
	display_height = HEIGHT - (BORDER * 2);
	
	switch(cover_index) {
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
			y = HEIGHT - (img_height + (BORDER * 4));	
	}
			
	cover->img_x = x;
	cover->img_y = y;
		
	if (cover_index == IMG_MAIN)
	{
		cover->img_height = img_height;
		cover->img_width = img_width;
	}
	else
	{
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
static void set_highlight (Cover_Item *cover, gint index, cairo_t * cr)
{	    	   																		
	if(index == IMG_MAIN)
		return;
		
	cairo_save(cr);
	cairo_pattern_t *pat;
	pat = cairo_pattern_create_linear (
				cover->img_x,
				cover->img_y,
				cover->img_x,
				cover->img_y + (((gdouble) cover->img_height) / 2.5));
		cairo_pattern_add_color_stop_rgba (pat, 0.0, 1, 1, 1, 0);
		cairo_pattern_add_color_stop_rgba (pat, 0.4, 1, 1, 1, 0.6);
		cairo_pattern_add_color_stop_rgba (pat, 0.9, 1, 1, 1, 0);
		cairo_pattern_set_extend (pat, CAIRO_EXTEND_REPEAT);
		
		cairo_rectangle (
				cr, 
				cover->img_x, 
				cover->img_y, 
				cover->img_width, 
				cover->img_height);
		cairo_rotate (cr, M_PI / 4);
		cairo_set_source (cr, pat);
		cairo_fill (cr);
		cairo_pattern_destroy (pat);
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
static void set_shadow_reflection (Cover_Item *cover, cairo_t *cr)
{
	GdkColor *color = coverart_get_background_display_color();
	gdouble r = ((gdouble) (color->red >> 8)) / 255; 
	gdouble g = ((gdouble) (color->green >>8)) / 255; 
	gdouble b = ((gdouble) (color->blue >> 8)) / 255;
	g_free (color);
	
	cairo_pattern_t *pat;
	pat = cairo_pattern_create_linear (
			cover->img_x,
			cover->img_y + cover->img_height + 2,
			cover->img_x,
			cover->img_y + cover->img_height + 2 + cover->img_height);
	cairo_pattern_add_color_stop_rgba (pat, 0, r, g, b, 0.3);
	cairo_pattern_add_color_stop_rgba (pat, 0.5, r, g, b, 1);
	cairo_rectangle (
			cr, 
			cover->img_x, 
			cover->img_y + cover->img_height + 2, 
			cover->img_width + 10, 
			cover->img_height);
	cairo_set_source (cr, pat);
	cairo_fill (cr);
	cairo_pattern_destroy (pat);
}

/**
 * 
 * Convenience function that will only allow set images to be
 * called if the track that was affected was in the list of displaytracks
 * used by the coverart display. So if a whole album is deleted then this
 * will only reset the display if the first track in the album is deleted.
 * 
 * @track: affected track
 */
void coverart_track_changed (Track *track, gint signal)
{
	GList *keypos;
	gchar *trk_key;
	Album_Item *album;
	gint index;
	gboolean findremove;
	/*
	 * Scenarios:
	 * a) A track is being deleted that is not in the display
	 * b) A track is being deleted that is in the display
	 * c) A track has changed is some way so maybe the coverart
	 * d) A track has been created and its artist and album are already in the displaylist
	 * e) A track has been created and its artist and album are not in the displaylist
	 */

	/* If coverart not displayed then ignore */
	if (! prefs_get_int (KEY_DISPLAY_COVERART))
  	return;
  		
	trk_key = g_strconcat (track->artist, "_", track->album, NULL); 	
 	/* Determine the index of the found album */
 	keypos = g_list_find_custom (album_key_list, trk_key, (GCompareFunc) compare_album_keys);
	
	switch (signal)
	{
		case COVERART_REMOVE_SIGNAL:				
			g_return_if_fail (keypos);
			g_free (trk_key);
			
			/* Reassign trk_key to the key from the list */
 			trk_key = keypos->data;
 			index = g_list_position (album_key_list, keypos);
 	
			album = g_hash_table_lookup (album_hash, trk_key);
			
 			/* Remove the track from the album item */
 			remove_track_from_album (album, track, trk_key, index, keypos);
 			 
 			/* Check if album is being displayed by checking the index */
			if (index >= cdwidget->first_imgindex && index <= (cdwidget->first_imgindex + IMG_TOTAL))
 			{	
				/* reset the covers and should reset to original position but without the index */
				redraw (FALSE);
 			}
 			
 			/* Size of key list may have changed so reset the slider 
 			 * to appropriate range and index.
 			 */
 			set_slider_range (index - IMG_MAIN);
			break;
		case COVERART_CREATE_SIGNAL:
			/* Check whether an album item has already been created in connection
			 * with the track's artist and album
			 */
			album = g_hash_table_lookup (album_hash, trk_key);
			if (album == NULL)
			{
				/* Album item not found so create a new one and populate */
				album = g_new0 (Album_Item, 1);
				album->albumart = NULL;
				album->albumname = g_strdup (track->album);
				album->artist = g_strdup (track->artist);
				album->tracks = NULL;
				album->tracks = g_list_append (album->tracks, track);
				
				/* Insert the new Album Item into the hash */
				g_hash_table_insert (album_hash, trk_key, album);
				
				/* Add the key to the list for sorting and other functions */
				/* But first ... */
				/* Remove all null tracks before any sorting should take place */	
 				album_key_list = g_list_remove_all (album_key_list, NULL);
 			
				if (prefs_get_int("st_sort") == SORT_ASCENDING)
				{
					album_key_list = g_list_insert_sorted (album_key_list, trk_key, (GCompareFunc) compare_album_keys);
				}
				else if (prefs_get_int("st_sort") == SORT_DESCENDING)
				{
					/* Already in descending order so reverse into ascending order */
					album_key_list = g_list_reverse (album_key_list);
					/* Insert the track */
					album_key_list = g_list_insert_sorted (album_key_list, trk_key, (GCompareFunc) compare_album_keys);
					/* Reverse again */
					album_key_list = g_list_reverse (album_key_list);
				}
				else
				{
					/* NO SORT */
					album_key_list = g_list_append (album_key_list, trk_key);
				}
			
				/* Readd in the null tracks */
				/* Add 4 null tracks to the end of the track list for padding */
				gint i;
				for (i = 0; i < IMG_MAIN; ++i)
					album_key_list = g_list_append (album_key_list, NULL);
	
				/* Add 4 null tracks to the start of the track list for padding */
				for (i = 0; i < IMG_MAIN; ++i)
					album_key_list = g_list_prepend (album_key_list, NULL);
		
				redraw (FALSE);
			}
			else
			{
				/* Album Item found in the album hash so append the track to
				 * the end of the track list
				 */
				 album->tracks = g_list_append (album->tracks, track);
			}
			
			/* Set the slider to the newly inserted track.
			 * In fact sets image_index to 4 albums previous
			 * to newly inserted album to ensure this album is
			 * the main middle one.
			 */
			keypos = g_list_find_custom (album_key_list, trk_key, (GCompareFunc) compare_album_keys);
			index = g_list_position (album_key_list, keypos);
 			set_slider_range (index - IMG_MAIN);
 				
			break;
		case COVERART_CHANGE_SIGNAL:
			/* A track is declaring itself as changed so what to do? */
			findremove = FALSE;
			if (keypos == NULL)
			{
				/* The track could not be found according to the key!
				 * The ONLY way this could happen is if the user changed the
				 * artist or album of the track. Well it should be rare but the only
				 * way to remove it from its "old" album item is to search each one
				 */
				 findremove = TRUE;
			}
			else
			{
			/* Track has a valid key so can get the album back.
			 * Either has happened:
			 * a) Artist/Album key has been changed so the track is being moved to another existing album
			 * b) Some other change has occurred that is irrelevant to this code.
			 */
			 
			 /* To determine if a) is the case need to determine whether track exists in the 
			  * album items track list. If it does then b) is true and nothing more is required.
			  */
			  album = g_hash_table_lookup (album_hash, trk_key);
			  g_return_if_fail (album);
			  
			  index = g_list_index (album->tracks, track);
			  if (index != -1)
			  {
			  	/* Track exists in the album list so ignore the change and return */
			  	ExtraTrackData *etd;
			  	etd = track->userdata;
			  	if (etd->tartwork_changed == TRUE)
			  	{
			  		etd->tartwork_changed = FALSE;
			  		redraw(TRUE);
			  	}
			  	
			  	return;
			  }
			  else
			  {
			  	/* Track does not exist in the album list so the artist/album key has definitely changed */
			  	findremove = TRUE;
			  }
			}
			
			if (findremove)
			{
				/* It has been determined that the track has had its key changed
				 * and thus a search must be performed to find the "original" album
				 * that the track belonged to, remove it then add the track to the new
				 * album.
				 */
			  GList *klist;
				gchar *key;
				klist = g_list_first (album_key_list);
				while (klist != NULL)
				{
					key = (gchar *) klist->data;
					index = g_list_index (album_key_list, key); 
					if (key != NULL)
					{
						album = g_hash_table_lookup (album_hash, key);
						
						gint album_trk_index;
						album_trk_index = g_list_index (album->tracks, track);
						if (album_trk_index != -1)
						{
							/* The track is in this album so remove it in preparation for readding
						 	* under the new album key
						 	*/
					 		remove_track_from_album (album, track, key, index, klist);
 							redraw(FALSE);
					 		/* Found the album and removed so no need to continue the loop */
					 		break;
						}
					}
					klist = klist->next;
				}
				
				/* Create a new album item or find existing album to house the "brand new" track */
				coverart_track_changed (track, COVERART_CREATE_SIGNAL);
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
static gboolean on_drawing_area_exposed (GtkWidget *draw_area, GdkEventExpose *event)
{
	cairo_t *cairo_draw_context;

	/* get a cairo_t */
	cairo_draw_context = gdk_cairo_create (draw_area->window);

	/* set a clip region for the expose event */
	cairo_rectangle (cairo_draw_context,
			event->area.x, event->area.y,
			event->area.width, event->area.height);
	cairo_clip (cairo_draw_context);
	draw(cairo_draw_context);
	cairo_destroy (cairo_draw_context);

	return FALSE;
}

/**
 * gtkpod_window_configure_callback:
 *
 * Callback for the gtkpod app window. When the window
 * is resized the background of the cdwidget is given a size
 * of the same size. Ensure the background is always black
 * and no overlapping internal components occur.
 * 
 * @widget: gtkpod app window
 * @event: gdk configure event
 * @data: any user data passed to the function
 * 
 * Returns:
 * boolean indicating whether other handlers should be run.
 */
static gboolean on_gtkpod_window_configure (GtkWidget *widget, GdkEventConfigure *event, gpointer data)
{
	if (cdwidget == NULL)
		return FALSE;
		
	redraw (FALSE);
	
	return FALSE;
}

/**
 * on_paned0_button_release_event:
 *
 * Callback fired when a button release event occurs on
 * paned0. Only worthwhile things are carried out when
 * the position of paned0 has changed, ie. the bar was moved.
 * Moving the bar will scale the cover images appropriately.
 * Signal connected via the glade XML file.
 * 
 * @widget: gtkpod app window
 * @event: gdk event button
 * @data: any user data passed to the function
 * 
 * Returns:
 * boolean indicating whether other handlers should be run.
 */
G_MODULE_EXPORT gboolean on_paned0_button_release_event (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
		if ( ! prefs_get_int (KEY_DISPLAY_COVERART))
		return FALSE;
		
	gint width;
	
	width = gtk_paned_get_position (GTK_PANED(widget));
	if ((width >= DEFAULT_WIDTH) && (width != WIDTH))
	{
		WIDTH = width;
		redraw (FALSE);		
	}
		
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
static gboolean on_contentpanel_scroll_wheel_turned (GtkWidget *widget, GdkEventScroll *event, gpointer user_data)
{
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
	gtk_range_set_value (GTK_RANGE (cdwidget->cdslider), cdwidget->first_imgindex);
	
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
static void on_cover_display_button_clicked (GtkWidget *widget, gpointer data)
{
	GtkButton *button;
	const gchar *label;
	gint displaytotal;
  
  button = GTK_BUTTON(widget);
  label = gtk_button_get_label(button);
    
  if(g_str_equal(label, (gchar *) ">"))
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
	gtk_range_set_value (GTK_RANGE (cdwidget->cdslider), cdwidget->first_imgindex);

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
static void on_cover_display_slider_value_changed (GtkRange *range, gpointer user_data)
{
  gint index, displaytotal;
	
	if (cdwidget->block_display_change)
		return;
		
	index = gtk_range_get_value (range);
	displaytotal = g_list_length(album_key_list);
  
  if (displaytotal <= 0)
  	return;
    
  /* Use the index value from the slider for the main image index */
  cdwidget->first_imgindex = index;
  
  if (cdwidget->first_imgindex > (displaytotal - IMG_MAIN))
  	cdwidget->first_imgindex = displaytotal - IMG_MAIN;
	
  redraw (FALSE);
}

/**
 * on_cover_up_button_clicked:
 *
 * callback for the cover_up_button. Shows all the cover_art widgets
 * when clicked.
 *
 * @widget, data unused standard parameters
 *  
 */
G_MODULE_EXPORT void on_cover_up_button_clicked (GtkWidget *widget, gpointer data)
{
	prefs_set_int (KEY_DISPLAY_COVERART, TRUE);
	
	if (cdwidget == NULL)
		coverart_display_update (TRUE);
		
	gtk_widget_show_all (cdwidget->contentpanel);
	gtk_widget_show (GTK_WIDGET(cdwidget->cdslider));
	gtk_widget_show (GTK_WIDGET(cdwidget->leftbutton));
	gtk_widget_show (GTK_WIDGET(cdwidget->rightbutton));
	
	gtk_widget_hide (widget);
	
	GtkWidget *downbutton = gtkpod_xml_get_widget (main_window_xml, "cover_down_button");
	gtk_widget_show (downbutton);
}

/**
 * on_cover_down_button_clicked:
 *
 * callback for the cover_down_button. Hides all the cover_art widgets
 * when clicked.
 *
 * @widget, data unused standard parameters
 *  
 */
G_MODULE_EXPORT void on_cover_down_button_clicked (GtkWidget *widget, gpointer data)
{
	prefs_set_int (KEY_DISPLAY_COVERART, FALSE);
	
	gtk_widget_hide_all (cdwidget->contentpanel);
	gtk_widget_hide (GTK_WIDGET(cdwidget->cdslider));
	gtk_widget_hide (GTK_WIDGET(cdwidget->leftbutton));
	gtk_widget_hide (GTK_WIDGET(cdwidget->rightbutton));
	
	if (cdwidget != NULL)
	{
		/* dispose of existing CD Widget */
		free_CDWidget();
	}
	gtk_widget_hide (widget);
	
	GtkWidget *upbutton = gtkpod_xml_get_widget (main_window_xml, "cover_up_button");
	gtk_widget_show (upbutton);
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
static gint on_main_cover_image_clicked (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	Cover_Item *cover;
	guint mbutton;
	
	if(event->type != GDK_BUTTON_PRESS)
		return FALSE;
		
	mbutton = event->button.button;
	
	if (mbutton == 1)
	{
		Track *track;
		Album_Item *album;
		/* Left mouse button clicked so find all tracks with displayed cover */
		cover = g_ptr_array_index(cdwidget->cdcovers, IMG_MAIN);
		/* Stop redisplay of the artwork as its already
		 * in the correct location
		 */
		coverart_block_change (TRUE);
	
		/* Select the correct track in the sorttabs */
		album = cover->album;
		g_return_val_if_fail (album, FALSE);
		
		/* Clear the tracks listed in the display */
		tm_remove_all_tracks ();
		
		GList *tracks = album->tracks;
		while (tracks)
		{
			track = (Track *) tracks->data;
			tm_add_track_to_track_model (track, NULL);
			tracks = tracks->next;
		}
		
		/* Turn the display change back on */
		coverart_block_change (FALSE);
	}
	else if ((mbutton == 3) && (event->button.state & GDK_SHIFT_MASK))
	{
		/* Right mouse button clicked and shift pressed.
		 * Go straight to edit details window
		 */
		 GList *tracks = coverart_get_displayed_tracks();
		details_edit (tracks);
	}
	else if (mbutton == 3)
	{
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
		cad_context_menu_init ();
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
GdkPixbuf *coverart_get_track_thumb (Track *track, Itdb_Device *device, gint default_size)
{
	GdkPixbuf *pixbuf = NULL;
	GdkPixbuf *image = NULL;	
	Thumb *thumb;
	ExtraTrackData *etd;
	gint w, h;
	float ratio;
	
	etd = track->userdata;
	g_return_val_if_fail (etd, NULL);

	thumb = itdb_artwork_get_thumb_by_type (track->artwork,
						ITDB_THUMB_COVER_LARGE);
	if (thumb)
	{
		image = itdb_thumb_get_gdk_pixbuf (device, thumb);
	  w = gdk_pixbuf_get_width (image);
	  h = gdk_pixbuf_get_height (image);
	  
	  if (default_size > 0)
		{
	  	if (w > default_size || h > default_size)
	  	{
	  		/* need to scale the image back down to size */
	  		if (w == h)
	  		{
	  			w = default_size;
	  			h = default_size;
	  		}
	  		else if (h > w)
	  		{
	  			ratio = h / w;
	  			h = default_size;
	  			w = (gint) (default_size / ratio);
	  		}
	  		else
	  		{
	  			ratio = w / h;
	  			w = default_size;
	  			h = (gint) (default_size / ratio);
	  		}
	  		pixbuf = gdk_pixbuf_scale_simple(image, w, h, GDK_INTERP_BILINEAR);
	  	}
	  	else
	  		pixbuf = gdk_pixbuf_copy (image);
			
			gdk_pixbuf_unref (image);
		}
		else
		{
			pixbuf = gdk_pixbuf_scale_simple(image, DEFAULT_IMG_SIZE, DEFAULT_IMG_SIZE, GDK_INTERP_BILINEAR);
  		gdk_pixbuf_unref (image);
		}
	}
	
	if (pixbuf ==  NULL)
	{
	 	/* Could not get a viable thumbnail so get default pixbuf */
	 	pixbuf = coverart_get_default_track_thumb (default_size);
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
GList *coverart_get_displayed_tracks (void)
{
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
GdkPixbuf *coverart_get_default_track_thumb (gint default_img_size)
{
	GdkPixbuf *pixbuf = NULL;
	GdkPixbuf *scaled = NULL;
	gdouble default_size = 140;
	GError *error = NULL;
	
	if (default_img_size != 0)
		default_size = (gdouble) default_img_size;
	
	pixbuf = gdk_pixbuf_new_from_file(DEFAULT_FILE, &error);
	if (error != NULL)
	{
			printf("Error occurred loading the default file - \nCode: %d\nMessage: %s\n", error->code, error->message);
			g_return_val_if_fail(pixbuf, NULL);
	}
	
	scaled = gdk_pixbuf_scale_simple(pixbuf, default_size, default_size, GDK_INTERP_BILINEAR);
  gdk_pixbuf_unref (pixbuf);
	

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
static void set_slider_range (gint index)
{
	gint slider_ubound = g_list_length (album_key_list) - IMG_TOTAL;
	if(slider_ubound < 1)
	{
		/* If only one album cover is displayed then slider_ubbound returns
		 * 0 and causes a slider assertion error. Avoid this by disabling the
		 * slider, which makes sense because only one cover is displayed.
		 */
		slider_ubound = 1;
		gtk_widget_set_sensitive (GTK_WIDGET(cdwidget->cdslider), FALSE); 
		gtk_widget_set_sensitive (GTK_WIDGET(cdwidget->leftbutton), FALSE); 
		gtk_widget_set_sensitive (GTK_WIDGET(cdwidget->rightbutton), FALSE); 
	}
	else
	{
	 	gtk_widget_set_sensitive (GTK_WIDGET(cdwidget->cdslider), TRUE);
	 	gtk_widget_set_sensitive (GTK_WIDGET(cdwidget->leftbutton), TRUE); 
		gtk_widget_set_sensitive (GTK_WIDGET(cdwidget->rightbutton), TRUE);
	}
	
	gtk_range_set_range (GTK_RANGE (cdwidget->cdslider), 0, slider_ubound);
	if (index >= 0 && index <= slider_ubound)
		gtk_range_set_value (GTK_RANGE (cdwidget->cdslider), index);
	else
		gtk_range_set_value (GTK_RANGE (cdwidget->cdslider), 0);
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
void coverart_select_cover (Track *track)
{
  gint displaytotal, index;
	
	/* Only select covers if the display is visible */
	if (! prefs_get_int (KEY_DISPLAY_COVERART) || cdwidget == NULL)
		return;
		
	/* Only select covers if fire display change is enabled */
	if (cdwidget->block_display_change)
		return;
			
	displaytotal = g_list_length(album_key_list);
  if (displaytotal <= 0)
  	return;
  	
  gchar *trk_key;
  trk_key = g_strconcat (track->artist, "_", track->album, NULL);
 	
 	/* Determine the index of the found track */
 	GList *key = g_list_find_custom (album_key_list, trk_key, (GCompareFunc) compare_album_keys);
 	g_return_if_fail (key);
 	index = g_list_position (album_key_list, key);
 	g_free (trk_key);
 	 
 	/* Use the index value for the main image index */
  cdwidget->first_imgindex = index - IMG_MAIN;
  if (cdwidget->first_imgindex < 0)
  	cdwidget->first_imgindex = 0;
  else if((cdwidget->first_imgindex + IMG_TOTAL) >= displaytotal)
  	cdwidget->first_imgindex = displaytotal - IMG_TOTAL;
      
  redraw (FALSE);
  
  /* Set the index value of the slider but avoid causing an infinite
   * cover selection by blocking the event
   */
  g_signal_handler_block (cdwidget->cdslider, slide_signal_id);
	gtk_range_set_value (GTK_RANGE (cdwidget->cdslider), index);
	g_signal_handler_unblock (cdwidget->cdslider, slide_signal_id);
 }

/**
 * coverart_display_big_artwork:
 * 
 * Display a big version of the artwork in a dialog
 * 
 */
void coverart_display_big_artwork ()
{
	Cover_Item *cover;
	ExtraTrackData *etd;
	GdkPixbuf *imgbuf = NULL;
	
	cover = g_ptr_array_index(cdwidget->cdcovers, IMG_MAIN);
	g_return_if_fail (cover);
	
	if (cover->album == NULL)
		return;
				
	Track *track;
	track = g_list_nth_data (cover->album->tracks, 0);
	etd = track->userdata;
	if (etd && etd->thumb_path_locale)
	{
		GError *error = NULL;
		imgbuf = gdk_pixbuf_new_from_file (etd->thumb_path_locale, &error);
		if (error != NULL)
		{
			g_error_free (error);
		}
	}
	
	/* Either thumb was null or the attempt at getting a pixbuf failed
	 * due to invalid file. For example, some nut (like me) decided to
	 * apply an mp3 file to the track as its cover file
	 */
	if (imgbuf ==  NULL)
	{
		/* Could not get a viable thumbnail so get default pixbuf */
		imgbuf = coverart_get_default_track_thumb (256);
	}
	
	display_image_dialog (imgbuf);
		
	/* Unreference pixbuf if it is not pointing to
	 * the album's artwork
	 */
	if (cover->album->albumart == NULL)
		gdk_pixbuf_unref (imgbuf);
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
 static gint compare_album_keys (gchar *a, gchar *b)
 {
 	if (a == NULL) return -1;
 	if (b == NULL) return -1;
 	
 	return g_ascii_strcasecmp (a, b);	
 	
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
static void coverart_sort_images (GtkSortType order)
{
 	if (order == SORT_NONE)
 	{
 		/* No sorting means original order so this should have been called after a coverart_display_update (TRUE)
 		 * when the TRUE means the tracks were freshly established from the playlist and the hash and key_list
 		 * recreated.
 		 */
 		return;
 	}
 	else
 	{
 		album_key_list = g_list_sort (album_key_list, (GCompareFunc) compare_album_keys);
 	}
 	
 	if (order == GTK_SORT_DESCENDING)
 	{
 		album_key_list = g_list_reverse (album_key_list);		
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
 static void remove_track_from_album (Album_Item *album, Track *track, gchar *key, gint index, GList *keylistitem)
 {		
 	album->tracks = g_list_remove (album->tracks, track);
 	if (g_list_length (album->tracks) == 0)
 	{
 		/* No more tracks related to this album item so delete it */
 		gboolean delstatus = g_hash_table_remove (album_hash, key);
 		if (! delstatus)
 			gtkpod_warning (_("Failed to remove the album from the album hash store."));
 		/*else
 			printf("Successfully removed album\n");
 		*/
 		album_key_list = g_list_remove_link (album_key_list, keylistitem);
 	
 		if (index < (cdwidget->first_imgindex + IMG_MAIN) && index > IMG_MAIN)
 		{
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
void coverart_set_cover_from_file ()
{
	gchar *filename;
	Track *track;
	Cover_Item *cover;
	GList *tracks;
	
  filename = fileselection_get_cover_filename ();

	if (filename)
  {
  	cover = g_ptr_array_index(cdwidget->cdcovers, IMG_MAIN);
		tracks = cover->album->tracks;
		
		while (tracks)
		{
			track = tracks->data;
		
 			if (gp_track_set_thumbnails (track, filename))
 				data_changed (track->itdb);
 				
 			tracks = tracks->next;
		}
		/* Nullify so that the album art is picked up from the tracks again */
		gdk_pixbuf_unref (cover->album->albumart);
		cover->album->albumart = NULL;
  }
    
  g_free (filename);
  
  redraw (FALSE);
}

GdkColor *coverart_get_background_display_color ()
{
	gchar *hex_string;
	GdkColor *color;
	
	if (album_key_list == NULL || g_list_length (album_key_list) == 0)
		hex_string = "#FFFFFF";
	else if ( ! prefs_get_string_value ("coverart_display_bg_color", NULL))
		hex_string = "#000000";
	else
		prefs_get_string_value ("coverart_display_bg_color", &hex_string);
		
	color = convert_hexstring_to_gdk_color (hex_string);
	return color;
}

GdkColor *coverart_get_foreground_display_color ()
{
	gchar *hex_string;
	GdkColor *color;
	
	if (album_key_list == NULL || g_list_length (album_key_list) == 0)
		hex_string = "#000000";
	else if ( ! prefs_get_string_value ("coverart_display_bg_color", NULL))
		hex_string = "#FFFFFF";
	else
		prefs_get_string_value ("coverart_display_fg_color", &hex_string);
		
	color = convert_hexstring_to_gdk_color (hex_string);
	return color;
}

/**
 * 
 * free_album:
 * 
 * Destroy an album struct once no longer needed.
 * 
 */ 
static void free_album (Album_Item *album)
{
	if (album != NULL)
	{
		if (album->tracks)
		{
			g_list_free (album->tracks);
		}
		
		g_free (album->albumname);
		g_free (album->artist);
		
		if (album->albumart)
			gdk_pixbuf_unref (album->albumart);
	}
}

/**
 * 
 * free_CDWidget
 * 
 * destroy the CD Widget and free everything currently
 * in memory.
 */
 static void free_CDWidget ()
 {
 	gint i;
 	g_signal_handler_disconnect (cdwidget->leftbutton, lbutton_signal_id);
 	g_signal_handler_disconnect (cdwidget->rightbutton, rbutton_signal_id);
 	g_signal_handler_disconnect (cdwidget->cdslider, slide_signal_id);
 	g_signal_handler_disconnect (cdwidget->contentpanel, contentpanel_signal_id);
 	g_signal_handler_disconnect (gtkpod_window, window_signal_id);

 	/* Components not freed as they are part of the glade xml file */
 	cdwidget->leftbutton = NULL;
 	cdwidget->rightbutton = NULL;
 	cdwidget->cdslider = NULL;
 	cdwidget->contentpanel = NULL;
 	cdwidget->canvasbox = NULL;
	cdwidget->controlbox = NULL;
		
	/* native variables rather than references so should be destroyed when freed */
 	cdwidget->first_imgindex = 0;
 	cdwidget->block_display_change = FALSE;

	Cover_Item *cover;
	for(i = 0; i < IMG_TOTAL; ++i)
  {
  	cover = g_ptr_array_index(cdwidget->cdcovers, i);
  	/* Nullify pointer to album reference. Will be freed below */
  	cover->album = NULL;
  }
  	
 	g_ptr_array_free (cdwidget->cdcovers, TRUE);
 		
 	/* Destroying canvas should destroy the background and cvrtext */
 	gtk_widget_destroy (GTK_WIDGET(cdwidget->draw_area));
	
	/* Remove all null tracks before using it to destory the hash table */	
 	album_key_list = g_list_remove_all (album_key_list, NULL);
 	g_hash_table_foreach_remove(album_hash, (GHRFunc) gtk_true, NULL);
	g_hash_table_destroy (album_hash);
	g_list_free (album_key_list);
		
	g_free (cdwidget);
	cdwidget = NULL;
}
 
static gboolean dnd_coverart_drag_drop(GtkWidget *widget, GdkDragContext *drag_context, gint x, gint y, guint time, gpointer user_data)
{
	GdkAtom target;
	target = gtk_drag_dest_find_target (widget, drag_context, NULL);

	if (target != GDK_NONE)
	{
		gtk_drag_get_data (widget, drag_context, target, time);
		return TRUE;	
	}
	/*
	printf ("drop item\n");
	gint i = 0;
	for (i = 0; i < g_list_length(drag_context->targets); ++i)
	{
		target = g_list_nth_data (drag_context->targets, i);
		printf ("Atom: %s\n", gdk_atom_name(target));
		gtk_drag_get_data (widget, drag_context, target, time);
		return TRUE;
	}
	*/
	return FALSE;
}

static gboolean dnd_coverart_drag_motion (GtkWidget *widget,
				GdkDragContext *dc,
				gint x,
				gint y,
				guint time,
				gpointer user_data)
{
	GdkAtom target;
	iTunesDB *itdb;
	ExtraiTunesDBData *eitdb;

	itdb = gp_get_selected_itdb ();
	/* no drop is possible if no playlist/repository is selected */
	if (itdb == NULL)
	{
		gdk_drag_status (dc, 0, time);
		return FALSE;
	}
	
	eitdb = itdb->userdata;
	g_return_val_if_fail (eitdb, FALSE);
	/* no drop is possible if no repository is loaded */
	if (!eitdb->itdb_imported)
	{
		gdk_drag_status (dc, 0, time);
		return FALSE;
	}
	    
	target = gtk_drag_dest_find_target (widget, dc, NULL);
	/* no drop possible if no valid target can be found */
	if (target == GDK_NONE)
	{
		gdk_drag_status (dc, 0, time);
		return FALSE;
	}
	    
	gdk_drag_status (dc, dc->suggested_action, time);

  return TRUE;
}

static void dnd_coverart_drag_data_received(GtkWidget *widget, GdkDragContext *dc, gint x, gint y, GtkSelectionData *data, guint info,
		guint time, gpointer user_data)
{
	g_return_if_fail (widget);
	g_return_if_fail (dc);
	g_return_if_fail (data);
	g_return_if_fail (data->data);
	g_return_if_fail (data->length > 0);
	
	/* mozilla bug 402394 */

#if DEBUG
		printf ("data length = %d\n", data->length);
		printf ("data->data = %s\n", data->data);
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
	tracks = cover->album->tracks;
			
	switch (info)
	{
		case DND_IMAGE_JPEG:
#if DEBUG
			printf ("Using DND_IMAGE_JPEG\n");
#endif
			pixbuf = gtk_selection_data_get_pixbuf (data);
			if (pixbuf != NULL)
			{
				/* initialise the url string with a safe value as not used if already have image */
				url = "local image";
				/* Initialise a fetchcover object */
				fcover = fetchcover_new (url, tracks);
				coverart_block_change (TRUE);
				
				/* find the filename with which to save the pixbuf to */
				if (fetchcover_select_filename (fcover))
				{
					filename = g_build_filename(fcover->dir, fcover->filename, NULL);
					if (! gdk_pixbuf_save (pixbuf, filename, "jpeg", &error, NULL))
					{
						/* Save failed for some reason */
						if (error->message)
							fcover->err_msg = g_strdup (error->message);
						else
							fcover->err_msg = "Saving image to file failed. No internal error message was returned.";
						
						g_error_free (error);
					}
					else
					{
						/* Image successfully saved */
						image_status = TRUE;
					}
				}
				/* record any errors and free the fetchcover */
				if (fcover->err_msg != NULL)
					image_error = g_strdup(fcover->err_msg);
				
				free_fetchcover (fcover);
				gdk_pixbuf_unref (pixbuf);
				coverart_block_change (FALSE);
			}
			else
			{
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
			url = g_strdup ((gchar *) data->data);
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
			image_error = g_strdup ("Item had to be downloaded but gtkpod was not compiled with curl.");
			image_status = FALSE;
#endif
	}
	
	if (!image_status || filename == NULL)
	{
		gtkpod_warning (_("Error occurred dropping an image onto the coverart display: %s\n"), image_error);
		
		if (image_error)
			g_free (image_error);
		if (filename)
			g_free (filename);
		
		gtk_drag_finish (dc, FALSE, FALSE, time);
		return;
	}
	
	while (tracks)
	{
		track = tracks->data;
					
		if (gp_track_set_thumbnails (track, filename))
			data_changed (track->itdb);
			 				
		tracks = tracks->next;
	}
	/* Nullify so that the album art is picked up from the tracks again */
	cover->album->albumart = NULL;
			    
	redraw (FALSE);

	if (image_error)
		g_free (image_error);
	
	g_free (filename);
	
	gtkpod_statusbar_message (_("Successfully set new coverart for selected tracks"));
	gtk_drag_finish (dc, FALSE, FALSE, time);
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
static GdkColor *convert_hexstring_to_gdk_color (gchar *hexstring) 
{
	GdkColor *colour;
	GdkColormap *map;
	map = gdk_colormap_get_system();
	
	colour = g_malloc (sizeof(GdkColor));
	
	if (! gdk_color_parse(hexstring, colour))
		return NULL;
	
	if (! gdk_colormap_alloc_color(map, colour, FALSE, TRUE))
		return NULL;
	
	return colour;
}

	
