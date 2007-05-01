/*
|  File conversion started by Simon Naunton <snaunton gmail.com> in 2007
|
|  Copyright (C) 2002-2007 Jorg Schuler <jcsjcs at users.sourceforge.net>
|  Part of the gtkpod project.
|
|  URL: http://gtkpod.sourceforge.net/
|  URL: http://www.gtkpod.org
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
#   include <config.h>
#endif

#include <errno.h>
#include <glib/gstdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "prefs.h"
#include "misc.h"
#include "misc_track.h"
#include "file_convert.h"
#include "display_itdb.h"

#define DEBUG_CONV
#ifdef DEBUG_CONV
#   define _TO_STR(x) #x
#   define TO_STR(x) _TO_STR(x)
#   define debug(...) do { fprintf(stderr,  __FILE__ ":" TO_STR(__LINE__) ":" __VA_ARGS__); } while(0)
#else
#   define debug(...)
#endif

#define conv_error(...) g_error_new(G_FILE_ERROR, 0, __VA_ARGS__)


/* ----------------------------------------------------------------
 *
 * Functions for threaded background conversion
 *
 * ---------------------------------------------------------------- */

/* How does it work?

   If a track is added to an iTunesDB with gp_track_add(), it is
   passed on file_convert_add_track().

   This function determines if conversion is needed for the track and
   then places the track either into the "scheduled" or "finished"
   lists. If conversion is needed because the type of track is not
   supported directly by the iPod, the track is added to the "failed"
   list.

   A timeout function examines the "scheduled" list and starts new
   conversion threads as long as the maximum number of allowed threads
   hasn't been reached and there are still tracks in the "scheduled"
   list.

   IO-Output of tracks in the processing, failed, and converted list
   is redirected to a multi-thread log window. Once a track appears in
   the "finished" or "failed" lists, the redirection IO-channel will
   be closed.

   The conversion threads continue processing tracks in the
   "scheduled" list as long as there are tracks left in the
   "scheduled" list and the maximum number of threads is not
   exceeded. If either is condition is not met, the thread will
   terminate after processing of the current track has
   finished. Tracks being processed are moved to the "processing"
   list. If the conversion was finished successfully, they are moved
   to the "converted" list, in case of failure to the "failed" list.

   Tracks that are removed from an iTunesDB with gp_track_remove() are
   propagated to file_convert_cancel_track() and flagged "invalid" in
   all lists. If currently being processed, the conversion process is
   kill()ed.

   If a whole iTunesDB is removed from the system, the event is
   propagated to file_convert_cancel_itdb() and all tracks in that
   iTunesDB are treated as explained for file_convert_cancel_track()
   above.

   Conversion of tracks is done in a FIFO fashion. Preference can be
   given to a specific iTunesDB with file_convert_itdb_first(), which
   should be called when the user wants write changes to an iPod or
   eject an iPod, so that conversion of the tracks needed next are
   processed next.
*/


/* Preferences keys */
const gchar *FILE_CONVERT_CACHEDIR = "file_convert_cachedir";
const gchar *FILE_CONVERT_MAXDIRSIZE = "file_convert_maxdirsize";
const gchar *FILE_CONVERT_TEMPLATE = "file_convert_template";
const gchar *FILE_CONVERT_MAX_THREADS_NUM = "file_convert_max_threads_num";
const gchar *FILE_CONVERT_DISPLAY_LOG = "file_convert_display_log";
const gchar *FILE_CONVERT_LOG_SIZE_X = "file_convert_log_size.x";
const gchar *FILE_CONVERT_LOG_SIZE_Y = "file_convert_log_size.y";

typedef struct _Conversion Conversion;
typedef struct _ConvTrack ConvTrack;

static gboolean conversion_scheduler (gpointer data);
static void conversion_update_default_sizes (Conversion *conv);
static gboolean conversion_log_window_delete (Conversion *conv);
static gpointer conversion_thread (gpointer data);
static gpointer conversion_update_dirsize (gpointer data);
static gpointer conversion_prune_dir (gpointer data);
static void conversion_convtrack_free (ConvTrack *ctr);
static gchar *conversion_get_fname_extension (Conversion *conv, ConvTrack *ctr);
static gboolean conversion_setup_cachedir (Conversion *conv);
static void conversion_log_add_pages (Conversion *conv, gint threads);
static gboolean conversion_add_track (Conversion *conv, Track *track);
static void conversion_prefs_changed (Conversion *conv);
static void conversion_itdb_first (Conversion *conv, iTunesDB *itdb);
static void conversion_cancel_itdb (Conversion *conv, iTunesDB *itdb);
static void conversion_cancel_track (Conversion *conv, Track *track);
static Track *conversion_timed_wait (Conversion *conv, iTunesDB *itdb, gint ms);


struct _Conversion
{
    GMutex *mutex;          /* shared lock                              */
    GList  *scheduled;      /* tracks scheduled for conversion          */
    GList  *processing;     /* tracks currently being converted         */
    GList  *failed;         /* tracks with failed conversion            */
    GList  *converted;      /* tracks successfully converted but not
			       yet unscheduled                          */
    GList  *finished;       /* tracks unscheduled but not yet
			       transferred */
    GCond  *finished_cond;  /* signals if a new track is added to the
			       finished list                            */
    gchar  *cachedir;       /* directory for converted files            */
    gchar  *template;       /* name template to use for converted files */
    gint   max_threads_num; /* maximum number of allowed threads        */
    GList  *threads;        /* list of threads                          */
    gint   threads_num;     /* number of threads currently running      */
    gboolean conversion_force;    /* force a new thread to start even if
				     the dirsize is too large           */
    gint64 max_dirsize;     /* maximum size of cache directory in bytes */
    gint64  dirsize;        /* current size of cache directory in bytes */
    gboolean dirsize_in_progress; /* currently determining dirsize      */
    GCond  *dirsize_cond;   /* signal when dirsize has been updated     */
    gboolean prune_in_progress;   /* currently pruning directory        */
    GCond  *prune_cond;     /* signal when dir has been pruned          */
    guint  timeout_id;
    /* data for log display */
    GtkWidget *log_window;  /* display log window                       */
    gboolean log_window_hidden;   /* whether the window was closed      */
    gboolean log_window_shown;    /* whether the window was closed      */
    gint log_window_posx;   /* last x-position of log window            */
    gint log_window_posy;   /* last x-position of log window            */
    GtkWidget *notebook;    /* notebook                                 */
    GList *textviews;       /* list with pages currently added          */
    GList *pages;           /* list with pages currently added          */
    GtkStatusbar *log_statusbar;  /* statusbar of log display           */
    guint log_context_id;   /* context ID for statusbar                 */
};

struct _ConvTrack
{
    gboolean valid;         /* TRUE if orig_track is valid.             */
    gchar *orig_file;       /* original filename of unconverted track   */
    gchar *converted_file;  /* filename of converted track              */
    gint32 converted_size;  /* size of converted file                   */
    gchar *conversion_cmd;  /* command to be used for conversion        */
    gboolean must_convert;  /* is conversion required for the iPod?     */
    gchar *errormessage;    /* error message if any                     */
    gchar *fname_root;      /* filename root of converted file          */
    gchar *fname_extension; /* filename extension of converted file     */
    GPid  pid;              /* PID of child doing the conversion        */
    gint  stderr;           /* stderr of child doing the conversion     */
    Track *track;           /* for reference, don't access inside threads! */
    iTunesDB *itdb;         /* for reference, don't access inside threads! */
    gint  threadnum;        /* number of thread working on this track   */
    Conversion *conv;       /* pointer back to the conversion struct    */
    GIOChannel *gio_channel;
    guint source_id;
    gchar *artist;
    gchar *album;
    gchar *track_nr;
    gchar *title;
    gchar *genre;
    gchar *year;
    gchar *comment;
};

enum 
{
    CONV_DIRSIZE_INVALID = -1,      /* dirsize not valid */
};

static Conversion *conversion;



/* Set up conversion infrastructure. Must only be called once. */
void file_convert_init ()
{
    GladeXML *log_xml;
    GtkWidget *vbox;

    g_return_if_fail (conversion==NULL);

    conversion = g_new0 (Conversion, 1);
    conversion->mutex = g_mutex_new ();

    conversion->finished_cond = g_cond_new ();
    conversion->dirsize_cond = g_cond_new ();
    conversion->prune_cond = g_cond_new ();
    conversion_setup_cachedir (conversion);

    if (!prefs_get_string_value (FILE_CONVERT_TEMPLATE, NULL))
    {
	prefs_set_string (FILE_CONVERT_TEMPLATE, "%A/%t_%T");
    }

    if (!prefs_get_string_value (FILE_CONVERT_DISPLAY_LOG, NULL))
    {
	prefs_set_int (FILE_CONVERT_DISPLAY_LOG, TRUE);
    }

    conversion->dirsize = CONV_DIRSIZE_INVALID;

    /* setup log window */
    log_xml = glade_xml_new (xml_file, "conversion_log", NULL);
    conversion->log_window = gtkpod_xml_get_widget (log_xml, "conversion_log");
    gtk_window_set_default_size (GTK_WINDOW (conversion->log_window),
				 prefs_get_int (FILE_CONVERT_LOG_SIZE_X),
				 prefs_get_int (FILE_CONVERT_LOG_SIZE_Y));
    g_signal_connect_swapped (GTK_OBJECT (conversion->log_window), "delete-event",
			      G_CALLBACK (conversion_log_window_delete),
			      conversion);
    vbox = gtkpod_xml_get_widget (log_xml, "conversion_vbox");
    conversion->notebook = gtk_notebook_new ();
    gtk_widget_show (conversion->notebook);
    gtk_box_pack_start (GTK_BOX (vbox), conversion->notebook, TRUE, TRUE, 0);
    conversion->log_window_posx = G_MININT;
    conversion->log_window_posy = G_MININT;
    conversion->log_statusbar = GTK_STATUSBAR (
	gtkpod_xml_get_widget (log_xml,
			       "conversion_statusbar"));
    conversion->log_context_id = gtk_statusbar_get_context_id (
	conversion->log_statusbar,
	_("Summary status of conversion processes"));
    conversion_log_add_pages (conversion, 1);

    /* initialize values from the preferences */
    file_convert_prefs_changed ();

    /* start timeout function for the scheduler */
    conversion->timeout_id = g_timeout_add (100,   /* every 100 ms */
					    conversion_scheduler,
					    conversion);
}


/* Shut down conversion infrastructure */
void file_convert_shutdown ()
{
    g_return_if_fail (conversion);

    /* nothing to do so far */

    /* in other words: not sure how we can shut this down... */
}


/* This is called just before gtkpod closes down */
void file_convert_update_default_sizes ()
{
    conversion_update_default_sizes (conversion);
}


/* Call this function each time the preferences have been updated */
void file_convert_prefs_changed ()
{
    conversion_prefs_changed (conversion);
}


/* Add @track to the list of tracks to be converted if conversion is
 * necessary.
 *
 * Return value: FALSE if an error occured, TRUE otherwise
 */
gboolean file_convert_add_track (Track *track)
{
    return conversion_add_track (conversion, track);
}


/* Reorder the scheduled list so that tracks in @itdb are converted first */
void file_convert_itdb_first (iTunesDB *itdb)
{
    conversion_itdb_first (conversion, itdb);
}


/* Cancel conversion for all tracks of @itdb */
void file_convert_cancel_itdb (iTunesDB *itdb)
{
    conversion_cancel_itdb (conversion, itdb);
}


/* Cancel conversion for @tracks */
void file_convert_cancel_track (Track *track)
{
    conversion_cancel_track (conversion, track);
}

/* Wait at most @ms milliseconds for the next track in @itdb to finish
 * conversion. If a track has finished conversion return a pointer to
 * that track, NULL otherwise. Returns immediately if a track in @itdb
 * is already in the finished list. If @itdb == NULL, wait for track
 * of any itdb. If @ms is zero, wait indefinitely. */
Track *file_convert_timed_wait (iTunesDB *itdb, gint ms)
{
    return conversion_timed_wait (conversion, itdb, ms);
}


/* ----------------------------------------------------------------

   from here on down only static functions

   ---------------------------------------------------------------- */


/* Update the prefs with the current size of the log window */
static void conversion_update_default_sizes (Conversion *conv)
{
    gint defx, defy;
    
    g_return_if_fail (conv && conv->log_window);

    g_mutex_lock (conv->mutex);

    gtk_window_get_size (GTK_WINDOW (conv->log_window), &defx, &defy);
    prefs_set_int(FILE_CONVERT_LOG_SIZE_X, defx);
    prefs_set_int(FILE_CONVERT_LOG_SIZE_Y, defy);

    g_mutex_unlock (conv->mutex);
}



/* used to show/hide the log window and adjust the View->menu
   items. g_mutex_lock(conv->mutex) before calling. Used in
   conversion_log_window_delete() and conversion_prefs_changed(). */
static void conversion_display_hide_log_window (Conversion *conv)
{
    GtkWidget *mi;
    /* show display log if it was previously hidden and should be
       shown again */
    mi = gtkpod_xml_get_widget (main_window_xml, "conversion_log");
    if (prefs_get_int (FILE_CONVERT_DISPLAY_LOG))
    {
	if (conv->log_window_hidden && !conv->log_window_shown)
	{
	    gtk_widget_show (conv->log_window);
	    if (conv->log_window_posx != G_MININT)
	    {
		gtk_window_move (GTK_WINDOW (conv->log_window),
				 conv->log_window_posx,
				 conv->log_window_posy);
	    }
	}
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (mi), TRUE);
	conv->log_window_shown = TRUE;
    }
    else
    {
	if (conv->log_window_shown)
	{   /* window has previously been shown */
	    gint posx, posy;
	    gtk_window_get_position (GTK_WINDOW (conv->log_window),
				     &posx, &posy);
	    conv->log_window_posx = posx;
	    conv->log_window_posy = posy;
	}
	conv->log_window_shown = FALSE;
	conv->log_window_hidden = TRUE;
	gtk_widget_hide (conv->log_window);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (mi), FALSE);
    }
}


/* Signal when user tries to close the log window */
static gboolean conversion_log_window_delete (Conversion *conv)
{
    g_return_val_if_fail (conv, TRUE);

    g_mutex_lock (conv->mutex);
 
    prefs_set_int (FILE_CONVERT_DISPLAY_LOG, FALSE);
    conversion_display_hide_log_window (conv);

    g_mutex_unlock (conv->mutex);

    return TRUE; /* don't close window -- it will be hidden instead by
		  * conversion_display_hid_log_window() */
}


/* set the labels of the notebook of the log window. If required
 * 'g_mutex_lock (conv->mutex)' before calling this function. */
static void conversion_log_set_status (Conversion *conv)
{
    GList *glpage, *glthread;
    gchar *buf;

    g_return_if_fail (conv);

    /* Set tab label text to active/inactive */
    glthread = conv->threads;
    for (glpage=conv->pages; glpage; glpage=glpage->next)
    {
	GtkWidget *child = glpage->data;
	g_return_if_fail (child);

	/* in the beginning we may have more pages than thread entries */
	if (glthread && glthread->data)
	{
	    gtk_notebook_set_tab_label_text (GTK_NOTEBOOK (conv->notebook),
					     child, _("active"));
	}
	else
	{
	    gtk_notebook_set_tab_label_text (GTK_NOTEBOOK (conv->notebook),
					     child, _("inactive"));
	}

	if (glthread)
	{
	    glthread = glthread->next;
	}
    }

    /* Show a summary status */
    gtk_statusbar_pop (conv->log_statusbar, conv->log_context_id);
    buf = g_strdup_printf (_("Active threads: %d. Scheduled tracks: %d."),
			   conv->threads_num,
			   g_list_length (conv->scheduled) + g_list_length (conv->processing));
    gtk_statusbar_push (conv->log_statusbar, conv->log_context_id, buf);
    g_free (buf);
}




/* adds pages to the notebook if the number of pages is less than the
 * number of threads. If required 'g_mutex_lock (conv->mutex)' before
 * calling this function. */
static void conversion_log_add_pages (Conversion *conv, gint threads)
{
    g_return_if_fail (conv);

    while ((g_list_length (conv->textviews) == 0) ||
	   (threads > g_list_length (conv->textviews)))
    {
	GtkWidget *scrolled_window;
	GtkWidget *textview;

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	conv->pages = g_list_append (conv->pages, scrolled_window);
	textview = gtk_text_view_new ();
	gtk_container_add (GTK_CONTAINER (scrolled_window), textview);
	conv->textviews = g_list_append (conv->textviews, textview);

	gtk_widget_show_all (scrolled_window);

	gtk_notebook_append_page (GTK_NOTEBOOK (conv->notebook),
				  scrolled_window, NULL);

	conversion_log_set_status (conv);
    }
}


static void conversion_prefs_changed (Conversion *conv)
{
    gdouble maxsize;

    g_return_if_fail (conv);

    g_mutex_lock (conv->mutex);

    if (prefs_get_double_value (FILE_CONVERT_MAXDIRSIZE, &maxsize))
    {
	conv->max_dirsize = 1024 * 1024 * 1024 * maxsize;
    }
    else
    {   /* set default of 4 GB */
	conv->max_dirsize = (gint64)4 * 1024 * 1024 * 1024;
	prefs_set_double (FILE_CONVERT_MAXDIRSIZE, 4);
    }

    if (conv->max_dirsize < 0)
    {   /* effectively disable caching */
	conv->max_dirsize = 0;
    }

    conv->max_threads_num = prefs_get_int (FILE_CONVERT_MAX_THREADS_NUM);
    if (conv->max_threads_num == 0)
    {   /* set to maximum available number of processors */
	conv->max_threads_num = sysconf (_SC_NPROCESSORS_ONLN);
	/* paranoia mode on */
	if (conv->max_threads_num <= 0)
	{
	    conv->max_threads_num = 1;
	}
    }

    g_free (conv->template);
    conv->template = prefs_get_string (FILE_CONVERT_TEMPLATE);

    if ((conv->dirsize == CONV_DIRSIZE_INVALID) ||
	(conv->dirsize > conv->max_dirsize))
    {
	GThread *thread;
	/* Prune dir of unused files if size is too big, calculate and set
	   the size of the directory. Do all that in the background. */
	thread = g_thread_create_full (conversion_prune_dir,
				       conv,        /* user data  */
				       0,           /* stack size */
				       FALSE,       /* joinable   */
				       TRUE,        /* bound      */
				       G_THREAD_PRIORITY_NORMAL,
				       NULL);       /* error      */
    }

    conversion_display_hide_log_window (conv);

    g_mutex_unlock (conv->mutex);
}


/* Reorder the scheduled list so that tracks in @itdb are converted first */
static void conversion_itdb_first (Conversion *conv, iTunesDB *itdb)
{
    GList *gl;
    GList *gl_itdb = NULL;
    GList *gl_other = NULL;

    g_return_if_fail (conv);
    g_return_if_fail (itdb);

    g_mutex_lock (conv->mutex);
    /* start from the end to keep the same order overall (we're
       prepending to the list for performance reasons */
    for (gl=g_list_last(conv->scheduled); gl; gl=gl->prev)
    {
	ConvTrack *ctr = gl->data;
	if (!ctr || !ctr->track)
	{
	    g_mutex_unlock (conv->mutex);
	    g_return_if_reached ();
	}
	g_return_if_fail (ctr);
	g_return_if_fail (ctr->track);
	if (ctr->track->itdb == itdb)
	{
	    gl_itdb = g_list_prepend (gl_itdb, ctr);
	}
	else
	{
	    gl_other = g_list_prepend (gl_other, ctr);
	}
    }
    g_list_free (conv->scheduled);
    conv->scheduled = g_list_concat (gl_other, gl_itdb);
    g_mutex_unlock (conv->mutex);
}


/* adds @text to the textview on page @threadnum of the log window. If
 * required 'g_mutex_lock (conv->mutex)' before calling this
 * function. */
static void conversion_log_append (Conversion *conv,
				   const gchar *text, gint threadnum)
{
    GtkWidget *textview;
    GtkTextBuffer *textbuffer;
    GtkTextIter start, end;
    const gchar *ptr, *next;

    g_return_if_fail (conv);

    /* add pages if necessary */
    conversion_log_add_pages (conv, threadnum+1);

    /* get appropriate textview */
    textview = g_list_nth_data (conv->textviews, threadnum);
    g_return_if_fail (textview);

    textbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));
    gtk_text_buffer_get_end_iter (textbuffer, &end);

    ptr = text;

    while (*ptr)
    {
	next = g_utf8_find_next_char (ptr, NULL);
	if (*ptr == '\b')
	{
	    start = end;
	    if (gtk_text_iter_backward_char (&start))
	    {
		gtk_text_buffer_delete (textbuffer, &start, &end);
	    }
	}
	else if(*ptr == '\r')
	{
	    start = end;
	    gtk_text_iter_set_line_offset (&start, 0);
	    gtk_text_buffer_delete (textbuffer, &start, &end);
	}
	else
	{
	    gtk_text_buffer_insert (textbuffer, &end, ptr, next - ptr);
	}
	ptr = next;
    }
    gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (textview),
				  &end, 0.0, FALSE, 0.0, 0.0); 

    if (prefs_get_int (FILE_CONVERT_DISPLAY_LOG))
    {
	gtk_widget_show (conv->log_window);
    }
}



/*
 * Called by scheduler for all running processes. Lock 'conv->mutex'
 * before calling this function.
 */
static void conversion_display_log (ConvTrack *ctr)
{
    gchar buf[PATH_MAX];
    gsize bytes_read = 0;
    GIOStatus status;
    Conversion *conv;

    g_return_if_fail (ctr && ctr->conv);
    conv = ctr->conv;

    do
    {
	status = g_io_channel_read_chars (ctr->gio_channel, buf,
					  PATH_MAX-1, &bytes_read, NULL);
	buf[bytes_read] = '\0';
	if (bytes_read > 0)
	{
	    switch (status)
	    {
	    case G_IO_STATUS_ERROR :
/*	    puts ("gio error");*/
		break;
	    case G_IO_STATUS_EOF :
		conversion_log_append (conv, buf, ctr->threadnum);
		break;
	    case G_IO_STATUS_NORMAL :
		conversion_log_append (conv, buf, ctr->threadnum);
		break;
	    case G_IO_STATUS_AGAIN :
		break;
	    }
	}
    } while (bytes_read > 0);

    return;
}



/* called by conversion_cancel_itdb to mark nodes invalid that are in
   the specified itdb */
static void conversion_cancel_itdb_fe (gpointer data, gpointer userdata)
{
    ConvTrack *ctr = data;
    iTunesDB *itdb = userdata;

    g_return_if_fail (ctr && ctr->track && ctr->track->userdata);

    if (ctr->valid && (ctr->itdb == itdb))
    {
	ExtraTrackData *etr = ctr->track->userdata;
	ctr->valid = FALSE;
	if (ctr->pid)
	{   /* if a conversion process is running kill the entire
	     * process group (i.e. all processes started within the shell) */
	    kill (-ctr->pid, SIGTERM);
	}
	if (etr->conversion_status != FILE_CONVERT_CONVERTED)
	    etr->conversion_status = FILE_CONVERT_KILLED;
    }
}

/* Cancel conversion for all tracks of @itdb */
static void conversion_cancel_itdb (Conversion *conv, iTunesDB *itdb)
{
    GList *gl, *next;

    g_return_if_fail (conv);
    g_return_if_fail (itdb);

    g_mutex_lock (conv->mutex);

    g_list_foreach (conv->scheduled, conversion_cancel_itdb_fe, itdb);
    g_list_foreach (conv->processing, conversion_cancel_itdb_fe, itdb);
    g_list_foreach (conv->failed, conversion_cancel_itdb_fe, itdb);
    g_list_foreach (conv->converted, conversion_cancel_itdb_fe, itdb);

    /* The finished list is more complicated because we remove all
       elements no longer valid */
    for (gl=conv->finished; gl; gl=next)
    {
	ConvTrack *ctr = gl->data;
	g_return_if_fail (ctr);
	next = gl->next;

	conversion_cancel_itdb_fe (ctr, itdb);

	if (!ctr->valid)
	{
	    conv->finished = g_list_delete_link (conv->finished, gl);
	    conversion_convtrack_free (ctr);
	}
    }

    g_mutex_unlock (conv->mutex);
}



/* called by conversion_cancel_track to mark nodes invalid that refer
   to @track (should only be one...) */
/* Used by conversion-cancel_track_sub() to find the list element that
   contains @track. Returns 0 if found. */
static int conversion_cancel_track_cmp (gconstpointer a, gconstpointer b)
{
    const ConvTrack *ctr = a;
    const Track *track = b;

    g_return_val_if_fail (ctr, 0);

    if (ctr->track == track) return 0;
    return -1;
}


/* Finds @track in @ctracks and marks it as invalid. If @remove is
   TRUE, the element is removed from the list altogether. */
static void conversion_cancel_track_sub (GList **ctracks,
					 Track *track,
					 gboolean remove)
{
    GList *gl;

    g_return_if_fail (track && track->userdata);

    gl = g_list_find_custom (*ctracks, track, conversion_cancel_track_cmp);
    if (gl)
    {
	ConvTrack *ctr = gl->data;
	if (ctr->valid && (ctr->track == track))
	{
	    ExtraTrackData *etr = track->userdata;
	    ctr->valid = FALSE;
	    if (ctr->pid)
	    {   /* if a conversion process is running kill the entire
		 * process group (i.e. all processes started within the shell) */
		kill (-ctr->pid, SIGTERM);
	    }
	    if (etr->conversion_status != FILE_CONVERT_CONVERTED)
		etr->conversion_status = FILE_CONVERT_KILLED;
	}
	if (remove)
	{
	    *ctracks = g_list_delete_link (*ctracks, gl);
	    conversion_convtrack_free (ctr);
	}
    }
}


/* Cancel conversion for @track */
static void conversion_cancel_track (Conversion *conv, Track *track)
{
    g_return_if_fail (conv);
    g_return_if_fail (track);

    g_mutex_lock (conv->mutex);

    conversion_cancel_track_sub (&conv->scheduled, track, FALSE);
    conversion_cancel_track_sub (&conv->processing, track, FALSE);
    conversion_cancel_track_sub (&conv->failed, track, FALSE);
    conversion_cancel_track_sub (&conv->converted, track, FALSE);
    conversion_cancel_track_sub (&conv->finished, track, TRUE);
	
    g_mutex_unlock (conv->mutex);
}


/* used by conversion_timed_wait() to find a matching track in
   conv->finished */
Track *conversion_timed_wait_sub (Conversion *conv, iTunesDB *itdb)
{
    GList *gl;
    Track *result = NULL;

    /* see if a track is already in the finished list */
    for (gl=g_list_last (conv->finished); gl && (!result); gl=gl->prev)
    {
	ConvTrack *ctr = gl->data;
	g_return_val_if_fail (ctr, NULL);

	if (ctr->valid)
	{
	    if ((itdb == NULL) || (ctr->itdb == itdb))
	    {
		result = ctr->track;
	    }
	}
    }

    return result;
}


/* Wait at most @ms milliseconds for the next track in @itdb to finish
 * conversion. If a track has finished conversion return a pointer to
 * that track, NULL otherwise. Returns immediately if a track in @itdb
 * is already in the finished list. If @itdb == NULL, wait for track
 * of any itdb. If @ms is zero, wait indefinitely. */
static Track *conversion_timed_wait (Conversion *conv, iTunesDB *itdb, gint ms)
{
    Track *result = NULL;

    g_return_val_if_fail (conv, NULL);
    
    g_mutex_lock (conv->mutex);

    result = conversion_timed_wait_sub (conv, itdb);

    if (!result)
    {   /* no result so far -- let's wait */
	GTimeVal *abs_time = NULL;

	if (ms != 0)
	{
	    abs_time = g_new (GTimeVal, 1);
	    g_get_current_time (abs_time);
	    g_time_val_add (abs_time, ((glong)ms)*1000);
	}

	do
	{
	    /* make sure at least one thread is started even if the
	     * dirsize is too large */
	    if (conv->threads_num == 0)
		conv->conversion_force = TRUE;

	    if (g_cond_timed_wait (conv->finished_cond, conv->mutex, abs_time))
	    {   /* a track has actually finished conversion! */
		result = conversion_timed_wait_sub (conv, itdb);
	    }
	} while ((ms == 0) && !result);

	g_free (abs_time);
    }

    g_mutex_unlock (conv->mutex);

    return result;
}



/* Add @track to the list of tracks to be converted if conversion is
 * necessary.
 *
 * Return value: FALSE if an error occured, TRUE otherwise
 */
static gboolean conversion_add_track (Conversion *conv, Track *track)
{
    ExtraTrackData *etr;
    ConvTrack *ctr;
    gchar *conversion_cmd = NULL;
    const gchar *typestr = NULL;
    gboolean convert=FALSE, must_convert = FALSE;
    gboolean result = TRUE;

    g_return_val_if_fail (conv, FALSE);
    g_return_val_if_fail (track, FALSE);
    g_return_val_if_fail (track->itdb, FALSE);
    etr = track->userdata;
    g_return_val_if_fail (etr, FALSE);

    if ((track->itdb->usertype & GP_ITDB_TYPE_LOCAL) ||
	(track->transferred))
    {   /* no conversion or transfer needed */
	return TRUE;
    }

    /* Create ConvTrack structure */
    ctr = g_new0 (ConvTrack, 1);
    ctr->track = track;
    ctr->itdb = track->itdb;
    ctr->conv = conv;
    ctr->orig_file = g_strdup (etr->pc_path_locale);
    ctr->converted_file = g_strdup (etr->converted_file);
    ctr->artist    = g_strdup (track->artist);
    ctr->album     = g_strdup (track->album);
    ctr->track_nr  = g_strdup_printf ("%02d", track->track_nr);
    ctr->title     = g_strdup (track->title);
    ctr->genre     = g_strdup (track->genre);
    ctr->year      = g_strdup (etr->year_str);
    ctr->comment   = g_strdup (track->comment);
    ctr->valid = TRUE;

    if (!etr->pc_path_locale || (strlen (etr->pc_path_locale) == 0))
    {
	gchar *buf = get_track_info (track, FALSE);
	gtkpod_warning (_("Original filename not available for '%s.'\n"), buf);
	g_free (buf);

	etr->conversion_status = FILE_CONVERT_FAILED;
	/* add to failed list */
	g_mutex_lock (conv->mutex);
	conv->failed = g_list_prepend (conv->failed, ctr);
	g_mutex_unlock (conv->mutex);
	debug ("added track to failed %p\n", track);
	return FALSE;
    }

    if (!g_file_test (etr->pc_path_locale, G_FILE_TEST_IS_REGULAR))
    {
	gchar *buf = get_track_info (track, FALSE);
	gtkpod_warning (_("Filename '%s' is no longer valid for '%s'.\n"),
			etr->pc_path_utf8, buf);
	g_free (buf);

	etr->conversion_status = FILE_CONVERT_FAILED;
	/* add to failed list */
	g_mutex_lock (conv->mutex);
	conv->failed = g_list_prepend (conv->failed, ctr);
	g_mutex_unlock (conv->mutex);
	debug ("added track to failed %p\n", track);
	return FALSE;
    }	

    /* Find the correct script for conversion */
    switch (determine_file_type(etr->pc_path_locale))
    {
        case FILE_TYPE_UNKNOWN:
        case FILE_TYPE_M4P:
        case FILE_TYPE_M4B:
        case FILE_TYPE_M4V:
        case FILE_TYPE_MP4:
        case FILE_TYPE_MOV:
        case FILE_TYPE_MPG:
        case FILE_TYPE_M3U:
        case FILE_TYPE_PLS:
        case FILE_TYPE_IMAGE:
        case FILE_TYPE_DIRECTORY:
	    /* we don't convert these (yet) */
	    etr->conversion_status = FILE_CONVERT_INACTIVE;
	    /* add to finished */
	    g_mutex_lock (conv->mutex);
	    conv->finished = g_list_prepend (conv->finished, ctr);
	    g_mutex_unlock (conv->mutex);
	    debug ("added track to finished %p\n", track);
            return TRUE;
        case FILE_TYPE_M4A:
	    convert = prefs_get_int ("convert_m4a");
            conversion_cmd = prefs_get_string ("path_conv_m4a");
            break;
        case FILE_TYPE_WAV:
	    convert = prefs_get_int ("convert_wav");
            conversion_cmd = prefs_get_string ("path_conv_wav");
            break;
        case FILE_TYPE_MP3:
	    convert = prefs_get_int ("convert_mp3");
            conversion_cmd = prefs_get_string ("path_conv_mp3");
            break;
        case FILE_TYPE_OGG:
	    convert = prefs_get_int ("convert_ogg");
	    conversion_cmd = prefs_get_string ("path_conv_ogg");
	    must_convert = TRUE;
	    typestr = _("Ogg Vorbis");
            break;
        case FILE_TYPE_FLAC:
	    convert = prefs_get_int ("convert_flac");
	    conversion_cmd = prefs_get_string ("path_conv_flac");
	    must_convert = TRUE;
	    typestr = _("FLAC");
            break;
    }

    ctr->must_convert = must_convert;
    ctr->conversion_cmd = conversion_cmd;
    conversion_cmd = NULL;

    if (convert)
    {
	gchar *template;

	g_mutex_lock (conv->mutex);
	template = g_strdup (conv->template);
	g_mutex_unlock (conv->mutex);

	ctr->fname_root = get_string_from_template (track, template, TRUE, TRUE);
	ctr->fname_extension = conversion_get_fname_extension (NULL, ctr);
	if (ctr->fname_extension)
	{
	    etr->conversion_status = FILE_CONVERT_SCHEDULED;
	    /* add to scheduled list */
	    g_mutex_lock (conv->mutex);
	    conv->scheduled = g_list_prepend (conv->scheduled, ctr);
	    g_mutex_unlock (conv->mutex);

	    result = TRUE;
	    debug ("added track %p\n", track);
	}
	else
	{   /* an error has occured */
	    if (ctr->errormessage)
	    {
puts(ctr->errormessage);
		gtkpod_warning (ctr->errormessage);
	    }

	    if (must_convert)
		etr->conversion_status = FILE_CONVERT_REQUIRED_FAILED;
	    else
		etr->conversion_status = FILE_CONVERT_FAILED;
	    /* add to failed list */
	    g_mutex_lock (conv->mutex);
	    conv->failed = g_list_prepend (conv->failed, ctr);
	    g_mutex_unlock (conv->mutex);
	    result = FALSE;
	    debug ("added track to failed %p\n", track);
	}
	g_free (template);
    }
    else if (must_convert)
    {
	gchar *buf = get_track_info (track, FALSE);
	g_return_val_if_fail (typestr, FALSE);
	gtkpod_warning (_("Files of type '%s' are not supported by the iPod. Please go to the Preferences to set up and turn on a suitable conversion script for '%s'.\n\n"), typestr, buf);
	g_free (buf);

	etr->conversion_status = FILE_CONVERT_REQUIRED;
	/* add to failed list */
	g_mutex_lock (conv->mutex);
	conv->failed = g_list_prepend (conv->failed, ctr);
	g_mutex_unlock (conv->mutex);
	result = FALSE;
	debug ("added track to failed %p\n", track);
    }
    else
    {
	etr->conversion_status = FILE_CONVERT_INACTIVE;
	/* add to finished */
	g_mutex_lock (conv->mutex);
	conv->finished = g_list_prepend (conv->finished, ctr);
	g_mutex_unlock (conv->mutex);
	debug ("added track to finished %p\n", track);
    }
    g_free (conversion_cmd);
    return result;
}


/* free the memory taken by @ctr */
static void conversion_convtrack_free (ConvTrack *ctr)
{
    g_return_if_fail (ctr);
    g_free (ctr->orig_file);
    g_free (ctr->converted_file);
    g_free (ctr->conversion_cmd);
    g_free (ctr->fname_root);
    g_free (ctr->fname_extension);
    g_free (ctr->errormessage);
    g_free (ctr->artist);
    g_free (ctr->album);
    g_free (ctr->track_nr);
    g_free (ctr->title);
    g_free (ctr->genre);
    g_free (ctr->year);
    g_free (ctr->comment);
    if (ctr->gio_channel)
    {
	g_io_channel_unref (ctr->gio_channel);
    }
    if (ctr->pid)
    {
	g_spawn_close_pid (ctr->pid);
    }
    g_free (ctr);
}


/* return some sensible input about @ctrack. You must free the
   returned string after use. */
static gchar *conversion_get_track_info (Conversion *conv, ConvTrack *ctr)
{
    gchar *str = NULL;

    if (conv) g_mutex_lock (conv->mutex);

    if ((ctr->title && strlen(ctr->title)))
    {
	str = g_strdup (ctr->title);
    }
    else if ((ctr->album && strlen(ctr->album)))
    {
	str = g_strdup_printf ("%s_%s", ctr->track_nr, ctr->album);
    }
    else if ((ctr->artist && strlen(ctr->artist)))
    {
	str = g_strdup_printf ("%s_%s", ctr->track_nr, ctr->artist);
    }
    else
    {
	str = g_strdup (_("No information available"));
    }

    if (conv) g_mutex_unlock (conv->mutex);

    return str;
}



/* Set and set up the conversion cachedir.
 *
 * Return value: TRUE if directory could be set up.
 *               FALSE if an error occured.
 */
static gboolean conversion_setup_cachedir (Conversion *conv)
{
    g_return_val_if_fail (conv, FALSE);

    g_mutex_lock (conv->mutex);

    g_free (conv->cachedir);
    conv->cachedir = NULL;
    conv->cachedir = prefs_get_string (FILE_CONVERT_CACHEDIR);
    if (!conv->cachedir)
    {
	gchar *cfgdir = prefs_get_cfgdir ();
	if (cfgdir)
	{
	    conv->cachedir = g_build_filename (cfgdir, "conversion_cache", NULL);
	    g_free (cfgdir);
	}
    }
    if (conv->cachedir)
    {
	if (!g_file_test (conv->cachedir, G_FILE_TEST_IS_DIR))
	{
	    if ((g_mkdir (conv->cachedir, 0777)) == -1)
	    {
		gtkpod_warning (_("Could not create '%s'. Filetype conversion will not work.\n"));
		g_free (conv->cachedir);
		conv->cachedir = NULL;
	    }
	}
    }

    if (conv->cachedir)
    {
	prefs_set_string (FILE_CONVERT_CACHEDIR, conv->cachedir);
    }

    g_mutex_unlock (conv->mutex);

    if (conv->cachedir)
	return TRUE;
    else
	return FALSE;
}



/* called in conversion_scheduler to g_spawn_close_pid() and unref the
   io-channel after the conversion has finished/has failed/has been
   killed. If output is buffered in the io-channel it's appended to
   the log before closing up the channel. */
static void conversion_free_resources (ConvTrack *ctr)
{
    g_return_if_fail (ctr);

    /* Free resources */
    if (ctr->pid)
    {
	g_spawn_close_pid (ctr->pid);
	ctr->pid = 0;
    }
    if (ctr->gio_channel)
    {
	conversion_display_log (ctr);
	g_io_channel_unref (ctr->gio_channel);
	ctr->gio_channel = NULL;
    }
}


/* timeout function, add new threads if unscheduled tracks are in the
   queue and maximum number of threads hasn't been reached */
static gboolean conversion_scheduler (gpointer data)
{
    Conversion *conv = data;

    g_return_val_if_fail (conv, FALSE);

/*     debug ("conversion_scheduler enter\n"); */

    g_mutex_lock (conv->mutex);

    if (!conv->cachedir)
    {
	/* Cachedir is not available. Not good! Remove the timeout function. */
	g_mutex_unlock (conv->mutex);
	g_return_val_if_reached (FALSE);
    }

    if (conv->dirsize == CONV_DIRSIZE_INVALID)
    {   /* dirsize has not been set up. Wait until that has been done. */
	g_mutex_unlock (conv->mutex);
	return TRUE;
    }

    if (conv->scheduled)
    {
	if ((conv->threads_num < conv->max_threads_num) &&
	    ((conv->dirsize <= conv->max_dirsize) || conv->conversion_force))
	{
	    GList *gl;
	    GThread *thread;

	    thread = g_thread_create_full (conversion_thread,
					   conv,   /* user data  */
					   0,      /* stack size */
					   FALSE,  /* joinable   */
					   TRUE,   /* bound      */
					   G_THREAD_PRIORITY_LOW,
					   NULL);  /* error      */

	    /* Add thread to thread list. Use first available slot */
	    gl = g_list_find (conv->threads, NULL);
	    if (gl)
	    {   /* found empty slot */
		gl->data = thread;
	    }
	    else
	    {   /* no empty slot available --> add to end */
		conv->threads = g_list_append (conv->threads, thread);
	    }

	    ++conv->threads_num;
	}
    }

    /* reset the conversion_force flag */
    conv->conversion_force = FALSE;

    if (conv->processing)
    {
	GList *gl;
	for (gl=conv->processing; gl; gl=gl->next)
	{
	    ConvTrack *ctr = gl->data;
	    g_return_val_if_fail (ctr, (g_mutex_unlock (conv->mutex), TRUE));
	    if (ctr->valid && ctr->gio_channel)
	    {
		conversion_display_log (ctr);
	    }
	}
    }

    if (conv->failed)
    {
	GList *gl;
	for (gl=conv->failed; gl; gl=gl->next)
	{
	    ConvTrack *ctr = gl->data;
	    g_return_val_if_fail (ctr, (g_mutex_unlock (conv->mutex), TRUE));

	    /* free resources (pid, flush io-channel) */
	    conversion_free_resources (ctr);

	    if (ctr->valid)
	    {
		ExtraTrackData *etr;
		if (ctr->errormessage)
		{
		    gtkpod_warning (ctr->errormessage);
		}
		g_return_val_if_fail (ctr->track && ctr->track->userdata,
				      (g_mutex_unlock (conv->mutex), TRUE));
		etr = ctr->track->userdata;
		if (ctr->must_convert)
		{
		    if (etr->conversion_status != FILE_CONVERT_REQUIRED)
		    {
			etr->conversion_status = FILE_CONVERT_REQUIRED_FAILED;
		    }
		}
		else
		{
		    etr->conversion_status = FILE_CONVERT_FAILED;
		}
		/* Add to finished so we can find it in case user
		   waits for next converted file */
		conv->finished = g_list_prepend (conv->finished, ctr);
		g_cond_broadcast (conv->finished_cond);
	    }
	    else
	    {   /* track is not valid any more */
		conversion_convtrack_free (ctr);
	    }
	}
	g_list_free (conv->failed);
	conv->failed = NULL;
    }

    if (conv->converted)
    {
	GList *gl;
	for (gl=conv->converted; gl; gl=gl->next)
	{
	    ConvTrack *ctr = gl->data;
	    g_return_val_if_fail (ctr, (g_mutex_unlock (conv->mutex), TRUE));

	    /* free resources (pid / flush io-channel) */
	    conversion_free_resources (ctr);

	    if (ctr->valid)
	    {
		GList *trackgl;
		g_return_val_if_fail (ctr->track,
				      (g_mutex_unlock (conv->mutex), TRUE));
		GList *tracks = gp_itdb_find_same_tracks_in_itdbs (ctr->track);
		for (trackgl=tracks; trackgl; trackgl=trackgl->next)
		{
		    ExtraTrackData *etr;
		    Track *tr = trackgl->data;
		    g_return_val_if_fail (tr && tr->itdb && tr->userdata,
					  (g_mutex_unlock (conv->mutex), TRUE));
		    etr = tr->userdata;

		    /* spread information to local databases for
		       future reference */
		    if (tr->itdb->usertype & GP_ITDB_TYPE_LOCAL)
		    {
			g_free (etr->converted_file);
			etr->converted_file = g_strdup (ctr->converted_file);
			pm_track_changed (tr);
			data_changed (tr->itdb);
		    }

		    /* don't forget to copy conversion data to the
		       track itself */
		    if (tr == ctr->track)
		    {
			g_free (etr->converted_file);
			etr->converted_file = g_strdup (ctr->converted_file);
			etr->conversion_status = FILE_CONVERT_CONVERTED;
			tr->size = ctr->converted_size;
			pm_track_changed (tr);
			data_changed (tr->itdb);
		    }

printf ("converted %p/%p: %s\n", tr->itdb, tr, ctr->converted_file);

		}
		g_list_free (tracks);
		/* add ctr to finished */
		conv->finished = g_list_prepend (conv->finished, ctr);
		g_cond_broadcast (conv->finished_cond);
	    }
	    else
	    {   /* track is not valid any more */
		conversion_convtrack_free (ctr);
	    }
	}
	g_list_free (conv->converted);
	conv->converted = NULL;
    }

    /* update the log window */
    if (conv->log_window_shown)
    {
	conversion_log_set_status (conv);
    }

    g_mutex_unlock (conv->mutex);

/*    debug ("conversion_scheduler exit\n");*/

    return TRUE;
}


/* Calculate the size of the directory */
static gpointer conversion_update_dirsize (gpointer data)
{
    Conversion *conv = data;
    gchar *dir;
    gint64 size = 0;

    g_return_val_if_fail (conv, NULL);

    debug ("%p update_dirsize enter\n", g_thread_self ());

    g_mutex_lock (conv->mutex);
    if (conv->dirsize_in_progress)
    {   /* another thread is already working on the directory
	   size. We'll wait until it has finished and just return. */
	g_cond_wait (conv->dirsize_cond, conv->mutex);
	g_mutex_unlock (conv->mutex);
	debug ("%p update_dirsize concurrent exit\n", g_thread_self ());
	return NULL;
    }
    conv->dirsize_in_progress = TRUE;
    dir = g_strdup (conv->cachedir);
    g_mutex_unlock (conv->mutex);

    if (dir)
    {
	debug ("%p update_dirsize getting size of dir (%s)\n",
	       g_thread_self (), dir);

	size = get_size_of_directory (dir);
	g_free (dir);
	dir = NULL;
    }

    g_mutex_lock (conv->mutex);
    /* Even in case of an error get_size_of_directory() will return
       0. This means setting the dirsize here will not block the
       conversion process even if there is a problem determining the
       size of the directory */
    conv->dirsize = size;
    /* We're finished doing the directory upgrade. Unset the flag and
       broadcast to all threads waiting to wake them up. */
    conv->dirsize_in_progress = FALSE;
    g_cond_broadcast (conv->dirsize_cond);
    g_mutex_unlock (conv->mutex);

    debug ("%p update_dirsize exit\n", g_thread_self ());
    return NULL;
}


struct conversion_prune_file
{
    gchar *filename;
    struct stat statbuf;
};


/* make a list of all existing files */
static GList *conversion_prune_dir_collect_files (const gchar *dir)
{
    GDir *gdir;
    const gchar *fname;
    GList *files = NULL;

    g_return_val_if_fail (dir, NULL);

    gdir = g_dir_open (dir, 0, NULL);
    if (!gdir)
    {
	/* do something */
	return files;
    }

    while ((fname = g_dir_read_name (gdir)))
    {
	gchar *fullname = g_build_filename (dir, fname, NULL);

	if (g_file_test (fullname, G_FILE_TEST_IS_DIR))
	{
	    files = g_list_concat (files, 
				   conversion_prune_dir_collect_files (fullname));
	}
	else if (g_file_test (fullname, G_FILE_TEST_IS_REGULAR))
	{
	    struct conversion_prune_file *cpf;
	    cpf = g_new0 (struct conversion_prune_file, 1);
	    if (g_stat (fullname, &cpf->statbuf) == 0)
	    {   /* OK, add */
		cpf->filename = fullname;
		fullname = NULL;
		files = g_list_prepend (files, cpf);
	    }
	    else
	    {   /* error -- ignore this file */
		g_free (cpf);
	    }
	}
	g_free (fullname);
    }

    g_dir_close (gdir);

    return files;
}
	
/* used to sort the list of unneeded files so that the oldest ones
   come first */
static gint conversion_prune_compfunc (gconstpointer a, gconstpointer b)
{
    const struct conversion_prune_file *cpf_a = a;
    const struct conversion_prune_file *cpf_b = b;

    return cpf_a->statbuf.st_mtime - cpf_b->statbuf.st_mtime;
}

/* free struct conversion_prune_file structure data, called from
   g_list_foreach */
static void conversion_prune_freefunc (gpointer data, gpointer user_data)
{
    struct conversion_prune_file *cpf = data;
    g_return_if_fail (cpf);
    g_free (cpf->filename);
    g_free (cpf);
}


/* Prune the directory of unused files */
static gpointer conversion_prune_dir (gpointer data)
{
    Conversion *conv = data;
    gchar *dir;
    gint64 dirsize;
    gint64 maxsize;

    g_return_val_if_fail (conv, NULL);

    debug ("%p prune_dir enter\n", g_thread_self ());

    g_mutex_lock (conv->mutex);
    if (conv->prune_in_progress)
    {   /* another thread is already working on the directory
	   prune. We'll wait until it has finished and just return. */
	g_cond_wait (conv->prune_cond, conv->mutex);
	g_mutex_unlock (conv->mutex);
	return NULL;
    }
    conv->prune_in_progress = TRUE;
    dir = g_strdup (conv->cachedir);
    g_mutex_unlock (conv->mutex);

    if (dir)
    {
	GHashTable *hash_needed_files;
	GList **tracklists[] = {&conv->scheduled, &conv->processing,
				&conv->converted, &conv->finished, NULL};
	gint i;
	GList *gl;
	GList *files = NULL;

	debug ("%p prune_dir creating list of available files\n", g_thread_self ());

	/* make a list of all available files */
	files = conversion_prune_dir_collect_files (dir);

	debug ("%p prune_dir creating hash_needed_files\n", g_thread_self ());

	/* make a hash of all the files still needed */
	hash_needed_files = g_hash_table_new_full (g_str_hash, g_str_equal,
						   g_free, NULL);
	g_mutex_lock (conv->mutex);

	/* we have to collect all valid filenames in all 4 track lists */
	for (i=0; tracklists[i]; ++i)
	{
	    for (gl=*tracklists[i]; gl; gl=gl->next)
	    {
		ConvTrack *ctr = gl->data;
		g_return_val_if_fail (ctr, (conv->prune_in_progress = FALSE,
					    g_cond_broadcast (conv->prune_cond),
					    g_mutex_unlock(conv->mutex),
					    NULL));
		if (ctr->valid && ctr->converted_file)
		{
		    g_hash_table_insert (hash_needed_files,
					 g_strdup (ctr->converted_file), ctr);
		}
	    }
	}

	g_mutex_unlock (conv->mutex);

	/* sort the list so that the oldest files are first */
	files = g_list_sort (files, conversion_prune_compfunc);

	/* get an up-to-date count of the directory size */
	conversion_update_dirsize (conv);

	g_mutex_lock (conv->mutex);
	dirsize = conv->dirsize;
	maxsize = conv->max_dirsize;
	g_mutex_unlock (conv->mutex);

	debug ("%p prune_dir removing files (%lld/%lld)\n",
	       g_thread_self (), (long long int)dirsize, (long long int)maxsize);

	/* remove files until dirsize is smaller than maxsize */
	/* ... */
	for (gl=files; gl && (dirsize > maxsize); gl=gl->next)
	{
	    struct conversion_prune_file *cpf = gl->data;
	    g_return_val_if_fail (cpf, (conv->prune_in_progress = FALSE,
					g_cond_broadcast (conv->prune_cond),
					NULL));
	    g_return_val_if_fail (cpf->filename,
				  (conv->prune_in_progress = FALSE,
				   g_cond_broadcast (conv->prune_cond),
				   NULL));
	    if (g_hash_table_lookup (hash_needed_files, cpf->filename) == NULL)
	    {   /* file is not among those remove */
		if (g_remove (cpf->filename) == 0)
		{
		    dirsize -= cpf->statbuf.st_size;
		}
	    }
	}

	debug ("%p prune_dir removed files (%lld/%lld)\n",
	       g_thread_self (), (long long int)dirsize, (long long int)maxsize);

	/* free all data */
	g_list_foreach (files, conversion_prune_freefunc, NULL);
	g_list_free (files);
	g_hash_table_destroy (hash_needed_files);
	g_free (dir);
	dir = NULL;
    }

    g_mutex_lock (conv->mutex);
    conv->prune_in_progress = FALSE;
    g_cond_broadcast (conv->prune_cond);
    g_mutex_unlock (conv->mutex);

    debug ("%p prune_dir exit\n", g_thread_self ());

    return NULL;
}


/* Get filename extension used by @ctr->conversion_cmd. If @conv==NULL,
   don't do locking.

   Return value: filename extension used by @ctr->conversion_cmd or
   NULL if an error occured. In case of an error, the error message is
   stored in ctr->errormessage */
static gchar *conversion_get_fname_extension (Conversion *conv, ConvTrack *ctr)
{
    gchar **argv;
    gboolean result;
    gchar *fname_extension = NULL;
    gchar *std_out = NULL;
    gint exit_status = 0;
    GError *error = NULL;

    g_return_val_if_fail (ctr, NULL);

    /* get filename extension */
    if (conv) g_mutex_lock (conv->mutex);
    argv = build_argv_from_strings (ctr->conversion_cmd, "-x", NULL);
    if (conv) g_mutex_unlock (conv->mutex);

    result = g_spawn_sync (NULL,                /* working dir */
			   argv,                /* argv        */
			   NULL,                /* envp        */
			   G_SPAWN_SEARCH_PATH, /* flags: use user's path */
			   NULL,                /* child_setup function   */
			   NULL,                /* user data              */
			   &std_out,
			   NULL,
			   &exit_status,
			   &error);

    if (result == FALSE)
    {
	if (conv) g_mutex_lock (conv->mutex);
	if (ctr->valid)
	{
	    gchar *buf = conversion_get_track_info (NULL, ctr);
	    ctr->errormessage =
		g_strdup_printf (_("Conversion of '%s' failed: '%s'.\n\n"),
				 buf,
				 error->message);
	    g_free (buf);
	}
	if (conv) g_mutex_unlock (conv->mutex);
	g_error_free (error);
    }
    else if (WEXITSTATUS (exit_status) != 0)
    {
	if (conv) g_mutex_lock (conv->mutex);
	if (ctr->valid)
	{
	    gchar *buf = conversion_get_track_info (NULL, ctr);
	    ctr->errormessage =
		g_strdup_printf (_("Conversion of '%s' failed: '%s %s' returned exit status %d.\n\n"),
				 buf,
				 argv[0], argv[1],
				 WEXITSTATUS (exit_status));
	    g_free (buf);
	}
	if (conv) g_mutex_unlock (conv->mutex);
	result = FALSE;
    }

    /* retrieve filename extension */
    if (std_out && (strlen (std_out) > 0))
    {
	gint len = strlen (std_out);
	fname_extension = std_out;
	std_out = NULL;
	if (fname_extension[len-1] == '\n')
	{
	    fname_extension[len-1] = 0;
	}
    }

    if (!fname_extension || (strlen (fname_extension) == 0))
    {   /* no a valid filename extension */
	if (conv) g_mutex_lock (conv->mutex);
	if (ctr->valid)
	{
	    gchar *buf = conversion_get_track_info (NULL, ctr);
	    ctr->errormessage =
		g_strdup_printf (_("Conversion of '%s' failed: '%s %s' did not return filename extension as expected.\n\n"),
				 buf,
				 argv[0], argv[1]);
	    g_free (buf);
	}
	if (conv) g_mutex_unlock (conv->mutex);
	result = FALSE;
    }

    g_strfreev (argv);
    argv = NULL;

    if (result == FALSE)
    {
	g_free (std_out);
	g_free (fname_extension);
	fname_extension = NULL;;
    }

    return fname_extension;
}


/* Sets a valid filename.

   Return value: TRUE if everything went fine. In this case
   ctr->converted_filename contains the filename to use. If that file
   already exists no conversion is necessary.
   FALSE is returned if some error occured, in which case
   ctr->errormessage may be set. */
static gboolean conversion_set_valid_filename (Conversion *conv, ConvTrack *ctr)
{
    gboolean result = TRUE;

    g_return_val_if_fail (conv, FALSE);
    g_return_val_if_fail (ctr, FALSE);

    g_mutex_lock (conv->mutex);
    if (ctr->valid)
    {
	gint i;
	gchar *rootdir;
	gchar *basename;
	gchar *fname = NULL;

	/* Check if converted_filename already exists. If yes, only
	   keep it if the file is newer than the original file and the
	   filename extension matches the intended
	   conversion. Otherwise remove it and set a new filename */
	if (ctr->converted_file)
	{
	    gboolean delete = FALSE;
	    if (g_file_test (ctr->converted_file, G_FILE_TEST_IS_REGULAR))
	    {
		struct stat conv_stat, orig_stat;
		if (g_str_has_suffix (ctr->converted_file, ctr->fname_extension))
		{
		    if (stat (ctr->converted_file, &conv_stat) == 0)
		    {
			if (stat (ctr->orig_file, &orig_stat) == 0)
			{
			    if (conv_stat.st_mtime > orig_stat.st_mtime)
			    {   /* converted file is newer than
				 * original file */
				g_mutex_unlock (conv->mutex);
				return TRUE;
			    }
			    else
			    {   /* converted file is older than original file */
				delete = TRUE;
			    }
			}
			else
			{   /* error reading original file */
			    char *buf = conversion_get_track_info (NULL, ctr);
			    ctr->errormessage =
				g_strdup_printf (_("Covnersion of '%s' failed: Could not access original file '%s' (%s).\n\n"),
						 buf,
						 ctr->orig_file,
						 strerror (errno));
			    g_free (buf);
			    g_mutex_unlock (conv->mutex);
			    return FALSE;
			}
		    }
		}
		else
		{
		    delete = TRUE;
		}
	    }
	    if (delete)
	    {
		g_remove (ctr->converted_file);
	    }
	    g_free (ctr->converted_file);
	    ctr->converted_file = NULL;
	}

	if (!conv->cachedir)
	{
	    g_mutex_unlock (conv->mutex);
	    g_return_val_if_reached (FALSE);
	}

	basename = g_build_filename (conv->cachedir, ctr->fname_root, NULL);

	g_mutex_unlock (conv->mutex);

	i=0;
	do
	{
	    g_free (fname);
	    fname = g_strdup_printf ("%s-%p-%d.%s",
				     basename,
				     g_thread_self (),
				     i,
				     ctr->fname_extension);
	    ++i;
	} while (g_file_test (fname, G_FILE_TEST_EXISTS));

	rootdir = g_path_get_dirname (fname);

	g_mutex_lock (conv->mutex);
	result = mkdirhier (rootdir, TRUE);
	if (result == FALSE)
	{
	    if (ctr->valid)
	    {
		gchar *buf = conversion_get_track_info (NULL, ctr);
		ctr->errormessage =
		    g_strdup_printf (_("Conversion of '%s' failed: Could not create directory '%s'.\n\n"),
				     buf,
				     rootdir);
		g_free (buf);
	    }
	}
	else
	{
	    ctr->converted_file = fname;
	    fname = NULL;
	}
	g_free (rootdir);
	g_free (fname);
    }
    g_mutex_unlock (conv->mutex);

    return result;
}


/* Used to set the process group ID of the spawned shell, so that we
   can kill the shell as well as all its children in case of an
   abort of the conversion.
   For some reason this does not work with setpgid(child_pid, 0) just
   after the g_spawn_async_with_pipes() and fails with "No such
   process" */
static void pgid_setup (gpointer user_data)
{
    /* this will not work nicely with windows for the reason alone
       that on windows this function is called in the parent process,
       whereas this has to be called in the child process as is done
       in LINUX */
    setpgid (0, 0);
}


/* Convert @ctr.
 *
 * Return value: TRUE if everything went well, FALSE otherwise.
 */
static gboolean conversion_convert_track (Conversion *conv, ConvTrack *ctr)
{
    gboolean result = FALSE;

    g_return_val_if_fail (conv, FALSE);
    g_return_val_if_fail (ctr, FALSE);

    /* set valid output filename */
    if (!conversion_set_valid_filename (conv, ctr))
    {   /* some error occured */
	return FALSE;
    }

    g_mutex_lock (conv->mutex);
    if (ctr->converted_file && ctr->valid)
    {
	if (g_file_test (ctr->converted_file, G_FILE_TEST_EXISTS))
	{   /* a valid converted file already exists. */
	    result = TRUE;
	}
	else
	{   /* start conversion */
	    gchar **argv;
	    GError *error = NULL;
	    GPid child_pid;

	    argv = build_argv_from_strings (ctr->conversion_cmd,
					    "-a", ctr->artist,
					    "-A", ctr->album,
					    "-T", ctr->track_nr,
					    "-t", ctr->title,
					    "-g", ctr->genre,
					    "-y", ctr->year,
					    "-c", ctr->comment,
					    "-f", ctr->converted_file,
					    ctr->orig_file,
					    NULL);

	    result = g_spawn_async_with_pipes (
		                  NULL,         /* working dir    */
				  argv,         /* command line   */
				  NULL,         /* envp           */
				  G_SPAWN_SEARCH_PATH |
				  G_SPAWN_DO_NOT_REAP_CHILD,
				  pgid_setup,   /* setup func     */
				  NULL,         /* user data      */
				  &ctr->pid,    /* child's PID    */
				  NULL,         /* child's stdin  */
				  NULL,         /* child's stdout */
				  &ctr->stderr, /* child's stderr */
				  &error);

	    child_pid = ctr->pid;

	    g_strfreev (argv);
	    argv = NULL;

	    if (result == FALSE)
	    {
		if (ctr->valid)
		{
		    gchar *buf = conversion_get_track_info (NULL, ctr);
		    ctr->errormessage =
			g_strdup_printf (_("Conversion of '%s' failed: '%s'.\n\n"),
					 buf,
					 error->message);
		    g_free (buf);
		}
		g_error_free (error);
	    }
	    else
	    {
		gint status;

		/* set up i/o channel to main thread */
		ctr->gio_channel = g_io_channel_unix_new (ctr->stderr);
		g_io_channel_set_flags (ctr->gio_channel,
					G_IO_FLAG_NONBLOCK, NULL);
		g_io_channel_set_close_on_unref (ctr->gio_channel, TRUE);

		g_mutex_unlock (conv->mutex);

		waitpid (child_pid, &status, 0);

		g_mutex_lock (conv->mutex);

		ctr->pid = 0;

		if (WIFEXITED(status) && (WEXITSTATUS(status) != 0))
		{   /* script exited normally but with an error */
		    if (ctr->valid)
		    {
			gchar *buf = conversion_get_track_info (NULL, ctr);
			ctr->errormessage =
			    g_strdup_printf (_("Conversion of '%s' failed: '%s' returned exit status %d.\n\n"),
					     buf,
					     ctr->conversion_cmd,
					     WEXITSTATUS (status));
			g_free (buf);
		    }
		    if (g_file_test (ctr->converted_file, G_FILE_TEST_EXISTS))
		    {
			g_remove (ctr->converted_file);
		    }
		    g_free (ctr->converted_file);
		    ctr->converted_file = NULL;
		    result = FALSE;
		}
		else if (WIFSIGNALED(status))
		{   /* script was terminated by a signal (killed) */
		    if (g_file_test (ctr->converted_file, G_FILE_TEST_EXISTS))
		    {
			g_remove (ctr->converted_file);
		    }
		    g_free (ctr->converted_file);
		    ctr->converted_file = NULL;
		    result = FALSE;
		}
	    }
	}
    }

    if (result == TRUE)
    {   /* determine size of new file */
	struct stat statbuf;
	if (g_stat (ctr->converted_file, &statbuf) == 0)
	{
	    ctr->converted_size = statbuf.st_size;
	}
	else
	{   /* an error occured after all */
	    gchar *buf = conversion_get_track_info (NULL, ctr);
	    ctr->errormessage = 
		g_strdup_printf (_("Conversion of '%s' failed: could not stat the converted file '%s'.\n\n"),
				 buf,
				 ctr->converted_file);
	    g_free (buf);
	    g_free (ctr->converted_file);
	    ctr->converted_file = NULL;
	    result = FALSE;
	}
    }

    g_mutex_unlock (conv->mutex);

    return result;
}



/* Work through the conv->scheduled list and convert the next waiting
   track.
   As long as there is a track in conv->scheduled, the conversion runs
   through at least once. The dirsize and number of threads is only
   checked _after_ each conversion.
   The next track processed is always the last one in the list -- new
   tracks should be prepended.
*/
static gpointer conversion_thread (gpointer data)
{
    Conversion *conv = data;
    GList *gl;

    g_return_val_if_fail (conv, NULL);

    g_mutex_lock (conv->mutex);

    debug ("%p thread entered\n", g_thread_self ());

    if (conv->scheduled)
    {
	do
	{
	    gboolean conversion_ok;
	    ConvTrack *ctr;

	    debug ("%p thread deep\n", g_thread_self ());

	    /* remove first scheduled entry and add it to processing */
	    gl = g_list_last (conv->scheduled);
	    g_return_val_if_fail (gl, (g_mutex_unlock(conv->mutex), NULL));
	    ctr = gl->data;
	    conv->scheduled = g_list_remove_link (conv->scheduled, gl);
	    g_return_val_if_fail (ctr, (g_mutex_unlock(conv->mutex), NULL));
	    if (ctr->valid)
	    {   /* attach to processing queue */
		conv->processing = g_list_concat (gl, conv->processing);
		/* indicate thread number processing this track */
		ctr->threadnum = g_list_index (conv->threads, g_thread_self());
		gl = NULL; /* gl is not guaranteed to remain the same
			      for a given ctr -- make sure we don't
			      use it by accident */
	    }
	    else
	    {   /* this node is not valid any more */
		conversion_convtrack_free (ctr);
		g_list_free (gl);
		continue;
	    }

	    g_mutex_unlock (conv->mutex);

	    debug ("%p thread converting\n", g_thread_self ());

	    /* convert */
	    conversion_ok = conversion_convert_track (conv, ctr);

	    debug ("%p thread conversion finished (%d:%s)\n",
		   g_thread_self (),
		   conversion_ok,
		   ctr->converted_file);

	    g_mutex_lock (conv->mutex);

	    /* remove from processing queue */
	    gl = g_list_find (conv->processing, ctr);
	    g_return_val_if_fail (gl, (g_mutex_unlock(conv->mutex), NULL));
	    conv->processing = g_list_remove_link (conv->processing, gl);

	    if (ctr->valid)
	    {   /* track is still valid */
		if (conversion_ok)
		{   /* add to converted */
		    conv->converted = g_list_concat (gl, conv->converted);
		}
		else
		{   /* add to failed */
		    conv->failed = g_list_concat (gl, conv->failed);
		}
	    }
	    else
	    {   /* track is no longer valid -> remove converted file
		 * and drop the entry */
		/* remove (converted_file) */
		g_list_free (gl);
		conversion_convtrack_free (ctr);
	    }

	    g_mutex_unlock (conv->mutex);

	    /* clean up directory and recalculate dirsize */
	    conversion_prune_dir (conv);

	    g_mutex_lock (conv->mutex);

	} while ((conv->dirsize <= conv->max_dirsize) &&
		 (conv->threads_num <= conv->max_threads_num) &&
		 (conv->scheduled != NULL));
    }

    /* remove thread from thread list */
    gl = g_list_find (conv->threads, g_thread_self());
    if (gl)
    {
	gl->data = NULL;
    }
    else
    {
	fprintf (stderr, "***** programming error -- g_thread_self not found in threads list\n");
    }

    /* reduce count of running threads */
    --conv->threads_num;

    g_mutex_unlock (conv->mutex);

    debug ("%p thread exit\n", g_thread_self ());

    return NULL;
}
