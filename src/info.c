/* Time-stamp: <2003-11-24 23:25:12 jcs>
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

/* This file provides functions for the info window */

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
static void info_get_track_view_info (struct info_track_view *tv);
static void info_get_playlist_view_info (struct info_playlist_view *pv);
static void info_get_totals_view_info (struct info_totals_view *tv);


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
    if (info_window)  return;  /* already open */
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

/* update track view section */
void info_update_track_view (void)
{
    struct info_track_view *tv;

    if (!info_window) return; /* not open */
    tv = g_malloc0 (sizeof (struct info_track_view));
    info_get_track_view_info (tv);
    fill_label_uint ("tracks_total", tv->tracks_total);
    fill_label_uint ("tracks_selected", tv->tracks_selected);
    fill_label_time ("playtime_total", tv->playtime_total);
    fill_label_time ("playtime_selected", tv->playtime_selected);
    fill_label_size ("filesize_total", tv->filesize_total);
    fill_label_size ("filesize_selected", tv->filesize_selected);
    g_free (tv);
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

/* fill in track_view_info structure */
static void info_get_track_view_info (struct info_track_view *tv)
{
    GList *displayed;
    GList *selected;

    if (!tv)  return;
    displayed = display_get_selection (prefs_get_sort_tab_num () - 1);
    selected = display_get_selection (prefs_get_sort_tab_num ());

    fill_in_info (displayed, &tv->tracks_total,
		  &tv->playtime_total, &tv->filesize_total);
    fill_in_info (selected, &tv->tracks_selected,
		  &tv->playtime_selected, &tv->filesize_selected);
    g_list_free (displayed);
    g_list_free (selected);
}

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
}
