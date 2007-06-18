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
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "fetchcover.h"
#include "display_coverart.h"
#include <glib/gprintf.h>
#include <glib/gstdio.h>

#define FETCHCOVER_DEBUG 1

static void fetchcover_statusbar_update (gchar *message);
static GtkDialog *fetchcover_display_dialog (Track *track, Itdb_Device *device);
static void fetchcover_debug(const gchar *format, ...);

typedef struct
{
	GdkPixbuf *image;
	GString *url;
	gchar *dir;
	gchar *filename;
} Fetch_Cover;

/* Track to search for a cover for */
static Track *fetchcover_track = NULL;
/* List of possible covers, including existing one (maybe default cover) */
static GList *fetchcover_image_list = NULL;
/* Index for controlling which cover image to display */
static gint displayed_cover_index = 0;
/* Pointer to the currently display cover */
static Fetch_Cover *displayed_cover;
/* Canvas widget for display of cover */
static GnomeCanvas *fetchcover_canvas;
/* Canvas Item widget for mounting the cover pixbuf on */
static GnomeCanvasItem *fetchcover_canvasitem;
/* Previous and Next buttons for cycling images */
static GtkWidget *next_button;
static GtkWidget *prev_button;
/* Status bar for displaying status messages */
static GtkWidget *fetchcover_statusbar;
/* Flag indicating whether a new net search should be initiated */
static gboolean netsearched = FALSE;
/* Display a dialog explaining the options if a file with the proposed name already exists */
static gchar *display_file_exist_dialog (gchar *filename);

#define IMGSCALE 256

#ifdef HAVE_CURL

#include <curl/curl.h>

/* Declarations */
static void free_fetchcover_list();
static void *safe_realloc(void *ptr, size_t size);
static size_t curl_write_fetchcover_func(void *ptr, size_t itemsize, size_t numitems, void *data);
static void net_search_track ();
static void net_retrieve_image (GString *url);
static void fetchcover_next_button_clicked (GtkWidget *widget, gpointer data);
static void fetchcover_prev_button_clicked (GtkWidget *widget, gpointer data);
static void fetchcover_cleanup();
static gchar *fetchcover_save ();

struct chunk
{
	gchar *memory;
	size_t size;
};

/* Data structure for use with curl */
struct chunk fetchcover_curl_data;

/**
 * safe_realloc:
 *
 * @void: ptr
 * @size_t: size
 *
 * Memory allocation function
 */
static void *safe_realloc(void *ptr, size_t size)
{
	if (ptr)
		return realloc(ptr, size);
	else
		return malloc(size);
}

/**
 * 
 * curl_write_fetchcover_func:
 * 
 * @void: *ptr
 * @size_t: itemsize
 * @size_t:numitems
 * @void: *data
 * 
 * Curl writing function
 * 
 * @Return size_t
 */
static size_t curl_write_fetchcover_func(void *ptr, size_t itemsize, size_t numitems, void *data)
{
	size_t size = itemsize * numitems;
	struct chunk *mem = (struct chunk*)data;
	mem->memory = (gchar*)safe_realloc(mem->memory, mem->size + size + 1);
	
	if (mem->memory)
	{
		memcpy(&(mem->memory[mem->size]), ptr, size);
		mem->size += size;
		mem->memory[mem->size] = 0;
	}
	return size;
}
		
/**
 * net_search_track:
 *
 * Use Amazon to locate an XML file containing links to
 * cover pictures consistent with track artist and album
 */
static void net_search_track ()		
{		
	/* This key for Amazon web services belongs to Charlie Head */
	const gchar amazonkey[21] = "10K4YZTZFS562NG7EZR2";
		
	/* possible internationalization: support
	 * different tld's (ja, fr, ca, etc.) - Amarok does this.
	 * we assume a lot here.. may not work
	 * for some international artists? --chead
	 */
	 
	 /* Create the url string and insert the artist and album */
	GString *url = g_string_new (NULL);
	g_string_printf(url,
		"http://xml.amazon.com/onca/xml3?t=webservices-20&dev-t=%s&KeywordSearch=%s+%s&mode=music&type=lite&locale=us&page=1&f=xml",
		amazonkey, fetchcover_track->artist, fetchcover_track->album);
	
	/* Replace any spaces in the url string with +s instead */
	gint i;
	for (i = 0; i < url->len; i++)
	{
		if (url->str[i] == ' ')
			url->str[i] = '+';
	}
		
	fetchcover_debug("fetchcover_next: amazon xml url: %s", url->str);
	
	fetchcover_curl_data.memory = NULL;
	fetchcover_curl_data.size = 0;

	/* Use curl to perform the internet search */	
	CURL *curl;	
	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, url->str);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_fetchcover_func);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) &fetchcover_curl_data);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	g_string_free(url, TRUE);
	
	/* Check whether curl received some hits.
	 * If not return
	 */
	if (!fetchcover_curl_data.memory || fetchcover_curl_data.size <= 0)
	{
		fetchcover_statusbar_update ("Failed to find any covers for track");
		if (fetchcover_curl_data.memory)
		{
			g_free(fetchcover_curl_data.memory);
			fetchcover_curl_data.memory = NULL;
			fetchcover_curl_data.size = 0;
		}
		return;
	}
	
	/* Curl net search successful so process the results */
	xmlDocPtr doc;
	xmlNodePtr cur;
	xmlChar *key;

	/* Process the data by parsing the XML and return if cannot parse */		
	if ((doc = xmlReadMemory(fetchcover_curl_data.memory, fetchcover_curl_data.size, "amazon.xml", NULL, 0)) == NULL)
	{
		fetchcover_statusbar_update ("Parsing failure on processing of possible track covers");
		if (fetchcover_curl_data.memory)
		{
			g_free(fetchcover_curl_data.memory);
			fetchcover_curl_data.memory = NULL;
			fetchcover_curl_data.size = 0;
		}
		return;
	}

	/* XML parsed ok so try and get the root element */
	if ((cur = xmlDocGetRootElement(doc)) == NULL)
	{
		fetchcover_statusbar_update ("Downloaded XML file appears to be empty. No covers found for track");
		xmlFreeDoc(doc);
		if (fetchcover_curl_data.memory)
		{
			g_free(fetchcover_curl_data.memory);
			fetchcover_curl_data.memory = NULL;
			fetchcover_curl_data.size = 0;
		}
		return;
	}

	/* Wind down through the XML nodes to obtain the urls
	 * to the returned album covers
	 */
	Fetch_Cover *fcover;		
	cur = cur->xmlChildrenNode;
	while (cur != NULL)
	{
		if ((!xmlStrcmp(cur->name, (const xmlChar*)"Details")))
		{
			xmlNodePtr details = cur->xmlChildrenNode;
			while (details != NULL)
			{
				if ((!xmlStrcmp(details->name, (const xmlChar*)"ImageUrlLarge")))
				{
					key = xmlNodeListGetString (doc, details->xmlChildrenNode, 1);
					/* Create a fetchcover object and the add to the list */
					fcover = g_new0(Fetch_Cover, 1);
					fcover->url = g_string_new ((gchar*) key);
					fcover->image = NULL;
					
					fetchcover_image_list = g_list_append (fetchcover_image_list, fcover);
					xmlFree(key);
				}
				details = details->next;
			}
		}
		cur = cur->next;
	}
		
	xmlFreeDoc(doc);

	/* Clean up the curl data */
	if (fetchcover_curl_data.memory)
	{
		g_free(fetchcover_curl_data.memory);
		fetchcover_curl_data.memory = NULL;
		fetchcover_curl_data.size = 0;
	}

	/* Check whether the XML served up any cover images */	
	if (g_list_length (fetchcover_image_list) == 1)
	{
		fetchcover_debug("fetchcover_next: no covers found\n");
		fetchcover_statusbar_update ("No cover images were found for this album.");
	}
	else
	{
		fetchcover_debug("fetchcover_next: successfully recovered covers\n");
		gchar *buf;
		buf = g_strdup_printf ("Found potentially %d covers. (Some covers may be blank)", g_list_length (fetchcover_image_list) - 1);
		fetchcover_statusbar_update (buf);
		g_free (buf);
	}
	
	return;
}

/**
 * net_retrieve_image:
 *
 * @GString: url
 *
 * Use the url acquired from the net search to fetch the image,
 * save it to a file inside the track's parent directory then display
 * it as a pixbuf
 */
static void net_retrieve_image (GString *url)
{
	gchar *path = NULL;
	
	fetchcover_debug("fetchcover_next: net_retrieve_image from: %s\n", url->str);
	
	fetchcover_curl_data.size = 0;
	fetchcover_curl_data.memory = NULL;
	
	/* Use curl to retrieve the data from the net */
	CURL *curl;	
	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, url->str);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_fetchcover_func);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&fetchcover_curl_data);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	
	g_return_if_fail(fetchcover_curl_data.memory);

	/* Check that the page returned is a valid web page */	
	if (strstr(fetchcover_curl_data.memory, "<html>") != NULL)
	{
		fetchcover_statusbar_update ("Image appears to no longer exist at http location");
		fetchcover_debug("fetchcover_next: http error (probably 404 or server error\n");
		return;
	}
	
	FILE *tmpf = NULL;
	ExtraTrackData *etd = fetchcover_track->userdata;
	g_return_if_fail(etd);
	
	gchar *dir = g_path_get_dirname(etd->pc_path_utf8);
	gchar *template = prefs_get_string("coverart_template");
	gchar **template_items = g_strsplit(template, ";", 0);
	
	gint i;
	gchar *filename = NULL;
	
	for (i = 0; filename == NULL && i < g_strv_length (template_items); ++i)
	{
		filename = get_string_from_template(fetchcover_track, template_items[i], FALSE, FALSE);
		if (strlen(filename) == 0)
			filename = NULL;
	}
	
	/* Check filename still equals null then take a default stance
	 * to ensure the file has a name.
	 */
	if (filename == NULL)
		filename = "folder.jpg";
		
	/* Use the index position of the cover in the glist to create a unique filename
	 * Convert the index number to a string and prefix with a dot (hidden file)
	 */
	gint display_cover_index;
	display_cover_index = g_list_index (fetchcover_image_list, displayed_cover);	
	gchar *dcstr_index = NULL;
	dcstr_index = (gchar *) g_malloc (sizeof(gint) + (sizeof(gchar) * 3));
	g_sprintf (dcstr_index, ".%d_@_", display_cover_index);
	
	gchar *fname = NULL;
	if (g_str_has_suffix(filename, ".jpg"))
		fname = g_strconcat(dcstr_index, filename, NULL);
	else
		fname = g_strconcat(dcstr_index, filename, ".jpg", NULL);
	
	displayed_cover->dir = dir;
	displayed_cover->filename = fname;
			
	fetchcover_debug("fetchcover_next: saving tmp cover image to %s / %s", displayed_cover->dir, displayed_cover->filename);
	
	path = g_build_filename(displayed_cover->dir, displayed_cover->filename, NULL);
	if ((tmpf = fopen(path, "wb")) == NULL)
	{
		fetchcover_debug("fetchcover_next: fopen failed\n");
		fetchcover_statusbar_update ("Downloaded image cover failed to open");
		if (fetchcover_curl_data.memory)
		{
			g_free(fetchcover_curl_data.memory);
			fetchcover_curl_data.memory = NULL;
			fetchcover_curl_data.size = 0;
		}
		g_free (path);
		return;
	}
	g_free (path);
	
	if (fwrite(fetchcover_curl_data.memory, fetchcover_curl_data.size, 1, tmpf) != 1)
	{
		fetchcover_debug("fetchcover_next: fwrite failed\n");
		if (fetchcover_curl_data.memory)
		{
			g_free(fetchcover_curl_data.memory);
			fetchcover_curl_data.memory = NULL;
			fetchcover_curl_data.size = 0;
		}
	}
	
	fclose(tmpf);
	
	GError *error = NULL;
	path = g_build_filename(displayed_cover->dir, displayed_cover->filename, NULL);
	displayed_cover->image = gdk_pixbuf_new_from_file(path, &error);
	if (error != NULL)
	{
		fetchcover_debug("fetchcover_next: gdk_pixbuf_new_from_file failed\n");
		if (fetchcover_curl_data.memory)
		{
			g_free(fetchcover_curl_data.memory);
			fetchcover_curl_data.memory = NULL;
			fetchcover_curl_data.size = 0;
		}
	}
	
	g_free(fetchcover_curl_data.memory);
	fetchcover_curl_data.memory = NULL;
	fetchcover_curl_data.size = 0;
	g_strfreev(template_items);
	g_free(template);
	g_free(filename);
	g_free(path);
}

/**
 * fetchcover_next_button:
 *
 * Gets new image URL list if necessary,
 * downloads next image to memory,
 * shows it in a GnomeCanvas in the fetchcover_window.
 */
static void fetchcover_next_button_clicked (GtkWidget *widget, gpointer data)
{
	GdkWindow *window = gtk_widget_get_parent_window (widget);
	gdk_window_set_cursor (window, gdk_cursor_new (GDK_WATCH));
	fetchcover_debug("fetchcover_next_button: getting cover for:\n- Artist: %s\n- Album:  %s",
																			fetchcover_track->artist, fetchcover_track->album);
	
	fetchcover_statusbar_update ("");
	
	if (netsearched == FALSE)
	{
		net_search_track ();
		netsearched = TRUE;
		gtk_button_set_label (GTK_BUTTON(next_button), "_Next");
	}
	
	/* Whether net search this time or not, should be another image to display
	 * to get this far.
	 */
	
	/* Increase the index by 1 */
	displayed_cover_index++;
			
	/* fetchcover_image_list has a valid entry so do a couple of tests*/
	if (displayed_cover_index >= (g_list_length(fetchcover_image_list) - 1))
	{
		/* Something went awry so make the index 
		 * the index of the last cover in the list
		 */
		displayed_cover_index = g_list_length(fetchcover_image_list) - 1;
		/* stop anymore button presses if last in the list */
		gtk_widget_set_sensitive (next_button, FALSE);
	}
		
	if (displayed_cover_index > 0)
	{
		/* stop anymore prev button press if first in the list */
		gtk_widget_set_sensitive (prev_button, TRUE);
	}
		
	/* Set the displayed cover to be the new image */	
	displayed_cover = g_list_nth_data (fetchcover_image_list, displayed_cover_index);
	
	/* If the image has not been retrieved then get it from the net */
	if (displayed_cover->image == NULL)
	{
		net_retrieve_image (displayed_cover->url);
		if (displayed_cover->image == NULL)
		{
			gdk_window_set_cursor (window, NULL);
			fetchcover_statusbar_update ("Failed to retrieve image.");
			g_return_if_fail (displayed_cover->image);
		}
	}
	
	fetchcover_debug("Displayed Image path: %s/%s\n", displayed_cover->dir, displayed_cover->filename);
	
	GdkPixbuf *scaled = gdk_pixbuf_scale_simple(displayed_cover->image, IMGSCALE, IMGSCALE, GDK_INTERP_NEAREST);
	gnome_canvas_item_set(fetchcover_canvasitem, "pixbuf", scaled, NULL);
	
	gdk_window_set_cursor (window, NULL);
	
	return;
}

/**
 * fetchcover_prev_button:
 *
 * Gets previous image and shows
 * it in a GnomeCanvas in the fetchcover_window.
 */
static void fetchcover_prev_button_clicked (GtkWidget *widget, gpointer data)
{
	/* decrease the index by 1 */
	displayed_cover_index--;
	
	fetchcover_statusbar_update ("");
	
	if (displayed_cover_index <= 0)
	{
		/* Something went awry so make the index
		 * the index of the first cover in the list
		 */
		displayed_cover_index = 0;
		/* stop anymore prev button press if first in the list */
		gtk_widget_set_sensitive (prev_button, FALSE);
	}
		
	/* fetchcover_image_list has a valid entry so do a couple of tests*/
	if (displayed_cover_index < (g_list_length(fetchcover_image_list) - 1))
	{
		/* stop anymore button presses if last in the list */
		gtk_widget_set_sensitive (next_button, TRUE);
	}	
		
	/* Set the displayed cover to be the new image */	
	displayed_cover = g_list_nth_data (fetchcover_image_list, displayed_cover_index);
	
	/* If the image has not been retrieved then get it from the net */
	if (displayed_cover->image == NULL)
	{
		net_retrieve_image (displayed_cover->url);
		if (displayed_cover->image == NULL)
		{
			fetchcover_statusbar_update ("Failed to retrieve image.");
			g_return_if_fail (displayed_cover->image);
		}
	}
	
	GdkPixbuf *scaled = gdk_pixbuf_scale_simple(displayed_cover->image, IMGSCALE, IMGSCALE, GDK_INTERP_NEAREST);
	gnome_canvas_item_set(fetchcover_canvasitem, "pixbuf", scaled, NULL);
	
	return;
}

/**
 * fetchcover_save:
 * 
 * @Detail: detail
 *
 * Save the displayed cover.
 * Set thumbnails, update details window.
 * Called on response to the clicking of the save button in the dialog
 * 
 * Returns:
 * Filename of chosen cover image file
 */
gchar *fetchcover_save ()
{
	gchar *newname = NULL;
	
	/* The default cover image will have both dir and filename set
	 * to null because no need to save because it is already saved (!!)
	 * Thus, this whole process is avoided. Added bonus that pressing
	 * save by accident if, for instance, no images are found means the
	 * whole thing safely completes
	 */
	if (displayed_cover->dir && displayed_cover->filename)
	{
		/* path is valid so first move the file to be the folder.jpg or 
		 * whatever is the preferred preference
		 */
		
		/* Split the existing filename to remove the prefix */
		gchar **fname_items = g_strsplit(displayed_cover->filename, "_@_", 2);
		/* Assign the filename ready to rename the file */
		newname = g_build_filename(displayed_cover->dir, fname_items[1], NULL);
		fetchcover_debug("New name of file is %s\n", newname);

		while (g_file_test (newname, G_FILE_TEST_EXISTS))
		{
			newname = display_file_exist_dialog (newname);
			if (newname == NULL)
				break;
		}
		
		/* Carry the nullified value back to the original called so the
		 * entire fetchcover process can be cancelled
		 */
		if (newname == NULL)
			return NULL;
		
		gchar *oldname = g_build_filename(displayed_cover->dir, displayed_cover->filename, NULL);
		/* Rename the preferred choice, ie. .2_@_After_Forever.jpg, to the preferred name, 
		 * ie. After_Forever.jpg
		 */
		g_rename (oldname, newname);
		
		/* Tidy up to ensure the path will not get cleaned up
		 * by fetchcover_clean_up
		 */
		g_free (oldname);
		g_strfreev(fname_items);
		g_free (displayed_cover->dir);
		g_free (displayed_cover->filename);		
		displayed_cover->dir = NULL;
		displayed_cover->filename = NULL;
	}
	return newname;	
}
#endif /* HAVE_CURL */

static gchar *display_file_exist_dialog (gchar *filename)
{
	gint result;
	gchar **splitarr = NULL;
	gchar *basename = NULL;
	gint i;
	gchar *message;	
	GtkWidget *label;
	GtkWidget *dialog = gtk_dialog_new_with_buttons ("Coverart file already exists",
                                            NULL,
                                            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                            GTK_STOCK_YES,
                                            GTK_RESPONSE_YES,
                                            GTK_STOCK_NO,
                                            GTK_RESPONSE_NO,
                                            GTK_STOCK_CANCEL,
                                            GTK_RESPONSE_REJECT,
                                            NULL);
	message = g_strdup_printf (_("The picture file %s already exists. \
\nThis may be associated with other music files in the directory. \
\n\n-  Clicking Yes will overwrite the existing file, possibly associating \
\n   other music files in the same directory with this coverart file. \
\n-  Clicking No will save the file with a unique file name. \
\n-  Clicking Cancel will abort the fetchcover operation."), filename);
		           
	label = gtk_label_new (message);
   
 	/* Add the label, and show everything we've added to the dialog. */
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), label);
			
	gtk_widget_show_all (dialog);	
	result = gtk_dialog_run (GTK_DIALOG(dialog));
	g_free (message);
			
	switch (result)
	{
		case GTK_RESPONSE_REJECT:
			/* Cancel has been clicked so no save */
			gtk_widget_destroy (dialog);
			return NULL;
		case GTK_RESPONSE_YES:
			/* Yes clicked so overwrite the file is okay. Leave final_filename intact
			 * and remove the original
			 */
			g_remove (filename);
			gtk_widget_destroy (dialog);
			return filename;
		case GTK_RESPONSE_NO:
			/* User doesnt want to overwrite anything so need to do some work on filename */
			splitarr = g_strsplit (filename, ".", 0);
			basename = splitarr[0];
			
			for (i = 1; g_file_test (filename, G_FILE_TEST_EXISTS); ++i)
			{
				g_sprintf (filename, "%s%d.jpg", basename, i);
			}
			
			/* Should have found a filename that really doesnt exist so this needs to be returned */
			basename = NULL;
			g_strfreev(splitarr);
			gtk_widget_destroy (dialog);
			
			return filename;
		default:
			gtk_widget_destroy (dialog);
			return NULL;
	}	
}

/**
 * free_fetchcover:
 * 
 * @Fetch_Cover: fcover
 * 
 * Free the elements of the passed in Fetch_Cover structure
 */
static void free_fetchcover (Fetch_Cover *fcover)
{
	if (! fcover)
		return;
		
	if (fcover->url)
		g_string_free (fcover->url, TRUE);
	
	if (fcover->image)
		gdk_pixbuf_unref (fcover->image);
		
	if (fcover->dir && fcover->filename)
	{
		gint status;
		gchar *path;
		path = g_build_filename (fcover->dir, fcover->filename, NULL);
		status = g_remove (path);
		g_free(path);
		g_free (fcover->dir);
		g_free (fcover->filename);
	}
}

/**
 * free_fetchcover_list:
 *
 * Nullify all the urls in the url list as part of
 * cleaning up.
 * 
 */
static void free_fetchcover_list()
{
	fetchcover_debug("free_fetchcover_list");
	
	gint i;
	Fetch_Cover *fcover = NULL;
	for (i = 0; i < g_list_length (fetchcover_image_list); ++i)
	{
		fcover = g_list_nth_data (fetchcover_image_list, i);
		free_fetchcover (fcover);
	}
	g_list_free (fetchcover_image_list);
	fetchcover_image_list = NULL;
}

/**
 * fetchcover_cleanup:
 *
 * Cleanup fetchcover bits and pieces
 */
static void fetchcover_cleanup()
{	
	#ifdef HAVE_CURL
	if (fetchcover_curl_data.memory)
	{
		g_free (fetchcover_curl_data.memory);
		fetchcover_curl_data.memory = NULL;
		fetchcover_curl_data.size = 0;
	}
	#endif /* CURL */
	free_fetchcover_list();
}

/**
 * fetchcover_display_dialog:
 *
 * Create fetchcover_window, get album cover from Amazon
 *
 * @track: track to look up images for
 */
static GtkDialog *fetchcover_display_dialog (Track *track, Itdb_Device *device)
{
	GnomeCanvasItem *art_border;
	GtkBox *canvasbutton_hbox;
	GtkWidget *fetchcover_dialog;
	GladeXML *fetchcover_xml;
	GdkPixbuf *imgbuf;
	
	g_return_val_if_fail (track, NULL);
	g_return_val_if_fail (device, NULL);
	
	/* Enable searching of internet for images */		
	netsearched = FALSE;
	
	fetchcover_xml = glade_xml_new (xml_file, "fetchcover_dialog", NULL);
	fetchcover_dialog = gtkpod_xml_get_widget (fetchcover_xml, "fetchcover_dialog");
	
	ExtraTrackData *etd;
	etd = track->userdata;
	if (etd && etd->thumb_path_locale)
	{
		GError *error = NULL;
		imgbuf = gdk_pixbuf_new_from_file(etd->thumb_path_locale, &error);
		if (error != NULL)
		{
			/* Artwork failed to load from file so try loading default */
			imgbuf = coverart_get_track_thumb (track, device);
			g_error_free (error);
		}
	}
	else
	{
		/* No thumb path available, fall back to getting the small thumbnail
		 * and if that fails, the default thumbnail image.
		 */
		imgbuf = coverart_get_track_thumb (track, device);
	}
	
	/* Add the cover to the image list */
	displayed_cover = g_new0 (Fetch_Cover, 1);
	displayed_cover->url = g_string_new ("default");
	/* No need to set the path for saving later as if this cover is selected:
	 * a) There is an image cover already set to the template name so no need to save
	 * b) There is no image cover so default ? is showing and again no save necessary
	 *
	 * Avoid saving by checking where url is "default"
	 */
	displayed_cover->dir = NULL;
	displayed_cover->filename = NULL;
	displayed_cover->image = gdk_pixbuf_scale_simple (imgbuf, IMGSCALE, IMGSCALE, GDK_INTERP_NEAREST);
	gdk_pixbuf_unref (imgbuf);
	
	fetchcover_image_list = g_list_append (fetchcover_image_list, displayed_cover);
	displayed_cover_index = 0;	
		
	fetchcover_debug("fetchcover_display_window: loaded cover file");
	
	/* Create the image cover canvas */
	fetchcover_canvas = GNOME_CANVAS (gnome_canvas_new());
	gtk_widget_set_size_request (GTK_WIDGET(fetchcover_canvas), IMGSCALE, IMGSCALE);
	gnome_canvas_set_scroll_region (fetchcover_canvas, 0.0, 0.0, IMGSCALE, IMGSCALE);
	fetchcover_canvasitem = gnome_canvas_item_new(	
					gnome_canvas_root(fetchcover_canvas),
					GNOME_TYPE_CANVAS_PIXBUF, 
					"x", (double) 0, 
			    "y", (double) 0,
			    "width", (double) IMGSCALE,
			    "height", (double) IMGSCALE,
					NULL);
	
	/* Apply the image cover to the canvas */
	gnome_canvas_item_set (	fetchcover_canvasitem,
																						"pixbuf", displayed_cover->image,
				NULL);
	
	/* Create the canvas border */
	art_border = gnome_canvas_item_new(
					gnome_canvas_root(fetchcover_canvas),
			    gnome_canvas_rect_get_type(),
			    "x1", (double) 0, 
			    "y1", (double) 0,
			    "x2", (double) IMGSCALE,
			    "y2", (double) IMGSCALE,
			    "outline-color-rgba", 0xAA000000,
			    "width-units", (double) 3,
			    NULL);
	
	gnome_canvas_item_raise_to_top (art_border);
	
	/* Add canvas to horizonontal box */
	canvasbutton_hbox = GTK_BOX (gtkpod_xml_get_widget (fetchcover_xml, "canvasbutton_hbox"));
	gtk_widget_set_size_request (GTK_WIDGET(canvasbutton_hbox), IMGSCALE + 150, IMGSCALE + 20);
	gtk_box_pack_start_defaults ( canvasbutton_hbox, GTK_WIDGET (fetchcover_canvas));
	
	/* Assign the status message bar */
	fetchcover_statusbar = gtkpod_xml_get_widget (fetchcover_xml, "fetchcover_statusbar");
		
	fetchcover_track = track;
  next_button = gtkpod_xml_get_widget (fetchcover_xml, "next_button");
	prev_button = gtkpod_xml_get_widget (fetchcover_xml, "prev_button");
	
  #ifdef HAVE_CURL
  
  g_signal_connect (G_OBJECT(next_button), "clicked",
		      G_CALLBACK(fetchcover_next_button_clicked), NULL);	
	
	g_signal_connect (G_OBJECT(prev_button), "clicked",
		      G_CALLBACK(fetchcover_prev_button_clicked), NULL);	
	
	/* Check there are valid values for artist and album, otherwise disable everything */
	if (fetchcover_track->artist == NULL || fetchcover_track->album == NULL)
	{
		gtk_widget_set_sensitive (next_button, FALSE);
		fetchcover_statusbar_update ("Search cannot be performed as either the artist or album are blank");
	}
	else
	{
		gchar *buf;
		buf = g_strdup_printf ("Artist: %s\tAlbum: %s", fetchcover_track->artist, fetchcover_track->album);
		fetchcover_statusbar_update (buf);
		g_free (buf);
	}
	
	#else
		gtk_widget_set_sensitive (next_button, FALSE);
		gtk_widget_set_sensitive (prev_button, FALSE);
		fetchcover_statusbar_update ("CURL has not been installed so this function is not available");
	#endif /* HAVE_CURL */
		
  gtk_widget_show_all (fetchcover_dialog);
  
  g_object_unref (fetchcover_xml);
  
	return GTK_DIALOG(fetchcover_dialog);
}

/**
 * fetchcover_debug:
 *
 * Print debug messages for debugging purposes
 *
 * @format of messages and message string
 */
static void fetchcover_debug(const gchar *format, ...)
{
	if (FETCHCOVER_DEBUG)
	{
		va_list args;
		va_start(args, format);
		gchar *s = g_strdup_vprintf(format, args);
		va_end(args);
		printf("%s\n", s);
		fflush(stdout);
		g_free(s);
	}
}


/**
 * fetchcover_statusbar_update:
 * 
 * @gchar*: messagel
 *
 * Display a message in the status bar component of the dialog
 */
static void fetchcover_statusbar_update (gchar *message)
{
	if (fetchcover_statusbar)
	{
		gtk_statusbar_pop(GTK_STATUSBAR(fetchcover_statusbar), 1);
		gtk_statusbar_push(GTK_STATUSBAR(fetchcover_statusbar), 1,  message);
	}
}


/**
 * on_coverart_context_menu_click:
 *
 * @Track: track
 *
 * Callback. Start cover selection from CoverArt Display's context menu.
 */
void on_coverart_context_menu_click (GList *tracks)
{
	Track *track;
  gint result;

  track = tracks->data;
	if (track == NULL)
	{
		g_fprintf (stderr, "Track was null so fetchcover dialog was not displayed.\nLenght of glist was %d\n", g_list_length (tracks));
		return;
	}
	
  GtkDialog *dialog = fetchcover_display_dialog (track, track->itdb->device);
  g_return_if_fail (dialog);

  result = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_hide (GTK_WIDGET(dialog));
	gtk_widget_destroy (GTK_WIDGET(dialog));
	
 #ifdef HAVE_CURL
 gchar *filename = NULL;
    	
 switch (result)
 {
 	case GTK_RESPONSE_ACCEPT:
      filename = fetchcover_save ();
      if (filename)
      {
	  while (tracks)
	  {
	      track = tracks->data;
	      if (gp_track_set_thumbnails (track, filename))
		  data_changed (track->itdb);
	      tracks = tracks->next;
	  }
      }
      g_free (filename);
  default:
  		break;
	}
#endif /* HAVE_CURL */
	
  fetchcover_cleanup();
}

/**
 * fetchcover_fetch_button:
 *
 * @widget: widget
 * @data: data
 *
 * Callback. Start cover selection.
 * Called when "Fetch Cover..." clicked from details_window.
 */
void on_fetchcover_fetch_button (GtkWidget *widget, gpointer data)
{
    gint result;
    Detail *detail = details_get_selected_detail ();
    GtkDialog *dialog = fetchcover_display_dialog (detail->track, detail->itdb->device);
    g_return_if_fail (dialog);
	
    result = gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_hide (GTK_WIDGET(dialog));
		gtk_widget_destroy (GTK_WIDGET(dialog));
#ifdef HAVE_CURL
    	
    gchar *filename = NULL;
    switch (result)
    {
    case GTK_RESPONSE_ACCEPT:  	
    	filename = fetchcover_save ();
    	if (filename)
    	{
	    if (details_writethrough(detail))
	    {
		GList *list;
		for (list = detail->tracks; list; list = list->next)
		{
		    ExtraTrackData *etd;
		    Track *track = list->data;
		    
		    if (!track)
			break;
		    
		    etd = track->userdata;
		    gp_track_set_thumbnails(track, filename);
		    etd->tchanged = TRUE;
		}
	    }
	    else
	    {
		ExtraTrackData *etd = fetchcover_track->userdata;
		if (etd)
		{
		    gp_track_set_thumbnails(fetchcover_track, filename);
		    etd->tchanged = TRUE;
		}
	    }
	    
	    detail->changed = TRUE;
	    details_update_thumbnail(detail);
	    details_update_buttons(detail);
    	}
    default:
	break;
    }
#endif /* HAVE_CURL */
	
    fetchcover_cleanup();
}
