/* Time-stamp: <2007-02-23 00:04:30 jcs>
|
|  Copyright (C) 2002-2005 Jorg Schuler <jcsjcs at users sourceforge net>
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

/* Declarations */
static void set_display_dimensions ();
static GdkPixbuf *draw_blank_cdimage ();
static void set_highlight (Cover_Item *cover);
static void raise_cdimages (GPtrArray *cdcovers);
static GdkPixbuf *get_thumb (Itdb_Track *track);
/*static void scroll_covers (gint direction);*/
static void on_cover_display_button_clicked (GtkWidget *widget, gpointer data);
static void on_main_cover_image_clicked (GnomeCanvas *canvas, GdkEvent *event, gpointer data);
static void on_cover_display_slider_value_changed (GtkRange *range, gpointer user_data);
static void set_cover_dimensions (Cover_Item *cover, int cover_index);
static void prepare_canvas ();
static void set_covers ();

/* The structure that holds values used throughout all the functions */
static CD_Widget *cdwidget = NULL;
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

#if 0
static void debug_albums ()
{
	gint i;
	Cover_Item *cover;
	Itdb_Track *track;
	
	printf("Album list\n");
	for(i = 0; i < g_list_length(cdwidget->displaytracks); ++i)
	{
		track = g_list_nth_data(cdwidget->displaytracks, i);
		if (track == NULL)
			printf("Track is null\n");
		else
			printf("Track Details: Artist = %s, Album = %s\n", track->artist, track->album);
	}
	
	printf("Cover List\n");
	for(i = 0; i < IMG_TOTAL; ++i)
  {
  	cover = g_ptr_array_index(cdwidget->cdcovers, i);
  	if (cover->track == NULL)
  		printf("Track is null\n");
  	else
  		printf("Cover Details: Artist = %s, Album = %s\n", cover->track->artist, cover->track->album);
  }
}
#endif

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
static void set_highlight (Cover_Item *cover)
{
    GdkPixbuf *image;
    GError *error = NULL;
    GdkPixbuf *scaled;

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
 * set_covers:
 *
 * Internal function responsible for the resetting of the artwork
 * covers in response to some kind of change in selection, eg. new
 * selection in sort tab, button click etc...
 * 
 */
static void set_covers ()
{
    GdkPixbuf *imgbuf, *reflection; 
    gint i, dataindex;
    Itdb_Track *track;
    Cover_Item *cover;

    for(i = 0; i < IMG_TOTAL; ++i)
    {
	cover = g_ptr_array_index(cdwidget->cdcovers, i);
	dataindex = cdwidget->first_imgindex + i;

	track = g_list_nth_data (cdwidget->displaytracks, dataindex);
	if (track == NULL)
	{
	    imgbuf = draw_blank_cdimage ();
	    gnome_canvas_item_hide (cover->highlight);
	}
	else
	{
	    imgbuf = get_thumb (track);
	    gnome_canvas_item_show (cover->highlight);	
	}

	if (imgbuf != NULL)
	{
	    GdkPixbuf *scaled;
	    /* Set the pixbuf into the cd image */
	    cover->track = track;
	    scaled = gdk_pixbuf_scale_simple (imgbuf, cover->img_width, cover->img_height, GDK_INTERP_NEAREST);
	    gdk_pixbuf_unref (imgbuf);
	    gnome_canvas_item_set (cover->cdimage,
				   "pixbuf", scaled,
				   NULL);
	    /* flip image vertically to create reflection */
	    reflection = gdk_pixbuf_flip (scaled, FALSE);
	    gnome_canvas_item_set (cover->cdreflection,
				   "pixbuf", reflection,
				   NULL);
	    gdk_pixbuf_unref (scaled);
	    gdk_pixbuf_unref (reflection);
	}

	if (i == IMG_MAIN)
	{
	    gchar *text;
	    if (track)
	    {
		text = g_strconcat (track->artist, "\n", track->album, NULL);
	    }
	    else
	    {
		text = g_strdup ("");
	    }
	    /* Set the text to display details of the main image */
	    gnome_canvas_item_set (GNOME_CANVAS_ITEM (cdwidget->cvrtext),
				   "text", text,
				   "fill_color", "white",
				   "justification", GTK_JUSTIFY_CENTER,
				   NULL);
	    g_free (text);
	}
    }
}

#if 0
/**
 * scroll_covers:
 *
 * Performs the work of cycling the cd covers when the left or
 * right buttons are clicked.
 * 
 * @direction: the direction of the cycling of the covers, either
 * IMG_PREV or IMG_NEXT.
 */
static void scroll_covers(gint direction)
{
	gint displaytotal;
  
  displaytotal = g_list_length(cdwidget->displaytracks);
  
  if (displaytotal <= 0)
  	return;
  
  switch (direction)
	{
		case IMG_NEXT:
  		cdwidget->first_imgindex++;
   		/* Check there is another possible image to display */
  		if ((cdwidget->first_imgindex + IMG_TOTAL) > displaytotal)
  		{
  			/* Return to as it was and get out */
  			cdwidget->first_imgindex--;
  			return;
  		}
  		break;
  	case IMG_PREV:
  		cdwidget->first_imgindex--;
   		/* Check the index has not dropped below 0 */
  		if (cdwidget->first_imgindex < 0)
  		{
  			/* Return to as it was and get out */
  			cdwidget->first_imgindex++;
  			return;
  		}
  		break;
	}
  
  set_covers();
  
}
#endif


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
	displaytotal = g_list_length(cdwidget->displaytracks);
  
  if (displaytotal <= 0)
  	return;
  
  /* Use the index value from the slider for the main image index */
  cdwidget->first_imgindex = index - IMG_MAIN - 1;
  if (cdwidget->first_imgindex < 0)
  	cdwidget->first_imgindex = 0;
  else if((cdwidget->first_imgindex + IMG_TOTAL) >= displaytotal)
  	cdwidget->first_imgindex = displaytotal - IMG_TOTAL;
      
  set_covers ();
  
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
  
  button = GTK_BUTTON(widget);
  label = gtk_button_get_label(button);
    
  if(g_str_equal(label, (gchar *) ">"))
  	cdwidget->first_imgindex++;
  else
   	cdwidget->first_imgindex--;
    	
	/* Change the value of the slider to do the work of scrolling the covers */
	gtk_range_set_value (GTK_RANGE (cdwidget->cdslider), (cdwidget->first_imgindex + IMG_MAIN + 1));

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
static void on_main_cover_image_clicked (GnomeCanvas *canvas, GdkEvent *event, gpointer data)
{
	Cover_Item *cover;
	gboolean status;
	
	if(event->type != GDK_BUTTON_PRESS)
		return;
	
	cover = g_ptr_array_index(cdwidget->cdcovers, IMG_MAIN);
	
	/* Stop redisplay of the artwork as its already
	 * in the correct location
	 */
	coverart_block_change (TRUE);
	
	/* Select the correct track in the sorttabs */
	status = st_set_selection (cover->track);
	
	/* Turn the display change back on */
	coverart_block_change (FALSE);
	
	g_return_if_fail (status);
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
		buf2 = gdk_pixbuf_scale_simple(buf, cover->img_width, cover->img_height, GDK_INTERP_NEAREST);
		gnome_canvas_item_set(cover->cdimage, "pixbuf", buf2, NULL);
		gnome_canvas_item_set(cover->cdreflection, "pixbuf", buf2, NULL);
		gnome_canvas_item_hide (cover->highlight);
		
		g_object_unref (buf2);
		
		/* Reset track list too */
		cover->track = NULL;
		
		if (i == IMG_MAIN)
		{
			/* Set the text to display details of the main image */
		    gnome_canvas_item_set (GNOME_CANVAS_ITEM (cdwidget->cvrtext),
					   "text", "No Artist",
					   "fill_color", "black",
					   NULL);
		}
	}
	g_object_unref(buf);
}

/**
 * get_thumb:
 *
 * Retrieve the artwork pixbuf from the given track.
 * 
 * @track: Track from where the pixbuf is obtained.
 * 
 * Returns:
 * pixbuf referenced by the provided track or the pixbuf of the
 * default file if track has no cover art.
 */
static GdkPixbuf *get_thumb (Itdb_Track *track)
{
	Itdb_Thumb *thumb = NULL;
	GdkPixbuf *pixbuf = NULL;
	GError *error = NULL;
		
	thumb = itdb_artwork_get_thumb_by_type (track->artwork, ITDB_THUMB_COVER_LARGE);
	if(thumb == NULL)
		thumb = itdb_artwork_get_thumb_by_type (track->artwork, ITDB_THUMB_COVER_SMALL);
	
	/* Track has a viable thumbnail */
	if(thumb != NULL)
	{
		pixbuf = GDK_PIXBUF (
	  				itdb_thumb_get_gdk_pixbuf (track->itdb->device, thumb) 
						);
	}
	else
	{
	 	/* Could not get a viable thumbnail so get default pixbuf */
	 	pixbuf = gdk_pixbuf_new_from_file(DEFAULT_FILE, &error);
		if (error != NULL)
		{
			printf("Error occurred loading the default file - \nCode: %d\nMessage: %s\n", error->code, error->message);
			g_return_val_if_fail(pixbuf, NULL);
		}
	}
	
	return pixbuf;
}

/**
 * search_tracks:
 *
 * find function used by glist_find_custom to compare 2
 * tracks
 * 
 * @a: First track to compare
 * @b: Second track to compare
 * 
 * Returns:
 * 0 if the two tracks match
 * -1 if the two tracks are different
 *  
 */
static gint search_tracks (Itdb_Track *a, Itdb_Track *b)
{
	if(a == NULL || b == NULL)
		return -1;
		
	if ((g_ascii_strcasecmp (a->artist, b->artist) == 0) && (g_ascii_strcasecmp (a->album, b->album) == 0))
		return 0;
				
	return -1;
}

/**
 * init_default_file:
 *
 * Initialises the image file used if an album has no cover. This
 * needs to be loaded early as it uses the path of the binary
 * to determine where to load the file from, in the same way as
 * main() determines where to load the glade file from.
 *
 * @progpath: path of the gtkpod binary being loaded.
 *  
 */
void init_default_file (gchar *progpath)
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
	     	DEFAULT_FILE = g_build_filename (progname, "pixmaps", "default-cover.png", NULL);
	     	HIGHLIGHT_FILE = g_build_filename (progname, "pixmaps", "cdshine.png", NULL);
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
    
  }
  
  if (!DEFAULT_FILE)
  {
      DEFAULT_FILE = g_build_filename (PACKAGE_DATA_DIR, PACKAGE, "pixmaps", "default-cover.png", NULL);
  }
  if (!HIGHLIGHT_FILE)
  {
      HIGHLIGHT_FILE = g_build_filename (PACKAGE_DATA_DIR, PACKAGE, "pixmaps", "cdshine.png", NULL);
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
	gtk_widget_show_all (cdwidget->contentpanel);
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
	gtk_widget_hide_all (cdwidget->contentpanel);
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
		set_highlight (cover);	
  	}
		
		set_covers ();
		
	}
		
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
static gboolean gtkpod_window_configure_callback (GtkWidget *widget, GdkEventConfigure *event, gpointer data)
{				
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
	GtkWidget *gtkpod_window;
	GtkWidget *podpane;

	gtkpod_window = gtkpod_xml_get_widget (main_window_xml, "gtkpod");
	g_return_if_fail (gtkpod_window);
		  		
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
 * set_cover_dimensions::
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

		set_highlight (cover);
		
		if(i == IMG_MAIN)
		{
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
		
		g_ptr_array_add(cdwidget->cdcovers, cover);
		cover = NULL;
	}
	
	raise_cdimages (cdwidget->cdcovers);
}

/**
 * coverart_init_display:
 *
 * Initialise the boxes and canvases of the coverart_display.
 *  
 */
static void coverart_init_display ()
{		
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
  
	set_display_dimensions ();
	
	prepare_canvas ();
	
	gtk_box_pack_start_defaults (GTK_BOX(cdwidget->canvasbox), GTK_WIDGET(cdwidget->canvas));
			
	g_signal_connect (G_OBJECT(cdwidget->leftbutton), "clicked",
		      G_CALLBACK(on_cover_display_button_clicked), NULL);
		      
	g_signal_connect (G_OBJECT(cdwidget->rightbutton), "clicked",
		      G_CALLBACK(on_cover_display_button_clicked), NULL);	
	
	g_signal_connect (G_OBJECT(cdwidget->cdslider), "value-changed",
		      G_CALLBACK(on_cover_display_slider_value_changed), NULL);
	
	g_signal_connect (gtkpod_window, "configure_event", 
  		G_CALLBACK (gtkpod_window_configure_callback), NULL);
  		
	coverart_block_change (FALSE);
	
	gtk_widget_show_all (cdwidget->contentpanel);
}

/**
 * sort_tracks:
 *
 * sort function used by glist_sort to compare 2
 * tracks and determine their order based on firstly
 * the artist and then the album
 * 
 * @a: First track from list to compare
 * @b: Second track from list to compare
 * 
 * Returns:
 * integer indicating order of tracks
 *  
 */
gint sort_tracks (Itdb_Track *a, Itdb_Track *b)
{
	gint artistval;
	
	g_return_val_if_fail (a, -1);
	g_return_val_if_fail (b, -1);
	
	artistval = g_ascii_strcasecmp (a->artist, b->artist);
	
	/* Artists are not the same so return, no more checking needed */
	if(artistval != 0)
		return artistval;
	else
		return g_ascii_strcasecmp (a->album, b->album);
}

/**
 * coverart_select_cover
 * 
 * When a track / album is selected, the artwork cover
 * is selected in the display
 * 
 * @track: chosen album
 * 
 */
void coverart_select_cover (Itdb_Track *track)
{
 	GList *selectedtrk;
  gint displaytotal, index;
		
	/* Only select covers if fire display change is enabled */
	if (cdwidget->block_display_change)
		return;
		
	displaytotal = g_list_length(cdwidget->displaytracks);
  if (displaytotal <= 0)
  	return;
  	
 	/* Find the track that matches the given album */
 	selectedtrk = g_list_find_custom (cdwidget->displaytracks, track, (GCompareFunc) search_tracks); 
 	g_return_if_fail (selectedtrk);
 
 	/* Determine the index of the found track */
 	index = g_list_position (cdwidget->displaytracks, selectedtrk);
 	
 	/* Use the index value for the main image index */
  cdwidget->first_imgindex = index - IMG_MAIN;
  if (cdwidget->first_imgindex < 0)
  	cdwidget->first_imgindex = 0;
  else if((cdwidget->first_imgindex + IMG_TOTAL) >= displaytotal)
  	cdwidget->first_imgindex = displaytotal - IMG_TOTAL;
      
  set_covers ();
 }


/**
 * coverart_sort_images:
 * 
 * When the alphabetize function is initiated this will
 * sort the covers in the same way. Used at any point to
 * sort the covers BUT must be called after an initial coverart_set_images
 * as the latter initialises the cdwidget->displaytracks list 
 * 
 * @order: order type
 * 
 */
void coverart_sort_images (GtkSortType order)
{ 
 	/*
	if (order == SORT_NONE)
 	{
 		pl = pm_get_selected_playlist ();
 		newlist = pl->members;
 		return;
 	}
 	else
 	{
 	*/
 	
 	if (order == SORT_NONE)
 	{
 		/* Due to list being prepended together and remaining unsorted need to reverse */
 		cdwidget->displaytracks = g_list_reverse (cdwidget->displaytracks);
 	}
 	else
 	{
 		/* Remove all the null tracks before sorting */	
 		cdwidget->displaytracks = g_list_remove_all (cdwidget->displaytracks, NULL);
 		cdwidget->displaytracks = g_list_sort (cdwidget->displaytracks, (GCompareFunc) sort_tracks);
 	}
 	
 	if (order == GTK_SORT_DESCENDING)
 	{
 		cdwidget->displaytracks = g_list_reverse (cdwidget->displaytracks);		
 	}
 }

/**
 * coverart_set_images:
 *
 * Takes a list of tracks and sets the 9 image cover display.
 *
 * @tracks: list of tracks taken from a playlist
 *  
 */
void coverart_set_images (GList *tracks)
{
	gint i;
	Itdb_Track *track;

	/* initialize display if not already done */
	if (!cdwidget)  coverart_init_display();

	/* Ensure that the setting of images hasnt been turned off
	 * due to being in the middle of a selection operation
	 */
	if (cdwidget->block_display_change)
		return;
	
	coverart_clear_images ();
	cdwidget->first_imgindex = 0;
  
  g_list_free (cdwidget->displaytracks);
	cdwidget->displaytracks = NULL; 
	
	if (tracks == NULL)
		return;
		
	while (tracks)
	{
		track = tracks->data;
		
		if (g_list_find_custom (cdwidget->displaytracks, track, (GCompareFunc) search_tracks) == NULL)
			cdwidget->displaytracks = g_list_prepend (cdwidget->displaytracks, track);

		tracks = tracks->next;
	}
	
	/* Sort the tracks to the order set in the preference */
	coverart_sort_images (prefs_get_int("st_sort"));
	
	/* Add 4 null tracks to the end of the track list for padding */
	for (i = 0; i < IMG_MAIN; ++i)
		cdwidget->displaytracks = g_list_append (cdwidget->displaytracks, NULL);
	
	/* Add 4 null tracks to the start of the track list for padding */
	for (i = 0; i < IMG_MAIN; ++i)
		cdwidget->displaytracks = g_list_prepend (cdwidget->displaytracks, NULL);
	
	set_covers ();
	
	/* Set the scale range */
	gtk_range_set_range (GTK_RANGE (cdwidget->cdslider), 0, g_list_length (cdwidget->displaytracks) - 1);
	
	gtk_range_set_value (GTK_RANGE (cdwidget->cdslider), 0);
	
	/*printf("######### ORIGINAL LINE UP ########\n");
	debug_albums ();
	printf("######### END OF ORIGINAL LINE UP #######\n");*/
}

/**
 * coverart_block_change:
 *
 * Select covers events can be switched off when automatic
 * slections of tracks are taking place.
 *
 * @val: indicating whether to block or unblock select cover events
 *  
 */
void coverart_block_change (gboolean val)
{
	if (cdwidget != NULL)
		cdwidget->block_display_change = val;
}
