/* Time-stamp: <2004-02-04 21:19:19 JST jcs>
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

static double get_ipod_free_space(void);
#if 0
static glong get_ipod_used_space(void);
#endif


/* fill in tracks, playtime and filesize from track list @tl */
static void fill_in_info (GList *tl, guint32 *tracks,
			  guint32 *playtime, double *filesize)
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

static void fill_label_size (gchar *w_name, double size)
{    
    GtkWidget *w = lookup_widget (info_window, w_name);
    if (w)
    {
	gchar *str = get_filesize_as_string (size);
	gtk_label_set_text (GTK_LABEL (w), str);
	g_free (str);
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
    if (!info_window) return; /* not open */
    info_update_default_sizes ();
    gtk_widget_destroy (info_window);
    info_window = NULL;
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
    double  filesize;         /* in bytes */
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
    double  filesize;         /* in bytes */
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
    double  filesize;         /* in bytes */
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
    double  filesize=0;           /* in bytes */
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
    double nt_filesize, del_filesize;
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
	double free_space = get_ipod_free_space() + del_filesize - nt_filesize;
	fill_label_size ("free_space", free_space);
    }
    else
    {
	GtkWidget *w = lookup_widget (info_window, "free_space");
	if (w)
	{
	    gtk_label_set_text (GTK_LABEL (w), _("offline"));
	}
    }
}


/*------------------------------------------------------------------*\
 *                                                                  *
 *                   Functions for Statusbar                        *
 *                                                                  *
\*------------------------------------------------------------------*/

void
gtkpod_statusbar_init(GtkWidget *sb)
{
    gtkpod_statusbar = sb;
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
gtkpod_tracks_statusbar_init(GtkWidget *w)
{
    gtkpod_tracks_statusbar = w;
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


static gchar*
get_drive_stats_from_df(const gchar *mp)
{
    FILE *fp;
    gchar buf[PATH_MAX];
    gchar *result = NULL;
    guint bytes_read = 0;

    snprintf(buf, PATH_MAX, "df -k -P | grep %s", mp);
    if((fp = popen(buf, "r")))
    {
	if((bytes_read = fread(buf, 1, PATH_MAX, fp)) > 0)
	{
	    if(g_strstr_len(buf, PATH_MAX, mp))
	    {
		int i = 0;
		int j = 0;
		gchar buf2[PATH_MAX];
		
		while(i < bytes_read)
		{
		    while(!g_ascii_isspace(buf[i]))
			buf2[j++] = buf[i++];
		    buf2[j++] = ' ';
		    while(g_ascii_isspace(buf[i]))
			i++;
		}
		buf2[j] = '\0';
		result = g_strdup_printf("%s", buf2);
	    }
	}
	pclose(fp);	
    }
    return(result);
}

/* @size: size in kB (block of 1024 Bytes) */
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

#if 0
static glong
get_ipod_used_space(void)
{
    glong result = 0;
    gchar **tokens = NULL;
    gchar *line = get_drive_stats_from_df(prefs_get_ipod_mount ());
    
    if(line)
    {
	if((tokens = g_strsplit(line, " ", 5)))
	{
	    if(tokens[0] && tokens[1] && tokens[2]) 
		result = atol(tokens[2]);
	    g_strfreev(tokens);
	}
    }
    g_free(line);
    return(result);
}
#endif

/* get free space in Bytes */
static double
get_ipod_free_space(void)
{
    double result = 0;
    gchar **tokens = NULL;
    gchar *line = get_drive_stats_from_df(prefs_get_ipod_mount ());

    if(line)
    {
	if((tokens = g_strsplit(line, " ", 5)))
	{
	    if(tokens[0] && tokens[1] && tokens[2] && tokens[3]) 
		result = strtod(tokens[3], NULL) * 1024;
	    g_strfreev(tokens);
	}
    }
    g_free(line);
    return(result);
}

static guint
gtkpod_space_statusbar_update(void)
{
    if(gtkpod_space_statusbar)
    {
	gchar *buf = NULL;
	gchar *str = NULL;
	double left, pending;

	left = get_ipod_free_space() + get_filesize_of_deleted_tracks (NULL);
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
	gtk_statusbar_pop(GTK_STATUSBAR(gtkpod_space_statusbar), 1);
	gtk_statusbar_push(GTK_STATUSBAR(gtkpod_space_statusbar), 1,  buf);
	g_free (buf);
	g_free (str);
    }
    info_update_totals_view_space ();
    return(TRUE);
}

void
gtkpod_space_statusbar_init(GtkWidget *w)
{
    gtkpod_space_statusbar = w;

    gtkpod_space_statusbar_update();
    gtk_timeout_add(5000, (GtkFunction) gtkpod_space_statusbar_update, NULL);
}
