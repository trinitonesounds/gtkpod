/*
|  Copyright (C) 2002 Corey Donohoe <atmos at atmos.org>
|
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
*/
#include <gtk/gtk.h>
#include <glib.h>
#include "misc.h"
#include "song.h"
#include "md5.h"
#include "threads.h"

/**
 * For debugging Messages
 */
#define THREAD_DEBUG 0

/* How often to check for updates to the thread.  The actual delay will be
 * (INTERVAL_TIMEOUT/1000) Seconds
 */
#define INTERVAL_TIMEOUT 250

/**
 * The number of threads we use in gtkpod
 */
#define GTKPOD_THREAD_MAX 2
/**
 * Status Options for the thread system
 */
enum {
    UNKNOWN = 0,
    RUNNING,
    COMPLETED
};

/* Some variables we use */
static GThreadPool *thread_pool = NULL;
G_LOCK_DEFINE_STATIC(thread_lock);
static guint thread_status = 0;

static gboolean gtkpod_thread_pool_timeout_cb(gpointer data);
static void gtkpod_thread_exec(gpointer data, gpointer user_data);

/**
 * gtkpod_thread_pool_init - initializes the thread pool we're going to
 * utilize through api later, also initializes a mutex so we can lock
 * variables down and be thread safe, or something like that =)
 */
void
gtkpod_thread_pool_init(void)
{
#if THREAD_DEBUG
    fprintf(stderr, "gtkpod_thread_pool_init\n");
#endif
    if(!g_thread_supported())	/* initialize the glib thread subsystem */
	g_thread_init(NULL);
    
    if((g_thread_supported()))	/* now it should be on */
    {
	if(!thread_pool)
	    thread_pool = g_thread_pool_new(gtkpod_thread_exec, NULL,
		    GTKPOD_THREAD_MAX, TRUE, NULL);
	thread_status = UNKNOWN;
    }
}

/**
 * gtkpod_thread_pool_free - free all the threads
 */
void
gtkpod_thread_pool_free(void)
{
	/* thread_status = UNKNOWN; */
    if(thread_pool) g_thread_pool_free(thread_pool, FALSE, FALSE);
}

/**
 * gtkpod_thread_pool_exec - send a predefined threaded request to be run
 * @e - the execution code
 */
void
gtkpod_thread_pool_exec(guint e)
{
    GError *g_err = NULL;

#if THREAD_DEBUG
    fprintf(stderr, "gtkpod_thread_pool_exec(%d)\n", e);
#endif
    if((e < 0) || (e > THREAD_TYPE_COUNT)) /* out of range */
	return;
    if(!thread_pool) return;
    
    g_timeout_add(INTERVAL_TIMEOUT, gtkpod_thread_pool_timeout_cb,
		    GUINT_TO_POINTER(e));
    g_thread_pool_push(thread_pool, GUINT_TO_POINTER(e), &g_err);
}

/** 
 * Timeout callback, when itunes_export_completed is 1, it's time to open up
 * the interface for interaction again.
 * Returns - FALSE when the timer should stop
 * 		TRUE if the timer should continue
 */ 		
static gboolean
gtkpod_thread_pool_timeout_cb(gpointer data)
{
    guint type = (guint)data;
    
#if THREAD_DEBUG
    fprintf(stderr, "thread status %d\ntype %d\n", thread_status, type);
#endif
    g_thread_pool_stop_unused_threads();
    if(thread_status == COMPLETED)
    {
	G_LOCK(thread_lock); 
	thread_status = UNKNOWN;
	G_UNLOCK(thread_lock); 
	gtkpod_main_window_set_active(TRUE);
	return(FALSE);
    }
    return(TRUE);
}

/**
 * gtkpod_thread_exec - the execution of the "sepaate" thread is handled
 * here, based on the value of data we do one of several things.
 * - md5_unique_file_init()
 * - handle_export();
 * - handle_import();
 * - update_id3();  ?? FIXME
 * @data - the enumerated type of thread request.
 * @user_data - unused
 */
static void
gtkpod_thread_exec(gpointer data, gpointer user_data)
{
    gint request = (gint)data;
    
#if THREAD_DEBUG
    fprintf(stderr, "gtkpod_thread_exec(%d,%d)\n", request, (gint)user_data);
#endif
    gtkpod_main_window_set_active(FALSE);
    switch(request)
    {
	case THREAD_ID3:
	    gtkpod_warning("Unable to handle THREAD_ID3 currently\n");
	    break;
	case THREAD_MD5:
	    G_LOCK(thread_lock); 
	    thread_status = RUNNING;
	    G_UNLOCK(thread_lock); 
	    md5_unique_file_init(get_song_list()); 
	    G_LOCK(thread_lock); 
	    thread_status = COMPLETED;
	    G_UNLOCK(thread_lock); 
	    break;
	case THREAD_READ_ITUNES:
	    G_LOCK(thread_lock); 
	    thread_status = RUNNING;
	    G_UNLOCK(thread_lock); 
	    handle_import();
	    G_LOCK(thread_lock); 
	    thread_status = COMPLETED;
	    G_UNLOCK(thread_lock); 
	    break;
	case THREAD_WRITE_ITUNES:
	    G_LOCK(thread_lock); 
	    thread_status = RUNNING;
	    G_UNLOCK(thread_lock); 
	    handle_export();
	    G_LOCK(thread_lock); 
	    thread_status = COMPLETED;
	    G_UNLOCK(thread_lock); 
	    break;
	default:
	    gtkpod_warning("Unknown thread execution requested(%d)\n",
		    request);
	    break;
    }
#if THREAD_DEBUG
    fprintf(stderr, "\nThread Completed(%d)\n", request);
#endif
}
