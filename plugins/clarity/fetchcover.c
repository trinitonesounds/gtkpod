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

#include <glib/gprintf.h>
#include <glib/gstdio.h>
#include <gdk/gdkkeysyms.h>
#include "libgtkpod/gp_itdb.h"
#include "libgtkpod/misc.h"
#include "plugin.h"
#include "fetchcover.h"

#undef FETCHCOVER_DEBUG

#define IMGSCALE 256

/* Display a dialog explaining the options if a file with the proposed name already exists */
static gchar *display_file_exist_dialog (Fetch_Cover *fetch_cover);
static gchar *fetchcover_check_file_exists (Fetch_Cover *fetch_cover);

#ifdef HAVE_CURL

	#include <curl/curl.h>

	/* Declarations */
	static void *safe_realloc(void *ptr, size_t size);
	static size_t curl_write_fetchcover_func(void *ptr, size_t itemsize, size_t numitems, void *data);

	struct chunk
	{
		gchar *memory;
		size_t size;
	};

	/* Data structure for use with curl */
	struct chunk fetchcover_curl_data;

	/****
	 * safe_realloc:
	 *
	 * @void: ptr
	 * @size_t: size
	 *
	 * Memory allocation function
	 **/
	static void *safe_realloc(void *ptr, size_t size)
	{
		if (ptr)
			return realloc(ptr, size);
		else
			return malloc(size);
	}

	/****
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
	 **/
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
#endif

	/*****
	 * fetchcover_new:
	 *
	 * @GString: url
	 * @GList: trks
	 *
	 * Initialise a new fetch cover object for use with the fetchcover functions
	 **/
	Fetch_Cover *fetchcover_new (gchar *url_path, GList *trks)
	{
		/*Create a fetchcover object */
		Fetch_Cover * fcover;

		fcover = g_new0(Fetch_Cover, 1);
		fcover->url = g_string_new ((gchar*) url_path);
		fcover->image = NULL;
		fcover->tracks = trks;
		fcover->err_msg = NULL;

		return fcover;
	}

#ifdef HAVE_CURL
	/*****
	 * net_retrieve_image:
	 *
	 * @GString: url
	 *
	 * Use the url acquired from the net search to fetch the image,
	 * save it to a file inside the track's parent directory then display
	 * it as a pixbuf
	 **/
	gboolean fetchcover_net_retrieve_image (Fetch_Cover *fetch_cover)
	{
		g_return_val_if_fail (fetch_cover, FALSE);

		if (! g_str_has_suffix(fetch_cover->url->str, ".jpg") && ! g_str_has_suffix(fetch_cover->url->str, ".JPG"))
		{
			fetch_cover->err_msg = g_strdup("Only jpg images are currently supported at this time\n");
			return FALSE;
		}

		gchar *path = NULL;
		FILE *tmpf = NULL;

		fetchcover_curl_data.size = 0;
		fetchcover_curl_data.memory = NULL;

		/* Use curl to retrieve the data from the net */
		CURL *curl;
		curl_global_init(CURL_GLOBAL_ALL);
		curl = curl_easy_init();
		curl_easy_setopt(curl, CURLOPT_URL, fetch_cover->url->str);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_fetchcover_func);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&fetchcover_curl_data);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
		curl_easy_perform(curl);
		curl_easy_cleanup(curl);

		if (fetchcover_curl_data.memory == NULL)
		{
			fetch_cover->err_msg = g_strdup("fetchcover curl data memory is null so failed to download anything!\n");
			return FALSE;
		}

		/* Check that the page returned is a valid web page */
		if (strstr(fetchcover_curl_data.memory, "<html>") != NULL)
		{
			fetch_cover->err_msg = g_strdup("fetchcover memory contains <html> tag so not a valid jpg image\n");
			return FALSE;
		}

		if (! fetchcover_select_filename (fetch_cover))
			return FALSE;

		path = g_build_filename(fetch_cover->dir, fetch_cover->filename, NULL);
#if DEBUG
		printf ("path of download file is %s\n", path);
#endif
		if ((tmpf = fopen(path, "wb")) == NULL)
		{
			if (fetchcover_curl_data.memory)
			{
				g_free(fetchcover_curl_data.memory);
				fetchcover_curl_data.memory = NULL;
				fetchcover_curl_data.size = 0;
			}
			g_free (path);
			fetch_cover->err_msg = g_strdup ("Failed to create a file with the filename\n");
			return FALSE;
		}

		if (fwrite(fetchcover_curl_data.memory, fetchcover_curl_data.size, 1, tmpf) != 1)
		{
			if (fetchcover_curl_data.memory)
			{
				g_free(fetchcover_curl_data.memory);
				fetchcover_curl_data.memory = NULL;
				fetchcover_curl_data.size = 0;
			}
			fclose(tmpf);
			g_free (path);
			fetch_cover->err_msg = g_strdup("fetchcover failed to write the data to the new file\n");
			return FALSE;
		}

		fclose(tmpf);

		/* Check the file is a valid pixbuf type file */
		GdkPixbufFormat *fileformat= NULL;
		fileformat = gdk_pixbuf_get_file_info (path, NULL, NULL);
		if (fileformat == NULL)
		{
			fetch_cover->err_msg = g_strdup("fetchcover downloaded file is not a valid image file\n");
			return FALSE;
		}

		GError *error = NULL;
		fetch_cover->image = gdk_pixbuf_new_from_file(path, &error);
		if (error != NULL)
		{
			g_error_free (error);
			if (fetchcover_curl_data.memory)
			{
				g_free(fetchcover_curl_data.memory);
				fetchcover_curl_data.memory = NULL;
				fetchcover_curl_data.size = 0;
			}
			g_free(path);
			fetch_cover->err_msg = g_strconcat ("fetchcover error occurred while creating a pixbuf from the file\n", error->message, NULL);
			return FALSE;
		}

		if (fetchcover_curl_data.memory)
			g_free(fetchcover_curl_data.memory);

		fetchcover_curl_data.memory = NULL;
		fetchcover_curl_data.size = 0;
		g_free(path);
		return TRUE;
	}
#endif

	gboolean fetchcover_select_filename (Fetch_Cover *fetch_cover)
	{
		GList *tracks = fetch_cover->tracks;

		if (tracks == NULL || g_list_length (tracks) <= 0)
		{
			fetch_cover->err_msg = g_strdup("fetchcover object's tracks list either NULL or no tracks were selected\n");
			return FALSE;
		}

		Track *track = g_list_nth_data (tracks, 0);
		ExtraTrackData *etd = track->userdata;

		fetch_cover->dir = g_path_get_dirname(etd->pc_path_utf8);
		gchar *template = prefs_get_string("coverart_template");
		gchar **template_items = g_strsplit(template, ";", 0);

		gint i;
		for (i = 0; fetch_cover->filename == NULL && i < g_strv_length (template_items); ++i)
		{
			fetch_cover->filename = get_string_from_template(track, template_items[i], FALSE, FALSE);
			if (strlen(fetch_cover->filename) == 0)
				fetch_cover->filename = NULL;
		}

		g_strfreev(template_items);
		g_free(template);

		/* Check filename still equals null then take a default stance
		 * to ensure the file has a name. Default stance applies if the
		 * extra track data has been left as NULL
		 */
		if (fetch_cover->filename == NULL)
			fetch_cover->filename = "folder.jpg";
		else
		{
			if (! g_str_has_suffix(fetch_cover->filename, ".jpg"))
			{
				gchar *oldname;
				oldname = fetch_cover->filename;
				fetch_cover->filename = g_strconcat(oldname, ".jpg", NULL);
				g_free (oldname);
			}
		}

		if (fetchcover_check_file_exists (fetch_cover) == NULL)
		{
			fetch_cover->err_msg = g_strdup("operation cancelled\n");
			return FALSE;
		}

		return TRUE;
	}

	/*****
	 * fetchcover_check_file_exists:
	 *
	 * @Fetch_Cover
	 *
	 * Save the displayed cover.
	 * Set thumbnails, update details window.
	 * Called on response to the clicking of the save button in the dialog
	 *
	 * Returns:
	 * Filename of chosen cover image file
	 ***/
	static gchar *fetchcover_check_file_exists (Fetch_Cover *fetch_cover)
	{
		gchar *newname = NULL;
		/* The default cover image will have both dir and filename set
		 * to null because no need to save because it is already saved (!!)
		 * Thus, this whole process is avoided. Added bonus that pressing
		 * save by accident if, for instance, no images are found means the
		 * whole thing safely completes
		 */
		if (fetch_cover->dir && fetch_cover->filename)
		{
			/* path is valid so first move the file to be the folder.jpg or
			 * whatever is the preferred preference
			 */

			/* Assign the full path name ready to rename the file */
			newname = g_build_filename(fetch_cover->dir, fetch_cover->filename, NULL);

			if (g_file_test (newname, G_FILE_TEST_EXISTS))
			{
				newname = display_file_exist_dialog (fetch_cover);
			}
		}
		return newname;
	}

	static gchar *display_file_exist_dialog (Fetch_Cover *fetch_cover)
	{
		gchar *filepath;
		gint result;
		gchar **splitarr = NULL;
		gchar *basename = NULL;
		gint i;
		gchar *message;

		filepath = g_build_filename(fetch_cover->dir, fetch_cover->filename, NULL);

		message = g_strdup_printf (_("The picture file %s already exists.\n" \
				"This may be associated with other music files in the directory.\n\n" \
				"Do you want to overwrite the existing file, possibly associating\n" \
				"other music files in the same directory with this cover art file,\n" \
				"to save the file with a unique file name, or to abort the fetchcover operation?"),
								   filepath);

		result = gtkpod_confirmation_hig (GTK_MESSAGE_WARNING,
									      _("Cover art file already exists"),
									      message,
									      _("Overwrite"),
									      _("Rename"),
									      _("Abort"),
									      NULL);
		g_free (message);

		switch (result)
		{
			case GTK_RESPONSE_APPLY:
				/* Abort has been clicked so no save */
				return NULL;
			case GTK_RESPONSE_OK:
				/*** Overwrite clicked so overwrite the file is okay. Leave final_filename intact
				 * and remove the original
				 **/
				g_remove (filepath);
				return filepath;
			case GTK_RESPONSE_CANCEL:
				/* User doesn't want to overwrite anything so need to do some work on filename */
				/* Remove the suffix from the end of the filename */
				splitarr = g_strsplit (fetch_cover->filename, ".", 0);
				basename = splitarr[0];

				gchar *newfilename = g_strdup (fetch_cover->filename);;

				for (i = 1; g_file_test (filepath, G_FILE_TEST_EXISTS); ++i)
				{
					g_free (newfilename);
					gchar *intext= NULL;
					intext = g_strdup_printf ("%d.jpg", i);
					newfilename = g_strconcat (basename, intext, NULL);

					g_free (filepath);
					g_free (intext);

					filepath = g_build_filename(fetch_cover->dir, newfilename, NULL);
				}

				/* Should have found a filename that really doesn't exist so this needs to be returned */
				g_free (fetch_cover->filename);
				fetch_cover->filename = g_strdup (newfilename);
				g_free (newfilename);
				newfilename = NULL;
				basename = NULL;
				g_strfreev(splitarr);

				return filepath;
			default:
				return NULL;
		}
	}

/****
 * free_fetchcover:
 *
 * @Fetch_Cover: fcover
 *
 * Free the elements of the passed in Fetch_Cover structure
 */
void free_fetchcover (Fetch_Cover *fcover)
{
	if (! fcover)
		return;

	if (fcover->url)
		g_string_free (fcover->url, TRUE);

	if (fcover->image)
		g_object_unref (fcover->image);

	if (fcover->dir)
		g_free (fcover->dir);

	if (fcover->filename)
		g_free (fcover->filename);

	if (fcover->err_msg)
		g_free (fcover->err_msg);

	g_free (fcover);
}
