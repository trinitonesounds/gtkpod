/*
|  Copyright (C) 2002-2007 Jorg Schuler <jcsjcs at users sourceforge net>
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
#  include <config.h>
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

/* Declarations */
static void free_album (Album_Item *album);
static void free_CDWidget ();
static gint compare_album_keys (gchar *a, gchar *b);
static void set_display_dimensions ();
static GdkPixbuf *draw_blank_cdimage ();
static void set_highlight (Cover_Item *cover, gboolean ismain);
static void raise_cdimages (GPtrArray *cdcovers);
/*static void scroll_covers (gint direction);*/
static void remove_track_from_album (Album_Item *album, Track *track, gchar *key, gint index, GList *keylistitem);
static void on_cover_display_button_clicked (GtkWidget *widget, gpointer data);
static gint on_main_cover_image_clicked (GnomeCanvasItem *canvasitem, GdkEvent *event, gpointer data);
static void on_cover_display_slider_value_changed (GtkRange *range, gpointer user_data);
static void set_cover_dimensions (Cover_Item *cover, int cover_index);
static void coverart_sort_images (GtkSortType order);
static void prepare_canvas ();
static void set_slider_range (gint index);
static void set_covers (gboolean force_imgupdate);
static void set_cover_item (gint ndex, Cover_Item *cover, gchar *key, gboolean force_imgupdate);

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
/* Dimensions used for the smaller 8 cd cover images */
static gdouble SMALL_IMG_WIDTH;
static gdouble SMALL_IMG_HEIGHT;
/* Dimensions used for the main cd cover image */
static gdouble BIG_IMG_WIDTH;
static gdouble BIG_IMG_HEIGHT;
/* Path of the png file used for albums without cd covers */
static gchar *DEFAULT_FILE;
/* Path of the png file used for the highlighting of cd covers */
static gchar *HIGHLIGHT_FILE;
/* Path of the png file used for the display of the main cd cover */
static gchar *HIGHLIGHT_FILE_MAIN;
/* signal handler id for the components */
static gulong slide_signal_id;
static gulong rbutton_signal_id;
static gulong lbutton_signal_id;
static gulong window_signal_id;
static gulong contentpanel_signal_id;

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
 * 
 * destroy the CD Widget and free everything currently
 * in memory.
 * 
 */
 static void free_CDWidget()
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
 	cdwidget->bground = NULL;
 	cdwidget->cvrtext = NULL;
 	gtk_widget_destroy (GTK_WIDGET(cdwidget->canvas));
	
	/* Remove all null tracks before any sorting should take place */	
 	album_key_list = g_list_remove_all (album_key_list, NULL);
 		
 	g_hash_table_foreach_remove(album_hash, (GHRFunc) gtk_true, NULL);
	g_hash_table_destroy (album_hash);
	g_list_free (album_key_list);
		
	g_free (cdwidget);
	cdwidget = NULL;
}
 
/**
 * draw_blank_cdimage:
 *
 * creates a blank "black" pixbuf cd image to represent the
 * display when the playlist has no tracks in it.
 * 
 * Returns:
 * pixbuf of blank pixbuf
 */
static GdkPixbuf *draw_blank_cdimage ()
{	
	gdouble width = 4;
	gdouble height = 4;
	gint pixel_offset, rowstride, x, y;
	guchar *drawbuf;

	drawbuf = g_malloc ((gint) (width * 16) * (gint) height);
	
	/* drawing buffer length multiplied by 4
	 * due to 1 width per R, G, B & ALPHA
	 */
	rowstride = width * 4;
			
  for(y = 0; y < height; ++y)
  {
  	for(x = 0; x < width; ++x)
  	{
  		pixel_offset = (y * rowstride) + (x * 4);
  		drawbuf[pixel_offset] = 0;
  		drawbuf[pixel_offset + 1] = 0;
  		drawbuf[pixel_offset + 2] = 0;
  		drawbuf[pixel_offset + 3] = 255;
  	}
  }
  return gdk_pixbuf_new_from_data(drawbuf,
  									GDK_COLORSPACE_RGB,
  									TRUE,
  									8,
  									width,
  									height,
  									rowstride,
  									(GdkPixbufDestroyNotify) g_free, NULL);
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
static void set_highlight (Cover_Item *cover, gboolean ismain)
{
    GdkPixbuf *image;
    GError *error = NULL;
    GdkPixbuf *scaled;

		if(ismain)
			image = gdk_pixbuf_new_from_file(HIGHLIGHT_FILE_MAIN, &error);
		else
    	image = gdk_pixbuf_new_from_file(HIGHLIGHT_FILE, &error);
    
    if(error != NULL)
    {	
	printf("Error occurred loading file - \nCode: %d\nMessage: %s\n", error->code, error->message); 
	g_return_if_fail (image);
    }

    scaled = gdk_pixbuf_scale_simple(image, cover->img_width, ((cover->img_height * 2) + 6), GDK_INTERP_NEAREST);
    gdk_pixbuf_unref (image);
		
    gnome_canvas_item_set (cover->highlight,
			   "pixbuf", scaled,
			   NULL);
    gdk_pixbuf_unref (scaled);

    gnome_canvas_item_hide (cover->highlight);				
}

/**
 * raise_cdimages:
 *
 * Ensures that once all the canvas items have been created on the
 * main canvas, they are have the correct Z-order, ie. main cd image
 * is at the top and fully visible.
 * 
 * @cdcovers: array of pointers to the cd cover objects
 *  
 */
static void raise_cdimages (GPtrArray *cdcovers)
{
	Cover_Item *cover;
	gint i, maxindex;
	
	maxindex = IMG_TOTAL - 1;
	
	for(i = 0; i < maxindex / 2; ++i)
	{
		cover = g_ptr_array_index(cdcovers, i);
		gnome_canvas_item_raise_to_top(cover->cdcvrgrp);
		
		cover = g_ptr_array_index(cdcovers, maxindex - i);
		gnome_canvas_item_raise_to_top(cover->cdcvrgrp);
	}	
	cover = g_ptr_array_index(cdcovers, IMG_MAIN);
	gnome_canvas_item_raise_to_top(cover->cdcvrgrp);
	
	gnome_canvas_item_raise_to_top(GNOME_CANVAS_ITEM(cdwidget->cvrtext));
}

/**
 * 
 * force_update_covers:
 * 
 * Call the resetting of the covers and override the cached images so that they
 * are loaded with the latest files existing on the filesystem.
 * 
 */
void force_update_covers ()
{
	set_covers (TRUE);
}

/**
 * set_covers:
 *
 * Internal function responsible for the resetting of the artwork
 * covers in response to some kind of change in selection, eg. new
 * selection in sort tab, button click etc...
 * 
 * @force_imgupdate: forces the resetting of the cached images so that the
 * values are reread from the tracks and updated. Used sparingly.
 */
static void set_covers (gboolean force_imgupdate)
{ 
  gint i, dataindex;
  gchar *key;
  Cover_Item *cover;

  if (cdwidget && cdwidget->cdcovers)
  {
    for(i = 0; i < IMG_TOTAL; ++i)
    {
		cover = g_ptr_array_index(cdwidget->cdcovers, i);
		dataindex = cdwidget->first_imgindex + i;

		/* Get the key from the key list appropriate to the index
		 * provided by the first image index property
		 */
		key = g_list_nth_data (album_key_list, dataindex);
		
		set_cover_item (i, cover, key, force_imgupdate);	
    }
  }
}

/**
 * set_cover_item:
 *
 * Internal function called  by set_covers to reset an artwork cover.
 * 
 */
static void set_cover_item (gint index, Cover_Item *cover, gchar *key, gboolean force_imgupdate)
{
	GdkPixbuf *reflection;
	GdkPixbuf *scaled;
	Album_Item *album;
  
	if (key == NULL)
	{
		GdkPixbuf *imgbuf;
		album = NULL;
	 	imgbuf = draw_blank_cdimage ();
	 	
	 	/* Hide the highlight */
	 	gnome_canvas_item_hide (cover->highlight);
	 	
	 	/* Set the cover */
	 	scaled = gdk_pixbuf_scale_simple (imgbuf, cover->img_width, cover->img_height, GDK_INTERP_NEAREST);
	  gnome_canvas_item_set (cover->cdimage,
			 		"pixbuf", scaled,
			 		NULL);
	  
	  /* Set the reflection to blank too */
	 reflection = gdk_pixbuf_flip (scaled, FALSE);
	 gnome_canvas_item_set (cover->cdreflection,
		  "pixbuf", reflection,
		  NULL);
	 
	 	if (index == IMG_MAIN)
		{
	 		/* Set the text to blank */
	 		gnome_canvas_item_set (GNOME_CANVAS_ITEM (cdwidget->cvrtext),
				   	"text", "No Artist",
				   	"fill_color", "black",
				   	"justification", GTK_JUSTIFY_CENTER,
				   	NULL);
		}
		
		gdk_pixbuf_unref (reflection);		   
		gdk_pixbuf_unref (scaled);
		gdk_pixbuf_unref (imgbuf);
		
		return;
	}
	
	/* Key is not null */
	
	/* Find the Album Item appropriate to the key */
	album  = g_hash_table_lookup (album_hash, key);
	cover->album = album;
	
	Track *track;
	if (force_imgupdate)
	{
		gdk_pixbuf_unref (album->albumart);
		album->albumart = NULL;
	}
	
	if (album->albumart == NULL)
	{
		track = g_list_nth_data (album->tracks, 0);
		album->albumart = coverart_get_track_thumb (track, track->itdb->device);
	}
	
	/* Display the highlight */
	gnome_canvas_item_show (cover->highlight);	
	
	/* Set the Cover */
	scaled = gdk_pixbuf_scale_simple (album->albumart, cover->img_width, cover->img_height, GDK_INTERP_NEAREST);
	gnome_canvas_item_set (cover->cdimage,
	 		"pixbuf", scaled,
	 		NULL);
	    		
	 /* flip image vertically to create reflection */
	reflection = gdk_pixbuf_flip (scaled, FALSE);
	gnome_canvas_item_set (cover->cdreflection,
		  "pixbuf", reflection,
		  NULL);
	    	
	gdk_pixbuf_unref (reflection);
	gdk_pixbuf_unref (scaled);
	
	/* Set the text if the index is the central image cover */
	if (index == IMG_MAIN)
	{
		gchar *text;
		text = g_strconcat (album->artist, "\n", album->albumname, NULL);
		gnome_canvas_item_set (GNOME_CANVAS_ITEM (cdwidget->cvrtext),
				 "text", text,
				 "fill_color", "white",
				 "justification", GTK_JUSTIFY_CENTER,
				 NULL);
		g_free (text);
		
		/*
		int i;
		Track *track;
		for (i = 0; i < g_list_length(album->tracks); ++i)
		{
			track = g_list_nth_data (album->tracks, i);
			printf ("Track artist:%s album:%s  title:%s\n", track->artist, track->album, track->title);
		}
		*/
	}
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
	
  set_covers (FALSE);
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
static gint on_main_cover_image_clicked (GnomeCanvasItem *canvasitem, GdkEvent *event, gpointer data)
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
 * coverart_clear_images:
 *
 * reset the cd cover images to default blank pixbufs and
 * nullify the covers' pointers to any tracks.
 * 
 */
void coverart_clear_images ()
{
	gint i;
	Cover_Item *cover = NULL;
	GdkPixbuf *buf;
	
	buf = draw_blank_cdimage ();
	
	for (i = 0; i < IMG_TOTAL; i++)
	{
	  GdkPixbuf *buf2;
		/* Reset the pixbuf */
		cover = g_ptr_array_index(cdwidget->cdcovers, i);
		cover->album = NULL;
		buf2 = gdk_pixbuf_scale_simple(buf, cover->img_width, cover->img_height, GDK_INTERP_NEAREST);
		gnome_canvas_item_set(cover->cdimage, "pixbuf", buf2, NULL);
		gnome_canvas_item_set(cover->cdreflection, "pixbuf", buf2, NULL);
		gnome_canvas_item_hide (cover->highlight);
		
		gdk_pixbuf_unref (buf2);
		
		/* Reset track list too */
		cover->album = NULL;
		
		if (i == IMG_MAIN)
		{
			/* Set the text to display details of the main image */
		    gnome_canvas_item_set (GNOME_CANVAS_ITEM (cdwidget->cvrtext),
					   "text", "No Artist",
					   "fill_color", "black",
					   NULL);
		}
	}
	gdk_pixbuf_unref(buf);
}

/**
 * coverart_get_track_thumb:
 *
 * Retrieve the artwork pixbuf from the given track.
 * 
 * @track: Track from where the pixbuf is obtained.
 * 
 * Returns:
 * pixbuf referenced by the provided track or the pixbuf of the
 * default file if track has no cover art.
 */
GdkPixbuf *coverart_get_track_thumb (Track *track, Itdb_Device *device)
{
	GdkPixbuf *pixbuf = NULL;	
	Thumb *thumb;
	ExtraTrackData *etd;

	etd = track->userdata;
	g_return_val_if_fail (etd, NULL);

	thumb = itdb_artwork_get_thumb_by_type (track->artwork,
						ITDB_THUMB_COVER_LARGE);
	if (thumb)
	{
	    pixbuf = itdb_thumb_get_gdk_pixbuf (device, thumb);
	}
	
	if (pixbuf ==  NULL)
	{
	 	/* Could not get a viable thumbnail so get default pixbuf */
	 	pixbuf = coverart_get_default_track_thumb ();
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
 * coverart_display_big_artwork:
 * 
 * Display a big version of the artwork in a dialog
 * 
 */
void coverart_display_big_artwork ()
{
	GtkWidget *dialog;
	Cover_Item *cover;
	GdkPixbuf *imgbuf;
	
	cover = g_ptr_array_index(cdwidget->cdcovers, IMG_MAIN);
	g_return_if_fail (cover);
	
	if (cover->album == NULL)
		return;
	
	/* Set the dialog title */
	gchar *text = g_strconcat (cover->album->artist, ": ", cover->album->albumname, NULL);
	dialog = gtk_dialog_new_with_buttons (	text,
																																	GTK_WINDOW (gtkpod_xml_get_widget (main_window_xml, "gtkpod")),
																																	GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
																																	GTK_STOCK_CLOSE,
																																	GTK_RESPONSE_CLOSE,
																																	NULL);
	
	g_free (text);
	
	if (cover->album->albumart != NULL)
		imgbuf = cover->album->albumart;
	else
	{
		Track *track;
		track = g_list_nth_data (cover->album->tracks, 0);
		imgbuf = coverart_get_track_thumb (track, track->itdb->device);
	}
		
	gint pixheight = gdk_pixbuf_get_height (imgbuf);
	gint pixwidth = gdk_pixbuf_get_width (imgbuf);
	
	gtk_window_resize ( GTK_WINDOW(dialog), pixwidth, pixheight + 40);
	
	GnomeCanvas *canvas;
	canvas = GNOME_CANVAS (gnome_canvas_new());
	gtk_widget_set_size_request ( GTK_WIDGET(canvas),
																									pixwidth,
																									pixheight);
	gnome_canvas_set_scroll_region (	canvas,
																													0.0, 0.0, 
																													pixwidth,
																													pixheight);
	GnomeCanvasItem *canvasitem;											
	canvasitem = gnome_canvas_item_new(	gnome_canvas_root(canvas),
																																		GNOME_TYPE_CANVAS_PIXBUF, NULL);
	
	/* Apply the image to the canvas */
	gnome_canvas_item_set (	canvasitem,
																						"pixbuf", imgbuf);

	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), GTK_WIDGET(canvas));
	
	/* Unreference pixbuf if it is not pointing to
	 * the album's artwork
	 */
	if (cover->album->albumart == NULL)
		gdk_pixbuf_unref (imgbuf);
	
	/* Display the dialog and block everything else until the
	 * dialog is closed.
	 */
	gtk_widget_show_all (dialog);
	gtk_dialog_run (GTK_DIALOG(dialog));
	
	/* Destroy the dialog as no longer required */
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

/**
 * coverart_get_default_track_thumb:
 *
 * Retrieve the artwork pixbuf from the default image file.
 * 
 * Returns:
 * pixbuf of the default file for tracks with no cover art.
 */
GdkPixbuf *coverart_get_default_track_thumb (void)
{
	GdkPixbuf *pixbuf = NULL;
	GError *error = NULL;
	
	pixbuf = gdk_pixbuf_new_from_file(DEFAULT_FILE, &error);
	if (error != NULL)
	{
			printf("Error occurred loading the default file - \nCode: %d\nMessage: %s\n", error->code, error->message);
			g_return_val_if_fail(pixbuf, NULL);
	}

	return pixbuf;
}

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

  if (progname)
  {
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
	     	HIGHLIGHT_FILE = g_build_filename (progname, "data", "cdshine.png", NULL);
	     	HIGHLIGHT_FILE_MAIN = g_build_filename (progname, "data", "cdshine_main.png", NULL);
	  	}
  	}
    
    g_free (progname);
    if (DEFAULT_FILE && !g_file_test (DEFAULT_FILE, G_FILE_TEST_EXISTS))
    {
	  	g_free (DEFAULT_FILE);
	  	DEFAULT_FILE = NULL;
    }
    
    if (HIGHLIGHT_FILE && !g_file_test (HIGHLIGHT_FILE, G_FILE_TEST_EXISTS))
    {
	  	g_free (HIGHLIGHT_FILE);
	  	HIGHLIGHT_FILE = NULL;
    }
    
    if (HIGHLIGHT_FILE_MAIN && !g_file_test (HIGHLIGHT_FILE_MAIN, G_FILE_TEST_EXISTS))
    {
	  	g_free (HIGHLIGHT_FILE_MAIN);
	  	HIGHLIGHT_FILE_MAIN = NULL;
    }
  }
  
  if (!DEFAULT_FILE)
  {
      DEFAULT_FILE = g_build_filename (PACKAGE_DATA_DIR, PACKAGE, "data", "default-cover.png", NULL);
  }
  if (!HIGHLIGHT_FILE)
  {
      HIGHLIGHT_FILE = g_build_filename (PACKAGE_DATA_DIR, PACKAGE, "data", "cdshine.png", NULL);
  }
  if (!HIGHLIGHT_FILE_MAIN)
  {
      HIGHLIGHT_FILE_MAIN = g_build_filename (PACKAGE_DATA_DIR, PACKAGE, "data", "cdshine_main.png", NULL);
  }
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
void on_cover_up_button_clicked (GtkWidget *widget, gpointer data)
{
	prefs_set_int (KEY_DISPLAY_COVERART, TRUE);
	
	if (cdwidget == NULL)
		coverart_set_images (TRUE);
		
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
void on_cover_down_button_clicked (GtkWidget *widget, gpointer data)
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
 * on_paned0_button_release_event:
 *
 * Callback fired when a button release event occurs on
 * paned0. Only worthwhile things are carried out when
 * the position of paned0 has changed, ie. the bar was moved.
 * Moving the bar will scale the cover images appropriately.
 * 
 * @widget: gtkpod app window
 * @event: gdk event button
 * @data: any user data passed to the function
 * 
 * Returns:
 * boolean indicating whether other handlers should be run.
 */
gboolean on_paned0_button_release_event (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
		if ( ! prefs_get_int (KEY_DISPLAY_COVERART))
		return FALSE;
		
	gint width, i;
	Cover_Item *cover;
	
	width = gtk_paned_get_position (GTK_PANED(widget));
	if ((width >= DEFAULT_WIDTH) && (width != WIDTH))
	{
		WIDTH = width;
		gnome_canvas_item_set (GNOME_CANVAS_ITEM(cdwidget->cvrtext), 
				       "x", (gdouble) WIDTH / 2,
				       NULL);
		
		for(i = 0; i < IMG_TOTAL; ++i)
  	{
  		cover = g_ptr_array_index(cdwidget->cdcovers, i);
  		set_cover_dimensions (cover, i);
  		if(i == IMG_MAIN)
				set_highlight (cover, FALSE);
			else
				set_highlight (cover, TRUE);
  	}
		
		set_covers (FALSE);
		
	}
		
	return FALSE;
}

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
static gboolean gtkpod_window_configure_callback (GtkWidget *widget, GdkEventConfigure *event, gpointer data)
{
	if (cdwidget == NULL)
		return FALSE;
		
	gnome_canvas_item_set (cdwidget->bground, 
			       "x1",(double) 0, 
			       "y1",(double) 0,
			       "x2",(double) event->width,
			       "y2",(double) event->height,
			       NULL);
	gnome_canvas_item_request_update (cdwidget->bground);
				
	return FALSE;
}

/**
 * set_display_dimensions:
 *
 * Initialises the display component width and height.
 * Sets the podpane's paned position value too.
 */
static void set_display_dimensions ()
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
static void set_cover_dimensions (Cover_Item *cover, int cover_index)
{
	gdouble x = 0, y = 0, img_width = 0, img_height = 0;
	
	SMALL_IMG_WIDTH = WIDTH / 3;
	SMALL_IMG_HEIGHT = HEIGHT / 3;
	BIG_IMG_WIDTH = WIDTH / 2;
	BIG_IMG_HEIGHT = HEIGHT / 2;
	
	img_width = SMALL_IMG_WIDTH;
	img_height = SMALL_IMG_HEIGHT;
		
		/* Set the x location of the cover image */
		switch(cover_index) {
			case 0:
				x = BORDER;
				break;
			case 1:
				x = BORDER * 1.6;
				break;
			case 2:
				x = BORDER * 2.2;
				break;
			case 3:
				x = BORDER * 2.8;
				break;
			case IMG_MAIN:
				/* The Main Image CD Cover Image */
				x = (WIDTH - BIG_IMG_WIDTH) / 2;
				img_width = BIG_IMG_WIDTH;
				break;
			case 5:
				x = WIDTH - (SMALL_IMG_WIDTH + (BORDER * 2.8));
				break;
			case 6:
				x = WIDTH - (SMALL_IMG_WIDTH + (BORDER * 2.2));
				break;
			case 7:
				x = WIDTH - (SMALL_IMG_WIDTH + (BORDER * 1.6));
				break;
			case 8:
				x =	WIDTH - (SMALL_IMG_WIDTH + BORDER);
				break;
		}
		
		/* Set the y location of the cover image */		
		switch(cover_index) {
			case 0:
			case 8:
				y = BORDER;
				break;
			case 1:
			case 7:
				y = BORDER * 3;
				break;
			case 2:
			case 6:
				y = BORDER * 5;
				break;
			case 3:
			case 5:
				y = BORDER * 7;
				break;
			case IMG_MAIN:
				/* The Main Image CD Cover Image */
				y = HEIGHT - (BIG_IMG_HEIGHT + (BORDER * 4));
				img_height = BIG_IMG_HEIGHT;	
		}
		
		cover->img_x = x;
		cover->img_y = y;
		cover->img_height = img_height;
		cover->img_width = img_width;
		
		gnome_canvas_item_set (cover->cdcvrgrp,
				       "x", (gdouble) cover->img_x,
				       "y", (gdouble) cover->img_y,
				       NULL);
		
		gnome_canvas_item_set (cover->cdreflection,
				       "y", (gdouble) (cover->img_height + 4),
				       NULL);
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
 * prepare_canvas:
 *
 * Initialise the canvas and prepare the 9 cover items, along
 * with shadow and correct positions/dimensions.
 */
static void prepare_canvas ()
{
	gint i;
	Cover_Item *cover;
	
	gtk_widget_set_size_request(GTK_WIDGET(cdwidget->canvasbox), WIDTH, HEIGHT);
	/* create a new canvas */
	cdwidget->canvas = GNOME_CANVAS(gnome_canvas_new_aa());
	g_return_if_fail (cdwidget->canvas);
	
	/* set the size and scroll region of the canvas */
	gtk_widget_set_size_request(GTK_WIDGET(cdwidget->canvas), WIDTH, HEIGHT);
	gnome_canvas_set_center_scroll_region (cdwidget->canvas, FALSE);
	
	cdwidget->bground = gnome_canvas_item_new(gnome_canvas_root(cdwidget->canvas),
			    gnome_canvas_rect_get_type(),
			    "x1",(double) 0, 
			    "y1",(double) 0,
			    "x2",(double) WIDTH,
			    "y2",(double) HEIGHT,
			    "fill_color", "black",
			    NULL);
			    
	gnome_canvas_item_lower_to_bottom(cdwidget->bground);
	
	cdwidget->cdcovers = g_ptr_array_sized_new (IMG_TOTAL);
	for(i = 0; i < IMG_TOTAL; ++i)
	{
		cover = g_new0(Cover_Item, 1);
		
		cover->cdcvrgrp = gnome_canvas_item_new(gnome_canvas_root(cdwidget->canvas),
												gnome_canvas_group_get_type(),
												NULL);
												
		cover->cdimage = gnome_canvas_item_new((GnomeCanvasGroup *) cover->cdcvrgrp,
										GNOME_TYPE_CANVAS_PIXBUF,
										NULL);
		
		cover->cdreflection = gnome_canvas_item_new((GnomeCanvasGroup *) cover->cdcvrgrp,
										GNOME_TYPE_CANVAS_PIXBUF,
										NULL);
	
		set_cover_dimensions (cover, i);
		
		cover->highlight = gnome_canvas_item_new((GnomeCanvasGroup *) cover->cdcvrgrp,
																						GNOME_TYPE_CANVAS_PIXBUF,
	    	   																	NULL);		
		
		if(i == IMG_MAIN)
		{
			set_highlight (cover, TRUE);
		
			/* set up some callback events on the main scaled image */
			g_signal_connect(GTK_OBJECT(cover->cdimage), "event", GTK_SIGNAL_FUNC(on_main_cover_image_clicked), NULL);
		
			cdwidget->cvrtext = GNOME_CANVAS_TEXT(gnome_canvas_item_new(gnome_canvas_root(cdwidget->canvas),
										GNOME_TYPE_CANVAS_TEXT,
										"text", "No Artist",
										"x", (gdouble) WIDTH / 2,
										"y", (gdouble) cover->img_y + cover ->img_height,
										"justification", GTK_JUSTIFY_CENTER,
  									"anchor", GTK_ANCHOR_NORTH,
  									"fill_color", "black",
  									"font", "-*-clean-medium-r-normal-*-12-*-*-*-*-*-*",
										NULL));
		}
		else
			set_highlight (cover, FALSE);
		
		g_ptr_array_add(cdwidget->cdcovers, cover);
		cover = NULL;
	}
	
	raise_cdimages (cdwidget->cdcovers);
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
  
	g_return_if_fail (cdwidget->contentpanel);
	g_return_if_fail (cdwidget->canvasbox);
	g_return_if_fail (cdwidget->controlbox);
	g_return_if_fail (cdwidget->leftbutton);
	g_return_if_fail (cdwidget->rightbutton);
	g_return_if_fail (cdwidget->cdslider);
  
  /* Initialise the album has backing store */
  album_hash = g_hash_table_new_full ( g_str_hash,
  																																			g_str_equal,
  																																			(GDestroyNotify) g_free,
  																																			(GDestroyNotify) free_album);
	album_key_list = NULL;
  
	set_display_dimensions ();
	
	prepare_canvas ();
	
	gtk_box_pack_start_defaults (GTK_BOX(cdwidget->canvasbox), GTK_WIDGET(cdwidget->canvas));
			
	contentpanel_signal_id = g_signal_connect (G_OBJECT(cdwidget->contentpanel), "scroll-event",
					G_CALLBACK(on_contentpanel_scroll_wheel_turned), NULL);
					
	lbutton_signal_id = g_signal_connect (G_OBJECT(cdwidget->leftbutton), "clicked",
		      G_CALLBACK(on_cover_display_button_clicked), NULL);
		      
	rbutton_signal_id = g_signal_connect (G_OBJECT(cdwidget->rightbutton), "clicked",
		      G_CALLBACK(on_cover_display_button_clicked), NULL);	
	
	slide_signal_id = g_signal_connect (G_OBJECT(cdwidget->cdslider), "value-changed",
		      G_CALLBACK(on_cover_display_slider_value_changed), NULL);
	
	window_signal_id = g_signal_connect (gtkpod_window, "configure_event", 
  		G_CALLBACK (gtkpod_window_configure_callback), NULL);
  
  gtk_widget_show_all (cdwidget->contentpanel);
  		
	coverart_block_change (FALSE);
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
      
  set_covers (FALSE);
  
  /* Set the index value of the slider but avoid causing an infinite
   * cover selection by blocking the event
   */
  g_signal_handler_block (cdwidget->cdslider, slide_signal_id);
	gtk_range_set_value (GTK_RANGE (cdwidget->cdslider), index);
	g_signal_handler_unblock (cdwidget->cdslider, slide_signal_id);
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
 * sort the covers BUT must be called after an initial coverart_set_images
 * as the latter initialises the album_key_list list 
 * 
 * @order: order type
 * 
 */
static void coverart_sort_images (GtkSortType order)
{
 	if (order == SORT_NONE)
 	{
 		/* No sorting means original order so this should have been called after a coverart_set_images (TRUE)
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
				set_covers (FALSE);
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
		
				set_covers (FALSE);
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
 							set_covers(FALSE);
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
	/* Use the index value to determine if the cover is being displayed */
 	if (index >= cdwidget->first_imgindex && index <= (cdwidget->first_imgindex + IMG_TOTAL))
 	{
 		/* Cover is being displayed so need to do some clearing up */
		coverart_clear_images ();
 	}
 			
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
 * coverart_set_images:
 *
 * Takes a list of tracks and sets the 9 image cover display.
 *
 * @clear_track_list: flag indicating whether to clear the displaytracks list or not
 *  
 */
void coverart_set_images (gboolean clear_track_list)
{
	gint i;
	GList *tracks;
	Track *track;
	Album_Item *album;
	Playlist *playlist;

	if ( ! prefs_get_int (KEY_DISPLAY_COVERART))
		return;
		
	/* initialize display if not already done */
	if (!cdwidget)  coverart_init_display ();

	/* Ensure that the setting of images hasnt been turned off
	 * due to being in the middle of a selection operation
	 */
	if (cdwidget->block_display_change)
		return;

	/* Reset the display back to black, black and more black */
	coverart_clear_images ();
	
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
			return;
		
		gchar *album_key;
		while (tracks)
		{
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
				album_key_list = g_list_append (album_key_list, g_strdup(album_key));
			}
			else
			{
				/* Album Item found in the album hash so append the track to
				 * the end of the track list
				 */
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
		
	set_covers (FALSE);
	
	set_slider_range (cdwidget->first_imgindex);
	
	/*
	printf("######### ORIGINAL LINE UP ########\n");
	debug_albums ();
	printf("######### END OF ORIGINAL LINE UP #######\n");
	*/
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
  
  set_covers (FALSE);
}

/**
 * coverart_set_cover_from_web:
 *
 * Find a cover on tinternet and apply it as the cover
 * of the main displayed album
 *
 */
void coverart_set_cover_from_web ()
{
	GList *tracks;
	Cover_Item *cover;
	
	cover = g_ptr_array_index(cdwidget->cdcovers, IMG_MAIN);
	tracks = cover->album->tracks;
	
	/*
	int i;
	for (i = 0; i < g_list_length (tracks); ++i)
	{
		Track *trk = g_list_nth_data(tracks, i);
		printf ("Track: %s-%s\n", trk->artist, trk->album);
	}
	*/
	
	/* Nullify and free the album art pixbuf so that it will pick it up
	 * from the art assigned to the tracks
	 */
	 if (cover->album->albumart)
	 {
		gdk_pixbuf_unref (cover->album->albumart);
		cover->album->albumart = NULL;
	 }
	 
	on_coverart_context_menu_click (tracks);
	
	set_covers (FALSE);
}
