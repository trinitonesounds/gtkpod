/* Time-stamp: <2005-11-25 00:27:50 jcs>
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

/* This file provides functions for the details window */

#include <stdlib.h>
#include <gtk/gtk.h>
#include "details.h"
#include "misc.h"
#include "misc_track.h"
#include "prefs.h"

/*
void details_close (void);
void details_update_default_sizes (void);
void details_update_track (Track *track);
void details_remove_track (Track *track);
*/

/* List with all detail windows */
static GList *details = NULL;

struct _Detail
{
    /* XML info */
    GladeXML *xml; 
    /* pointer to details window */
    GtkWidget *window;
    /* tracks displayed in details window */
    GList *orig_tracks;
    /* tracks displayed in details window */
    GList *tracks;
    /* currently displayed track */
    Track *track;
    /* index of currently displayed track */
    gint tracknr;
};

typedef struct _Detail Detail;


/* string constants for preferences */
static const gchar *DETAILS_WINDOW_DEFX="details_window_defx";
static const gchar *DETAILS_WINDOW_DEFY="details_window_defy";
static const gchar *DETAILS_WINDOW_NOTEBOOK_PAGE="details_window_notebook_page";


/* Declarations */
static void details_set_track (Detail *detail, Track *track);
static void details_free (Detail *detail);



/* Store the window size */
static void details_store_window_state (Detail *detail)
{
    GtkWidget *w;
    gint defx, defy;

    g_return_if_fail (detail);

    gtk_window_get_size (GTK_WINDOW (detail->window), &defx, &defy);
    prefs_set_int_value (DETAILS_WINDOW_DEFX, defx);
    prefs_set_int_value (DETAILS_WINDOW_DEFY, defy);

    if ((w = gtkpod_xml_get_widget (detail->xml, "details_notebook")))
    {
	gint page = gtk_notebook_get_current_page (GTK_NOTEBOOK (w));
	prefs_set_int_value (DETAILS_WINDOW_NOTEBOOK_PAGE, page);
    }
}



/* ------------------------------------------------------------
 *
 *        Callback functions
 *
 * ------------------------------------------------------------ */

static void details_entry_activate (GtkEntry *entry,
				    Detail *detail)
{
    T_item item;

    g_return_if_fail (entry);

    item = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (entry),
					       "details_item"));

    g_return_if_fail ((item > 0) && (item < T_ITEM_NUM));

    printf ("details_entry_activate: %d\n", item);
}


static void details_checkbutton_toggled (GtkCheckButton *button,
					 Detail *detail)
{
    T_item item;

    g_return_if_fail (button);

    item = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button),
					       "details_item"));
    
    g_return_if_fail ((item > 0) && (item < T_ITEM_NUM));

    printf ("details_checkbutton_toggled: %d\n", item);
}


/****** Navigation *****/
void details_button_first_clicked (GtkCheckButton *button,
				   Detail *detail)
{
    GList *first;
    g_return_if_fail (detail);

    first = g_list_first (detail->tracks);

    if (first)
	details_set_track (detail, first->data);
}

void details_button_previous_clicked (GtkCheckButton *button,
				      Detail *detail)
{
    g_return_if_fail (detail);

    if (detail->tracknr != 0)
	details_set_track (detail,
			   g_list_nth_data (detail->tracks,
					    detail->tracknr - 1));
}

void details_button_next_clicked (GtkCheckButton *button,
				  Detail *detail)
{
    Track *next;
    g_return_if_fail (detail);

    next = g_list_nth_data (detail->tracks, detail->tracknr + 1);
    if (next)
	details_set_track (detail, next);
}

void details_button_last_clicked (GtkCheckButton *button,
				  Detail *detail)
{
    GList *last;
    g_return_if_fail (detail);

    last = g_list_last (detail->tracks);
    if (last)
	details_set_track (detail, last->data);
}


/****** Window Control *****/
static void details_button_apply_clicked (GtkButton *button,
					  Detail *detail)
{

}

static void details_button_cancel_clicked (GtkButton *button,
					   Detail *detail)
{
    g_return_if_fail (detail);

    details_store_window_state (detail);

    details = g_list_remove (details, detail);

    details_free (detail);
}

static void details_button_ok_clicked (GtkButton *button,
				       Detail *detail)
{

}

static void details_delete_event (GtkWidget *widget,
				  GdkEvent *event,
				  Detail *detail)
{
    details_button_cancel_clicked (NULL, detail);
}




/****** Setup of widgets ******/
static void details_setup_widget (Detail *detail, T_item item)
{
    GtkWidget *w;
    gchar *buf;

    g_return_if_fail (detail);
    g_return_if_fail ((item > 0) && (item < T_ITEM_NUM));

    buf = g_strdup_printf ("details_label_%d", item);
    if ((w = gtkpod_xml_get_widget (detail->xml, buf)))
    {
	gtk_label_set_text (GTK_LABEL (w),
			    gettext (get_t_string (item)));
    }
    g_free (buf);

    buf = NULL;
    w = NULL;

    switch (item)
    {
    case T_ALBUM:
    case T_ARTIST:
    case T_TITLE:
    case T_GENRE:
    case T_COMPOSER:
    case T_FILETYPE:
    case T_GROUPING:
    case T_CATEGORY:
    case T_PODCASTURL:
    case T_PODCASTRSS:
    case T_PC_PATH:
    case T_IPOD_PATH:
    case T_IPOD_ID:
    case T_SIZE:
    case T_TRACKLEN:
    case T_BITRATE:
    case T_SAMPLERATE:
    case T_PLAYCOUNT:
    case T_BPM:
    case T_RATING:
    case T_VOLUME:
    case T_SOUNDCHECK:
    case T_CD_NR:
    case T_TRACK_NR:
    case T_YEAR:
    case T_TIME_ADDED:
    case T_TIME_PLAYED:
    case T_TIME_MODIFIED:
    case T_TIME_RELEASED:
	buf = g_strdup_printf ("details_entry_%d", item);
	if ((w = gtkpod_xml_get_widget (detail->xml, buf)))
	{
	    g_signal_connect (w, "activate",
			      G_CALLBACK (details_entry_activate),
			      detail);
	}
	break;
    case T_COMPILATION:
    case T_TRANSFERRED:
    case T_CHECKED:
	buf = g_strdup_printf ("details_checkbutton_%d", item);
	if ((w = gtkpod_xml_get_widget (detail->xml, buf)))
	{
	    g_signal_connect (w, "toggled",
			      G_CALLBACK (details_checkbutton_toggled),
			      detail);
	}
	break;
    case T_DESCRIPTION:
    case T_SUBTITLE:
    case T_COMMENT:
	buf = g_strdup_printf ("details_textview_%d", item);
	if ((w = gtkpod_xml_get_widget (detail->xml, buf)))
	{

	}
	break;
    case T_ALL:
    case T_ITEM_NUM:
	/* cannot happen because of assertion above */
	g_return_if_reached ();
    }

    if (w)
    {
	g_object_set_data (G_OBJECT (w),
			   "details_item", GINT_TO_POINTER (item));
    }

    g_free (buf);
}


static void details_set_item (Detail *detail, Track *track, T_item item)
{
    GtkWidget *w = NULL;
    gchar *text;
    gchar *entry, *checkbutton, *textview;

    g_return_if_fail (detail);
    g_return_if_fail (track);
    g_return_if_fail ((item > 0) && (item < T_ITEM_NUM));

    entry = g_strdup_printf ("details_entry_%d", item);
    checkbutton = g_strdup_printf ("details_checkbutton_%d", item);
    textview = g_strdup_printf ("details_textview_%d", item);
    text = track_get_text (track, item);

    switch (item)
    {
    case T_ALBUM:
    case T_ARTIST:
    case T_TITLE:
    case T_GENRE:
    case T_COMPOSER:
    case T_FILETYPE:
    case T_GROUPING:
    case T_CATEGORY:
    case T_PODCASTURL:
    case T_PODCASTRSS:
    case T_PC_PATH:
    case T_IPOD_PATH:
    case T_IPOD_ID:
    case T_SIZE:
    case T_TRACKLEN:
    case T_BITRATE:
    case T_SAMPLERATE:
    case T_PLAYCOUNT:
    case T_BPM:
    case T_RATING:
    case T_VOLUME:
    case T_SOUNDCHECK:
    case T_CD_NR:
    case T_TRACK_NR:
    case T_YEAR:
    case T_TIME_ADDED:
    case T_TIME_PLAYED:
    case T_TIME_MODIFIED:
    case T_TIME_RELEASED:
	if ((w = gtkpod_xml_get_widget (detail->xml, entry)))
	{
	    gtk_entry_set_text (GTK_ENTRY (w), text);
	}
	break;
    case T_COMMENT:
    case T_DESCRIPTION:
    case T_SUBTITLE:
	if ((w = gtkpod_xml_get_widget (detail->xml, textview)))
	{
	    GtkTextBuffer *tb = gtk_text_view_get_buffer (
		GTK_TEXT_VIEW (w));
	    gtk_text_buffer_set_text (tb, text, -1);
	}
	break;
    case T_COMPILATION:
	if ((w = gtkpod_xml_get_widget (detail->xml, checkbutton)))
	{
	    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
					  track->compilation);
	}
	break;
    case T_TRANSFERRED:
	if ((w = gtkpod_xml_get_widget (detail->xml, checkbutton)))
	{
	    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
					  track->transferred);
	}
	break;
    case T_CHECKED:
	if ((w = gtkpod_xml_get_widget (detail->xml, checkbutton)))
	{
	    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
					  !track->checked);
	}
	break;
    case T_ALL:
    case T_ITEM_NUM:
	/* cannot happen because of assertion above */
	g_return_if_reached ();
    }

    g_free (entry);
    g_free (checkbutton);
    g_free (textview);
    g_free (text);
}


/* Set the display to @track */
static void details_set_track (Detail *detail, Track *track)
{
    GtkWidget *w;
    gint index;
    T_item item;
    gboolean first, last;

    g_return_if_fail (detail);
    g_return_if_fail (track);
    g_return_if_fail (track->itdb);

    index = g_list_index (detail->tracks, track);
    g_return_if_fail (index != -1);

    detail->tracknr = index;
    detail->track = track;

    if ((w = gtkpod_xml_get_widget (detail->xml, "details_label_index")))
    {
	gchar *buf = g_strdup_printf ("%d / %d",
				      index+1,
				      g_list_length (detail->tracks));
	gtk_label_set_text (GTK_LABEL (w), buf);
	g_free (buf);
    }

    for (item=1; item<T_ITEM_NUM; ++item)
    {
	details_set_item (detail, track, item);
    }

    /* Set Artist/Title label */
    if ((w = gtkpod_xml_get_widget (detail->xml,
				   "details_label_artist_title")))
    {
	gchar *buf = g_markup_printf_escaped ("<b>%s / %s</b>",
				      track->artist, track->title);
	gtk_label_set_markup (GTK_LABEL (w), buf);
	g_free (buf);
    }

    /* Set thumbnail */
    if ((w = gtkpod_xml_get_widget (detail->xml, "details_image_thumbnail")))
    {
	GList *gl;
	GtkImage *img = GTK_IMAGE (w);

	gtk_image_set_from_pixbuf (img, NULL);

	/* Find large cover */
	for (gl=track->thumbnails; gl; gl=gl->next)
	{
	    Image *thumb = gl->data;
	    g_return_if_fail (thumb);

	    if (thumb->type == 1)
	    {   /* 1 == IPOD_COVER_LARGE */
		GdkPixbuf *pixbuf;
		pixbuf = itdb_image_get_gdk_pixbuf (track->itdb, thumb);
		if (pixbuf)
		{
		    gtk_image_set_from_pixbuf (img, pixbuf);
		    gdk_pixbuf_unref (pixbuf);
		}
	    }
	}

	if (gtk_image_get_storage_type (img) == GTK_IMAGE_EMPTY)
	{
	    gtk_image_set_from_stock (img, GTK_STOCK_MISSING_IMAGE,
				      GTK_ICON_SIZE_DIALOG);
	}
    }
    

    /* inactivate prev/next buttons when at the beginning/end of the
     * list */
    if (detail->tracknr == 0)  first = TRUE;
    else                       first = FALSE;
    if (detail->tracknr == (g_list_length (detail->tracks)-1))
	   last = TRUE;
    else   last = FALSE;
    if ((w = gtkpod_xml_get_widget (detail->xml, "details_button_first")))
	gtk_widget_set_sensitive (w, !first);
    if ((w = gtkpod_xml_get_widget (detail->xml, "details_button_previous")))
	gtk_widget_set_sensitive (w, !first);
    if ((w = gtkpod_xml_get_widget (detail->xml, "details_button_next")))
	gtk_widget_set_sensitive (w, !last);
    if ((w = gtkpod_xml_get_widget (detail->xml, "details_button_last")))
	gtk_widget_set_sensitive (w, !last);
}


/* Set the first track of @tracks. If detail->tracks is already set,
 * replace. */
static void details_set_tracks (Detail *detail, GList *tracks)
{
    g_return_if_fail (detail);

    if (detail->tracks)
	g_list_free (detail->tracks);
    detail->tracks = glist_duplicate (tracks);

    detail->track = NULL;

    details_set_track (detail, g_list_nth_data (detail->tracks, 0));
}



/* Open the details window and display the selected tracks, starting
 * with the first track */
void details_show (GList *selected_tracks)
{
    Detail *detail;
    GtkWidget *w;
    gint defx, defy;
    T_item i;

    g_return_if_fail (selected_tracks);

    detail = g_malloc0 (sizeof (Detail));

    detail->xml = glade_xml_new (xml_file, "details_window", NULL);
/*  no signals to connect -> comment out */
/*     glade_xml_signal_autoconnect (detail->xml); */
    detail->window = gtkpod_xml_get_widget (detail->xml, "details_window");
    g_return_if_fail (detail->window);

    details = g_list_append (details, detail);

    for (i=1; i<T_ITEM_NUM; ++i)
    {
	details_setup_widget (detail, i);
    }

    /* Navigation */
    if ((w = gtkpod_xml_get_widget (detail->xml, "details_button_first")))
	g_signal_connect (w, "clicked",
			  G_CALLBACK (details_button_first_clicked),
			  detail);
    if ((w = gtkpod_xml_get_widget (detail->xml, "details_button_previous")))
	g_signal_connect (w, "clicked",
			  G_CALLBACK (details_button_previous_clicked),
			  detail);
    if ((w = gtkpod_xml_get_widget (detail->xml, "details_button_next")))
	g_signal_connect (w, "clicked",
			  G_CALLBACK (details_button_next_clicked),
			  detail);
    if ((w = gtkpod_xml_get_widget (detail->xml, "details_button_last")))
	g_signal_connect (w, "clicked",
			  G_CALLBACK (details_button_last_clicked),
			  detail);

    /* Window control */
    if ((w = gtkpod_xml_get_widget (detail->xml, "details_button_apply")))
    {
	g_signal_connect (w, "clicked",
			  G_CALLBACK (details_button_apply_clicked),
			  detail);
	gtk_widget_set_sensitive (w, FALSE);
    }
    if ((w = gtkpod_xml_get_widget (detail->xml, "details_button_cancel")))
	g_signal_connect (w, "clicked",
			  G_CALLBACK (details_button_cancel_clicked),
			  detail);
    if ((w = gtkpod_xml_get_widget (detail->xml, "details_button_ok")))
    {
	g_signal_connect (w, "clicked",
			  G_CALLBACK (details_button_ok_clicked),
			  detail);
	gtk_widget_set_sensitive (w, FALSE);
    }
    g_signal_connect (detail->window, "delete_event",
		      G_CALLBACK (details_delete_event), detail);

    details_set_tracks (detail, selected_tracks);

    /* set notebook page */
    if ((w = gtkpod_xml_get_widget (detail->xml, "details_notebook")))
    {
	gint page = prefs_get_int (DETAILS_WINDOW_NOTEBOOK_PAGE);
	if ((page >= 0) && (page < 3))
	    gtk_notebook_set_current_page (GTK_NOTEBOOK (w), page);
    }

    /* set default size */
    defx = prefs_get_int (DETAILS_WINDOW_DEFX);
    defy = prefs_get_int (DETAILS_WINDOW_DEFY);

    if ((defx != 0) && (defy != 0))
/* 	gtk_window_set_default_size (GTK_WINDOW (detail->window), */
/* 				     defx, defy); */
	gtk_window_resize (GTK_WINDOW (detail->window),
			   defx, defy);

    gtk_widget_show (detail->window);
}


/* Free memory taken by @detail */
static void details_free (Detail *detail)
{
    g_return_if_fail (detail);

    /* FIXME: how do we free the detail->xml? */

    if (detail->window)
    {
	gtk_widget_destroy (detail->window);
    }

    if (detail->orig_tracks)
    {
	g_list_free (detail->orig_tracks);
    }

    if (detail->tracks)
    {
	g_list_free (detail->tracks);
    }

    g_free (detail);
}

