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

/* This file provides functions for the info window as well as for the
 * statusbar handling */

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include "info.h"
#include "misc.h"
#include "misc_track.h"
#include "prefs.h"

/* pointer to info window */
static GtkWidget *info_window = NULL;

GladeXML *info_xml; 

/* stuff for statusbar */
static GtkWidget *gtkpod_statusbar = NULL;
static GtkWidget *gtkpod_tracks_statusbar = NULL;
static GtkWidget *gtkpod_space_statusbar = NULL;
static guint statusbar_timeout_id = 0;
static guint statusbar_timeout = STATUSBAR_TIMEOUT;

#define SPACE_TIMEOUT 1000
/* lock for size related variables (used by child and parent) */
static GMutex *space_mutex = NULL;
static GThread *space_thread = NULL;
static gboolean space_uptodate = FALSE;
static gchar *space_mp = NULL;      /* thread save access through mutex */
static iTunesDB *space_itdb = NULL; /* semi thread save access
				     * (space_itdb will not be changed
				     * while locked) */
static gdouble space_ipod_free = 0; /* thread save access through mutex */
static gdouble space_ipod_used = 0; /* thread save access through mutex */


static gdouble get_ipod_free_space(void);
#if 0
static gdouble get_ipod_used_space(void);
#endif

/* fill in tracks, playtime and filesize from track list @tl */
static void fill_in_info (GList *tl, guint32 *tracks,
			  guint32 *playtime, gdouble *filesize)
{
    GList *gl;

    g_return_if_fail (tracks);
    g_return_if_fail (playtime);
    g_return_if_fail (filesize);

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
    GtkWidget *w;

    g_return_if_fail (info_window);
    g_return_if_fail (w_name);
    w = gtkpod_xml_get_widget (info_xml, w_name);
    if (w)
    {
	gchar *str = g_strdup_printf ("%u", nr);
	gtk_label_set_text (GTK_LABEL (w), str);
	g_free (str);
    }
}

static void fill_label_time (gchar *w_name, guint32 secs)
{    
    GtkWidget *w;

    g_return_if_fail (info_window);
    g_return_if_fail (w_name);
    w = gtkpod_xml_get_widget (info_xml, w_name);
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
    GtkWidget *w;

    g_return_if_fail (info_window);
    g_return_if_fail (w_name);
    w = gtkpod_xml_get_widget (info_xml, w_name);
    if (w)
    {
	gchar *str = get_filesize_as_string (size);
	gtk_label_set_text (GTK_LABEL (w), str);
	g_free (str);
    }
}

static void fill_label_string (gchar *w_name, const char *str)
{    
    GtkWidget *w;

    g_return_if_fail (info_window);
    g_return_if_fail (w_name);
    w = gtkpod_xml_get_widget (info_xml, w_name);
    if (w)
    {
	gtk_label_set_text (GTK_LABEL (w), str);
    }
}



/* open info window */
void info_open_window (void)
{
    if (info_window)
    {   /* info window already open -- raise to the top */
	gdk_window_raise (info_window->window);
	return;
    }
    
    info_xml = glade_xml_new (xml_file, "gtkpod_info", NULL);
    glade_xml_signal_autoconnect (info_xml);
    info_window = gtkpod_xml_get_widget (info_xml, "gtkpod_info");
    
    if (info_window)
    {
	gint defx, defy;
	defx = prefs_get_int("size_info.x");
  defy = prefs_get_int("size_info.y");
	gtk_window_set_default_size (GTK_WINDOW (info_window), defx, defy);
	prefs_set_int("info_window", TRUE); /* notify prefs */
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
     prefs_set_int("info_window", FALSE); /* notify prefs */
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
	prefs_set_int("size_info.x", defx);
	prefs_set_int("size_info.y", defy);
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

static void info_update_track_view_displayed (void)
{
    guint32 tracks, playtime; /* playtime in secs */
    gdouble  filesize;        /* in bytes */
    GList *displayed;

    g_return_if_fail (info_window);
    displayed = display_get_selected_members (prefs_get_int("sort_tab_num")-1);
    fill_in_info (displayed, &tracks, &playtime, &filesize);
    fill_label_uint ("tracks_displayed", tracks);
    fill_label_time ("playtime_displayed", playtime);
    fill_label_size ("filesize_displayed", filesize);
}

void info_update_track_view_selected (void)
{
    guint32 tracks, playtime; /* playtime in secs */
    gdouble  filesize;        /* in bytes */
    GList *selected;

    if (!info_window) return; /* not open */
    selected = display_get_selection (prefs_get_int("sort_tab_num"));
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
    info_update_track_view_displayed ();
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


/* Get the iPod itdb */
/* FIXME: This function must be expanded if support for several iPods
   is implemented */
static iTunesDB *get_itdb_ipod (void)
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
	if (itdb->usertype & GP_ITDB_TYPE_IPOD)
	    return itdb;
    }
    return NULL;
}


/* update "totals" view section */
void info_update_totals_view (void)
{
    guint32 tracks=0, playtime=0; /* playtime in secs */
    gdouble  filesize=0;          /* in bytes */
    Playlist *pl;
    iTunesDB *itdb;

    if (!info_window) return; /* not open */
    
    itdb = get_itdb_ipod ();
    if (itdb)
    {
	pl = itdb_playlist_mpl (itdb);
	g_return_if_fail (pl);
	fill_in_info (pl->members, &tracks, &playtime, &filesize);
	fill_label_uint ("total_playlists_ipod",
			 itdb_playlists_number (itdb)-1);
	fill_label_uint ("total_tracks_ipod", tracks);
	fill_label_time ("total_playtime_ipod", playtime);
	fill_label_size ("total_filesize_ipod", filesize);
    }
    itdb = get_itdb_local ();
    if (itdb)
    {
	pl = itdb_playlist_mpl (itdb);
	g_return_if_fail (pl);
	fill_in_info (pl->members, &tracks, &playtime, &filesize);
	fill_label_uint ("total_playlists_local",
			 itdb_playlists_number (itdb)-1);
	fill_label_uint ("total_tracks_local", tracks);
	fill_label_time ("total_playtime_local", playtime);
	fill_label_size ("total_filesize_local", filesize);
    }
    info_update_totals_view_space ();
}

/* update "free space" section of totals view */
void info_update_totals_view_space (void)
{
    gdouble nt_filesize, del_filesize;
    guint32 nt_tracks, del_tracks;
    iTunesDB *itdb;

    if (!info_window) return;
    itdb = get_itdb_ipod ();
    if (itdb)
    {
	gp_info_nontransferred_tracks (itdb, &nt_filesize, &nt_tracks);
	fill_label_uint ("non_transferred_tracks", nt_tracks);
	fill_label_size ("non_transferred_filesize", nt_filesize);
	gp_info_deleted_tracks (itdb, &del_filesize, &del_tracks);
	fill_label_uint ("deleted_tracks", del_tracks);
	fill_label_size ("deleted_filesize", del_filesize);
	if (!get_offline (itdb))
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
}


/*------------------------------------------------------------------*\
 *                                                                  *
 *                   Functions for Statusbar                        *
 *                                                                  *
\*------------------------------------------------------------------*/

void
gtkpod_statusbar_init(void)
{
    gtkpod_statusbar = gtkpod_xml_get_widget (main_window_xml, "gtkpod_status");
    statusbar_timeout = STATUSBAR_TIMEOUT;
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


static void
gtkpod_statusbar_reset_timeout (void)
{
    if (statusbar_timeout_id != 0) /* remove last timeout, if still present */
	gtk_timeout_remove (statusbar_timeout_id);
    statusbar_timeout_id = gtk_timeout_add (statusbar_timeout,
					    (GtkFunction) gtkpod_statusbar_clear,
					    NULL);
}

void
gtkpod_statusbar_timeout (guint timeout)
{
    if (timeout == 0)
	statusbar_timeout = STATUSBAR_TIMEOUT;
    else
	statusbar_timeout = timeout;

    gtkpod_statusbar_reset_timeout ();
}


void
gtkpod_statusbar_message(const gchar *message, ...)
{
    if(gtkpod_statusbar)
    {
	va_list arg;
	gchar *text;
	guint context = 1;

	va_start (arg, message);
	text = g_strdup_vprintf (message, arg);
	va_end (arg);

	gtk_statusbar_pop(GTK_STATUSBAR(gtkpod_statusbar), context);
	gtk_statusbar_push(GTK_STATUSBAR(gtkpod_statusbar), context,  text);
	gtkpod_statusbar_reset_timeout ();
    }
}

void
gtkpod_tracks_statusbar_init()
{
    gtkpod_tracks_statusbar =
	gtkpod_xml_get_widget (main_window_xml, "tracks_statusbar");
    gtkpod_tracks_statusbar_update();
}

void 
gtkpod_tracks_statusbar_update(void)
{
    if(gtkpod_tracks_statusbar)
    {
	gchar *buf;
	Playlist *pl;
	pl = pm_get_selected_playlist ();
	/* select of which iTunesDB data should be displayed */
	if (pl)
	{
	    iTunesDB *itdb = pl->itdb;
	    g_return_if_fail (itdb);

	    buf = g_strdup_printf (_(" P:%d T:%d/%d"),
				   itdb_playlists_number (itdb) - 1,
				   tm_get_nr_of_tracks (),
				   itdb_tracks_number (itdb));
	}
	else
	{
	    buf = g_strdup ("");
	}
	/* gets called before itdbs are setup up -> fail silently */
/*	g_return_if_fail (itdb);*/
	
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
void space_set_ipod_itdb (iTunesDB *itdb)
{
    const gchar *mp = NULL;

    if (itdb)
    {
	ExtraiTunesDBData *eitdb = itdb->userdata;
	g_return_if_fail (eitdb);

	if (!eitdb->ipod_ejected)
	{
	    mp = itdb_get_mountpoint (itdb);
	}
    }

    if (space_mutex)  g_mutex_lock (space_mutex);

    space_itdb = itdb;

    /* update the free space data if mount point changed */
    if (!space_mp || !mp || (strcmp (space_mp, mp) != 0))
    {
	g_free (space_mp);
	space_mp = g_strdup (mp);

	space_data_update ();
    }

    if (space_mutex)   g_mutex_unlock (space_mutex);

}

/* retrieve the currently set ipod itdb -- needed in case the itdb is
   deleted */
iTunesDB *space_get_ipod_itdb (void)
{
    return space_itdb;
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



/* we'll use statvfs to determine free space on the iPod where
   available, df otherwise */
#ifdef HAVE_STATVFS
#include <sys/types.h>
#include <sys/statvfs.h>
/* update space_ipod_free and space_ipod_used */
static void th_space_update (void)
{
    gchar *mp=NULL;
    struct statvfs stat;
    int	status;

    g_mutex_lock (space_mutex);

    /* don't read info when in offline mode */
    if (space_itdb && !get_offline (space_itdb))
    {
	mp = g_strdup (space_mp);
    }
    if (mp)
    {
	status = statvfs (mp, &stat);
	if (status != 0) {
	    /* XXX: why would this fail - what to do here??? */
	    goto done;
	}
	space_ipod_free = (gdouble)stat.f_bavail * stat.f_frsize;
	space_ipod_used = ((gdouble)stat.f_blocks * stat.f_frsize) -
	    space_ipod_free;
	space_uptodate = TRUE;
	
    } else { /* mp == NULL */
 
	/* this is set even if offline mode */
	space_ipod_free = 0;
	space_ipod_used = 0;
	space_uptodate = FALSE;  /* this way we will detect when the
				    iPod is connected */
    }

done:  
    g_mutex_unlock (space_mutex);
    g_free (mp);
}

#else
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
	gchar *df_str = getenv ("GTKPOD_DF_COMMAND");
	if (df_str == NULL) df_str = "df -k -P";
	if (strlen (df_str))
	{
	    snprintf(bufc, PATH_MAX, "%s \"%s\"", df_str, mp);
	    fp = popen(bufc, "r");
	    if(fp)
	    {
		if((bytes_read = fread(buf, 1, PATH_MAX, fp)) > 0)
		{
		    if((bufp = strchr (buf, '\n')))
		    {
			int i = 0;
			int j = 0;
			gchar buf2[PATH_MAX+3];

			++bufp; /* skip '\n' */
			while((bufp - buf + i < bytes_read) &&
			      (j < PATH_MAX))
			{
			    while(!g_ascii_isspace(bufp[i]) &&
				  (j<PATH_MAX))
			    {
				buf2[j++] = bufp[i++];
			    }
			    buf2[j++] = ' ';
			    while((bufp - buf + i < bytes_read) &&
				  g_ascii_isspace(bufp[i]))
			    {
				i++;
			    }
			}
			buf2[j] = '\0';
			result = g_strdup_printf("%s", buf2);
		    }
		}
		pclose(fp);	
	    }
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
    gchar *mp=NULL, *line=NULL;
    gchar **tokens = NULL;

    /* don't read info when in offline mode */
    if (!prefs_get_offline ())
    {
	g_mutex_lock (space_mutex);
	mp = g_strdup (space_mp);
	g_mutex_unlock (space_mutex);

	line = get_drive_stats_from_df (mp);
    }

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
	/* this is set even if offline mode */
	space_ipod_free = 0;
	space_ipod_used = 0;
	space_uptodate = FALSE;  /* this way we will detect when the
				    iPod is connected */
    }
    g_mutex_unlock (space_mutex);
    g_free (mp);
    g_strfreev(tokens);
}
#endif


/* keep space_ipod_free/used updated in regular intervals */
static gpointer th_space_thread (gpointer gp)
{
    struct timespec req;

    req.tv_sec = SPACE_TIMEOUT / 1000;
    req.tv_nsec = (SPACE_TIMEOUT % 1000) * 1000000;

    for (;;)
    {
	nanosleep (&req, NULL);
	if (!space_uptodate)   th_space_update ();
    }
    /* To make gcc happy (never reached) */
    return (gpointer)NULL;
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

    while((fabs(size) > 1024) && (i<4))
    {
	size /= 1024;
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
    if(space_itdb && gtkpod_space_statusbar)
    {
	gchar *buf = NULL;
	gchar *str = NULL;

	if (!get_offline (space_itdb))
	{
	    if (ipod_connected ())
	    {
		gdouble left, pending, deleted;

		gp_info_deleted_tracks (space_itdb, &deleted, NULL);
		gp_info_nontransferred_tracks (space_itdb, &pending, NULL);
		left = get_ipod_free_space() + deleted;
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
    return TRUE;
}

void
gtkpod_space_statusbar_init(void)
{
    gtkpod_space_statusbar = gtkpod_xml_get_widget (main_window_xml, "space_statusbar");

    if (!space_mutex)
    {
	space_mutex = g_mutex_new ();
	if (!space_mp)
	{
	    iTunesDB *itdb = gp_get_ipod_itdb ();

	    if (itdb)
	    {
		space_mp = get_itdb_prefs_string (itdb, KEY_MOUNTPOINT);
		th_space_update ();  /* make sure we have current data */
	    }
	}
	space_thread = g_thread_create (th_space_thread,
					    NULL, FALSE, NULL);
    }
    gtkpod_space_statusbar_update();
    gtk_timeout_add(1000, (GtkFunction) gtkpod_space_statusbar_update, NULL);
}



/*------------------------------------------------------------------*\
 *                                                                  *
 *              Frequently used error messages                      *
 *                                                                  *
\*------------------------------------------------------------------*/

void message_sb_no_itdb_selected ()
{
    gtkpod_statusbar_message (_("No database or playlist selected"));
}

void message_sb_no_tracks_selected ()
{
    gtkpod_statusbar_message (_("No tracks selected"));
}

void message_sb_no_playlist_selected ()
{
    gtkpod_statusbar_message (_("No playlist selected"));
}

void message_sb_no_ipod_itdb_selected ()
{
    gtkpod_statusbar_message (_("No iPod or iPod playlist selected"));
}

