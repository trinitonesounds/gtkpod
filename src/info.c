/* Time-stamp: <2004-07-07 23:40:03 jcs>
|
|  Copyright (C) 2002-2003 Jorg Schuler <jcsjcs at users.sourceforge.net>
|  Part of the gtkpod project.
| 
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

/* This file provides functions for the info window as well as for the
 * statusbar handling */

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "info.h"
#include "interface.h"
#include "misc.h"
#include "prefs.h"
#include "track.h"
#include "support.h"

/* pointer to info window */
static GtkWidget *info_window = NULL;

/* stuff for statusbar */
static GtkWidget *gtkpod_statusbar = NULL;
static GtkWidget *gtkpod_tracks_statusbar = NULL;
static GtkWidget *gtkpod_space_statusbar = NULL;
static guint statusbar_timeout_id = 0;

/* lock for size related variables (used by child and parent) */
static GMutex *space_mutex = NULL;
static GThread *space_thread = NULL;
static gboolean space_uptodate = FALSE;
static gchar *space_mp = NULL;
static gdouble space_ipod_free = 0;
static gdouble space_ipod_used = 0;


static gdouble get_ipod_free_space(void);
#if 0
static gdouble get_ipod_used_space(void);
#endif

/* fill in tracks, playtime and filesize from track list @tl */
static void fill_in_info (GList *tl, guint32 *tracks,
			  guint32 *playtime, gdouble *filesize)
{
    GList *gl;

    if (!tracks || !playtime || !filesize) return; /* sanity */

    *tracks = 0;
    *playtime = 0;
    *filesize = 0;

    for (gl=tl; gl; gl=gl->next)
    {
	Track *s = gl->data;
	*tracks += 1;
	*playtime += s->tracklen/1000;
	*filesize += s->size;
    }
}

static void fill_label_uint (gchar *w_name, guint32 nr)
{    
    GtkWidget *w = lookup_widget (info_window, w_name);
    if (w)
    {
	gchar *str = g_strdup_printf ("%u", nr);
	gtk_label_set_text (GTK_LABEL (w), str);
	g_free (str);
    }
}

static void fill_label_time (gchar *w_name, guint32 secs)
{    
    GtkWidget *w = lookup_widget (info_window, w_name);
    if (w)
    {
	gchar *str = g_strdup_printf ("%u:%02u:%02u",
				      secs / 3600,
				      (secs % 3600) / 60,
				      secs % 60);
	gtk_label_set_text (GTK_LABEL (w), str);
	g_free (str);
    }
}

static void fill_label_size (gchar *w_name, gdouble size)
{    
    GtkWidget *w = lookup_widget (info_window, w_name);
    if (w)
    {
	gchar *str = get_filesize_as_string (size);
	gtk_label_set_text (GTK_LABEL (w), str);
	g_free (str);
    }
}

static void fill_label_string (gchar *w_name, const char *str)
{    
    GtkWidget *w = lookup_widget (info_window, w_name);
    if (w)
    {
	gtk_label_set_text (GTK_LABEL (w), str);
    }
}


/* open info window */
void info_open_window (void)
{
    if (info_window)  return;            /* already open */
    info_window = create_gtkpod_info ();
    if (info_window)
    {
	gint defx, defy;
	prefs_get_size_info (&defx, &defy);
	gtk_window_set_default_size (GTK_WINDOW (info_window), defx, defy);
	prefs_set_info_window (TRUE); /* notify prefs */
	info_update ();
	gtk_widget_show (info_window);
	/* set the menu item for the info window correctly */
	display_set_info_window_menu ();
    }
}

/* close info window */
void info_close_window (void)
{
    GtkWidget *win;

    if (!info_window) return; /* not open */
    info_update_default_sizes ();
    win = info_window;
    info_window = NULL;
    gtk_widget_destroy (win);
    prefs_set_info_window (FALSE); /* notify prefs */
    /* set the menu item for the info window correctly */
    display_set_info_window_menu ();
}

/* save current window size */
void info_update_default_sizes (void)
{
    if (info_window)
    {
	gint defx, defy;
	gtk_window_get_size (GTK_WINDOW (info_window), &defx, &defy);
	prefs_set_size_info (defx, defy);
    }
}

/* update all sections of info window */
void info_update (void)
{
    if (!info_window) return; /* not open */
    info_update_track_view ();
    info_update_playlist_view ();
    info_update_totals_view ();
}

static void info_update_track_view_total (void)
{
    guint32 tracks, playtime; /* playtime in secs */
    gdouble  filesize;        /* in bytes */
    GList *displayed;

    if (!info_window) return; /* not open */
    displayed = display_get_selected_members (prefs_get_sort_tab_num()-1);
    fill_in_info (displayed, &tracks, &playtime, &filesize);
    fill_label_uint ("tracks_total", tracks);
    fill_label_time ("playtime_total", playtime);
    fill_label_size ("filesize_total", filesize);
}

void info_update_track_view_selected (void)
{
    guint32 tracks, playtime; /* playtime in secs */
    gdouble  filesize;        /* in bytes */
    GList *selected;

    if (!info_window) return; /* not open */
    selected = display_get_selection (prefs_get_sort_tab_num ());
    fill_in_info (selected, &tracks, &playtime, &filesize);
    g_list_free (selected);
    fill_label_uint ("tracks_selected", tracks);
    fill_label_time ("playtime_selected", playtime);
    fill_label_size ("filesize_selected", filesize);
}

/* update track view section */
void info_update_track_view (void)
{
    if (!info_window) return; /* not open */
    info_update_track_view_total ();
    info_update_track_view_selected ();
}

/* update playlist view section */
void info_update_playlist_view (void)
{
    guint32 tracks, playtime; /* playtime in secs */
    gdouble  filesize;        /* in bytes */
    GList   *tl;

    if (!info_window) return; /* not open */
    tl = display_get_selected_members (-1);
    fill_in_info (tl, &tracks, &playtime, &filesize);
    fill_label_uint ("playlist_tracks", tracks);
    fill_label_time ("playlist_playtime", playtime);
    fill_label_size ("playlist_filesize", filesize);
}


/* update "totals" view section */
void info_update_totals_view (void)
{
    guint32 tracks=0, playtime=0; /* playtime in secs */
    gdouble  filesize=0;          /* in bytes */
    Playlist *pl;

    if (!info_window) return; /* not open */
    pl = get_playlist_by_nr (0);
    if (pl)  fill_in_info (pl->members, &tracks, &playtime, &filesize);
    fill_label_uint ("total_playlists", get_nr_of_playlists ()-1);
    fill_label_uint ("total_tracks", tracks);
    fill_label_time ("total_playtime", playtime);
    fill_label_size ("total_filesize", filesize);
    info_update_totals_view_space ();
}

/* update "free space" section of totals view */
void info_update_totals_view_space (void)
{
    gdouble nt_filesize, del_filesize;
    guint32 nt_tracks, del_tracks;


    if (!info_window) return;
    nt_filesize = get_filesize_of_nontransferred_tracks (&nt_tracks);
    fill_label_uint ("non_transfered_tracks", nt_tracks);
    fill_label_size ("non_transfered_filesize", nt_filesize);
    del_filesize = get_filesize_of_deleted_tracks (&del_tracks);
    fill_label_uint ("deleted_tracks", del_tracks);
    fill_label_size ("deleted_filesize", del_filesize);
    if (!prefs_get_offline ())
    {
	if (ipod_connected ())
	{
	    gdouble free_space = get_ipod_free_space()
		+ del_filesize - nt_filesize;
	    fill_label_size ("free_space", free_space);
	}
	else
	{
	    fill_label_string ("free_space", _("n/c"));
	}
    }
    else
    {
	fill_label_string ("free_space", _("offline"));
    }
}


/*------------------------------------------------------------------*\
 *                                                                  *
 *                   Functions for Statusbar                        *
 *                                                                  *
\*------------------------------------------------------------------*/

void
gtkpod_statusbar_init(void)
{
    gtkpod_statusbar = lookup_widget (gtkpod_window, "gtkpod_status");
}

static gint
gtkpod_statusbar_clear(gpointer data)
{
    if(gtkpod_statusbar)
    {
	gtk_statusbar_pop(GTK_STATUSBAR(gtkpod_statusbar), 1);
    }
    statusbar_timeout_id = 0; /* indicate that timeout handler is
				 clear (0 cannot be a handler id) */
    return FALSE;
}

void
gtkpod_statusbar_message(const gchar *message)
{
    if(gtkpod_statusbar)
    {
	gchar buf[PATH_MAX];
	guint context = 1;

	snprintf(buf, PATH_MAX, "  %s", message);
	gtk_statusbar_pop(GTK_STATUSBAR(gtkpod_statusbar), context);
	gtk_statusbar_push(GTK_STATUSBAR(gtkpod_statusbar), context,  buf);
	if (statusbar_timeout_id != 0) /* remove last timeout, if still present */
	    gtk_timeout_remove (statusbar_timeout_id);
	statusbar_timeout_id = gtk_timeout_add(prefs_get_statusbar_timeout (),
					       (GtkFunction) gtkpod_statusbar_clear,
					       NULL);
    }
}

void
gtkpod_tracks_statusbar_init()
{
    gtkpod_tracks_statusbar =
	lookup_widget (gtkpod_window, "tracks_statusbar");
    gtkpod_tracks_statusbar_update();
}

void 
gtkpod_tracks_statusbar_update(void)
{
    if(gtkpod_tracks_statusbar)
    {
	gchar *buf;
	
	buf = g_strdup_printf (_(" P:%d S:%d/%d"),
			       get_nr_of_playlists () - 1,
			       tm_get_nr_of_tracks (),
			       get_nr_of_tracks ());
	gtk_statusbar_pop(GTK_STATUSBAR(gtkpod_tracks_statusbar), 1);
	gtk_statusbar_push(GTK_STATUSBAR(gtkpod_tracks_statusbar), 1,  buf);
	g_free (buf);
    }
    /* Update info window */
    info_update ();
}

/*------------------------------------------------------------------*\
 *                                                                  *
 *                       free space stuff                           *
 *                                                                  *
\*------------------------------------------------------------------*/


/* Since the mount point is used by two separate threads, it can only
   be accessed securely by using a locking mechanism. Therefore we
   keep a copy of the mount point here. Access must only be done
   after locking. */
void space_set_ipod_mount (const gchar *mp)
{
    if (space_mutex)  g_mutex_lock (space_mutex);
    g_free (space_mp);
    space_mp = g_strdup (mp);
    if (space_mutex)   g_mutex_unlock (space_mutex);
}


/* iPod space has to be reread */
void space_data_update (void)
{
    space_uptodate = FALSE;
}


/* Is the iPod connected? If space_ipod_used and space_ipod_free are
   both zero, we assume the iPod is not connected */
gboolean ipod_connected (void)
{
    gboolean result;
    g_return_val_if_fail (space_mutex!=NULL, FALSE);
    g_mutex_lock (space_mutex);
    if ((space_ipod_used == 0) && (space_ipod_free == 0)) result = FALSE;
    else                                                  result = TRUE;
    g_mutex_unlock (space_mutex);
    return result;
}


static gchar*
get_drive_stats_from_df(const gchar *mp)
{
    FILE *fp;
    gchar buf[PATH_MAX+1];
    gchar bufc[PATH_MAX+1];
    gchar *bufp;
    gchar *result = NULL;
    guint bytes_read = 0;

#if 0
    GTimeVal gtv1, gtv2;
    long micros;
    g_get_current_time (&gtv1);
#endif

    if (g_file_test (mp, G_FILE_TEST_EXISTS))
    {
	snprintf(bufc, PATH_MAX, "df -k -P %s", mp);
	if((fp = popen(bufc, "r")))
	{
	    if((bytes_read = fread(buf, 1, PATH_MAX, fp)) > 0)
	    {
		if((bufp = strchr (buf, '\n')))
		{
		    int i = 0;
		    int j = 0;
		    gchar buf2[PATH_MAX+3];
		    
		    ++bufp; /* skip '\n' */
		    while((i < bytes_read) && (j < PATH_MAX))
		    {
			while(!g_ascii_isspace(bufp[i]) && (j<PATH_MAX))
			    buf2[j++] = bufp[i++];
			buf2[j++] = ' ';
			while(g_ascii_isspace(bufp[i]))
			    i++;
		    }
		    buf2[j] = '\0';
		    result = g_strdup_printf("%s", buf2);
		}
	    }
	    pclose(fp);	
	}
    }
#if 0
    g_get_current_time (&gtv2);
    micros = (gtv2.tv_sec-gtv1.tv_sec)*10000000 + (gtv2.tv_usec-gtv1.tv_usec);
    printf ("df: %ld usec\n", micros);
#endif
    return(result);
}


/* update space_ipod_free and space_ipod_used */
static void th_space_update (void)
{
    gchar *mp, *line;
    gchar **tokens = NULL;

    g_mutex_lock (space_mutex);
    mp = g_strdup (space_mp);
    g_mutex_unlock (space_mutex);

    line = get_drive_stats_from_df(mp);

    g_mutex_lock (space_mutex);

    if (line) tokens = g_strsplit(line, " ", 5);
    if (tokens && tokens[0] && tokens[1] && tokens[2] && tokens[3])
    {
	space_ipod_free = g_strtod (tokens[3], NULL) * 1024;
	space_ipod_used = g_strtod (tokens[2], NULL) * 1024;
	space_uptodate = TRUE;
    }
    else
    {
	space_ipod_free = 0;
	space_ipod_used = 0;
	space_uptodate = FALSE;  /* this way we will detect when the
				    iPod is connected */
    }
    g_mutex_unlock (space_mutex);
    g_free (mp);
    g_strfreev(tokens);
}


/* keep space_ipod_free/used updated in regular intervals */
static gpointer th_space_thread (gpointer gp)
{
    for (;;)
    {
	usleep (SPACE_TIMEOUT*1000);
	if (!space_uptodate)   th_space_update ();
    }
}


/* in Bytes */
static gdouble get_ipod_free_space(void)
{
    gdouble result;
    g_mutex_lock (space_mutex);
    result = space_ipod_free;
    g_mutex_unlock (space_mutex);
    return result;
}

#if 0
/* in Bytes */
static gdouble get_ipod_used_space(void)
{
    gdouble result;
    g_mutex_lock (space_mutex);
    result = space_ipod_used;
    g_mutex_unlock (space_mutex);
    return result;
}
#endif


/* @size: size in B */
gchar*
get_filesize_as_string(gdouble size)
{
    guint i = 0;
    gchar *result = NULL;
    gchar *sizes[] = { _("B"), _("kB"), _("MB"), _("GB"), _("TB"), NULL };

    while((fabs(size) > 1000) && (i<4))
    {
	size /= 1000;
	++i;
    }
    if (i>0)
    {
	if (fabs(size) < 10)
	    result = g_strdup_printf("%0.2f %s", size, sizes[i]);
	else if (fabs(size) < 100)
	    result = g_strdup_printf("%0.1f %s", size, sizes[i]);
	else
	    result = g_strdup_printf("%0.0f %s", size, sizes[i]);
    }
    else
    {   /* Bytes do not have decimal places */
	result = g_strdup_printf ("%0.0f %s", size, sizes[i]);
    }
    return result;
}

static guint
gtkpod_space_statusbar_update(void)
{
    if(gtkpod_space_statusbar)
    {
	gchar *buf = NULL;
	gchar *str = NULL;

	if (!prefs_get_offline ())
	{
	    if (ipod_connected ())
	    {
		gdouble left, pending;

		left = get_ipod_free_space() + 
		    get_filesize_of_deleted_tracks (NULL);
		pending = get_filesize_of_nontransferred_tracks(NULL);
		if((left-pending) > 0)
		{
		    str = get_filesize_as_string(left - pending);
		    buf = g_strdup_printf (_(" %s Free"), str);
		}
		else
		{
		    str = get_filesize_as_string(pending - left);
		    buf = g_strdup_printf (_(" %s Pending"), str);
		}
	    }
	    else
	    {
		buf = g_strdup (_(" disconnected"));
	    }
	}
	else
	{
	    buf = g_strdup (_("offline"));
	}
	gtk_statusbar_pop(GTK_STATUSBAR(gtkpod_space_statusbar), 1);
	gtk_statusbar_push(GTK_STATUSBAR(gtkpod_space_statusbar), 1,  buf);
	g_free (buf);
	g_free (str);
    }
    info_update_totals_view_space ();
    return(TRUE);
}

void
gtkpod_space_statusbar_init(void)
{
    gtkpod_space_statusbar = lookup_widget (gtkpod_window, "space_statusbar");

    if (!space_mutex)
    {
	space_mutex = g_mutex_new ();
	if (!space_mp)
	    space_mp = g_strdup (prefs_get_ipod_mount ());
	th_space_update ();  /* make sure we have current data */
	space_thread = g_thread_create (th_space_thread, NULL, FALSE, NULL);
    }
    gtkpod_space_statusbar_update();
    gtk_timeout_add(1000, (GtkFunction) gtkpod_space_statusbar_update, NULL);
}
