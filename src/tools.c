/* Time-stamp: <2004-03-21 15:05:15 JST jcs>
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif


#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include "info.h"
#include "math.h"
#include "misc.h"
#include "mp3file.h"
#include "tools.h"
#include "prefs.h"
#include "support.h"
#include <stdlib.h>


/*pipe's definition*/
enum {
    READ = 0,
    WRITE = 1
};
enum {
    BUFLEN = 1000,
};


/* ------------------------------------------------------------

		     Normalize Volume

   ------------------------------------------------------------ */


#ifdef G_THREADS_ENABLED
static  GMutex *mutex = NULL;
static GCond  *cond = NULL;
static gboolean mutex_data = FALSE;
#endif

static gint32 replaygain_to_volume(gint replaygain)
{
    double tv;
    gint32 volume = 0;

    tv = ((double) replaygain) / (5.0 * log10(2.0));
    volume =  floor(tv + 0.5);
		
    if (volume < -500) {
	volume = -500;
    } else {
	if (volume > 500) volume = 500;
    }
/* 		printf("radio_gain: %i\n", track->radio_gain); */
/* 		printf("volume: %i\n", track->volume); */
    return volume;
}


/* this function returns the @track volume */
/* mp3gain version 1.4.2 */
static gboolean nm_mp3gain_calc_gain (Track *track)
{
    gint k,n;  /*for's counter*/
    gchar *filename=NULL; /*track's filename*/
    gchar *mp3gain_path;
    gchar *mp3gain_exec;
    const gchar *mp3gain_set;
    gchar *buf;
    pid_t pid,tpid;
    int ret = 2;
    int status;
    int errsv;

    k=0;
    n=0;

    /* see if full path to mp3gain was set using the prefs dialogue */
    mp3gain_set = prefs_get_mp3gain_path ();
    /* use default if not */
    if (!mp3gain_set || !(*mp3gain_set)) mp3gain_set = "mp3gain";
    /* find full path */
    mp3gain_path = which (mp3gain_set);
    /* show error message if mp3gain cannot be found */
    if (!mp3gain_path)
    {
	gtkpod_warning (_("Could not find mp3gain. I tried to use the following executable: '%s'.\n\nIf the mp3gain executable is not in your path or named differently, you can set the full path in the 'Tools' section of the preferences dialog.\n\nIf you do not have mp3gain installed, you can download it from http://www.sourceforge.net/projects/mp3gain."), mp3gain_set);
	return FALSE;
    }

    mp3gain_exec = g_path_get_basename (mp3gain_path);

    buf = g_malloc (BUFLEN);

    filename=get_track_name_on_disk_verified (track);

    /*fork*/
    pid=fork();

    /*and cast mp3gain*/
    switch (pid)
    {
    case -1: /* parent and error, now what?*/
	break;
    case 0: /*child*/
	/* this call may add a tag to the mp3file!! */
        execl(mp3gain_path, mp3gain_exec, 
			"-q", /* quiet */
			"-k", /* set ReplayGain so that clipping is prevented */
			"-o", /* database friendly output */
			filename, NULL);
	errsv = errno;
        fprintf(stderr, "execl() failed: %s\n", strerror(errsv));
	/* mp3gain (can) return 1 on success. So only values greater 1 can
	 * designate failure. */
	exit(2);
    default: /*parent*/
	tpid = waitpid (pid, &status, 0); /*wait mp3gain termination */
	if WIFEXITED(status) ret = WEXITSTATUS(status);
	if (ret > 1) gtkpod_warning (_("Execution of mp3gain ('%s') failed."),
			mp3gain_set);
	break;
    }/*end switch*/

    /*free everything left*/
    g_free (filename);
    g_free (mp3gain_path);
    g_free (mp3gain_exec);
    g_free (buf);

    /*and happily return the right value*/
    return (ret > 1) ? FALSE : TRUE;
}


/* will get the volume either from mp3gain or from LAME's ReplayGain */
static gint32 nm_get_volume (Track *track)
{
    if (track)
    {
	gchar *path;

	if (track->radio_gain_set) 
		return replaygain_to_volume (track->radio_gain);
	
	path = get_track_name_on_disk_verified (track);

	mp3_get_track_lame_replaygain (path, track);
	if (track->radio_gain_set) 
		return replaygain_to_volume (track->radio_gain);
	
	mp3_get_track_ape_replaygain (path, track);
	if (track->radio_gain_set) 
		return replaygain_to_volume (track->radio_gain);
	    
	if (nm_mp3gain_calc_gain (track)) {
	    mp3_get_track_ape_replaygain (path, track);
	    if (track->radio_gain_set) 
		    return replaygain_to_volume (track->radio_gain);
	}
    }

    return TRACKVOLERROR;
}



#ifdef G_THREADS_ENABLED
/* Threaded getTrackGain*/
static gpointer th_nm_get_volume (gpointer track)
{
   gint32 gain=nm_get_volume ((Track *)track);
   g_mutex_lock (mutex);
   mutex_data = TRUE; /* signal that thread will end */
   g_cond_signal (cond);
   g_mutex_unlock (mutex);
   return (gpointer)gain;
}
#endif

/* normalize the newly inserted tracks (i.e. non-transferred tracks) */
void nm_new_tracks (void)
{
    GList *tracks=NULL;
    gint i=0;
    Track *track;

    while ((track=get_next_track(i)))
    {
	i=1; /* for get_next_track() */
	if(!track->transferred)
	{
	    tracks = g_list_append(tracks, track);
	}
    }
    nm_tracks_list(tracks);
    g_list_free (tracks);
}

static void normalization_abort(gboolean *abort)
{
   *abort=TRUE;
}

void nm_tracks_list(GList *list)
{
  gint count, n, nrs;
  gchar *buf;
  Track  *track;
  gint32 new_volume = 0;
  static gboolean abort;
  GtkWidget *dialog, *progress_bar, *label, *image, *hbox;
  time_t diff, start, mins, secs;
  char *progtext = NULL;

#ifdef G_THREADS_ENABLED
  GThread *thread = NULL;
  GTimeVal gtime;
  if (!mutex) mutex = g_mutex_new ();
  if (!cond) cond = g_cond_new ();
#endif

  block_widgets ();

  /* create the dialog window */
  dialog = gtk_dialog_new_with_buttons (_("Information"),
					 GTK_WINDOW (gtkpod_window),
					 GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_STOCK_CANCEL,
					 GTK_RESPONSE_NONE,
					 NULL);


  /* emulate gtk_message_dialog_new */
  image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_INFO,
				    GTK_ICON_SIZE_DIALOG);
  label = gtk_label_new (
      _("Press button to abort."));

  gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);

  /* hbox to put the image+label in */
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  /* Create the progress bar */
  progress_bar = gtk_progress_bar_new ();
  progtext = g_strdup (_("Normalizing..."));
  gtk_progress_bar_set_text(GTK_PROGRESS_BAR (progress_bar), progtext);
  g_free (progtext);

  /* Indicate that user wants to abort */
  g_signal_connect_swapped (GTK_OBJECT (dialog), "response",
			    G_CALLBACK (normalization_abort),
			    &abort);

  /* Add the image/label + progress bar to dialog */
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
		      hbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
		      progress_bar, FALSE, FALSE, 0);
  gtk_widget_show_all (dialog);

  while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();

  /* count number of tracks to be normalized */
  n = g_list_length(list);
  count = 0; /* tracks normalized */
  nrs = 0;
  abort = FALSE;
  start = time(NULL);

  if(n==0)
  {
     /* FIXME we should tell something*/

  }
  while (!abort &&  (list!=NULL)) /*FIXME:change it in a do-while cycle*/
  {
     track=list->data;
#ifdef G_THREADS_ENABLED
	mutex_data = FALSE;
	thread = g_thread_create (th_nm_get_volume, track, TRUE, NULL);
	if (thread)
	{
	   gboolean first_abort = TRUE;
	   g_mutex_lock (mutex);
	   do
	   {
	      while (widgets_blocked && gtk_events_pending ())
		 gtk_main_iteration ();
	      /* wait a maximum of 10 ms */

	      if (abort && first_abort)
	      {
		  first_abort = FALSE;
		  progtext = g_strdup (_("Aborting..."));
		  gtk_progress_bar_set_text(GTK_PROGRESS_BAR (progress_bar),
					    progtext);
		  g_free (progtext);
		  gtkpod_statusbar_message(_("Will abort after current mp3gain process ends."));
		  while (widgets_blocked && gtk_events_pending ())
		      gtk_main_iteration ();
	      }
	      g_get_current_time (&gtime);
	      g_time_val_add (&gtime, 20000);
	      g_cond_timed_wait (cond, mutex, &gtime);
	   }
	   while(!mutex_data);
	   new_volume = (gint)g_thread_join (thread);
	   g_mutex_unlock (mutex);
	}
	else
	{
	   g_warning ("Thread creation failed, falling back to default.\n");
	   new_volume = nm_get_volume (track);
	}
#else
	new_volume = nm_get_volume (track);
#endif

/*normalization part*/
	if(new_volume == TRACKVOLERROR)
	{
	   abort=TRUE;
	}
	else
	{
	    if(new_volume != track->volume)
	    {
		track->volume = new_volume;
		pm_track_changed (track);
		data_changed ();
	    }
	}
/*end normalization*/

	++count;
	if (count == 1)  /* we need ***much*** longer timeout */
	   prefs_set_statusbar_timeout (30*STATUSBAR_TIMEOUT);

	buf = g_strdup_printf (ngettext ("Normalized %d of %d new tracks.",
					       "Normalized %d of %d new tracks.", n),
					 count, n);
	gtkpod_statusbar_message(buf);
	g_free (buf);

	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR (progress_bar),
					(gdouble) count/n);

	diff = time(NULL) - start;
	mins = ((diff*n/count)-diff)/60;
	secs = ((((diff*n/count)-diff) & 60) / 5) * 5;
      /* don't bounce up too quickly (>10% change only) */
/*	      left = ((mins < left) || (100*mins >= 110*left)) ? mins : left;*/
	progtext = g_strdup_printf (_("%d%% (%d:%02d) left"),
			  count*100/n, (int)mins, (int)secs);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR (progress_bar),
					progtext);
	g_free (progtext);

     if (abort && (count != n))
	gtkpod_statusbar_message (_("Some tracks were not normalized. Normalization aborted!"));
     while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
     list=g_list_next(list);
  } /*end while*/

  prefs_set_statusbar_timeout (0);

  gtk_widget_destroy (dialog);
  release_widgets();
}



/* ------------------------------------------------------------

	    Synchronize Contacts / Calendar

   ------------------------------------------------------------ */

typedef enum
{
    SYNC_CONTACTS,
    SYNC_CALENDAR
} SyncType;


/* replace %i in all strings of argv with prefs_get_ipod_mount() */
static void tools_sync_replace_percent_i (gchar **argv)
{
    const gchar *ipod_mount = prefs_get_ipod_mount ();
    gint ipod_mount_len = strlen (ipod_mount);
    gint offset = 0;

    while (argv && *argv)
    {
	gchar *str = *argv;
	gchar *pi = strstr (str+offset, "%i");
	if (pi)
	{
	    /* len: -2: replace "%i"; +1: trailing 0 */
	    gint len = strlen (str) - 2 + ipod_mount_len + 1;
	    gchar *new_str = g_malloc0 (sizeof (gchar) * len);
	    strncpy (new_str, str, pi-str);
	    strcpy (new_str + (pi-str), ipod_mount);
	    strcpy (new_str + (pi-str) + ipod_mount_len, pi+2);
	    g_free (str);
	    str = new_str;
	    *argv = new_str;
	    /* set offset to point behind the inserted ipod_path in
	       case ipod_path contains "%i" */
	    offset = (pi-str) + ipod_mount_len;
	}
	else
	{
	    offset = 0;
	    ++argv;
	}
    }
}


/* execute the specified script, giving out error/status messages on
   the way */
static gboolean tools_sync_script (SyncType type)
{
    gchar *script=NULL;
    gchar *script_path, *buf;
    gchar **argv = NULL;
    GString *script_output;
    gint fdpipe[2];  /*a pipe*/
    gint len;
    pid_t pid,tpid;

    switch (type)
    {
    case SYNC_CONTACTS:
	script = g_strdup (prefs_get_sync_contacts_path ());
	break;
    case SYNC_CALENDAR:
	script = g_strdup (prefs_get_sync_calendar_path ());
	break;
    default:
	fprintf (stderr, "Programming error: tools_sync_script () called with %d\n", type);
	return FALSE;
    }

    /* remove leading and trailing whitespace */
    if (script) g_strstrip (script);

    if (!script || (strlen (script) == 0))
    {
	gtkpod_warning (_("Please specify the command to be called on the 'Tools' section of the preferences dialog.\n"));
	g_free (script);
	return FALSE;
    }

    argv = g_strsplit (script, " ", -1);

    tools_sync_replace_percent_i (argv);

    script_path = which (argv[0]);
    if (!script_path)
    {
	gtkpod_warning (_("Could not find the command '%s'.\n\nPlease verify the setting in the 'Tools' section of the preferences dialog.\n\n"), argv[0]);
	g_free (script);
	g_strfreev (argv);
	return FALSE;
    }

    /* set up arg list (first parameter should be basename of script */
    g_free (argv[0]);
    argv[0] = g_path_get_basename (script_path);

    buf = g_malloc (BUFLEN);
    script_output = g_string_sized_new (BUFLEN);

    /*create the pipe*/
    pipe(fdpipe);
    /*than fork*/
    pid=fork();

    /*and cast mp3gain*/
    switch (pid)
    {
    case -1: /* parent and error, now what?*/
	break;
    case 0: /*child*/
	close(1); /*close stdout*/
	dup2(fdpipe[WRITE],fileno(stdout));
	close(fdpipe[READ]);
	close(fdpipe[WRITE]);
	execv(script_path, argv);
	break;
    default: /*parent*/
	close(fdpipe[WRITE]);
	tpid = waitpid (pid, NULL, 0); /*wait for script termination */
	do
	{
	    len = read (fdpipe[READ], buf, BUFLEN);
	    if (len > 0) g_string_append_len (script_output, buf, len);
	} while (len > 0);
	close(fdpipe[READ]);
	/* display output in window, if any */
	if (strlen (script_output->str))
	{
	    gtkpod_warning (_("'%s' returned the following output:\n%s\n"),
			      script_path, script_output->str);
	}
	break;
    } /*end switch*/

    /*free everything left*/
    g_free (script_path);
    g_free (buf);
    g_strfreev (argv);
    g_string_free (script_output, TRUE);
    return TRUE;
}

gboolean tools_sync_contacts (void)
{
    return tools_sync_script (SYNC_CONTACTS);
}

gboolean tools_sync_calendar (void)
{
    return tools_sync_script (SYNC_CALENDAR);
}


/* ------------------------------------------------------------

	    Play Now / Enqueue

   ------------------------------------------------------------ */


/*
 * do_command_on_entries - execute @play on tracks in @selected_tracks
 * @play: the command to execute (e.g. "xmms -e %s")
 * @what: e.g. "Enqueue" or "Play Now" (used for error messages)
 * @selected tracks: list of tracks to to be placed in the command line
 * at the position of "%s"
 *
 */
void
do_command_on_entries (const gchar *command, const gchar *what,
		       GList *selected_tracks)
{
    GList *l;
    gchar *str, *commandc, *next;
    gboolean percs = FALSE; /* did "%s" already appear? */
    GPtrArray *args;

    if ((!command) || (strlen (command) == 0))
    {
	gchar *buf = g_strdup_printf (_("No command set for '%s'"), what);
	gtkpod_statusbar_message (buf);
	C_FREE (buf);
	return;
    }

    /* find the command itself -- separated by ' ' */
    next = strchr (command, ' ');
    if (!next)
    {
	str = g_strdup (command);
    }
    else
    {
	str = g_strndup (command, next-command);
    }
    while (g_ascii_isspace (*command))  ++command;
    /* get the full path */
    commandc = which (str);
    if (!commandc)
    {
	gchar *buf = g_strdup_printf (_("Could not find command '%s' specified for '%s'"),
				      str, what);
	gtkpod_statusbar_message (buf);
	C_FREE (buf);
	C_FREE (str);
	return;
    }
    C_FREE (str);

    /* Create the command line */
    args = g_ptr_array_sized_new (g_list_length (selected_tracks) + 10);
    /* first the full path */
    g_ptr_array_add (args, commandc);
    do
    {
	const gchar *next;
	gboolean end;

	next = strchr (command, ' ');
	if (next == NULL) next = command + strlen (command);

	if (next == command)  end = TRUE;
	else                  end = FALSE;

	if (!end && (strncmp (command, "%s", 2) != 0))
	{   /* current token is not "%s" */
	    gchar *buf;
	    buf = g_strndup (command, next-command);
	    g_ptr_array_add (args, buf);
	}
	else if (!percs)
	{
	    for(l = selected_tracks; l; l = l->next)
	    {
		if((str = get_track_name_on_disk_verified((Track*)l->data)))
		    g_ptr_array_add (args, str);
	    }
	    percs = TRUE; /* encountered a '%s' */
	}
	command = next;
	/* skip whitespace */
	while (g_ascii_isspace (*command))  ++command;
    } while (*command);
    /* need NULL pointer */
    g_ptr_array_add (args, NULL);

    switch(fork())
    {
    case 0: /* we are the child */
    {
	gchar **argv = (gchar **)args->pdata;
#if DEBUG_MISC
	gchar **bufp = argv;
	while (*bufp)	{ puts (*bufp); ++bufp;	}
#endif
	execv(argv[0], &argv[1]);
	g_ptr_array_free (args, TRUE);
	exit(0);
	break;
    }
    case -1: /* we are the parent, fork() failed  */
	g_ptr_array_free (args, TRUE);
	break;
    default: /* we are the parent, everything's fine */
	break;
    }
}


/*
 * play_entries_now - play the entries currently selected in xmms
 * @selected_tracks: list of tracks to be played
 */
void tools_play_tracks (GList *selected_tracks)
{
    do_command_on_entries (prefs_get_play_now_path (),
			   _("Play Now"),
			   selected_tracks);
}

/*
 * play_entries_now - play the entries currently selected in xmms
 * @selected_tracks: list of tracks to be played
 */
void tools_enqueue_tracks (GList *selected_tracks)
{
    do_command_on_entries (prefs_get_play_enqueue_path (),
			   _("Enqueue"),
			   selected_tracks);
}



