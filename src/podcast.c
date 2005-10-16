/* Time-stamp: <2005-07-01 23:16:54 asd>
|
|  Copyright (C) 2002-2005 Alexander Dutton <alexdutton at f2s dot com>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <pthread.h>
#ifdef HAVE_GETOPT_LONG_ONLY
#  include <getopt.h>
#else
#  include "getopt.h"
#endif
#include "misc.h"
#include "prefs.h"
#include "podcast.h"
#include "info.h"
#include <time.h>
#include <curl/curl.h>
#include <curl/multi.h>
#include <curl/types.h>
GList *podcasts = NULL;
GList *podcast_files = NULL;
GList *abort_urls = NULL;
GList *abort_urls_to_add = NULL;

GladeXML *podcast_window_xml;
static GtkWidget *podcast_window = NULL;
static GtkTreeView *podcast_list = NULL;

gchar *url_being_fetched = NULL;

static long transfer_total, transfer_done;
static gint retrieve_url_to_path (gchar *url, gchar *path);
static gint parse_file_for_podcast_files(gchar *file);
static void podcast_log (gchar *msg);
static void create_podcast_list ();
static gchar *choose_filename(gchar *url);
static iTunesDB *get_itdb_local();

static void podcast_window_create(void);


void podcast_add (gchar *name, gchar *url)
{
    // assign some memory to hold data for new podcast
    struct podcast *podcast = malloc(sizeof(struct podcast));
    podcast->name = g_strdup(name);
    podcast->url = g_strdup(url);
    //g_free(name);
    //g_free(url);
    podcasts = g_list_insert(podcasts, podcast, -1);
}

gboolean podcast_delete_by_url(gchar *url)
{
    int i = 0;
    struct podcast *podcast = NULL;

    while(i < g_list_length(podcasts)) 
    {
        podcast = g_list_nth_data(podcasts, i);
        if (g_ascii_strcasecmp(podcast->url, url) == 0)
        {
            podcasts = g_list_remove(podcasts, podcast);
            g_free(podcast);
            return TRUE;
        }
        ++i;
    }
    return FALSE;
}

static void podcast_delete_all ()
{
    int i = 0;
    struct podcast *podcast = NULL;
    while(i < g_list_length(podcasts))
    {
        podcast = g_list_nth_data(podcasts, i);
        g_free(podcast->name);
        g_free(podcast->url);
        ++i;
    }
    i = g_list_length(podcasts) - 1;
    while(i > -1)
    {
        podcast = g_list_nth_data(podcasts, i);
        podcasts = g_list_remove(podcasts, podcast);
        --i;
    }
}

void podcast_write_from_store (GtkListStore *store)
{
    gchar *cfgdir = NULL;
    gchar filename[PATH_MAX+1];
    FILE *fp = NULL;
    gboolean have_prefs = FALSE;

    cfgdir = prefs_get_cfgdir ();

    if (cfgdir)
    {
        snprintf(filename, PATH_MAX, "%s/podcast.list", cfgdir);
        filename[PATH_MAX] = 0;

        if((fp = fopen(filename, "w")))
        {

            GtkTreeIter iter;
            gchar *name, *url;

            podcast_delete_all();

            if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL(store), &iter))
            {
                do
                {
                    gtk_tree_model_get (GTK_TREE_MODEL(store), &iter,
                                        PC_SUBS_NAME, &name,
                                        PC_SUBS_URL, &url,
                                        -1);
                    fprintf (fp, "begin podcast\n");
                    fprintf (fp, "name=%s\n", name);
                    fprintf (fp, "url=%s\n", url);
                    fprintf (fp, "end podcast\n");

                    podcast_add(name, url);

                    g_free(name);
                    g_free(url);
                } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter));
            }

            fclose(fp);
            have_prefs = TRUE; /* read prefs */
        }
        else
        {
            gtkpod_warning(_("Unable to open podcast file '%s' for writing\n"), filename);
        }
    }
}

void podcast_read_into_store (GtkListStore *store)
{
    int i = 0;
    struct podcast *podcast = NULL;

    GtkTreeIter iter;

    while(i < g_list_length(podcasts))
    {
        podcast = g_list_nth_data(podcasts, i);
        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter,
                            PC_SUBS_NAME, podcast->name,
                            PC_SUBS_URL, podcast->url,
                            -1);
        ++i;
    }

}

void podcast_read_from_file ()
{
    gchar *cfgdir = NULL, *name = NULL, *url = NULL;
    gchar filename[PATH_MAX+1];
    gchar buf[PATH_MAX];
    gchar *line, *arg, *bufp;
    gint len;

    FILE *fp = NULL;

    cfgdir = prefs_get_cfgdir ();

    if (cfgdir)
    {
        snprintf(filename, PATH_MAX, "%s/podcast.list", cfgdir);
        filename[PATH_MAX] = 0;

        if((fp = fopen(filename, "r")))
        {
            while (fgets (buf, PATH_MAX, fp))
            {
                /* allow comments */
                if ((buf[0] == ';') || (buf[0] == '#')) continue;
                arg = strchr (buf, '=');
                if (!arg || (arg == buf))
                {
                    len = strlen(buf);
                    if((len>0) && (buf[len-1] == 0x0a))  buf[len-1] = 0;
                    line = g_strdup(buf);
                    if (g_ascii_strcasecmp (line, "end podcast") == 0 && name != NULL && url != NULL)
                    {
                        podcast_add(name, url);
                        g_free(name);
                        g_free(url);
                    }
                    g_free(line);
                    continue;
                }
                /* skip whitespace */
                bufp = buf;
                while (g_ascii_isspace(*bufp)) ++bufp;
                line = g_strndup (buf, arg-bufp);
                ++arg;
                len = strlen (arg); /* remove newline */
                if((len>0) && (arg[len-1] == 0x0a))  arg[len-1] = 0;
                /* skip whitespace */
                while (g_ascii_isspace(*arg)) ++arg;
                if(g_ascii_strcasecmp (line, "name") == 0)
                {
                    name = g_strdup(arg);
                }
                else if (g_ascii_strcasecmp (line, "url") == 0)
                {
                    url = g_strdup(arg);
                }
                g_free(line);
            }
            fclose(fp);
        }
    }
}

void podcast_file_add (gchar *title, gchar *url, 
                       gchar *desc, gchar *artist, 
                       gchar pubdate[14], gchar fetchdate[14], 
                       glong size, gchar *local,
                       gboolean fetched, gboolean tofetch)
{
    // assign some memory to hold data for new podcast
    struct podcast_file *podcast_file = malloc(sizeof(struct podcast_file));
    podcast_file->title = g_strdup(title);
    podcast_file->url = g_strdup(url);
    podcast_file->desc = g_strdup(desc);
    podcast_file->artist = g_strdup(artist);
    podcast_file->local = g_strdup(local);
//    podcast_file->pubdate = pubdate;
//    podcast_file->fetchdate = fetchdate;
    podcast_file->size = size;
    podcast_file->fetched = fetched;
    podcast_file->tofetch = tofetch;
    podcast_files = g_list_insert(podcast_files, podcast_file, -1);

}

void podcast_file_delete_by_url (gchar *url)
{

}

gboolean podcast_already_have_url (gchar *url)
{
    int i = 0;
    struct podcast_file *podcast_file = NULL;

    while(i < g_list_length(podcast_files))
    {
        podcast_file = g_list_nth_data(podcast_files, i);
        if (g_ascii_strcasecmp(podcast_file->url, url) == 0)
            return TRUE;
        ++i;
    }
    return FALSE;
}

gint podcast_file_read_from_file()
{
    gchar *cfgdir = prefs_get_cfgdir ();
    if (cfgdir)
    {
        gchar *filename = g_strdup_printf("%s/podcast.xml", cfgdir);
        parse_file_for_podcast_files(filename);
        g_free(filename);
        g_free(cfgdir);
        return 0;
    } else {
        g_free(cfgdir);
        return 1;
    }
}

gint podcast_file_write_to_file()
{
    gchar *cfgdir = prefs_get_cfgdir ();

    if (cfgdir)
    {
        gchar *filename = g_strdup_printf("%s/podcast.xml", cfgdir);
        FILE *fp = fopen(filename, "w");
        int i = 0;
        struct podcast_file *podcast_file;

        fprintf(fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
        fprintf(fp, "<rss xmlns:itunes=\"http://example.com/DTDs/Podcast-1.0.dtd\" version=\"2.0\">\n");
        fprintf(fp, "<channel>\n");
        fprintf(fp, "  <title>gtkpod local podcast file</title>\n\n");

        while(i < g_list_length(podcast_files))
        {
            podcast_file = g_list_nth_data(podcast_files, i);

            fprintf(fp, "  <item>\n");
            fprintf(fp, "    <title>%s</title>\n",                   podcast_file->title);
            fprintf(fp, "    <itunes:author>%s</itunes:author>\n",    podcast_file->artist);
            fprintf(fp, "    <enclosure url=\"%s\" length=%ld />\n",  podcast_file->url, podcast_file->size);
            fprintf(fp, "    <description>%s</description>\n",       podcast_file->desc);
            if (podcast_file->local)
                fprintf(fp, "    <gtkpod:local>%s</gtkpod:local>\n",     podcast_file->local);
            fprintf(fp, "    <gtkpod:fetched>%d</gtkpod:fetched>\n", podcast_file->fetched);
            fprintf(fp, "    <gtkpod:tofetch>%d</gtkpod:tofetch>\n", podcast_file->tofetch);
            fprintf(fp, "  </item>\n");       

            ++i;
        }

        fprintf(fp, "</channel>\n</rss>\n");

        fclose(fp);
        return 0;
    } else {
        return 1;
    }
    
}

void podcast_fetch ()
{
    //pthread_create(&podcast_fetch_tid, NULL, &podcast_fetch_thread, NULL);
    podcast_fetch_thread();
    //g_thread_create(podcast_fetch_thread, NULL, FALSE, NULL);
}

void podcast_fetch_thread(gpointer data)
{
    podcast_fetch_in_progress = TRUE;

    struct podcast *podcast;
    struct podcast_file *podcast_file;
    guint i = 0;
    gint found = 0;
    gchar *status_msg = NULL, *cfgdir = NULL, *filename = NULL;

    gdk_threads_enter();
    podcast_window_create();
    gdk_threads_leave();

    while (gtk_events_pending())  gtk_main_iteration();

    if (g_list_length(podcasts) == 0)
    {
        podcast_log(_("Fetch started, but no podcasts to fetch"));
        return;
    }

/*
    gchar *pc_dir = prefs_get_pc_dir();
    struct stat buf;
    if (stat (pc_dir, &buf) != -1)
    {
        gtkpod_warning(_("The podcast directory (\"%s\") does not exist."), pc_dir);
        g_free(pc_dir);
        return;
    } else {
        g_free(pc_dir);
        if (!(S_IFDIR & buf.st_mode))
        {
            gtkpod_warning(_("The path given as the podcast directory is not a directory."));
            return;
        }
    }
*/

    podcast_log(_("Beginning to fetch podcasts"));
    cfgdir = prefs_get_cfgdir ();

    if (cfgdir)
    {
        filename = g_strdup_printf("%s/tmp.xml", cfgdir);

        while(i < g_list_length(podcasts))
        {
            podcast = g_list_nth_data(podcasts, i);
            if (retrieve_url_to_path(podcast->url, filename))
            {
                status_msg = g_strdup_printf("Could not fetch '%s' (%d of %d metafiles)", podcast->name, (i+1), g_list_length(podcasts));
                podcast_log(_(status_msg));
                podcast_set_status(_(status_msg));
                g_free(status_msg);
            } else {
                found = parse_file_for_podcast_files(filename);
                if (found == 1)
                  status_msg = g_strdup_printf("Fetched '%s' (%d of %d metafiles), finding %d new podcast", podcast->name, (i+1), g_list_length(podcasts), found);
                else
                  status_msg = g_strdup_printf("Fetched '%s' (%d of %d metafiles), finding %d new podcasts", podcast->name, (i+1), g_list_length(podcasts), found);

                podcast_log(_(status_msg));
                podcast_set_status(_(status_msg));
                g_free(status_msg);
            }
            unlink(filename);
            ++i;
        }
        g_free(filename);
        i = 0;

        gdk_threads_enter();

        GtkListStore *model = GTK_LIST_STORE(gtk_tree_view_get_model(podcast_list));
        GtkTreeIter iter;
        GtkTreePath *path;
        gtk_list_store_clear (model);

        transfer_total = 0;
        while(i < g_list_length(podcast_files))
        {
            podcast_file = g_list_nth_data(podcast_files, i);
            if (podcast_file->tofetch)
            {
                status_msg = g_strdup_printf("%.2fMb", (double) podcast_file->size / 1024 / 1024);
                transfer_total += podcast_file->size;
                gtk_list_store_append (model, &iter);
                gtk_list_store_set (model, &iter,
                                    PCL_TITLE, podcast_file->title,
                                    PCL_URL, podcast_file->url,
                                    PCL_SIZE, status_msg,
                                    PCL_PROGRESS, "Pending",
                                    -1);
                g_free(status_msg);
            }
            ++i;
        }

        gdk_threads_leave();
        while (gtk_events_pending())  gtk_main_iteration();

        iTunesDB *itdb = get_itdb_local ();
        Playlist *pl = itdb_playlist_by_name(itdb, "Podcasts");

        i = 0;
        while(i < g_list_length(podcast_files))
        {
            podcast_file = g_list_nth_data(podcast_files, i);
            if (podcast_file->tofetch)
            {
                path = gtk_tree_path_new_from_indices(i, -1);
                gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &iter, path);
                gtk_tree_path_free(path);

                //gtk_list_store_set(model, &iter, PCL_PROGRESS, "Fetching");

                filename = choose_filename(podcast_file->url);
                status_msg = g_strdup_printf("Starting download of '%s' to '%s' (%ld bytes)", podcast_file->url, filename, podcast_file->size);
                podcast_log(_(status_msg));
                g_free(status_msg);
                if (retrieve_url_to_path(podcast_file->url, filename))
                {
        //            gtk_list_store_set(model, &iter, PCL_PROGRESS, "Failed");
                    status_msg = g_strdup_printf("Could not fetch '%s'", podcast_file->title);
                    podcast_log(_(status_msg));
                    podcast_set_status(_(status_msg));
                    g_free(status_msg);
                } else {
        //            gtk_list_store_set(model, &iter, PCL_PROGRESS, "Done");
                    transfer_done += podcast_file->size;
                    podcast_file->tofetch = FALSE;
                    podcast_file->fetched = TRUE;
                    g_free(podcast_file->local);
                    podcast_file->local = g_strdup(filename);
                    status_msg = g_strdup_printf("Fetched '%s'", podcast_file->title);
                    podcast_log(_(status_msg));
                    podcast_set_status(_(status_msg));
                    g_free(status_msg);

                    add_track_by_filename (itdb, filename, pl, FALSE, NULL, NULL);
                    
                }
                g_free(filename);
            }
    //        gtk_tree_model_iter_next (GTK_TREE_MODEL(model), &iter);
    podcast_file_write_to_file();
            ++i;
        }

    }
    podcast_log(_("Completed fetch"));
    podcast_set_status(_("Completed fetch"));

    podcast_fetch_in_progress = FALSE;
}

static gchar *choose_filename(gchar *url)
{
    gchar *working = g_strdup_printf("%s/%s", prefs_get_pc_dir(), g_strrstr(url, "/")+1);
    return working;
}

static gint retrieve_url_to_path (gchar *url, gchar *path)
{
gdk_threads_enter();
podcast_set_cur_file_name(url);
gdk_threads_leave();
int ret = 0;

CURL *curl = curl_easy_init();
CURLM *curlm = curl_multi_init();

url_being_fetched = g_strdup(url);

if (curl)
{
  FILE *fp = fopen(path, "w");

  if (fp != NULL) {
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, update_progress);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, FALSE);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, NULL);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15);

    curl_multi_add_handle(curlm, curl);

    int running_handles = 1;
    while (running_handles > 0)
    {
      curl_multi_perform(curlm, &running_handles);
      while (gtk_events_pending())  gtk_main_iteration();
    }

    fclose(fp);
  } else {
    gtkpod_warning (_("Could not open \"%s\" for writing podcast download.\n"), path);
    ret = -1;
  }

  g_free(url_being_fetched);
  url_being_fetched = NULL;

  curl_easy_cleanup(curl);
  curl_global_cleanup();
}
return ret;
}

int update_progress(gpointer *data,
                    double t, /* dltotal */
                    double d, /* dlnow */
                    double ultotal,
                    double ulnow)
{
/*  printf("%d / %d (%g %%)\n", d, t, d*100.0/t);*/

/*if (t == 0)
{
    gtk_progress_bar_pulse(GTK_PROGRESS_BAR (glade_xml_get_widget (podcast_window_xml, "file_progressbar")));
    while (gtk_events_pending())  gtk_main_iteration();
}*/

if (t == 0 || transfer_total == 0) return 0;
if (d/t > 1 || (d+transfer_done)/transfer_total > 1) return 0;
  gdk_threads_enter();
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR (glade_xml_get_widget (podcast_window_xml, "file_progressbar")), d/t);
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR (glade_xml_get_widget (podcast_window_xml, "total_progressbar")), (d+transfer_done)/transfer_total);
  gchar *tmp = g_strdup_printf("%.2fMb of %.2fMb (%.0f%%), 12:32 remaining", (double) (d+transfer_done)/1024/1024, (double) transfer_total/1024/1024, 100*(d+transfer_done)/transfer_total);
  gtk_progress_bar_set_text(GTK_PROGRESS_BAR (glade_xml_get_widget (podcast_window_xml, "total_progressbar")), tmp);
  g_free(tmp);

  gdk_threads_leave();
    while (gtk_events_pending())  gtk_main_iteration();
  return 0;
}

static gint parse_file_for_podcast_files(gchar *file)
{
    /* This function will get all the information we want  *
     * but it does so ignoring all heirachies in the file. */
    FILE *fp = NULL;
    gchar buf[20000], cur;
    gint i = 0, found = 0;
    gboolean is_xml = FALSE, is_rss = FALSE;
    gchar *tag = NULL, *info = NULL, *value = NULL, *status_msg = NULL, *local = NULL;

    gchar *title = NULL, *desc = NULL, *url = NULL, *artist = NULL;
    glong size;
    gboolean tofetch = TRUE, fetched = FALSE;
    gboolean looking_for_cdata_end = FALSE;

    if((fp = fopen(file, "r")))
    {
        while (fgetc(fp) != 0x3C && !feof(fp)) {}     /* search for our first '<' */

        while (!feof(fp))
        {
            while ((cur = fgetc(fp)))
            {
                looking_for_cdata_end = FALSE;
                if (cur != 0x20 && cur !=0x3E)            /* a space or a '>' will denote the end of the tag name */
                {
                    buf[i++] = cur;
                    if (i == 8)
                    {
                        buf[i] = 0;
                        if (strcasecmp(buf, "![CDATA[") == 0)
                          {  looking_for_cdata_end = TRUE; i = 0; }
                    }
                } else {
                    break;
                }
                if (looking_for_cdata_end) break;
            }

            if (!looking_for_cdata_end)
            {

                buf[i++] = 0;
                tag = strdup(buf);                          /* retrieve the tag name from the buffer */
                i = 0;

                while (cur != 0x3E)                           // make our way to the end of the tag
                {
                    cur = fgetc(fp);
                    buf[i++] = cur;
                }

                if (i > 0)
                {
                    buf[i] = 0;
                    info = strdup(buf);
                    i = 0;
                }

            }

            while ((cur = fgetc(fp)))
            {
                if (feof(fp)) break;

                if (!(cur == 0x3C && !looking_for_cdata_end))  /* if we're in a CDATA, we're looking for the end of a tag */
                {
                     buf[i++] = cur;
                }
                else
                { break; }

                if (buf[i-3] == 0x5D && buf[i-2] == 0x5D && buf[i-1] == 0x3E && looking_for_cdata_end)
                {
                    i -= 3;
                    looking_for_cdata_end = 0;
                }
            }

            buf[i++] = 0;
            value = g_strdup(buf);
            i = 0;
            if (g_ascii_strcasecmp (tag, "?xml") == 0)
                is_xml = TRUE;
            if (g_ascii_strcasecmp (tag, "rss") == 0)
                is_rss = TRUE;
            if (g_ascii_strcasecmp (tag, "title") == 0)
                { g_free(title);  title = g_strdup(value); }
            if (g_ascii_strcasecmp (tag, "itunes:author") == 0)
                { g_free(artist); artist = g_strdup(value); }
            if (g_ascii_strcasecmp (tag, "description") == 0)
                { g_free(desc);   desc = g_strdup(value); }
            if (g_ascii_strcasecmp (tag, "gtkpod:tofetch") == 0)
                tofetch = atoi(value);
            if (g_ascii_strcasecmp (tag, "gtkpod:fetched") == 0)
                fetched = atoi(value);
            if (g_ascii_strcasecmp (tag, "gtkpod:local") == 0)
                { g_free(local);  local = g_strdup(value); }
            if (strcasecmp (tag, "enclosure") == 0)
            {
                url = podcast_get_tag_attr(info, "url=");
                info = podcast_get_tag_attr(info, "length=");
                size = atoi(info);
            }

            if (g_ascii_strcasecmp (tag, "/item") == 0)
            {
                if (!podcast_already_have_url(url))
                {
                    podcast_file_add(title, url,
                                     desc, artist,
                                     NULL, NULL,
                                     size, local,
                                     fetched, tofetch);
                    tofetch = TRUE; fetched = FALSE;
                    found++;
                    status_msg = g_strdup_printf("  Found and added '%s'", url);
                    podcast_log(_(status_msg));
                    g_free(status_msg);
                } else {
                    status_msg = g_strdup_printf("  Found but already have '%s'", url);
                    podcast_log(_(status_msg));
                    g_free(status_msg);
                }
            }

//            g_free(value);
//            g_free(tag);
//            g_free(info);
        }
        fclose(fp);
    } else {
    //    gtkpod_warning(_("Failed to open '%s' to read podcast list."), file);
    //        gchar *status_msg = g_strdup_printf("Error number is: %d", errno);
    //        gtkpod_statusbar_message(_(status_msg));

    }

    return found;
}

gchar *podcast_get_tag_attr(gchar *attrs, gchar *req)
{
    char *ret = strstr(attrs, req) + strlen(req);
    char *ret2 = g_strndup(ret, strstr(ret, " ") - ret);            /* unless malformed, the end of this tag
                                                                     reads: ' />'. That space has to be there */
    if ((*ret2) == 0x27 && *(ret2 + strlen(ret2) - 1) == 0x27)
    {
        ++ret2;
        *(ret2 + strlen(ret2) - 1) = 0x00;
    }
    if ((*ret2) == 0x22 && *(ret2 + strlen(ret2) - 1) == 0x22)
    {
        ++ret2;
        *(ret2 + strlen(ret2) - 1) = 0x00;
    }
    //free(ret);
    return g_strdup(ret2);
}

static void podcast_log (gchar *msg)
{
    if (prefs_get_pc_log())
    {
        FILE *fp = NULL;
        gchar *filename = prefs_get_pc_log_file();

        time_t rawtime;
        struct tm * timeinfo;

        time ( &rawtime );
        timeinfo = localtime ( &rawtime );

        fp = fopen(filename, "a");
        if (fp)
        {
            fprintf (fp, "%s: %s\n", asctime (timeinfo), msg);
            fclose(fp);
        }
    }
}

gboolean
on_podcast_window_delete_event         (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  if (podcast_fetch_in_progress)
  {
/*    GtkWidget *dialog, *label;
    dialog = gtk_dialog_new_with_buttons ("Notice",
                                          NULL,
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_OK,
                                          GTK_RESPONSE_NONE,
                                          NULL);
    label = gtk_label_new ("You cannot close this window until all downloads have finished or you click 'Abort All'");
    g_signal_connect_swapped (dialog,
                              "response", 
                              G_CALLBACK (gtk_widget_destroy),
                              dialog);
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),
                       label);
    gtk_widget_show_all (dialog); */
  }
  return podcast_fetch_in_progress;
}

gboolean
on_abort_selected_button_clicked       (GtkWidget       *widget,
                                        gpointer         user_data)
{
  abort_fetch(ABORT_SELECTED);
  return FALSE;
}

gboolean
on_abort_current_button_clicked        (GtkWidget       *widget,
                                        gpointer         user_data)
{
  abort_fetch(ABORT_CURRENT);
  return FALSE;
}

gboolean
on_abort_all_button_clicked            (GtkWidget       *widget,
                                        gpointer         user_data)
{
  abort_fetch(ABORT_ALL);
  return FALSE;
}

void abort_fetch (gint what)
{
    struct podcast_file *podcast_file = NULL;
    switch (what)
    {
        case ABORT_SELECTED :
            break;
            GtkTreeModel *model;
            GtkTreeIter   iter;
            gchar *url;

            model = gtk_tree_view_get_model(podcast_list);
        //    gtk_tree_model_get_iter(model, &iter);
        //    gtk_tree_model_get (model, &iter,
        //                        PCL_TITLE, &url,
        //                        -1);
            break;

        case ABORT_CURRENT :
            if (url_being_fetched)
                abort_urls_to_add = g_list_append(abort_urls_to_add, g_strdup(url_being_fetched));
            break;
        case ABORT_ALL :
            podcast_files = g_list_first(podcast_files);
            do
            {
                podcast_file = podcast_files->data;
                if (podcast_file->tofetch)
                    abort_urls_to_add = g_list_append(abort_urls_to_add, g_strdup(podcast_file->url));
            } while (podcast_files = g_list_next(podcast_files));
            break;
    }

    if (g_list_length(abort_urls_to_add) == 0) return;

    GtkWidget *dialog, *label;

    dialog = gtk_dialog_new_with_buttons ("Aborting podcast fetching",
                                          podcast_window,
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_YES,
                                          GTK_RESPONSE_YES,
                                          GTK_STOCK_NO,
                                          GTK_RESPONSE_NO,
                                          GTK_STOCK_CANCEL,
                                          GTK_RESPONSE_CANCEL,
                                          NULL);
    label = gtk_label_new("You have chosen to abort one or more podcasts.\nWould you like these to be aborted permanently?\n");
    
    g_signal_connect(dialog, "response", abort_fetch_response, NULL);

    gtk_container_add(GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), label);
    gtk_widget_show_all(dialog);
}

void        abort_fetch_response           (GtkDialog *dialog,
                                            gint arg1,
                                            gpointer user_data)
{
    gtk_widget_destroy(GTK_WIDGET(dialog));

    switch (arg1)
    {
        case GTK_RESPONSE_YES :
            break;
        case GTK_RESPONSE_NO :
            break;
        case GTK_RESPONSE_CANCEL :
            return;
            break;
    }
    abort_urls = g_list_concat(abort_urls, abort_urls_to_add);
}

static void
podcast_window_create(void)
{
/*    if (podcast_window)
    {    podcast window already open -- raise to the top 
        gdk_window_raise (podcast_window->window);
        return;
    } */

    podcast_window_xml = glade_xml_new (xml_file, "podcast_window", NULL);
    glade_xml_signal_autoconnect (podcast_window_xml);

    podcast_window = glade_xml_get_widget(podcast_window_xml,"podcast_window");

    g_return_if_fail (podcast_window);

    create_podcast_list();

    gtk_widget_show(podcast_window);

}

static void create_podcast_list ()
{
    GtkCellRenderer     *renderer;
    GtkTreeModel        *model;

    podcast_list = GTK_TREE_VIEW(gtk_tree_view_new ());
//    gtk_widget_set_size_request(GTK_WIDGET (podcast_list), 270, 105);

    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_insert_column_with_attributes (podcast_list,
                                                 -1,
                                                 "Title",
                                                 renderer,
                                                 "text", PCL_TITLE,
                                                 NULL);

    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_insert_column_with_attributes (podcast_list,
                                                 -1,
                                                 "URL",
                                                 renderer,
                                                 "text", PCL_URL,
                                                 NULL);

    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_insert_column_with_attributes (podcast_list,
                                                 -1,
                                                 "Size",
                                                 renderer,
                                                 "text", PCL_SIZE,
                                                 NULL);

    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_insert_column_with_attributes (podcast_list,
                                                 -1,
                                                 "Progress",
                                                 renderer,
                                                 "text", PCL_PROGRESS,
                                                 NULL); 

    model = GTK_TREE_MODEL(gtk_list_store_new (PCL_NUM_COLS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING));
    gtk_tree_view_set_model (GTK_TREE_VIEW (podcast_list), model);

    /* The tree view has acquired its own reference to the
     *  model, so we can drop ours. That way the model will
     *  be freed automatically when the tree view is destroyed */

    g_object_unref (model);

    GtkWidget *podcast_list_window = glade_xml_get_widget (podcast_window_xml, "podcast_list_window");

    gtk_container_add (GTK_CONTAINER (podcast_list_window), GTK_WIDGET(podcast_list));
if(podcast_list_window)
    gtk_widget_show_all (podcast_list_window);
return;
}




void podcast_set_status(gchar *status)
{
    gdk_threads_enter();
    gtk_label_set_text (GTK_LABEL (glade_xml_get_widget (podcast_window_xml, "status_label")), status);
    gdk_threads_leave();
    while (gtk_events_pending())  gtk_main_iteration();
}

void podcast_set_cur_file_name(gchar *text)
{
    gchar *working = g_strdup_printf("%s", g_strrstr(text, "/")+1);
    gtk_progress_bar_set_text (GTK_PROGRESS_BAR (glade_xml_get_widget (podcast_window_xml, "file_progressbar")), working);
    g_free(working);
}

/* Get the local itdb */
static iTunesDB *get_itdb_local (void)
{
    struct itdbs_head *itdbs_head;
    GList *gl;

    g_return_val_if_fail (gtkpod_window, NULL);
    itdbs_head = g_object_get_data (G_OBJECT (gtkpod_window),
                                    "itdbs_head");
    if (!itdbs_head) return NULL;
    for (gl=itdbs_head->itdbs; gl; gl=gl->next)
    {
        iTunesDB *itdb = gl->data;
        g_return_val_if_fail (itdb, NULL);
        if (itdb->usertype & GP_ITDB_TYPE_LOCAL)
            return itdb;
    }
    return NULL;
}

