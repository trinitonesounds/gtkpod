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

#include "gp_itdb.h"
#include "file_convert.h"
#include "misc.h"
#include "misc_track.h"
#include "prefs.h"
#include <errno.h>
#include <glib/gstdio.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <glib/gi18n-lib.h>

#undef DEBUG_CONV
#ifdef DEBUG_CONV
#   define _TO_STR(x) #x
#   define TO_STR(x) _TO_STR(x)
#   define debug(...) do { fprintf(stderr,  __FILE__ ":" TO_STR(__LINE__) ":" __VA_ARGS__); } while(0)
#else
#   define debug(...)
#endif

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
const gchar *FILE_CONVERT_BACKGROUND_TRANSFER = "file_convert_background_transfer";

typedef struct _Conversion Conversion;
typedef struct _ConvTrack ConvTrack;
typedef struct _TransferItdb TransferItdb;

static gboolean conversion_scheduler(gpointer data);
static void conversion_update_default_sizes(Conversion *conv);
static gboolean conversion_log_window_delete(Conversion *conv);
static gpointer conversion_thread(gpointer data);
static gpointer conversion_update_dirsize(gpointer data);
static gpointer conversion_prune_dir(gpointer data);
static void conversion_convtrack_free(ConvTrack *ctr);
static gchar *conversion_get_fname_extension(Conversion *conv, ConvTrack *ctr);
static gboolean conversion_setup_cachedir(Conversion *conv);
static void conversion_log_add_pages(Conversion *conv, gint threads);
static gboolean conversion_add_track(Conversion *conv, Track *track);
static void conversion_prefs_changed(Conversion *conv);
static void conversion_itdb_first(Conversion *conv, iTunesDB *itdb);
static void conversion_cancel_itdb(Conversion *conv, iTunesDB *itdb);
static void conversion_cancel_track(Conversion *conv, Track *track);
static void conversion_continue(Conversion *conv);

static TransferItdb *transfer_get_tri(Conversion *conv, iTunesDB *itdb);
static void transfer_free_transfer_itdb(TransferItdb *tri);
static gpointer transfer_thread(gpointer data);
static GList *transfer_get_failed_tracks(Conversion *conv, iTunesDB *itdb);
static FileTransferStatus
        transfer_get_status(Conversion *conv, iTunesDB *itdb, gint *to_convert_num, gint *converting_num, gint *to_transfer_num, gint *transferred_num, gint *failed_num);
static void transfer_ack_itdb(Conversion *conv, iTunesDB *itdb);
static void transfer_continue(Conversion *conv, iTunesDB *itdb);
static void transfer_activate(Conversion *conv, iTunesDB *itdb, gboolean active);
static void transfer_reset(Conversion *conv, iTunesDB *itdb);
static void transfer_reschedule(Conversion *conv, iTunesDB *itdb);

struct GaplessData {
    guint32 pregap; /* number of pregap samples */
    guint64 samplecount; /* number of actual music samples */
    guint32 postgap; /* number of postgap samples */
    guint32 gapless_data; /* number of bytes from the first sync frame
     * to the 8th to last frame */
    guint16 gapless_track_flag;
};

struct _Conversion {
    GMutex *mutex; /* shared lock                              */
    GList *scheduled; /* tracks scheduled for conversion          */
    GList *processing; /* tracks currently being converted         */
    GList *failed; /* tracks with failed conversion            */
    GList *converted; /* tracks successfully converted but not
     yet unscheduled                          */
    GList *finished; /* tracks unscheduled but not yet
     transferred */
    GCond *finished_cond; /* signals if a new track is added to the
     finished list                            */
    gchar *cachedir; /* directory for converted files            */
    gchar *template; /* name template to use for converted files */
    gint max_threads_num; /* maximum number of allowed threads        */
    GList *threads; /* list of threads                          */
    gint threads_num; /* number of threads currently running      */
    gboolean conversion_force; /* force a new thread to start even if
     the dirsize is too large           */
    gint64 max_dirsize; /* maximum size of cache directory in bytes */
    gint64 dirsize; /* current size of cache directory in bytes */
    gboolean dirsize_in_progress; /* currently determining dirsize      */
    GCond *dirsize_cond; /* signal when dirsize has been updated     */
    gboolean prune_in_progress; /* currently pruning directory        */
    GCond *prune_cond; /* signal when dir has been pruned          */
    gboolean force_prune_in_progress; /* do another prune right after
     the current process finishes   */
    guint timeout_id;
    /* data for log display */
    GtkWidget *log_window; /* display log window                       */
    gboolean log_window_hidden; /* whether the window was closed      */
    gboolean log_window_shown; /* whether the window was closed      */
    gint log_window_posx; /* last x-position of log window            */
    gint log_window_posy; /* last x-position of log window            */
    GtkWidget *notebook; /* notebook                                 */
    GList *textviews; /* list with pages currently added          */
    GList *pages; /* list with pages currently added          */
    GtkStatusbar *log_statusbar; /* statusbar of log display           */
    guint log_context_id; /* context ID for statusbar                 */
    /* data for background transfer */
    GList *transfer_itdbs; /* list with TransferItdbs for background
     transfer                                 */
};

struct _ConvTrack {
    gboolean valid; /* TRUE if orig_track is valid.             */
    gchar *orig_file; /* original filename of unconverted track   */
    gchar *converted_file; /* filename of converted track              */
    gint32 converted_size; /* size of converted file                   */
    gchar *conversion_cmd; /* command to be used for conversion        */
    gboolean must_convert; /* is conversion required for the iPod?     */
    gchar *errormessage; /* error message if any                     */
    gchar *fname_root; /* filename root of converted file          */
    gchar *fname_extension; /* filename extension of converted file     */
    GPid pid; /* PID of child doing the conversion        */
    gint child_stderr; /* stderr of child doing the conversion     */
    Track *track; /* for reference, don't access inside threads! */
    iTunesDB *itdb; /* for reference, don't access inside threads! */
    gint threadnum; /* number of thread working on this track   */
    Conversion *conv; /* pointer back to the conversion struct    */
    GIOChannel *gio_channel;
    guint source_id;
    gchar *artist;
    gchar *album;
    gchar *track_nr;
    gchar *title;
    gchar *genre;
    gchar *year;
    gchar *comment;
    struct GaplessData gapless; /* only used for MP3 */
    /* needed for transfering */
    gchar *dest_filename;
    gchar *mountpoint;
};

struct _TransferItdb {
    gboolean valid; /* TRUE if still valid                  */
    iTunesDB *itdb; /* for reference                        */
    Conversion *conv; /* pointer back to conv                 */
    gboolean transfer; /* OK to transfer in the background?    */
    FileTransferStatus status; /* current status                       */
    GThread *thread; /* thread working on transfer           */
    GList *scheduled; /* ConvTracks scheduled for transfer    */
    GList *processing; /* ConvTracks currently transferring    */
    GList *transferred; /* ConvTracks copied to the iPod        */
    GList *finished; /* ConvTracks copied to the iPod        */
    GList *failed; /* ConvTracks failed to transfer/convert*/
};

enum {
    CONV_DIRSIZE_INVALID = -1,
/* dirsize not valid */
};

static Conversion *conversion = NULL;

/* Set up conversion infrastructure. Must only be called once. */
void file_convert_init() {
    GladeXML *log_xml;
    GtkWidget *vbox;

    if (conversion != NULL)
        return;

    conversion = g_new0 (Conversion, 1);
    conversion->mutex = g_mutex_new ();

    conversion->finished_cond = g_cond_new ();
    conversion->dirsize_cond = g_cond_new ();
    conversion->prune_cond = g_cond_new ();
    conversion_setup_cachedir(conversion);

    if (!prefs_get_string_value(FILE_CONVERT_TEMPLATE, NULL)) {
        prefs_set_string(FILE_CONVERT_TEMPLATE, "%A/%t_%T");
    }

    if (!prefs_get_string_value(FILE_CONVERT_DISPLAY_LOG, NULL)) {
        prefs_set_int(FILE_CONVERT_DISPLAY_LOG, TRUE);
    }

    if (!prefs_get_string_value(FILE_CONVERT_BACKGROUND_TRANSFER, NULL)) {
        prefs_set_int(FILE_CONVERT_BACKGROUND_TRANSFER, TRUE);
    }

    conversion->dirsize = CONV_DIRSIZE_INVALID;

    /* setup log window */
    log_xml = gtkpod_xml_new(gtkpod_get_glade_xml(), "conversion_log");
    conversion->log_window = gtkpod_xml_get_widget(log_xml, "conversion_log");
    gtk_window_set_default_size(GTK_WINDOW (conversion->log_window), prefs_get_int(FILE_CONVERT_LOG_SIZE_X), prefs_get_int(FILE_CONVERT_LOG_SIZE_Y));
    g_signal_connect_swapped (GTK_OBJECT (conversion->log_window), "delete-event",
            G_CALLBACK (conversion_log_window_delete),
            conversion);
    vbox = gtkpod_xml_get_widget(log_xml, "conversion_vbox");
    conversion->notebook = gtk_notebook_new();
    gtk_widget_show(conversion->notebook);
    gtk_box_pack_start(GTK_BOX (vbox), conversion->notebook, TRUE, TRUE, 0);
    conversion->log_window_posx = G_MININT;
    conversion->log_window_posy = G_MININT;
    conversion->log_statusbar = GTK_STATUSBAR (
            gtkpod_xml_get_widget (log_xml,
                    "conversion_statusbar"));
    conversion->log_context_id
            = gtk_statusbar_get_context_id(conversion->log_statusbar, _("Summary status of conversion processes"));
    conversion_log_add_pages(conversion, 1);

    /* initialize values from the preferences */
    file_convert_prefs_changed();

    /* start timeout function for the scheduler */
    conversion->timeout_id = g_timeout_add(100, /* every 100 ms */
    conversion_scheduler, conversion);
    g_object_unref(G_OBJECT (log_xml));
}

/* Shut down conversion infrastructure */
void file_convert_shutdown() {
    if (!conversion)
        return;

    /* nothing to do so far */
    g_free(conversion);

    /* in other words: not sure how we can shut this down... */
}

/* This is called just before gtkpod closes down */
void file_convert_update_default_sizes() {
    file_convert_init();
    conversion_update_default_sizes(conversion);
}

/* Call this function each time the preferences have been updated */
void file_convert_prefs_changed() {
    file_convert_init();
    conversion_prefs_changed(conversion);
}

/* Add @track to the list of tracks to be converted if conversion is
 * necessary.
 *
 * Return value: FALSE if an error occured, TRUE otherwise
 */
gboolean file_convert_add_track(Track *track) {
    file_convert_init();
    return conversion_add_track(conversion, track);
}

/* Reorder the scheduled list so that tracks in @itdb are converted first */
void file_convert_itdb_first(iTunesDB *itdb) {
    file_convert_init();
    conversion_itdb_first(conversion, itdb);
}

/* Cancel conversion for all tracks of @itdb */
void file_convert_cancel_itdb(iTunesDB *itdb) {
    file_convert_init();
    conversion_cancel_itdb(conversion, itdb);
}

/* Cancel conversion for @tracks */
void file_convert_cancel_track(Track *track) {
    file_convert_init();
    conversion_cancel_track(conversion, track);
}

void file_convert_continue() {
    file_convert_init();
    conversion_continue(conversion);
}

/* ----------------------------------------------------------------

 from here on file_transfer_... functions

 ---------------------------------------------------------------- */

/* return current status of transfer process */
FileTransferStatus file_transfer_get_status(iTunesDB *itdb, gint *to_convert_num, gint *converting_num, gint *to_transfer_num, gint *transferred_num, gint *failed_num) {
    return transfer_get_status(conversion, itdb, to_convert_num, converting_num, to_transfer_num, transferred_num, failed_num);
}

/* This has to be called after all tracks have been transferred and the
 iTunesDB has been written, otherwise the transferred tracks will be
 removed again when calling file_convert_cancel_itdb */
void file_transfer_ack_itdb(iTunesDB *itdb) {
    file_convert_init();
    transfer_ack_itdb(conversion, itdb);
}

/* Call this to force transfer to continue in case of a
 * FILE_TRANSFER_DISK_FULL status. Of course, you should make sure
 * additional space is available. */
void file_transfer_continue(iTunesDB *itdb) {
    file_convert_init();
    transfer_continue(conversion, itdb);
}

/* Call this to make sure the transfer process is active independently
 from the settings in the preferences */
void file_transfer_activate(iTunesDB *itdb, gboolean active) {
    file_convert_init();
    transfer_activate(conversion, itdb, active);
}

/* Call this to set the transfer process to on/off as determined by
 * the preferences */
void file_transfer_reset(iTunesDB *itdb) {
    file_convert_init();
    transfer_reset(conversion, itdb);
}

/* Get a list of tracks (Track *) that failed either transfer or
 conversion */
GList *file_transfer_get_failed_tracks(iTunesDB *itdb) {
    file_convert_init();
    return transfer_get_failed_tracks(conversion, itdb);
}

/* Reschedule all tracks for conversion/transfer that have previously
 failed conversion/transfer */
void file_transfer_reschedule(iTunesDB *itdb) {
    file_convert_init();
    transfer_reschedule(conversion, itdb);
}

/* ----------------------------------------------------------------

 from here on down only static functions

 ---------------------------------------------------------------- */

/* Update the prefs with the current size of the log window */
static void conversion_update_default_sizes(Conversion *conv) {
    gint defx, defy;

    g_return_if_fail (conv && conv->log_window);

    g_mutex_lock (conv->mutex);

    gtk_window_get_size(GTK_WINDOW (conv->log_window), &defx, &defy);
    prefs_set_int(FILE_CONVERT_LOG_SIZE_X, defx);
    prefs_set_int(FILE_CONVERT_LOG_SIZE_Y, defy);

    g_mutex_unlock (conv->mutex);
}

/* used to show/hide the log window and adjust the View->menu
 items. g_mutex_lock(conv->mutex) before calling. Used in
 conversion_log_window_delete() and conversion_prefs_changed(). */
static void conversion_display_hide_log_window(Conversion *conv) {
    /* show display log if it was previously hidden and should be
     shown again */
    if (prefs_get_int(FILE_CONVERT_DISPLAY_LOG)) {
        if (conv->log_window_hidden && !conv->log_window_shown) {
            gtk_widget_show(conv->log_window);
            if (conv->log_window_posx != G_MININT) {
                gtk_window_move(GTK_WINDOW (conv->log_window), conv->log_window_posx, conv->log_window_posy);
            }
        }
        conv->log_window_shown = TRUE;
    }
    else {
        if (conv->log_window_shown) { /* window has previously been shown */
            gint posx, posy;
            gtk_window_get_position(GTK_WINDOW (conv->log_window), &posx, &posy);
            conv->log_window_posx = posx;
            conv->log_window_posy = posy;
        }
        conv->log_window_shown = FALSE;
        conv->log_window_hidden = TRUE;
        gtk_widget_hide(conv->log_window);
    }
}

/* Signal when user tries to close the log window */
static gboolean conversion_log_window_delete(Conversion *conv) {
    g_return_val_if_fail (conv, TRUE);

    g_mutex_lock (conv->mutex);

    prefs_set_int(FILE_CONVERT_DISPLAY_LOG, FALSE);
    conversion_display_hide_log_window(conv);

    g_mutex_unlock (conv->mutex);

    return TRUE; /* don't close window -- it will be hidden instead by
     * conversion_display_hid_log_window() */
}

/* set the labels of the notebook of the log window. If required
 * 'g_mutex_lock (conv->mutex)' before calling this function. */
static void conversion_log_set_status(Conversion *conv) {
    GList *glpage, *glthread;
    gchar *buf;

    g_return_if_fail (conv);

    /* Set tab label text to active/inactive */
    glthread = conv->threads;
    for (glpage = conv->pages; glpage; glpage = glpage->next) {
        GtkNotebook *notebook = GTK_NOTEBOOK (conv->notebook);
        GtkWidget *child = glpage->data;
        const gchar *current_label;
        g_return_if_fail (child);

        current_label = gtk_notebook_get_tab_label_text(notebook, child);

        /* in the beginning we may have more pages than thread entries */
        if (glthread && glthread->data) {
            if (!current_label || strcmp(current_label, _("active")) != 0) { /* only change the label if it has changed --
             otherwise our tooltips will be switched off */
                gtk_notebook_set_tab_label_text(notebook, child, _("active"));
            }
        }
        else {
            if (!current_label || strcmp(current_label, _("inactive")) != 0) {
                gtk_notebook_set_tab_label_text(notebook, child, _("inactive"));
            }
        }

        if (glthread) {
            glthread = glthread->next;
        }
    }

    /* Show a summary status */
    gtk_statusbar_pop(conv->log_statusbar, conv->log_context_id);
    buf
            = g_strdup_printf(_("Active threads: %d. Scheduled tracks: %d."), conv->threads_num, g_list_length(conv->scheduled)
                    + g_list_length(conv->processing));
    gtk_statusbar_push(conv->log_statusbar, conv->log_context_id, buf);
    g_free(buf);
}

/* adds pages to the notebook if the number of pages is less than the
 * number of threads. If required 'g_mutex_lock (conv->mutex)' before
 * calling this function. */
static void conversion_log_add_pages(Conversion *conv, gint threads) {
    g_return_if_fail (conv);

    while ((g_list_length(conv->textviews) == 0) || (threads > g_list_length(conv->textviews))) {
        GtkWidget *scrolled_window;
        GtkWidget *textview;

        scrolled_window = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        conv->pages = g_list_append(conv->pages, scrolled_window);
        textview = gtk_text_view_new();
        gtk_container_add(GTK_CONTAINER (scrolled_window), textview);
        conv->textviews = g_list_append(conv->textviews, textview);

        gtk_widget_show_all(scrolled_window);

        gtk_notebook_append_page(GTK_NOTEBOOK (conv->notebook), scrolled_window, NULL);

        conversion_log_set_status(conv);
    }
}

static void conversion_prefs_changed(Conversion *conv) {
    gboolean background_transfer;
    gdouble maxsize;
    GList *gl;

    g_return_if_fail (conv);

    g_mutex_lock (conv->mutex);

    if (prefs_get_double_value(FILE_CONVERT_MAXDIRSIZE, &maxsize)) {
        conv->max_dirsize = 1024 * 1024 * 1024 * maxsize;
    }
    else { /* set default of 4 GB */
        conv->max_dirsize = (gint64) 4 * 1024 * 1024 * 1024;
        prefs_set_double(FILE_CONVERT_MAXDIRSIZE, 4);
    }

    if (conv->max_dirsize < 0) { /* effectively disable caching */
        conv->max_dirsize = 0;
    }

    conv->max_threads_num = prefs_get_int(FILE_CONVERT_MAX_THREADS_NUM);
    if (conv->max_threads_num == 0) { /* set to maximum available number of processors */
        conv->max_threads_num = sysconf(_SC_NPROCESSORS_ONLN);
        /* paranoia mode on */
        if (conv->max_threads_num <= 0) {
            conv->max_threads_num = 1;
        }
    }

    g_free(conv->template);
    conv->template = prefs_get_string(FILE_CONVERT_TEMPLATE);

    if ((conv->dirsize == CONV_DIRSIZE_INVALID) || (conv->dirsize > conv->max_dirsize)) {
        GThread *thread;
        /* Prune dir of unused files if size is too big, calculate and set
         the size of the directory. Do all that in the background. */
        thread = g_thread_create_full(conversion_prune_dir, conv, /* user data  */
        0, /* stack size */
        FALSE, /* joinable   */
        TRUE, /* bound      */
        G_THREAD_PRIORITY_NORMAL, NULL); /* error      */
    }

    background_transfer = prefs_get_int(FILE_CONVERT_BACKGROUND_TRANSFER);
    for (gl = conv->transfer_itdbs; gl; gl = gl->next) {
        TransferItdb *tri = gl->data;
        if (!tri) {
            g_mutex_unlock (conv->mutex);
            g_return_if_reached ();
        }
        tri->transfer = background_transfer;
    }

    conversion_display_hide_log_window(conv);

    g_mutex_unlock (conv->mutex);
}

/* Reorder the scheduled list so that tracks in @itdb are converted first */
static void conversion_itdb_first(Conversion *conv, iTunesDB *itdb) {
    GList *gl;
    GList *gl_itdb = NULL;
    GList *gl_other = NULL;

    g_return_if_fail (conv);
    g_return_if_fail (itdb);

    g_mutex_lock (conv->mutex);
    /* start from the end to keep the same order overall (we're
     prepending to the list for performance reasons */
    for (gl = g_list_last(conv->scheduled); gl; gl = gl->prev) {
        ConvTrack *ctr = gl->data;
        if (!ctr || !ctr->track) {
            g_mutex_unlock (conv->mutex);
            g_return_if_reached ();
        }
        g_return_if_fail (ctr);
        g_return_if_fail (ctr->track);
        if (ctr->track->itdb == itdb) {
            gl_itdb = g_list_prepend(gl_itdb, ctr);
        }
        else {
            gl_other = g_list_prepend(gl_other, ctr);
        }
    }
    g_list_free(conv->scheduled);
    conv->scheduled = g_list_concat(gl_other, gl_itdb);
    g_mutex_unlock (conv->mutex);
}

/* adds @text to the textview on page @threadnum of the log window. If
 * required 'g_mutex_lock (conv->mutex)' before calling this
 * function. */
static void conversion_log_append(Conversion *conv, const gchar *text, gint threadnum) {
    GtkWidget *textview;
    GtkTextBuffer *textbuffer;
    GtkTextIter start, end;
    const gchar *run, *ptr, *next;

    g_return_if_fail (conv);

    /* add pages if necessary */
    conversion_log_add_pages(conv, threadnum + 1);

    /* get appropriate textview */
    textview = g_list_nth_data(conv->textviews, threadnum);
    g_return_if_fail (textview);

    textbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW (textview));
    gtk_text_buffer_get_end_iter(textbuffer, &end);

    run = ptr = text;

    while (*ptr) {
        next = g_utf8_find_next_char(ptr, NULL);
        if (*ptr == '\b') {
            if (ptr > run) {
                gtk_text_buffer_insert(textbuffer, &end, run, ptr - run);
            }
            run = next;
            start = end;
            if (gtk_text_iter_backward_char(&start)) {
                gtk_text_buffer_delete(textbuffer, &start, &end);
            }
        }
        else if (*ptr == '\r') {
            if (ptr > run) {
                gtk_text_buffer_insert(textbuffer, &end, run, ptr - run);
            }
            run = next;
            start = end;
            gtk_text_iter_set_line_offset(&start, 0);
            gtk_text_buffer_delete(textbuffer, &start, &end);
        }
        ptr = next;
    }
    if (ptr > run) {
        gtk_text_buffer_insert(textbuffer, &end, run, ptr - run);
    }
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW (textview), &end, 0.0, FALSE, 0.0, 0.0);

    if (prefs_get_int(FILE_CONVERT_DISPLAY_LOG)) {
        gtk_widget_show(conv->log_window);
    }
}

/*
 * Called by scheduler for all running processes. Lock 'conv->mutex'
 * before calling this function.
 */
static void conversion_display_log(ConvTrack *ctr) {
    gchar buf[PATH_MAX];
    gsize bytes_read = 0;
    GIOStatus status;
    Conversion *conv;

    g_return_if_fail (ctr && ctr->conv);
    conv = ctr->conv;

    do {
        status = g_io_channel_read_chars(ctr->gio_channel, buf, PATH_MAX - 1, &bytes_read, NULL);
        buf[bytes_read] = '\0';
        if (bytes_read > 0) {
            switch (status) {
            case G_IO_STATUS_ERROR:
                /*	    puts ("gio error");*/
                break;
            case G_IO_STATUS_EOF:
                conversion_log_append(conv, buf, ctr->threadnum);
                break;
            case G_IO_STATUS_NORMAL:
                conversion_log_append(conv, buf, ctr->threadnum);
                break;
            case G_IO_STATUS_AGAIN:
                break;
            }
        }
    }
    while (bytes_read > 0);

    return;
}

static void conversion_cancel_mark_track(ConvTrack *ctr) {
    g_return_if_fail (ctr && ctr->track);

    if (ctr->valid) {
        ExtraTrackData *etr = ctr->track->userdata;
        g_return_if_fail (etr);
        ctr->valid = FALSE;
        if (ctr->pid) { /* if a conversion or transfer process is running kill
         * the entire process group (i.e. all processes
         * started within the shell) */
            kill(-ctr->pid, SIGTERM);
            etr->conversion_status = FILE_CONVERT_KILLED;
        }
        /* if a file has already been copied remove it again */
        if (ctr->dest_filename) {
            g_unlink(ctr->dest_filename);
            g_free(ctr->dest_filename);
            ctr->dest_filename = NULL;
        }
    }
}

/* called by conversion_cancel_itdb to mark nodes invalid that are in
 the specified itdb */
static void conversion_cancel_itdb_fe(gpointer data, gpointer userdata) {
    ConvTrack *ctr = data;
    iTunesDB *itdb = userdata;

    g_return_if_fail (ctr && ctr->track && ctr->track->userdata);

    if (ctr->itdb == itdb) {
        conversion_cancel_mark_track(ctr);
    }
}

/* Marks all tracks in @*ctracks as invalid. If a dest_filename
 * exists, the file is removed. If @remove is TRUE, the element is
 * removed from the list altogether. */
static void conversion_cancel_itdb_sub(GList **ctracks, gboolean remove) {
    GList *gl, *next;

    g_return_if_fail (ctracks);

    for (gl = *ctracks; gl; gl = next) {
        ConvTrack *ctr = gl->data;
        g_return_if_fail (ctr);
        next = gl->next;
        conversion_cancel_mark_track(ctr);
        if (remove) {
            *ctracks = g_list_delete_link(*ctracks, gl);
            conversion_convtrack_free(ctr);
        }
    }
}

/* Cancel conversion for all tracks of @itdb */
static void conversion_cancel_itdb(Conversion *conv, iTunesDB *itdb) {
    TransferItdb *itr;

    g_return_if_fail (conv);
    g_return_if_fail (itdb);

    g_mutex_lock (conv->mutex);

    g_list_foreach(conv->scheduled, conversion_cancel_itdb_fe, itdb);
    g_list_foreach(conv->processing, conversion_cancel_itdb_fe, itdb);
    g_list_foreach(conv->failed, conversion_cancel_itdb_fe, itdb);
    g_list_foreach(conv->converted, conversion_cancel_itdb_fe, itdb);

    itr = transfer_get_tri(conv, itdb);
    conversion_cancel_itdb_sub(&itr->scheduled, TRUE);
    conversion_cancel_itdb_sub(&itr->processing, FALSE);
    conversion_cancel_itdb_sub(&itr->transferred, FALSE);
    conversion_cancel_itdb_sub(&itr->finished, TRUE);
    conversion_cancel_itdb_sub(&itr->failed, TRUE);
    itr->valid = FALSE;

    g_mutex_unlock (conv->mutex);
}

/* called by conversion_cancel_track to mark nodes invalid that refer
 to @track (should only be one...) */
/* Used by conversion-cancel_track_sub() to find the list element that
 contains @track. Returns 0 if found. */
static int conversion_cancel_track_cmp(gconstpointer a, gconstpointer b) {
    const ConvTrack *ctr = a;
    const Track *track = b;

    g_return_val_if_fail (ctr, 0);

    if (ctr->track == track)
        return 0;
    return -1;
}

/* Finds @track in @ctracks and marks it as invalid. If a
 * dest_filename exists, the file is removed. If @remove is TRUE, the
 * element is removed from the list altogether. */
static void conversion_cancel_track_sub(GList **ctracks, Track *track, gboolean remove) {
    GList *gl;

    g_return_if_fail (track && track->userdata);

    gl = g_list_find_custom(*ctracks, track, conversion_cancel_track_cmp);
    if (gl) {
        ConvTrack *ctr = gl->data;
        g_return_if_fail (ctr);
        if (ctr->track == track) {
            conversion_cancel_mark_track(ctr);
        }
        if (remove) {
            *ctracks = g_list_delete_link(*ctracks, gl);
            conversion_convtrack_free(ctr);
        }
    }
}

/* Cancel conversion for @track */
static void conversion_cancel_track(Conversion *conv, Track *track) {
    g_return_if_fail (conv);
    g_return_if_fail (track);

    g_mutex_lock (conv->mutex);

    conversion_cancel_track_sub(&conv->scheduled, track, FALSE);
    conversion_cancel_track_sub(&conv->processing, track, FALSE);
    conversion_cancel_track_sub(&conv->failed, track, FALSE);
    conversion_cancel_track_sub(&conv->converted, track, FALSE);
    conversion_cancel_track_sub(&conv->finished, track, TRUE);
    if (track->itdb) {
        TransferItdb *itr = transfer_get_tri(conv, track->itdb);
        conversion_cancel_track_sub(&itr->scheduled, track, TRUE);
        conversion_cancel_track_sub(&itr->processing, track, FALSE);
        conversion_cancel_track_sub(&itr->transferred, track, FALSE);
        conversion_cancel_track_sub(&itr->finished, track, TRUE);
        conversion_cancel_track_sub(&itr->failed, track, TRUE);
    }
    g_mutex_unlock (conv->mutex);
}

/* Force the conversion process to continue even if the allocated
 * disk space is used up */
static void conversion_continue(Conversion *conv) {
    g_return_if_fail (conv);

    g_mutex_lock (conv->mutex);
    if (conv->threads_num == 0) { /* make sure at least one conversion is started even if
     directory is full */
        conv->conversion_force = TRUE;
    }
    g_mutex_unlock (conv->mutex);
}

/* Add @track to the list of tracks to be converted if conversion is
 * necessary.
 *
 * Return value: FALSE if an error occured, TRUE otherwise
 */
static gboolean conversion_add_track(Conversion *conv, Track *track) {
    ExtraTrackData *etr;
    ConvTrack *ctr;
    gchar *conversion_cmd = NULL;
    const gchar *typestr = NULL;
    gboolean convert = FALSE, must_convert = FALSE;
    gboolean result = TRUE;
    FileType *filetype;

    debug ("entering conversion_add_track\n");

    g_return_val_if_fail (conv, FALSE);
    g_return_val_if_fail (track, FALSE);
    g_return_val_if_fail (track->itdb, FALSE);
    etr = track->userdata;
    g_return_val_if_fail (etr, FALSE);

    debug ("considering '%s' to conversion/transfer list\n", etr->pc_path_locale);

    if ((track->itdb->usertype & GP_ITDB_TYPE_LOCAL) || (track->transferred)) {
        debug ("adding aborted: lcl:%d, trsfrd:%d\n",
                track->itdb->usertype & GP_ITDB_TYPE_LOCAL,
                track->transferred);
        /* no conversion or transfer needed */
        return TRUE;
    }

    /* Create ConvTrack structure */
    ctr = g_new0 (ConvTrack, 1);
    ctr->track = track;
    ctr->itdb = track->itdb;
    ctr->conv = conv;
    ctr->orig_file = g_strdup(etr->pc_path_locale);
    ctr->converted_file = g_strdup(etr->converted_file);
    ctr->artist = g_strdup(track->artist);
    ctr->album = g_strdup(track->album);
    ctr->track_nr = g_strdup_printf("%02d", track->track_nr);
    ctr->title = g_strdup(track->title);
    ctr->genre = g_strdup(track->genre);
    ctr->year = g_strdup(etr->year_str);
    ctr->comment = g_strdup(track->comment);
    ctr->valid = TRUE;

    if (!etr->pc_path_locale || (strlen(etr->pc_path_locale) == 0)) {
        gchar *buf = get_track_info(track, FALSE);
        gtkpod_warning(_("Original filename not available for '%s.'\n"), buf);
        g_free(buf);

        etr->conversion_status = FILE_CONVERT_FAILED;
        /* add to failed list */
        g_mutex_lock (conv->mutex);
        conv->failed = g_list_prepend(conv->failed, ctr);
        g_mutex_unlock (conv->mutex);
        debug ("added track to failed %p\n", track);
        return FALSE;
    }

    filetype = determine_filetype(etr->pc_path_locale);

    if (!g_file_test(etr->pc_path_locale, G_FILE_TEST_IS_REGULAR) || !filetype) {
        gchar *buf = get_track_info(track, FALSE);
        gtkpod_warning(_("Filename '%s' is no longer valid for '%s'.\n"), etr->pc_path_utf8, buf);
        g_free(buf);

        etr->conversion_status = FILE_CONVERT_FAILED;
        /* add to failed list */
        g_mutex_lock (conv->mutex);
        conv->failed = g_list_prepend(conv->failed, ctr);
        g_mutex_unlock (conv->mutex);
        debug ("added track to failed %p\n", track);
        return FALSE;
    }

    /* Find the correct script for conversion */
    if (!filetype_can_convert(filetype)) {
        /* we don't convert these (yet) */
        etr->conversion_status = FILE_CONVERT_INACTIVE;
        /* add to finished */
        g_mutex_lock (conv->mutex);
        conv->finished = g_list_prepend(conv->finished, ctr);
        g_mutex_unlock (conv->mutex);
        debug ("added track to finished %p\n", track);
        return TRUE;
    }

    conversion_cmd = filetype_get_conversion_cmd(filetype);
    convert = conversion_cmd && conversion_cmd[0];

    ctr->must_convert = must_convert;
    ctr->conversion_cmd = conversion_cmd;
    conversion_cmd = NULL;

    if (convert) {
        gchar *template;

        g_mutex_lock (conv->mutex);
        template = g_strdup(conv->template);
        g_mutex_unlock (conv->mutex);

        ctr->fname_root = get_string_from_template(track, template, TRUE, TRUE);
        ctr->fname_extension = conversion_get_fname_extension(NULL, ctr);
        if (ctr->fname_extension) {
            etr->conversion_status = FILE_CONVERT_SCHEDULED;
            /* add to scheduled list */
            g_mutex_lock (conv->mutex);
            conv->scheduled = g_list_prepend(conv->scheduled, ctr);
            g_mutex_unlock (conv->mutex);

            result = TRUE;
            debug ("added track %p\n", track);
        }
        else { /* an error has occured */
            if (ctr->errormessage) {
                gtkpod_warning(ctr->errormessage);
                debug("Conversion error: %s\n", ctr->errormessage);
                g_free(ctr->errormessage);
                ctr->errormessage = NULL;
            }

            if (must_convert)
                etr->conversion_status = FILE_CONVERT_REQUIRED_FAILED;
            else
                etr->conversion_status = FILE_CONVERT_FAILED;
            /* add to failed list */
            g_mutex_lock (conv->mutex);
            conv->failed = g_list_prepend(conv->failed, ctr);
            g_mutex_unlock (conv->mutex);
            result = FALSE;
            debug ("added track to failed %p\n", track);
        }
        g_free(template);
    }
    else if (must_convert) {
        gchar *buf = get_track_info(track, FALSE);
        g_return_val_if_fail (typestr, FALSE);
        gtkpod_warning(_("Files of type '%s' are not supported by the iPod. Please go to the Preferences to set up and turn on a suitable conversion script for '%s'.\n\n"), typestr, buf);
        g_free(buf);

        etr->conversion_status = FILE_CONVERT_REQUIRED;
        /* add to failed list */
        g_mutex_lock (conv->mutex);
        conv->failed = g_list_prepend(conv->failed, ctr);
        g_mutex_unlock (conv->mutex);
        result = FALSE;
        debug ("added track to failed %p\n", track);
    }
    else {
        etr->conversion_status = FILE_CONVERT_INACTIVE;
        /* remove reference to any former converted file */
        g_free(etr->converted_file);
        etr->converted_file = NULL;
        g_free(ctr->converted_file);
        ctr->converted_file = NULL;
        /* add to finished */
        g_mutex_lock (conv->mutex);
        conv->finished = g_list_prepend(conv->finished, ctr);
        g_mutex_unlock (conv->mutex);
        debug ("added track to finished %p\n", track);
    }
    g_free(conversion_cmd);
    return result;
}

/* free the memory taken by @ctr */
static void conversion_convtrack_free(ConvTrack *ctr) {
    g_return_if_fail (ctr);
    g_free(ctr->orig_file);
    g_free(ctr->converted_file);
    g_free(ctr->conversion_cmd);
    g_free(ctr->fname_root);
    g_free(ctr->fname_extension);
    g_free(ctr->errormessage);
    g_free(ctr->artist);
    g_free(ctr->album);
    g_free(ctr->track_nr);
    g_free(ctr->title);
    g_free(ctr->genre);
    g_free(ctr->year);
    g_free(ctr->comment);
    if (ctr->gio_channel) {
        g_io_channel_unref(ctr->gio_channel);
    }
    if (ctr->pid) {
        g_spawn_close_pid(ctr->pid);
    }

    /* transfer stuff */
    g_free(ctr->dest_filename);
    g_free(ctr->mountpoint);

    g_free(ctr);
}

/* return some sensible input about @ctrack. You must free the
 returned string after use. */
static gchar *conversion_get_track_info(ConvTrack *ctr) {
    gchar *str = NULL;

    if ((ctr->title && strlen(ctr->title))) {
        str = g_strdup(ctr->title);
    }
    else if ((ctr->album && strlen(ctr->album))) {
        str = g_strdup_printf("%s_%s", ctr->track_nr, ctr->album);
    }
    else if ((ctr->artist && strlen(ctr->artist))) {
        str = g_strdup_printf("%s_%s", ctr->track_nr, ctr->artist);
    }
    else {
        str = g_strdup(_("No information available"));
    }

    return str;
}

/* Set and set up the conversion cachedir.
 *
 * Return value: TRUE if directory could be set up.
 *               FALSE if an error occured.
 */
static gboolean conversion_setup_cachedir(Conversion *conv) {
    g_return_val_if_fail (conv, FALSE);

    g_mutex_lock (conv->mutex);

    g_free(conv->cachedir);
    conv->cachedir = NULL;
    conv->cachedir = prefs_get_string(FILE_CONVERT_CACHEDIR);
    if (!conv->cachedir) {
        gchar *cfgdir = prefs_get_cfgdir();
        if (cfgdir) {
            conv->cachedir = g_build_filename(cfgdir, "conversion_cache", NULL);
            g_free(cfgdir);
        }
    }
    if (conv->cachedir) {
        if (!g_file_test(conv->cachedir, G_FILE_TEST_IS_DIR)) {
            if ((g_mkdir(conv->cachedir, 0777)) == -1) {
                gtkpod_warning(_("Could not create '%s'. Filetype conversion will not work.\n"), conv->cachedir);
                g_free(conv->cachedir);
                conv->cachedir = NULL;
            }
        }
    }

    if (conv->cachedir) {
        prefs_set_string(FILE_CONVERT_CACHEDIR, conv->cachedir);
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
static void conversion_free_resources(ConvTrack *ctr) {
    g_return_if_fail (ctr);

    /* Free resources */
    if (ctr->pid) {
        g_spawn_close_pid(ctr->pid);
        ctr->pid = 0;
    }
    if (ctr->gio_channel) {
        conversion_display_log(ctr);
        g_io_channel_unref(ctr->gio_channel);
        ctr->gio_channel = NULL;
    }
}

/* the scheduler code without the locking mechanism -- has to be
 called with conv->mutex locked */
static gboolean conversion_scheduler_unlocked(Conversion *conv) {
    GList *gli, *nextgli;

    g_return_val_if_fail (conv, TRUE);
    debug("Conversion scheduler unlocked\n");

    if (!conv->cachedir) {
        /* Cachedir is not available. Not good! Remove the timeout function. */
        g_return_val_if_reached (FALSE);
    }

    if (conv->dirsize == CONV_DIRSIZE_INVALID) { /* dirsize has not been set up. Wait until that has been done. */
        return TRUE;
    }

    if (conv->scheduled) {
        debug("Conversion scheduled. Setting up thread\n");
        if ((conv->threads_num < conv->max_threads_num) && ((conv->dirsize <= conv->max_dirsize)
                || conv->conversion_force)) {
            GList *gl;
            GThread *thread;

            thread = g_thread_create_full(conversion_thread, conv, /* user data  */
            0, /* stack size */
            FALSE, /* joinable   */
            TRUE, /* bound      */
            G_THREAD_PRIORITY_LOW, NULL); /* error      */

            /* Add thread to thread list. Use first available slot */
            gl = g_list_find(conv->threads, NULL);
            if (gl) { /* found empty slot */
                gl->data = thread;
            }
            else { /* no empty slot available --> add to end */
                conv->threads = g_list_append(conv->threads, thread);
            }

            ++conv->threads_num;
        }
    }
    else {
        conv->conversion_force = FALSE;
    }

    if (conv->processing) {
        debug("Conversion is processing\n");
        GList *gl;
        for (gl = conv->processing; gl; gl = gl->next) {
            ConvTrack *ctr = gl->data;
            g_return_val_if_fail (ctr, TRUE);
            if (ctr->valid && ctr->gio_channel) {
                conversion_display_log(ctr);
            }
        }
    }

    if (conv->failed) {
        debug("Conversion has failed\n");
        GList *gl;
        for (gl = conv->failed; gl; gl = gl->next) {
            ConvTrack *ctr = gl->data;
            g_return_val_if_fail (ctr, TRUE);

            /* free resources (pid, flush io-channel) */
            conversion_free_resources(ctr);

            if (ctr->valid) {
                ExtraTrackData *etr;
                if (ctr->errormessage) {
                    gtkpod_warning(ctr->errormessage);
                    debug("Conversion error: %s\n", ctr->errormessage);
                    g_free(ctr->errormessage);
                    ctr->errormessage = NULL;
                }
                g_return_val_if_fail (ctr->track && ctr->track->userdata, TRUE);
                etr = ctr->track->userdata;
                if (ctr->must_convert) {
                    if (etr->conversion_status != FILE_CONVERT_REQUIRED) {
                        etr->conversion_status = FILE_CONVERT_REQUIRED_FAILED;
                    }
                }
                else {
                    etr->conversion_status = FILE_CONVERT_FAILED;
                }
                /* Add to finished so we can find it in case user
                 waits for next converted file */
                conv->finished = g_list_prepend(conv->finished, ctr);
            }
            else { /* track is not valid any more */
                conversion_convtrack_free(ctr);
            }
        }
        g_list_free(conv->failed);
        conv->failed = NULL;
    }

    if (conv->converted) {
        debug("Conversion has successfully converted\n");
        GList *gl;
        for (gl = conv->converted; gl; gl = gl->next) {
            ConvTrack *ctr = gl->data;
            g_return_val_if_fail (ctr, TRUE);

            /* free resources (pid / flush io-channel) */
            conversion_free_resources(ctr);

            if (ctr->valid) {
                GList *trackgl, *tracks;

                g_return_val_if_fail (ctr->track, TRUE);

                tracks = gp_itdb_find_same_tracks_in_itdbs(ctr->track);
                for (trackgl = tracks; trackgl; trackgl = trackgl->next) {
                    ExtraTrackData *etr;
                    Track *tr = trackgl->data;
                    g_return_val_if_fail (tr && tr->itdb && tr->userdata,
                            TRUE);
                    etr = tr->userdata;

                    /* spread information to local databases for
                     future reference */
                    if (tr->itdb->usertype & GP_ITDB_TYPE_LOCAL) {
                        g_free(etr->converted_file);
                        etr->converted_file = g_strdup(ctr->converted_file);
                        gtkpod_track_updated(tr);
                        data_changed(tr->itdb);
                    }

                    /* don't forget to copy conversion data to the
                     track itself */
                    if (tr == ctr->track) {
                        g_free(etr->converted_file);
                        etr->converted_file = g_strdup(ctr->converted_file);
                        etr->conversion_status = FILE_CONVERT_CONVERTED;
                        tr->size = ctr->converted_size;
                        tr->pregap = ctr->gapless.pregap;
                        tr->samplecount = ctr->gapless.samplecount;
                        tr->postgap = ctr->gapless.postgap;
                        tr->gapless_data = ctr->gapless.gapless_data;
                        tr->gapless_track_flag = ctr->gapless.gapless_track_flag;
                        gtkpod_track_updated(tr);
                        data_changed(tr->itdb);
                    }
                }
                g_list_free(tracks);
                /* add ctr to finished */
                conv->finished = g_list_prepend(conv->finished, ctr);
            }
            else { /* track is not valid any more */
                conversion_convtrack_free(ctr);
            }
        }
        g_list_free(conv->converted);
        conv->converted = NULL;
    }

    if (conv->finished) {
        debug("Conversion has finished converting\n");
        GList *gl;
        for (gl = conv->finished; gl; gl = gl->next) {
            ConvTrack *ctr = gl->data;
            g_return_val_if_fail (ctr, TRUE);
            if (ctr->valid) {
                TransferItdb *tri;
                ExtraTrackData *etr;
                Track *tr = ctr->track;
                g_return_val_if_fail (tr && tr->itdb && tr->userdata, TRUE);
                etr = tr->userdata;
                /* broadcast finished track */
                g_cond_broadcast (conv->finished_cond);

                tri = transfer_get_tri(conv, tr->itdb);
                g_return_val_if_fail (tri, TRUE);
                /* Provide mountpoint */
                ctr->mountpoint = g_strdup(itdb_get_mountpoint(ctr->itdb));

                switch (etr->conversion_status) {
                case FILE_CONVERT_INACTIVE:
                case FILE_CONVERT_CONVERTED:
                    tri->scheduled = g_list_prepend(tri->scheduled, ctr);
                    break;
                case FILE_CONVERT_FAILED:
                case FILE_CONVERT_REQUIRED_FAILED:
                case FILE_CONVERT_REQUIRED:
                    tri->failed = g_list_prepend(tri->failed, ctr);
                    break;
                case FILE_CONVERT_KILLED:
                case FILE_CONVERT_SCHEDULED:
                    fprintf(stderr, _("Programming error, conversion type %d not expected in conversion_scheduler()\n"), etr->conversion_status);
                    conversion_convtrack_free(ctr);
                    break;
                }
            }
            else { /* track is not valid any more */
                conversion_convtrack_free(ctr);
            }
        }
        g_list_free(conv->finished);
        conv->finished = NULL;
    }

    for (gli = conv->transfer_itdbs; gli; gli = nextgli) {
        TransferItdb *tri = gli->data;
        nextgli = gli->next;

        g_return_val_if_fail (tri, TRUE);
        if (tri->scheduled) {
            if ((tri->thread == NULL) && (tri->transfer == TRUE) && (tri->status != FILE_TRANSFER_DISK_FULL)) { /* start new thread */
                tri->thread = g_thread_create_full(transfer_thread, tri, /* user data  */
                0, /* stack size */
                FALSE, /* joinable   */
                TRUE, /* bound      */
                G_THREAD_PRIORITY_LOW, NULL); /* error      */
            }
        }

        if (tri->transferred) {
            GList *gl;
            for (gl = tri->transferred; gl; gl = gl->next) {
                ConvTrack *ctr = gl->data;
                GError *error;
                g_return_val_if_fail (ctr, TRUE);

                if (tri->valid && ctr->valid) {
                    if (itdb_cp_finalize(ctr->track, NULL, ctr->dest_filename, &error)) { /* everything's fine */
                        tri->finished = g_list_prepend(tri->finished, ctr);
                        /* otherwise new free space status from iPod
                         is never read and free space keeps
                         increasing while we copy more and more
                         files to the iPod */
                        debug ("transfer finalized: %s (%d)\n",
                                conversion_get_track_info (ctr),
                                ctr->track->transferred);
                    }
                    else { /* error!? */
                        tri->failed = g_list_prepend(tri->failed, ctr);
                        gchar *buf = conversion_get_track_info(ctr);
                        gtkpod_warning(_("Transfer of '%s' failed. %s\n\n"), buf, error ? error->message : "");
                        g_free(buf);
                        if (error) {
                            g_error_free(error);
                        }
                    }
                    gtkpod_track_updated(ctr->track);
                    data_changed(ctr->itdb);
                }
                else { /* track is not valid any more */
                    conversion_convtrack_free(ctr);
                }
            }
            g_list_free(tri->transferred);
            tri->transferred = NULL;
        }

        if (tri->failed) {
            GList *gl;
            for (gl = tri->failed; gl; gl = gl->next) {
                ConvTrack *ctr = gl->data;
                g_return_val_if_fail (ctr, TRUE);

                if (tri->valid && ctr->valid) {
                    if (ctr->errormessage) {
                        gtkpod_warning(ctr->errormessage);
                        debug("Conversion error: %s\n", ctr->errormessage);
                        g_free(ctr->errormessage);
                        ctr->errormessage = NULL;
                    }
                    g_free(ctr->dest_filename);
                    ctr->dest_filename = NULL;
                    tri->finished = g_list_prepend(tri->finished, ctr);
                }
                else { /* track is not valid any more */
                    conversion_convtrack_free(ctr);
                }
            }
            g_list_free(tri->failed);
            tri->failed = NULL;
        }

        /* remove TransferItdb completely if invalid and all tracks
         * have been removed */
        if (!tri->valid) {
            if (!!tri->scheduled && !tri->processing && !tri->transferred && !tri->finished && !tri->failed
                    && !tri->thread) {
                transfer_free_transfer_itdb(tri);
                conv->transfer_itdbs = g_list_delete_link(conv->transfer_itdbs, gli);
            }
        }
    }

    /* update the log window */
    if (conv->log_window_shown) {
        conversion_log_set_status(conv);
    }

    return TRUE;
}

/* timeout function, add new threads if unscheduled tracks are in the
 queue and maximum number of threads hasn't been reached */
static gboolean conversion_scheduler(gpointer data) {
    Conversion *conv = data;
    gboolean result;
    g_return_val_if_fail (data, FALSE);

//    debug ("conversion_scheduler enter\n");

    gdk_threads_enter();
    if (!g_mutex_trylock(conv->mutex)) {
        gdk_threads_leave();
        return FALSE;
    }

    result = conversion_scheduler_unlocked(conv);

    g_mutex_unlock (conv->mutex);
    gdk_threads_leave();

//    debug ("conversion_scheduler exit\n");

    return result;
}

/* Calculate the size of the directory */
static gpointer conversion_update_dirsize(gpointer data) {
    Conversion *conv = data;
    gchar *dir;
    gint64 size = 0;

    g_return_val_if_fail (conv, NULL);

    debug ("%p update_dirsize enter\n", g_thread_self ());

    g_mutex_lock (conv->mutex);
    if (conv->dirsize_in_progress) { /* another thread is already working on the directory
     size. We'll wait until it has finished and just return. */
        g_cond_wait (conv->dirsize_cond, conv->mutex);
        g_mutex_unlock (conv->mutex);
        debug ("%p update_dirsize concurrent exit\n", g_thread_self ());
        return NULL;
    }
    conv->dirsize_in_progress = TRUE;
    dir = g_strdup(conv->cachedir);
    g_mutex_unlock (conv->mutex);

    if (dir) {
        debug ("%p update_dirsize getting size of dir (%s)\n",
                g_thread_self (), dir);

        size = get_size_of_directory(dir);
        g_free(dir);
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

struct conversion_prune_file {
    gchar *filename;
    struct stat statbuf;
};

/* make a list of all existing files */
static GList *conversion_prune_dir_collect_files(const gchar *dir) {
    GDir *gdir;
    const gchar *fname;
    GList *files = NULL;

    g_return_val_if_fail (dir, NULL);

    gdir = g_dir_open(dir, 0, NULL);
    if (!gdir) {
        /* do something */
        return files;
    }

    while ((fname = g_dir_read_name(gdir))) {
        gchar *fullname = g_build_filename(dir, fname, NULL);

        if (g_file_test(fullname, G_FILE_TEST_IS_DIR)) {
            files = g_list_concat(files, conversion_prune_dir_collect_files(fullname));
        }
        else if (g_file_test(fullname, G_FILE_TEST_IS_REGULAR)) {
            struct conversion_prune_file *cpf;
            cpf = g_new0 (struct conversion_prune_file, 1);
            if (g_stat(fullname, &cpf->statbuf) == 0) { /* OK, add */
                cpf->filename = fullname;
                fullname = NULL;
                files = g_list_prepend(files, cpf);
            }
            else { /* error -- ignore this file */
                g_free(cpf);
            }
        }
        g_free(fullname);
    }

    g_dir_close(gdir);

    return files;
}

/* used to sort the list of unneeded files so that the oldest ones
 come first */
static gint conversion_prune_compfunc(gconstpointer a, gconstpointer b) {
    const struct conversion_prune_file *cpf_a = a;
    const struct conversion_prune_file *cpf_b = b;

    return cpf_a->statbuf.st_mtime - cpf_b->statbuf.st_mtime;
}

/* free struct conversion_prune_file structure data, called from
 g_list_foreach */
static void conversion_prune_freefunc(gpointer data, gpointer user_data) {
    struct conversion_prune_file *cpf = data;
    g_return_if_fail (cpf);
    g_free(cpf->filename);
    g_free(cpf);
}

/* Add tracks still needed to hash. Called by conversion_prune_dir() */
static void conversion_prune_needed_add(GHashTable *hash_needed_files, GList *ctracks) {
    GList *gl;

    g_return_if_fail (hash_needed_files);

    for (gl = ctracks; gl; gl = gl->next) {
        ConvTrack *ctr = gl->data;
        g_return_if_fail (ctr);
        if (ctr->valid && ctr->converted_file) {
            g_hash_table_insert(hash_needed_files, g_strdup(ctr->converted_file), ctr);
        }
    }
}

/* Prune the directory of unused files */
static gpointer conversion_prune_dir(gpointer data) {
    Conversion *conv = data;
    gchar *dir;
    gint64 dirsize;
    gint64 maxsize;

    g_return_val_if_fail (conv, NULL);

    debug ("%p prune_dir enter\n", g_thread_self ());

    g_mutex_lock (conv->mutex);
    if (conv->prune_in_progress) { /* another thread is already working on the directory
     prune. We'll wait until it has finished and just return. */
        g_cond_wait (conv->prune_cond, conv->mutex);
        g_mutex_unlock (conv->mutex);
        return NULL;
    }
    conv->prune_in_progress = TRUE;
    dir = g_strdup(conv->cachedir);
    g_mutex_unlock (conv->mutex);

    if (dir) {
        GHashTable *hash_needed_files;
        GList *gl;
        GList *files = NULL;

        debug ("%p prune_dir creating list of available files\n", g_thread_self ());

        /* make a list of all available files */
        files = conversion_prune_dir_collect_files(dir);

        debug ("%p prune_dir creating hash_needed_files\n", g_thread_self ());

        /* make a hash of all the files still needed */
        hash_needed_files = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
        g_mutex_lock (conv->mutex);

        /* add needed files to hash */
        conversion_prune_needed_add(hash_needed_files, conv->scheduled);
        conversion_prune_needed_add(hash_needed_files, conv->processing);
        conversion_prune_needed_add(hash_needed_files, conv->converted);
        conversion_prune_needed_add(hash_needed_files, conv->finished);
        for (gl = conv->transfer_itdbs; gl; gl = gl->next) {
            TransferItdb *tri = gl->data;
            g_return_val_if_fail (tri, (conv->prune_in_progress = FALSE,
                            g_cond_broadcast (conv->prune_cond),
                            g_mutex_unlock(conv->mutex),
                            NULL));
            conversion_prune_needed_add(hash_needed_files, tri->scheduled);
            conversion_prune_needed_add(hash_needed_files, tri->processing);
            conversion_prune_needed_add(hash_needed_files, tri->failed);
        }

        g_mutex_unlock (conv->mutex);

        /* sort the list so that the oldest files are first */
        files = g_list_sort(files, conversion_prune_compfunc);

        /* get an up-to-date count of the directory size */
        conversion_update_dirsize(conv);

        g_mutex_lock (conv->mutex);
        dirsize = conv->dirsize;
        maxsize = conv->max_dirsize;
        g_mutex_unlock (conv->mutex);

        debug ("%p prune_dir removing files (%lld/%lld)\n",
                g_thread_self (), (long long int)dirsize, (long long int)maxsize);

        /* remove files until dirsize is smaller than maxsize */
        /* ... */
        for (gl = files; gl && (dirsize > maxsize); gl = gl->next) {
            struct conversion_prune_file *cpf = gl->data;
            g_return_val_if_fail (cpf, (conv->prune_in_progress = FALSE,
                            g_cond_broadcast (conv->prune_cond),
                            NULL));
            g_return_val_if_fail (cpf->filename,
                    (conv->prune_in_progress = FALSE,
                            g_cond_broadcast (conv->prune_cond),
                            NULL));
            if (g_hash_table_lookup(hash_needed_files, cpf->filename) == NULL) { /* file is not among those remove */
                if (g_remove(cpf->filename) == 0) {
                    dirsize -= cpf->statbuf.st_size;
                }
            }
        }

        debug ("%p prune_dir removed files (%lld/%lld)\n",
                g_thread_self (), (long long int)dirsize, (long long int)maxsize);

        /* free all data */
        g_list_foreach(files, conversion_prune_freefunc, NULL);
        g_list_free(files);
        g_hash_table_destroy(hash_needed_files);
        g_free(dir);
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
static gchar *conversion_get_fname_extension(Conversion *conv, ConvTrack *ctr) {
    gchar **argv;
    gboolean result;
    gchar *fname_extension = NULL;
    gchar *std_out = NULL;
    gint exit_status = 0;
    GError *error = NULL;

    g_return_val_if_fail (ctr, NULL);

    /* get filename extension */
    if (conv)
        g_mutex_lock (conv->mutex);
    argv = build_argv_from_strings(ctr->conversion_cmd, "-x", NULL);
    if (conv)
        g_mutex_unlock (conv->mutex);

    result = g_spawn_sync(NULL, /* working dir */
    argv, /* argv        */
    NULL, /* envp        */
    G_SPAWN_SEARCH_PATH, /* flags: use user's path */
    NULL, /* child_setup function   */
    NULL, /* user data              */
    &std_out, NULL, &exit_status, &error);

    if (result == FALSE) {
        if (conv)
            g_mutex_lock (conv->mutex);
        if (ctr->valid) {
            gchar *buf = conversion_get_track_info(ctr);
            ctr->errormessage = g_strdup_printf(_("Conversion of '%s' failed: '%s'.\n\n"), buf, error->message);
            debug("Conversion error: %s\n", ctr->errormessage);
            g_free(buf);
        }
        if (conv)
            g_mutex_unlock (conv->mutex);
        g_error_free(error);
    }
    else if (
    WEXITSTATUS (exit_status) != 0) {
        if (conv)
            g_mutex_lock (conv->mutex);
        if (ctr->valid) {
            gchar *buf = conversion_get_track_info(ctr);
            ctr->errormessage
                    = g_strdup_printf(_("Conversion of '%s' failed: '%s %s' returned exit status %d.\n\n"), buf, argv[0], argv[1],
                    WEXITSTATUS (exit_status));
            debug("Conversion error: %s\n", ctr->errormessage);
            g_free(buf);
        }
        if (conv)
            g_mutex_unlock (conv->mutex);
        result = FALSE;
    }

    /* retrieve filename extension */
    if (std_out && (strlen(std_out) > 0)) {
        gint len = strlen(std_out);
        fname_extension = std_out;
        std_out = NULL;
        if (fname_extension[len - 1] == '\n') {
            fname_extension[len - 1] = 0;
        }
    }

    if (!fname_extension || (strlen(fname_extension) == 0)) { /* no a valid filename extension */
        if (conv)
            g_mutex_lock (conv->mutex);
        if (ctr->valid) {
            gchar *buf = conversion_get_track_info(ctr);
            ctr->errormessage
                    = g_strdup_printf(_("Conversion of '%s' failed: '\"%s\" %s' did not return filename extension as expected.\n\n"), buf, argv[0], argv[1]);
            debug("Conversion error: %s\n", ctr->errormessage);
            g_free(buf);
        }
        if (conv)
            g_mutex_unlock (conv->mutex);
        result = FALSE;
    }

    g_strfreev(argv);
    argv = NULL;

    if (result == FALSE) {
        g_free(std_out);
        g_free(fname_extension);
        fname_extension = NULL;
        ;
    }

    return fname_extension;
}

/* Sets a valid filename.

 Return value: TRUE if everything went fine. In this case
 ctr->converted_filename contains the filename to use. If that file
 already exists no conversion is necessary.
 FALSE is returned if some error occured, in which case
 ctr->errormessage may be set. */
static gboolean conversion_set_valid_filename(Conversion *conv, ConvTrack *ctr) {
    gboolean result = TRUE;

    g_return_val_if_fail (conv, FALSE);
    g_return_val_if_fail (ctr, FALSE);

    g_mutex_lock (conv->mutex);
    if (ctr->valid) {
        gint i;
        gchar *rootdir;
        gchar *basename;
        gchar *fname = NULL;

        /* Check if converted_filename already exists. If yes, only
         keep it if the file is newer than the original file and the
         filename extension matches the intended
         conversion. Otherwise remove it and set a new filename */
        if (ctr->converted_file) {
            gboolean delete = FALSE;
            if (g_file_test(ctr->converted_file, G_FILE_TEST_IS_REGULAR)) {
                struct stat conv_stat, orig_stat;
                if (g_str_has_suffix(ctr->converted_file, ctr->fname_extension)) {
                    if (stat(ctr->converted_file, &conv_stat) == 0) {
                        if (stat(ctr->orig_file, &orig_stat) == 0) {
                            if (conv_stat.st_mtime > orig_stat.st_mtime) { /* converted file is newer than
                             * original file */
                                g_mutex_unlock (conv->mutex);
                                return TRUE;
                            }
                            else { /* converted file is older than original file */
                                delete = TRUE;
                            }
                        }
                        else { /* error reading original file */
                            char *buf = conversion_get_track_info(ctr);
                            ctr->errormessage
                                    = g_strdup_printf(_("Conversion of '%s' failed: Could not access original file '%s' (%s).\n\n"), buf, ctr->orig_file, strerror(errno));
                            debug("Conversion error: %s\n", ctr->errormessage);
                            g_free(buf);
                            g_mutex_unlock (conv->mutex);
                            return FALSE;
                        }
                    }
                }
                else {
                    delete = TRUE;
                }
            }
            if (delete) {
                g_remove(ctr->converted_file);
            }
            g_free(ctr->converted_file);
            ctr->converted_file = NULL;
        }

        if (!conv->cachedir) {
            g_mutex_unlock (conv->mutex);
            g_return_val_if_reached (FALSE);
        }

        basename = g_build_filename(conv->cachedir, ctr->fname_root, NULL);

        g_mutex_unlock (conv->mutex);

        i = 0;
        do {
            g_free(fname);
            fname = g_strdup_printf("%s-%p-%d.%s", basename, g_thread_self(), i, ctr->fname_extension);
            ++i;
        }
        while (g_file_test(fname, G_FILE_TEST_EXISTS));

        rootdir = g_path_get_dirname(fname);

        g_mutex_lock (conv->mutex);
        result = mkdirhier(rootdir, TRUE);
        if (result == FALSE) {
            if (ctr->valid) {
                gchar *buf = conversion_get_track_info(ctr);
                ctr->errormessage
                        = g_strdup_printf(_("Conversion of '%s' failed: Could not create directory '%s'.\n\n"), buf, rootdir);
                debug("Conversion error: %s\n", ctr->errormessage);
                g_free(buf);
            }
        }
        else {
            ctr->converted_file = fname;
            fname = NULL;
        }
        g_free(rootdir);
        g_free(fname);
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
static void pgid_setup(gpointer user_data) {
    /* this will not work nicely with windows for the reason alone
     that on windows this function is called in the parent process,
     whereas this has to be called in the child process as is done
     in LINUX */
    setpgid(0, 0);
}

/* Convert @ctr.
 *
 * Return value: TRUE if everything went well, FALSE otherwise.
 */
static gboolean conversion_convert_track(Conversion *conv, ConvTrack *ctr) {
    gboolean result = FALSE;
    FileType *filetype;

    g_return_val_if_fail (conv, FALSE);
    g_return_val_if_fail (ctr, FALSE);

    /* set valid output filename */
    if (!conversion_set_valid_filename(conv, ctr)) { /* some error occured */
        return FALSE;
    }

    g_mutex_lock (conv->mutex);
    if (ctr->converted_file && ctr->valid) {
        if (g_file_test(ctr->converted_file, G_FILE_TEST_EXISTS)) { /* a valid converted file already exists. */
            result = TRUE;
            g_mutex_unlock (conv->mutex);
        }
        else { /* start conversion */
            gchar **argv;
            GError *error = NULL;
            GPid child_pid;

            argv
                    = build_argv_from_strings(ctr->conversion_cmd, "-a", ctr->artist, "-A", ctr->album, "-T", ctr->track_nr, "-t", ctr->title, "-g", ctr->genre, "-y", ctr->year, "-c", ctr->comment, "-f", ctr->converted_file, ctr->orig_file, NULL);

            result = g_spawn_async_with_pipes(NULL, /* working dir    */
            argv, /* command line   */
            NULL, /* envp           */
            G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_STDOUT_TO_DEV_NULL, pgid_setup, /* setup func     */
            NULL, /* user data      */
            &ctr->pid, /* child's PID    */
            NULL, /* child's stdin  */
            NULL, /* child's stdout */
            &ctr->child_stderr, /* child's stderr */
            &error);

            child_pid = ctr->pid;

            g_strfreev(argv);
            argv = NULL;

            if (result == FALSE) {
                if (ctr->valid) {
                    gchar *buf = conversion_get_track_info(ctr);
                    ctr->errormessage = g_strdup_printf(_("Conversion of '%s' failed: '%s'.\n\n"), buf, error->message);
                    debug("Conversion error: %s\n", ctr->errormessage);
                    g_free(buf);
                }
                g_error_free(error);
            }
            else {
                gint status;

                /* set up i/o channel to main thread */
                ctr->gio_channel = g_io_channel_unix_new(ctr->child_stderr);
                g_io_channel_set_flags(ctr->gio_channel, G_IO_FLAG_NONBLOCK, NULL);
                g_io_channel_set_close_on_unref(ctr->gio_channel, TRUE);

                g_mutex_unlock (conv->mutex);

                waitpid(child_pid, &status, 0);

                g_mutex_lock (conv->mutex);

                ctr->pid = 0;

                if (
                WIFEXITED(status) && (
                WEXITSTATUS(status) != 0)) { /* script exited normally but with an error */
                    if (ctr->valid) {
                        gchar *buf = conversion_get_track_info(ctr);
                        ctr->errormessage
                                = g_strdup_printf(_("Conversion of '%s' failed: '%s' returned exit status %d.\n\n"), buf, ctr->conversion_cmd,
                                WEXITSTATUS (status));
                        debug("Conversion error: %s\n", ctr->errormessage);
                        g_free(buf);
                    }
                    if (g_file_test(ctr->converted_file, G_FILE_TEST_EXISTS)) {
                        g_remove(ctr->converted_file);
                    }
                    g_free(ctr->converted_file);
                    ctr->converted_file = NULL;
                    result = FALSE;
                }
                else if (WIFSIGNALED(status)) { /* script was terminated by a signal (killed) */
                    if (g_file_test(ctr->converted_file, G_FILE_TEST_EXISTS)) {
                        g_remove(ctr->converted_file);
                    }
                    g_free(ctr->converted_file);
                    ctr->converted_file = NULL;
                    result = FALSE;
                }
            }
            g_mutex_unlock (conv->mutex);
        }
    }

    if (result == TRUE) { /* determine size of new file */
        g_mutex_lock (conv->mutex);
        struct stat statbuf;
        if (g_stat(ctr->converted_file, &statbuf) == 0) {
            ctr->converted_size = statbuf.st_size;
        }
        else { /* an error occured after all */
            gchar *buf = conversion_get_track_info(ctr);
            ctr->errormessage
                    = g_strdup_printf(_("Conversion of '%s' failed: could not stat the converted file '%s'.\n\n"), buf, ctr->converted_file);
            debug("Conversion error: %s\n", ctr->errormessage);
            g_free(buf);
            g_free(ctr->converted_file);
            ctr->converted_file = NULL;
            result = FALSE;
        }
        g_mutex_unlock (conv->mutex);
    }

    /* Fill in additional info (currently only gapless info for MP3s */
    if (result == TRUE) {
        Track *track;
        gboolean retval;

        filetype = determine_filetype(ctr->converted_file);
        if (filetype) {
            g_mutex_lock (conv->mutex);
            track = gp_track_new();
            retval = filetype_read_gapless(filetype, ctr->converted_file, track);

            if (ctr->valid && (retval == TRUE)) {
                ctr->gapless.pregap = track->pregap;
                ctr->gapless.samplecount = track->samplecount;
                ctr->gapless.postgap = track->postgap;
                ctr->gapless.gapless_data = track->gapless_data;
                ctr->gapless.gapless_track_flag = track->gapless_track_flag;
            }

            itdb_track_free(track);
            g_mutex_unlock (conv->mutex);
        }
    }

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
static gpointer conversion_thread(gpointer data) {
    Conversion *conv = data;
    GList *gl;

    g_return_val_if_fail (conv, NULL);

    g_mutex_lock (conv->mutex);

    debug ("%p thread entered\n", g_thread_self ());

    if (conv->scheduled) {
        do {
            gboolean conversion_ok;
            ConvTrack *ctr;

            debug ("%p thread deep\n", g_thread_self ());

            /* remove first scheduled entry and add it to processing */
            gl = g_list_last(conv->scheduled);
            g_return_val_if_fail (gl, (g_mutex_unlock(conv->mutex), NULL));
            ctr = gl->data;
            conv->scheduled = g_list_remove_link(conv->scheduled, gl);
            g_return_val_if_fail (ctr, (g_mutex_unlock(conv->mutex), NULL));
            if (ctr->valid) { /* attach to processing queue */
                conv->processing = g_list_concat(gl, conv->processing);
                /* indicate thread number processing this track */
                ctr->threadnum = g_list_index(conv->threads, g_thread_self());
                gl = NULL; /* gl is not guaranteed to remain the same
                 for a given ctr -- make sure we don't
                 use it by accident */
            }
            else { /* this node is not valid any more */
                conversion_convtrack_free(ctr);
                g_list_free(gl);
                continue;
            }

            conv->conversion_force = FALSE;

            g_mutex_unlock (conv->mutex);

            debug ("%p thread converting\n", g_thread_self ());

            /* convert */
            conversion_ok = conversion_convert_track(conv, ctr);

            debug ("%p thread conversion finished (%d:%s)\n",
                    g_thread_self (),
                    conversion_ok,
                    ctr->converted_file);

            g_mutex_lock (conv->mutex);

            /* remove from processing queue */
            gl = g_list_find(conv->processing, ctr);
            g_return_val_if_fail (gl, (g_mutex_unlock(conv->mutex), NULL));
            conv->processing = g_list_remove_link(conv->processing, gl);

            if (ctr->valid) { /* track is still valid */
                if (conversion_ok) { /* add to converted */
                    conv->converted = g_list_concat(gl, conv->converted);
                }
                else { /* add to failed */
                    conv->failed = g_list_concat(gl, conv->failed);
                    /* remove (converted_file) */
                    g_idle_add((GSourceFunc) gp_remove_track_cb, ctr->track);
                }
            }
            else { /* track is no longer valid -> remove converted file
             * and drop the entry */
                /* remove (converted_file) */
                g_idle_add((GSourceFunc) gp_remove_track_cb, ctr->track);
                g_list_free(gl);
                conversion_convtrack_free(ctr);
            }

            g_mutex_unlock (conv->mutex);

            /* clean up directory and recalculate dirsize */
            conversion_prune_dir(conv);

            g_mutex_lock (conv->mutex);

        }
        while (((conv->dirsize <= conv->max_dirsize) || conv->conversion_force) && (conv->threads_num
                <= conv->max_threads_num) && (conv->scheduled != NULL));
    }

    /* remove thread from thread list */
    gl = g_list_find(conv->threads, g_thread_self());
    if (gl) {
        gl->data = NULL;
    }
    else {
        fprintf(stderr, _("***** programming error -- g_thread_self not found in threads list\n"));
    }

    /* reset force flag if we weren't cancelled because of too many
     threads */
    if (conv->threads_num <= conv->max_threads_num)
        conv->conversion_force = FALSE;

    /* reduce count of running threads */
    --conv->threads_num;

    g_mutex_unlock (conv->mutex);

    debug ("%p thread exit\n", g_thread_self ());

    return NULL;
}

/* ------------------------------------------------------------
 *
 * Background-transfer specific code
 *
 * ------------------------------------------------------------*/

/* Count the number of ConvTracks in @list that belong to @itdb */
static gint transfer_get_status_count(iTunesDB *itdb, GList *list) {
    GList *gl;
    gint count = 0;
    for (gl = list; gl; gl = gl->next) {
        ConvTrack *ctr = gl->data;
        g_return_val_if_fail (ctr, 0);
        if (ctr->valid && (ctr->itdb == itdb)) {
            ++count;
        }
    }
    return count;
}

/* return the status of the current transfer process or -1 when an
 * assertion fails. */
static FileTransferStatus transfer_get_status(Conversion *conv, iTunesDB *itdb, gint *to_convert_num, gint *converting_num, gint *to_transfer_num, gint *transferred_num, gint *failed_num) {
    TransferItdb *tri;
    FileTransferStatus status;

    g_return_val_if_fail (conv && itdb, -1);

    g_mutex_lock (conv->mutex);

    tri = transfer_get_tri(conv, itdb);
    g_return_val_if_fail (tri, (g_mutex_unlock (conv->mutex), -1));
    status = tri->status;

    if (to_convert_num) {
        *to_convert_num = transfer_get_status_count(itdb, conv->scheduled)
                + transfer_get_status_count(itdb, conv->processing);
    }

    if (converting_num) {
        *converting_num = transfer_get_status_count(itdb, conv->processing);
    }

    if (to_transfer_num) {
        *to_transfer_num = transfer_get_status_count(itdb, conv->converted)
                + transfer_get_status_count(itdb, conv->finished) + transfer_get_status_count(itdb, tri->scheduled)
                + transfer_get_status_count(itdb, tri->processing);
    }

    if (transferred_num || failed_num) {
        GList *gl;
        gint transferred = 0;
        gint failed = 0;

        for (gl = tri->finished; gl; gl = gl->next) {
            ConvTrack *ctr = gl->data;
            g_return_val_if_fail (ctr, (g_mutex_unlock (conv->mutex), -1));

            if (ctr->valid) {
                if (ctr->track->transferred)
                    ++transferred;
                else
                    ++failed;
            }
        }

        if (transferred_num) {
            *transferred_num = transferred + transfer_get_status_count(itdb, tri->transferred);
        }

        if (failed_num) {
            *failed_num = failed + transfer_get_status_count(itdb, conv->failed)
                    + transfer_get_status_count(itdb, tri->failed);
        }
    }

    g_mutex_unlock (conv->mutex);

    return status;
}

/* removes all transferred tracks of @itdb to make sure the files will
 not be removed from the iPod when file_convert_cancel is
 called. This function will wait until all tracks in the
 'transferred' list have been finalized and put into 'finished'. */
static void transfer_ack_itdb(Conversion *conv, iTunesDB *itdb) {
    GList *gl;
    TransferItdb *tri;

    g_return_if_fail (conv && itdb);

    g_mutex_lock (conv->mutex);

    tri = transfer_get_tri(conv, itdb);
    if (!tri) {
        g_mutex_unlock (conv->mutex);
        g_return_if_reached ();
    }

    if (tri->transferred) { /* finalize the tracks by calling the scheduler directly */
        conversion_scheduler_unlocked(conv);
    }

    for (gl = tri->finished; gl; gl = gl->next) {
        ConvTrack *ctr = gl->data;
        if (!ctr) {
            g_mutex_unlock (conv->mutex);
            g_return_if_reached ();
        }
        conversion_convtrack_free(ctr);
    }
    g_list_free(tri->finished);
    tri->finished = NULL;

    g_mutex_unlock (conv->mutex);
}

/* Get a list of all failed tracks. Examine etr->conversion_status to
 see whether the conversion or transfer has failed. In the latter
 conversion_status is either FILE_CONVERT_INACTIVE or
 FILE_CONVERT_CONVERTED. You must g_list_free() the returned list
 after use. */
static GList *transfer_get_failed_tracks(Conversion *conv, iTunesDB *itdb) {
    TransferItdb *tri;
    GList *gl;
    GList *tracks = NULL;

    g_return_val_if_fail (conv && itdb, NULL);

    g_mutex_lock (conv->mutex);

    tri = transfer_get_tri(conv, itdb);
    g_return_val_if_fail (tri, (g_mutex_unlock (conv->mutex), NULL));

    /* Make sure all failed tracks are forwarded to tri->finished */
    conversion_scheduler_unlocked(conv);

    for (gl = tri->finished; gl; gl = gl->next) {
        ConvTrack *ctr = gl->data;
        g_return_val_if_fail (ctr, (g_mutex_unlock (conv->mutex), NULL));

        if (ctr->valid) {
            if (!ctr->dest_filename) {
                tracks = g_list_prepend(tracks, ctr->track);
            }
        }
    }

    g_mutex_unlock (conv->mutex);

    return tracks;
}

static void transfer_reschedule(Conversion *conv, iTunesDB *itdb) {
    TransferItdb *tri;
    GList *gl, *next;
    GList *tracks = NULL;

    g_return_if_fail (conv && itdb);

    g_mutex_lock (conv->mutex);

    tri = transfer_get_tri(conv, itdb);
    if (!tri) {
        g_mutex_unlock (conv->mutex);
        g_return_if_reached ();
    }

    if (conv->failed || tri->failed) { /* move the tracks over to tri->finished by calling the
     scheduler directly */
        conversion_scheduler_unlocked(conv);
    }

    for (gl = tri->finished; gl; gl = next) {
        ConvTrack *ctr = gl->data;
        next = gl->next;
        if (!ctr) {
            g_mutex_unlock (conv->mutex);
            g_return_if_reached ();
        }

        if (ctr->valid) {
            if (!ctr->dest_filename) {
                ExtraTrackData *etr;
                if (!ctr->track || !ctr->track->userdata) {
                    g_mutex_unlock (conv->mutex);
                    g_return_if_reached ();
                }
                etr = ctr->track->userdata;
                switch (etr->conversion_status) {
                case FILE_CONVERT_INACTIVE:
                case FILE_CONVERT_CONVERTED:
                    /* This track failed during transfer */
                    tri->finished = g_list_remove_link(tri->finished, gl);
                    tri->scheduled = g_list_concat(gl, tri->scheduled);
                    break;
                default:
                    /* This track failed during conversion */
                    tri->finished = g_list_delete_link(tri->finished, gl);
                    tracks = g_list_prepend(tracks, ctr->track);
                    conversion_convtrack_free(ctr);
                    break;
                }
            }
        }
    }

    g_mutex_unlock (conv->mutex);

    /* reschedule all failed conversion tracks */
    for (gl = tracks; gl; gl = gl->next) {
        conversion_add_track(conv, gl->data);
    }

    g_list_free(tracks);
}

/* reset the FILE_TRANSFER_DISK_FULL status to force the continuation
 of the transfering process even if the disk was previously full. At
 the same time the conversion process is forced to restart even if
 the disk space was already used up. */
static void transfer_continue(Conversion *conv, iTunesDB *itdb) {
    TransferItdb *tri;

    g_return_if_fail (conv && itdb);

    g_mutex_lock (conv->mutex);

    tri = transfer_get_tri(conv, itdb);
    if (!tri) {
        g_mutex_unlock (conv->mutex);
        g_return_if_reached ();
    }

    /* signal to continue transfer even if disk was full previously */
    if (tri->status == FILE_TRANSFER_DISK_FULL) {
        tri->status = FILE_TRANSFER_IDLE;
    }

    /* make sure at least one thread is started even if the
     * dirsize is too large */
    if (conv->threads_num == 0)
        conv->conversion_force = TRUE;

    g_mutex_unlock (conv->mutex);
}

/* set the tri->transfer flag to TRUE */
static void transfer_activate(Conversion *conv, iTunesDB *itdb, gboolean active) {
    TransferItdb *tri;

    g_return_if_fail (itdb);

    // Initialise Conversion infrastructure is not already initialised.
    file_convert_init();

    g_mutex_lock (conv->mutex);

    tri = transfer_get_tri(conv, itdb);
    if (!tri) {
        g_mutex_unlock (conv->mutex);
        g_return_if_reached ();
    }

    /* signal to continue transfer even if disk was full previously */
    tri->transfer = active;

    g_mutex_unlock (conv->mutex);
}

/* set the tri->transfer flag to whatever the preferences settings are */
static void transfer_reset(Conversion *conv, iTunesDB *itdb) {
    TransferItdb *tri;

    g_return_if_fail (conv && itdb);

    g_mutex_lock (conv->mutex);

    tri = transfer_get_tri(conv, itdb);
    if (!tri) {
        g_mutex_unlock (conv->mutex);
        g_return_if_reached ();
    }

    /* signal to continue transfer even if disk was full previously */
    tri->transfer = prefs_get_int(FILE_CONVERT_BACKGROUND_TRANSFER);

    g_mutex_unlock (conv->mutex);
}

/* You must free the GLists before calling this function. */
static void transfer_free_transfer_itdb(TransferItdb *tri) {
    g_return_if_fail (tri);
    g_return_if_fail (!tri->scheduled && !tri->processing &&
            !tri->transferred && !tri->finished &&
            !tri->failed);
    g_return_if_fail (tri->thread);

    g_free(tri);
}

/* Add a TransferItdb entry for @itdb. conv->mutex must be locked. */
static TransferItdb *transfer_add_itdb(Conversion *conv, iTunesDB *itdb) {
    TransferItdb *tri;

    g_return_val_if_fail (conv && itdb, NULL);

    tri = g_new0 (TransferItdb, 1);
    tri->transfer = prefs_get_int(FILE_CONVERT_BACKGROUND_TRANSFER);
    tri->status = FILE_TRANSFER_IDLE;
    tri->valid = TRUE;
    tri->conv = conv;
    tri->itdb = itdb;
    conv->transfer_itdbs = g_list_prepend(conv->transfer_itdbs, tri);

    return tri;
}

static int transfer_get_tri_cmp(gconstpointer a, gconstpointer b) {
    const TransferItdb *tri = a;
    const iTunesDB *itdb = b;

    g_return_val_if_fail (tri, 0);

    if (tri->itdb == itdb)
        return 0;
    return -1;
}

/* Return a pointer to the TransferItdb belonging to @itdb. You must
 * lock conv->mutex yourself. If no entry exists, one is created. If
 * the TransferItdb is invalid, a new one is created. */
static TransferItdb *transfer_get_tri(Conversion *conv, iTunesDB *itdb) {
    GList *link;

    g_return_val_if_fail (conv && itdb, NULL);

    link = g_list_find_custom(conv->transfer_itdbs, itdb, transfer_get_tri_cmp);

    if (link) {
        TransferItdb *tri = link->data;
        g_return_val_if_fail (tri, NULL);
        /* only return if valid */
        if (tri->valid)
            return tri;
    }

    /* no entry exists -- let's create one */
    return transfer_add_itdb(conv, itdb);
}

/* force at least one new call to conversion_prune_dir */
static gpointer transfer_force_prune_dir(gpointer data) {
    Conversion *conv = data;

    g_mutex_lock (conv->mutex);

    if (conv->prune_in_progress) { /* wait until current prune process is finished before calling
     conversion_prune_dir() again */
        if (conv->force_prune_in_progress) { /* we already have another process waiting  */
            g_mutex_unlock (conv->mutex);
            return NULL;
        }
        conv->force_prune_in_progress = TRUE;
        g_cond_wait (conv->prune_cond, conv->mutex);
        conv->force_prune_in_progress = FALSE;
    }

    g_mutex_unlock (conv->mutex);

    return conversion_prune_dir(conv);
}

/* return value:
 FILE_TRANSFER_DISK_FULL: file could not be copied because the iPod
 is full. tri->status is set as well.
 FILE_TRANSFER_ERROR: another error occurred
 FILE_TRANSFER_ACTIVE: copy went fine.
 */
static FileTransferStatus transfer_transfer_track(TransferItdb *tri, ConvTrack *ctr) {
    FileTransferStatus result = FILE_TRANSFER_ERROR;
    gboolean copy_success;
    const gchar *source_file = NULL;
    gchar *dest_file = NULL;
    gchar *mountpoint = NULL;
    Conversion *conv;
    GError *error = NULL;

    g_return_val_if_fail (tri && tri->conv, result);
    g_return_val_if_fail (ctr, result);

    conv = tri->conv;

    g_mutex_lock (conv->mutex);

    if (ctr->converted_file) {
        source_file = ctr->converted_file;
    }
    else {
        source_file = ctr->orig_file;
    }

    mountpoint = g_strdup(ctr->mountpoint);

    g_mutex_unlock (conv->mutex);

    g_return_val_if_fail (source_file && mountpoint, FALSE);

    dest_file = itdb_cp_get_dest_filename(NULL, mountpoint, source_file, &error);

    /* an error occurred */
    if (!dest_file) {
        g_mutex_lock (conv->mutex);
        if (error) {
            ctr->errormessage = g_strdup_printf("%s\n", error->message);
            debug("Conversion error: %s\n", ctr->errormessage);
            g_error_free(error);
        }
        g_mutex_unlock (conv->mutex);
        return result;
    }

    copy_success = itdb_cp(source_file, dest_file, &error);

    g_mutex_lock (conv->mutex);

    if (!copy_success) {
        if (error && (error->code == G_FILE_ERROR_NOSPC)) { /* no space left on device */
            tri->status = FILE_TRANSFER_DISK_FULL;
            result = FILE_TRANSFER_DISK_FULL;
        }
        else {
            if (ctr->valid) {
                gchar *buf = conversion_get_track_info(ctr);
                ctr->errormessage
                        = g_strdup_printf(_("Transfer of '%s' failed. %s\n\n"), buf, error ? error->message : "");
                debug("Conversion error: %s\n", ctr->errormessage);
                g_free(buf);
            }
        }
        if (error) {
            g_error_free(error);
            error = NULL;
        }
    }
    else { /* copy was successful */
        debug ("%p copied\n", tri->itdb);
        if (ctr->valid) {
            ctr->dest_filename = dest_file;
            dest_file = NULL;
            result = FILE_TRANSFER_ACTIVE;
        }
    }

    g_mutex_unlock (conv->mutex);

    if (dest_file) { /* unsuccessful -- remove destination file if exists */
        if (g_file_test(dest_file, G_FILE_TEST_EXISTS)) {
            g_remove(dest_file);
        }
    }

    g_free(dest_file);
    g_free(mountpoint);

    return result;
}

static gpointer transfer_thread(gpointer data) {
    TransferItdb *tri = data;
    Conversion *conv;

    g_return_val_if_fail (tri && tri->conv, NULL);
    conv = tri->conv;

    g_mutex_lock (conv->mutex);

    debug ("%p transfer thread enter\n", tri->itdb);

    while (tri->scheduled && (tri->transfer == TRUE) && (tri->status != FILE_TRANSFER_DISK_FULL)) {
        GList *gl;
        ConvTrack *ctr;
        FileTransferStatus status;

        /* reset transfer force flag */
        tri->status = FILE_TRANSFER_ACTIVE;

        /* remove first scheduled entry and add it to processing */
        gl = g_list_last(tri->scheduled);
        g_return_val_if_fail (gl, (g_mutex_unlock(conv->mutex), NULL));

        ctr = gl->data;
        tri->scheduled = g_list_remove_link(tri->scheduled, gl);
        g_return_val_if_fail (ctr, (g_mutex_unlock(conv->mutex), NULL));
        if (tri->valid && ctr->valid) { /* attach to processing queue */
            tri->processing = g_list_concat(gl, tri->processing);
            /* indicate thread number processing this track */
            gl = NULL; /* gl is not guaranteed to remain the same
             for a given ctr -- make sure we don't
             use it by accident */
        }
        else { /* this node is not valid any more */
            conversion_convtrack_free(ctr);
            g_list_free(gl);
            continue;
        }

        g_mutex_unlock (conv->mutex);

        debug ("%p thread transfer\n", ctr->itdb);

        status = transfer_transfer_track(tri, ctr);

        debug ("%p thread transfer finished (%d:%s)\n",
                ctr->itdb,
                status,
                ctr->dest_filename);

        g_mutex_lock (conv->mutex);

        /* remove from processing queue */
        gl = g_list_find(tri->processing, ctr);
        g_return_val_if_fail (gl, (g_mutex_unlock(conv->mutex), NULL));
        tri->processing = g_list_remove_link(tri->processing, gl);

        if (ctr->valid) { /* track is still valid */
            if (status == FILE_TRANSFER_ACTIVE) { /* add to converted */
                tri->transferred = g_list_concat(gl, tri->transferred);
            }
            else if (status == FILE_TRANSFER_DISK_FULL) { /* reschedule */
                tri->scheduled = g_list_concat(tri->scheduled, gl);
            }
            else /* status == -1 */
            { /* add to failed */
                tri->failed = g_list_concat(gl, tri->failed);
            }
        }
        else { /* track is no longer valid -> remove the copied file and
         * drop the track */
            g_list_free(gl);
            if (ctr->dest_filename) {
                g_unlink(ctr->dest_filename);
            }
            conversion_convtrack_free(ctr);
        }

        if (conv->dirsize > conv->max_dirsize) { /* we just transferred a track -- there should be space
         available again -> force a directory prune */
            g_thread_create_full(transfer_force_prune_dir, conv, /* user data  */
            0, /* stack size */
            FALSE, /* joinable   */
            TRUE, /* bound      */
            G_THREAD_PRIORITY_NORMAL, NULL); /* error      */
        }

    }

    tri->thread = NULL;
    if (tri->status != FILE_TRANSFER_DISK_FULL)
        tri->status = FILE_TRANSFER_IDLE;

    g_mutex_unlock (conv->mutex);

    debug ("%p transfer thread exit\n", tri->itdb);

    return NULL;
}
