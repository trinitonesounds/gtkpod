/* Time-stamp: <2003-11-25 23:05:59 jcs>
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
#include "song.h"
#include "support.h"

/* pointer to info window */
static GtkWidget *info_window = NULL;

/* structure to transfer info about track view */
struct info_track_view {
    guint32 tracks_total, tracks_selected;
    guint32 playtime_total, playtime_selected;  /* in seconds */
    double  filesize_total, filesize_selected;  /* in bytes   */
};

/* structure to transfer info about track view */
struct info_playlist_view {
    guint32 tracks;
    guint32 playtime; /* in seconds */
    double  filesize; /* in bytes   */
};

/* structure to transfer info about the "totals" section */
struct info_totals_view {
    guint32 tracks;
    guint32 playtime;    /* in seconds */
    double  filesize;    /* in bytes   */
    guint32 nt_tracks;
    double  nt_filesize; /* in bytes   */
    double  free_space;  /* in bytes   */
};

/* static function declarations */
static void info_get_track_view_info_total (struct info_track_view *tv);
static void info_get_track_view_info_selected (struct info_track_view *tv);
static void info_get_playlist_view_info (struct info_playlist_view *pv);
static void info_get_totals_view_info (struct info_totals_view *tv);
static void info_get_totals_view_info_space (struct info_totals_view *tv);

/* stuff for statusbar */
static GtkWidget *gtkpod_statusbar = NULL;
static GtkWidget *gtkpod_tracks_statusbar = NULL;
static GtkWidget *gtkpod_space_statusbar = NULL;
static guint statusbar_timeout_id = 0;

static double get_ipod_free_space(void);
#if 0
static glong get_ipod_used_space(void);
#endif


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
	prefs_set_info_window (TRUE); /* notify prefs */
	info_update ();
	gtk_widget_show (info_window);
    }
}

/* close info window */
void info_close_window (void)
{
    if (!info_window) return; /* not open */
    gtk_widget_destroy (info_window);
    info_window = NULL;
    prefs_set_info_window (FALSE); /* notify prefs */
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
    struct info_track_view *tv;

    if (!info_window) return; /* not open */
    tv = g_malloc0 (sizeof (struct info_track_view));
    info_get_track_view_info_total (tv);
    fill_label_uint ("tracks_total", tv->tracks_total);
    fill_label_time ("playtime_total", tv->playtime_total);
    fill_label_size ("filesize_total", tv->filesize_total);
    g_free (tv);
}

void info_update_track_view_selected (void)
{
    struct info_track_view *tv;

    if (!info_window) return; /* not open */
    tv = g_malloc0 (sizeof (struct info_track_view));
    info_get_track_view_info_selected (tv);
    fill_label_uint ("tracks_selected", tv->tracks_selected);
    fill_label_time ("playtime_selected", tv->playtime_selected);
    fill_label_size ("filesize_selected", tv->filesize_selected);
    g_free (tv);
}

/* update track view section */
void info_update_track_view (void)
{
    info_update_track_view_total ();
    info_update_track_view_selected ();
}

/* update playlist view section */
void info_update_playlist_view (void)
{
    struct info_playlist_view *pv;

    if (!info_window) return; /* not open */
    pv = g_malloc0 (sizeof (struct info_playlist_view));
    info_get_playlist_view_info (pv);
    fill_label_uint ("playlist_tracks", pv->tracks);
    fill_label_time ("playlist_playtime", pv->playtime);
    fill_label_size ("playlist_filesize", pv->filesize);
    g_free (pv);
}

/* update "totals" view section */
void info_update_totals_view (void)
{
    struct info_totals_view *tv;

    if (!info_window) return; /* not open */
    tv = g_malloc0 (sizeof (struct info_totals_view));
    info_get_totals_view_info (tv);
    fill_label_uint ("total_tracks", tv->tracks);
    fill_label_time ("total_playtime", tv->playtime);
    fill_label_size ("total_filesize", tv->filesize);
    fill_label_uint ("non_transfered_tracks", tv->nt_tracks);
    fill_label_size ("non_transfered_filesize", tv->nt_filesize);
    fill_label_size ("free_space", tv->free_space);
    g_free (tv);
}

/* update "free space" section of totals view */
void info_update_totals_view_space (void)
{
    struct info_totals_view *tv;

    if (!info_window) return; /* not open */
    tv = g_malloc0 (sizeof (struct info_totals_view));
    info_get_totals_view_info_space (tv);
    fill_label_uint ("non_transfered_tracks", tv->nt_tracks);
    fill_label_size ("non_transfered_filesize", tv->nt_filesize);
    fill_label_size ("free_space", tv->free_space);
    g_free (tv);
}


/* ------------------------------------------------------------

           Functions for retrieving statistics 

   ------------------------------------------------------------ */

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

/* fill in track_view_info structure (total part) */
static void info_get_track_view_info_total (struct info_track_view *tv)
{
    GList *displayed;

    if (!tv)  return;
    displayed = display_get_selection (prefs_get_sort_tab_num () - 1);
    fill_in_info (displayed, &tv->tracks_total,
		  &tv->playtime_total, &tv->filesize_total);
    g_list_free (displayed);
}

/* fill in track_view_info structure (total part) */
static void info_get_track_view_info_selected (struct info_track_view *tv)
{
    GList *selected;

    if (!tv)  return;
    selected = display_get_selection (prefs_get_sort_tab_num ());

    fill_in_info (selected, &tv->tracks_selected,
		  &tv->playtime_selected, &tv->filesize_selected);
    g_list_free (selected);
}

#if 0
/* not used */
/* fill in track_view_info structure */
static void info_get_track_view_info (struct info_track_view *tv)
{
    if (!tv)  return;

    info_get_track_view_info_total (tv);
    info_get_track_view_info_selected (tv);
}
#endif

static void info_get_playlist_view_info (struct info_playlist_view *pv)
{
    GList *tl;
    if (!pv)  return;
    tl = display_get_selection (-1);
    fill_in_info (tl, &pv->tracks, &pv->playtime, &pv->filesize);
    g_list_free (tl);
}

static void info_get_totals_view_info (struct info_totals_view *tv)
{
    GList *tl;
    Playlist *pl;
    if (!tv)  return;
    pl = get_playlist_by_nr (0);
    if (!pl)  return;
    tl = pl->members;
    fill_in_info (tl, &tv->tracks, &tv->playtime, &tv->filesize);
    info_get_totals_view_info_space (tv);
}

static void info_get_totals_view_info_space (struct info_totals_view *tv)
{
    if (!tv)  return;
    tv->nt_filesize = get_filesize_of_nontransferred_tracks (&tv->nt_tracks);
    tv->free_space = get_ipod_free_space () +
	             get_filesize_of_deleted_tracks ();
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
    gchar *line = NULL;
    gchar **tokens = NULL;
    gchar *mp = prefs_get_ipod_mount ();
    
    if((line = get_drive_stats_from_df(mp)))
    {
	if((tokens = g_strsplit(line, " ", 5)))
	{
	    if(tokens[0] && tokens[1] && tokens[2]) 
		result = atol(tokens[2]);
	    g_strfreev(tokens);
	}
    }
    g_free(line);
    g_free (mp);
    return(result);
}
#endif

/* get free space in Bytes */
static double
get_ipod_free_space(void)
{
    double result = 0;
    gchar *line = NULL;
    gchar **tokens = NULL;
    gchar *mp = prefs_get_ipod_mount ();

    if((line = get_drive_stats_from_df(mp)))
    {
	if((tokens = g_strsplit(line, " ", 5)))
	{
	    if(tokens[0] && tokens[1] && tokens[2] && tokens[3]) 
		result = strtod(tokens[3], NULL) * 1024;
	    g_strfreev(tokens);
	}
    }
    g_free(line);
    g_free(mp);
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

	left = get_ipod_free_space() + get_filesize_of_deleted_tracks ();
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
