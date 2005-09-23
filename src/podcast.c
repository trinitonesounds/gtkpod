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
//#include "podcast_window.h"
#include "info.h"
#include <time.h>
#include <sys/socket.h>

GList *podcasts = NULL;
GList *podcast_files = NULL;

GladeXML *podcast_window_xml;
static GtkWidget *podcast_window = NULL;

static pthread_t      podcast_fetch_tid;

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

void podcast_write_from_store (GtkTreeStore *store)
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

void podcast_read_into_store (GtkTreeStore *store)
{
    int i = 0;
    struct podcast *podcast = NULL;

    GtkTreeIter iter;

    while(i < g_list_length(podcasts))
    {
        podcast = g_list_nth_data(podcasts, i);
        gtk_list_store_append (GTK_LIST_STORE(store), &iter);
        gtk_list_store_set (GTK_LIST_STORE(store), &iter,
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
                       glong size,
                       gboolean fetched, gboolean tofetch)
{
    // assign some memory to hold data for new podcast
    struct podcast_file *podcast_file = malloc(sizeof(struct podcast_file));
    podcast_file->title = g_strdup(title);
    podcast_file->url = g_strdup(url);
    podcast_file->desc = g_strdup(desc);
    podcast_file->artist = g_strdup(artist);
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

GList *podcast_file_find_to_fetch ()
{

}

void podcast_fetch ()
{
    podcast_window_create();
    pthread_create(&podcast_fetch_tid, NULL, &podcast_fetch_thread, NULL);
}

void podcast_fetch_thread(gpointer data)
{
    podcast_fetch_in_progress = TRUE;

    struct podcast *podcast;
    struct podcast_file *podcast_file;
    guint i = 0;
    gint found = 0;
    gchar *status_msg = NULL, *cfgdir = NULL, *filename = NULL;

    if (g_list_length(podcasts) == 0)
    {
//        gtkpod_statusbar_message(_("No podcasts to fetch"));
        return;
    }

//    gtkpod_statusbar_message(_("Beginning to fetch podcasts"));
    cfgdir = prefs_get_cfgdir ();

    if (cfgdir)
    {
        filename = g_strdup_printf("%s/tmp.xml", cfgdir);

        while(i < g_list_length(podcasts))
        {
            podcast = g_list_nth_data(podcasts, i);
            if (retrieve_url_to_path(podcast->url, filename))
            {
                status_msg = g_strdup_printf("Could not fetch '%s' (%d of %d metafiles), finding %d podcasts", podcast->name, (i+1), g_list_length(podcasts), found);
                podcast_set_status(_(status_msg));
                g_free(status_msg);
            } else {
                found = parse_file_for_podcast_files(filename);
                status_msg = g_strdup_printf("Fetched '%s' (%d of %d metafiles), finding %d podcasts", podcast->name, (i+1), g_list_length(podcasts), found);
                podcast_set_status(_(status_msg));
                g_free(status_msg);
            }
            unlink(filename);
            ++i;
        }
        g_free(filename);
        i = 0;

        while(i < g_list_length(podcast_files))
        {
            podcast_file = g_list_nth_data(podcast_files, i);
            if (podcast_file->tofetch)
            {
                filename = g_strdup_printf("%s%i.mp3", prefs_get_pc_dir(), i);
                if (retrieve_url_to_path(podcast_file->url, filename))
                {
                    status_msg = g_strdup_printf("Could not fetch '%s'", podcast_file->title);
                    podcast_set_status(_(status_msg));
                    g_free(status_msg);
                } else {
                    podcast_file->tofetch = FALSE;
                    podcast_file->fetched = TRUE;
                    status_msg = g_strdup_printf("Fetched '%s'", podcast_file->title);
                    podcast_set_status(_(status_msg));
                    g_free(status_msg);
                }
            }
            ++i;
        }

    }

    gtkpod_statusbar_message(_("All podcasts fetched"));

    podcast_fetch_in_progress = FALSE;
}

static gint retrieve_url_to_path (gchar *url, gchar *path)
{
#if 1
    podcast_set_cur_file_name(url);
    gchar *to_run = g_strconcat("wget '", url, "' -O ", path, " ", NULL);
    gint ret = system(to_run);
    podcast_set_cur_file_name("");
//    g_free(to_run);
    return ret;
#else

//int socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);


#endif
}

static gint parse_file_for_podcast_files(gchar *file)
{
    /* This function will get all the information we want  *
     * but it does so ignoring all heirachies in the file. */
    FILE *fp = NULL;
    gchar buf[1023], cur;
    gint i = 0, found = 0;
    gboolean is_xml = FALSE, is_rss = FALSE;
    gchar *tag = NULL, *info = NULL, *value = NULL, *status_msg = NULL;

    gchar *title = NULL, *desc = NULL, *url = NULL, *artist = NULL;
    glong size;
    gboolean tofetch = TRUE, fetched = FALSE;

    if((fp = fopen(file, "r")))
    {
        while (fgetc(fp) != 0x3C && !feof(fp)) {}     /* search for our first '<' */

        while (!feof(fp))
        {
            while ((cur = fgetc(fp)))
            {
                if (cur != 0x20 && cur != 0x3E)           /* a ' ' or a '>' will denote the end of the tag name */
                { buf[i++] = cur; }
                else
                { break; }
            }
            buf[i++] = 0;
            tag = g_strdup(buf);                          /* retrieve the tag name from the buffer */
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

            while ((cur = fgetc(fp)))
            {
                if (feof(fp) || i == 1023) break;         /* either we've reached the end of the file
                                                           * or we've run out of space in the buffer */
                if (cur != 0x3C)                          /* we're looking for the beginning of the next tag */
                { buf[i++] = cur; }
                else
                { break; }
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
                                     size,
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
    char *ret2 = strndup(ret, strstr(ret, " ") - ret);            /* unless malformed, the end of this tag
                                                                reads: ' />'. That space has to be there */
    if ((*ret2) == 0x22 && *(ret2 + strlen(ret2) - 1) == 0x22)
    {
        ++ret2;
        *(ret2 + strlen(ret2) - 1) = 0x00;
    }
    //free(ret);
    return strdup(ret2);
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

        if (fp = fopen(filename, "a"))
        {
            fprintf (fp, "%s: %s", asctime (timeinfo), msg);
            fclose(fp);
        }
    }
}






void
podcast_window_create(void)
{
    GtkWidget *w = NULL;
    GtkTooltips *tt;
    GtkTooltipsData *tooltipsdata;

    if (podcast_window)
    {   /* prefs window already open -- raise to the top */
        gdk_window_raise (podcast_window->window);
        return;
    }

    podcast_window_xml = glade_xml_new (xml_file, "podcast_window", NULL);
    glade_xml_signal_autoconnect (podcast_window_xml);

    podcast_window = glade_xml_get_widget(podcast_window_xml,"podcast_window");

    g_return_if_fail (podcast_window);

    gtk_widget_show(podcast_window);

}

void podcast_set_status(gchar *status)
{ gtk_label_set_text (GTK_LABEL (glade_xml_get_widget (podcast_window_xml, "status_label")), status); }

void podcast_set_cur_file_name(gchar *text)
{
    gchar *tmp = g_strrstr(text, "/");
    gtk_progress_bar_set_text (GTK_PROGRESS_BAR (glade_xml_get_widget (podcast_window_xml, "file_progressbar")), tmp);
}
