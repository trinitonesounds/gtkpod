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
    #include <config.h>
#endif

#include "lastfm.h"

#ifdef HAVE_CURL
#include <curl/curl.h>

#include <glib/gstdio.h>
#include <stdlib.h>

#include "misc.h"

/* Define some constants */
#define AS_URL "http://post.audioscrobbler.com/"
#define AS_CLI "gkp"
#define AS_VER "0.1"

/* GIOChannel for the cache file */
static GIOChannel *cache = NULL;

/* Temporarily stores new tracks to be added */
static GSList *tracks = NULL;

static void lastfm_open_cache(gchar *mode)
{
    if (!cache) {
        gchar *filename;
        gchar *config_dir;
        GError **err = NULL;

        config_dir = prefs_get_cfgdir();
        if (config_dir) {
            filename = g_build_filename(config_dir, "lastfm", NULL);
            if (filename) {
                cache = g_io_channel_new_file(filename, mode, err);
                g_free(filename);
            }
            g_free(config_dir);
        }
    }
}

static void lastfm_close_cache()
{
    if (cache) {
        GError **err = NULL;
        g_io_channel_shutdown(cache, TRUE, err);
        g_io_channel_unref(cache);
    }    
    cache = NULL;
}

static void lastfm_read_and_merge_cache()
{
    gsize *written = NULL;
    GString *buffer = g_string_new(NULL);
    GError **err = NULL;
    GSList *t = NULL;

    lastfm_open_cache("r");
    while (g_io_channel_read_line_string(cache, buffer, NULL, err)
            != G_IO_STATUS_EOF)
        tracks = g_slist_prepend(tracks, buffer); 
    lastfm_close_cache();

    lastfm_open_cache("w+");
    while ((t = g_slist_next(tracks))) {
        char *td = t->data;
        g_io_channel_write_chars(cache, td, -1, written, err);
    }
    lastfm_close_cache();
}

static int lastfm_handshake()
{
    int status=-1;
    CURL *curl;
    gchar *filename = NULL;
    gchar *config_dir;
    FILE *res = NULL;
    GString *url = g_string_new(NULL);
    gchar *username;

    config_dir = prefs_get_cfgdir();
    if (config_dir) {
        filename = g_build_filename(config_dir, "lastfm.response", NULL);
	g_return_val_if_fail (filename, -1);
	res = fopen(filename, "w+");
	if (res == NULL)
	{
	    gtkpod_warning (_("Could not create 'lastfm.response' in '%s'."),
			    config_dir);
	    g_free(config_dir);
	    return -1;
	}
	g_free(config_dir);
    
	username = prefs_get_string("lastfm_username"); 
	g_string_printf(url, "%s?hs=true&p=%s&c=%s&v=%s&u=%s",
                        AS_URL, AS_VER, AS_CLI, VERSION, username);

	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, res);

	status = curl_easy_perform(curl);
    
	curl_easy_cleanup(curl);
	fclose(res);
	g_string_free(url, TRUE);
	/* g_unlink (filename); */
	g_free (filename);
    }
    return status;
}
#endif


void lastfm_add_track(Track *track)
{
#ifdef HAVE_CURL
    GString *tr = g_string_new(NULL);
    gint32 length = (track->tracklen) / 100.0;
    time_t timet  = track->time_played;
    g_string_printf(tr, "artist=%s&track=%s&album=%s&length=%d&time=%ld\n",
                    track->artist, track->title, track->album, length, (long)timet);
    tracks = g_slist_prepend(tracks, tr);
    g_string_free(tr, TRUE);
#endif
}

void lastfm_send_request()
{
#ifdef HAVE_CURL
    printf("Read and merged\n");
    lastfm_read_and_merge_cache();
    printf("Sending Handshake");
    lastfm_handshake();
#endif
}
