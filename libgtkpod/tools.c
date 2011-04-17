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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "misc.h"
#include "misc_track.h"
#include "prefs.h"
#include "tools.h"
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <glib/gi18n-lib.h>


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


/* Run @command on @track_path.
 *
 * Command may include options, like "mp3gain -q -k %s"
 *
 * %s is replaced by @track_path if present, otherwise @track_path is
 * added at the end.
 *
 * Return value: TRUE if the command ran successfully, FALSE if any
 * error occurred.
 */
static gboolean run_exec_on_track (const gchar *commandline,
				   const gchar *track_path)
{
    gchar *command_full_path = NULL;
    gchar *command = NULL;
    gchar *command_base = NULL;
    const gchar *nextarg;
    gboolean success = FALSE;
    gboolean percs = FALSE;
    GPtrArray *args;

    int status, fdnull, ret;
    pid_t tpid;

    g_return_val_if_fail (commandline, FALSE);
    g_return_val_if_fail (track_path, FALSE);


    /* skip whitespace */
    while (g_ascii_isspace (*commandline))  ++commandline;

    /* find the command itself -- separated by ' ' */
    nextarg = strchr (commandline, ' ');
    if (!nextarg)
    {
	nextarg = commandline + strlen (commandline);
    }

    command = g_strndup (commandline, nextarg-commandline);

    command_full_path = g_find_program_in_path (command);

    if (!command_full_path)
    {
	gtkpod_warning (_("Could not find '%s'.\nPlease specifiy the exact path in the Tools section of the preference dialog or install the program if it is not installed on your system.\n\n"), command);
	goto cleanup;
    }

    command_base = g_path_get_basename (command_full_path);

    /* Create the command line to be used with execv(). */
    args =  g_ptr_array_sized_new (strlen (commandline));
    /* add the full path */
    g_ptr_array_add (args, command_full_path);
    /* add the basename */
    g_ptr_array_add (args, command_base);
    /* add the command line arguments */

    commandline = nextarg;

    /* skip whitespace */
    while (g_ascii_isspace (*commandline))  ++commandline;

    while (*commandline != 0)
    {
	const gchar *next;

	next = strchr (commandline, ' ');
	/* next argument is everything to the end */
	if (!next)
	    next = commandline + strlen (commandline);

	if (strncmp (commandline, "%s", 2) == 0)
	{   /* substitute %s with @track_path */
	    g_ptr_array_add (args, g_strdup (track_path));
	    percs = TRUE;
	}
	else
	{
	    g_ptr_array_add (args,
			     g_strndup (commandline, next-commandline));
	}

	/* skip to next argument */
	commandline = next;

	/* skip whitespace */
	while (g_ascii_isspace (*commandline))  ++commandline;
    }

    /* Add @track_path if "%s" was not present */
    if (!percs)
	g_ptr_array_add (args, g_strdup (track_path));

    /* need NULL pointer */
    g_ptr_array_add (args, NULL);

    tpid = fork ();

    switch (tpid)
    {
    case 0: /* we are the child */
    {
	gchar **argv = (gchar **)args->pdata;
#if 0
	gchar **bufp = argv;
	while (*bufp)	{ puts (*bufp); ++bufp;	}
#endif
	/* redirect output to /dev/null */
	if ((fdnull = open("/dev/null", O_WRONLY | O_NDELAY)) != -1)
	{
	    dup2(fdnull, fileno(stdout));
	}
	execv(argv[0], &argv[1]);
	exit(0);
	break;
    }
    case -1: /* we are the parent, fork() failed  */
	g_ptr_array_free (args, TRUE);
	break;
    default: /* we are the parent, everything's fine */
	tpid = waitpid (tpid, &status, 0);
	g_ptr_array_free (args, TRUE);
	if (WIFEXITED(status))
	    ret = WEXITSTATUS(status);
	else
	    ret = 2;
	if (ret > 1)
	{
	    gtkpod_warning (_("Execution of '%s' failed.\n\n"),
			    command_full_path);
	}
	else
	{
	    success = TRUE;
	}
	break;
    }


  cleanup:
    g_free (command_full_path);
    g_free (command);
    g_free (command_base);

    return success;
}




/* reread the soundcheck value from the file */
static gboolean nm_get_soundcheck (Track *track)
{
    gchar *path;
    gchar *commandline = NULL;
    FileType *filetype;

    g_return_val_if_fail (track, FALSE);

    if (read_soundcheck (track))
	return TRUE;

    path = get_file_name_from_source (track, SOURCE_PREFER_LOCAL);
    filetype = determine_filetype (path);

    if (!path || !filetype) {
        gchar *buf = get_track_info (track, FALSE);
        gtkpod_warning (
                _("Normalization failed: file not available (%s).\n\n"),
                buf);
        g_free (buf);
        return FALSE;
    }

	commandline = filetype_get_gain_cmd(filetype);
	if (commandline) {
	    if (run_exec_on_track (commandline, path)) {
	        g_free(path);
	        return read_soundcheck (track);
	    }
	}
	else {
	    gtkpod_warning (
	            _("Normalization failed for file %s: file type not supported.\n"
	                    "To normalize mp3 and aac files ensure the following commands paths have been set in the Tools section\n"
	                    "\tmp3 files: mp3gain\n"
	                    "\taac files: aacgain"), path);
	}

	return FALSE;
}



#ifdef G_THREADS_ENABLED
/* Threaded getTrackGain*/
static gpointer th_nm_get_soundcheck (gpointer track)
{
   gboolean success = nm_get_soundcheck ((Track *)track);
   g_mutex_lock (mutex);
   mutex_data = TRUE; /* signal that thread will end */
   g_cond_signal (cond);
   g_mutex_unlock (mutex);
   return GUINT_TO_POINTER(success);
}
#endif

/* normalize the newly inserted tracks (i.e. non-transferred tracks) */
void nm_new_tracks (iTunesDB *itdb)
{
    GList *tracks=NULL;
    GList *gl;

    g_return_if_fail (itdb);

    for (gl=itdb->tracks; gl; gl=gl->next)
    {
	Track *track = gl->data;
	g_return_if_fail (track);
	if(!track->transferred)
	{
	    tracks = g_list_append (tracks, track);
	}
    }
    nm_tracks_list (tracks);
    g_list_free (tracks);
}

static void normalization_abort(gboolean *abort)
{
   *abort=TRUE;
}

void nm_tracks_list (GList *list)
{
  gint count, succ_count, n, nrs;
  guint32 old_soundcheck;
  gboolean success;
  static gboolean abort;
  GtkWidget *dialog, *progress_bar, *label, *track_label;
  GtkWidget *image, *hbox;
  GtkWidget *content_area;
  time_t diff, start, fullsecs, hrs, mins, secs;
  gchar *progtext = NULL;

#ifdef G_THREADS_ENABLED
  GThread *thread = NULL;
  GTimeVal gtime;
  if (!mutex) mutex = g_mutex_new ();
  if (!cond) cond = g_cond_new ();
#endif

  block_widgets ();

  /* create the dialog window */
  dialog = gtk_dialog_new_with_buttons (_("Information"),
					 GTK_WINDOW (gtkpod_app),
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

  /* Create label for track name */
  track_label = gtk_label_new (NULL);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);

  /* Indicate that user wants to abort */
  g_signal_connect_swapped (G_OBJECT (dialog), "response",
			    G_CALLBACK (normalization_abort),
			    &abort);

  /* Add the image/label + progress bar to dialog */
  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  gtk_box_pack_start (GTK_BOX (content_area), hbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (content_area), track_label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (content_area), progress_bar, FALSE, FALSE, 0);
  gtk_widget_show_all (dialog);

  while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();

  /* count number of tracks to be normalized */
  n = g_list_length(list);
  count = 0; /* tracks processed */
  succ_count = 0;  /* tracks normalized */
  nrs = 0;
  abort = FALSE;
  start = time(NULL);

  if(n==0)
  {
     /* FIXME we should tell something*/

  }
  else
  {
      /* we need ***much*** longer timeout */
      g_message("TODO tools:nm_tracks_list - statusbar\n");
//      gtkpod_statusbar_timeout (30*STATUSBAR_TIMEOUT);
  }
  while (!abort &&  (list!=NULL))
  {
     Track  *track = list->data;
     gchar *label_buf = g_strdup_printf ("%d/%d", count, n);

     gtk_label_set_text (GTK_LABEL (track_label), label_buf);

     g_message("TODO tools:nm_tracks_list - statusbar\n");
//     gtkpod_statusbar_message (_("%s - %s"),
//			       track->artist, track->title);
     C_FREE (label_buf);

     while (widgets_blocked && gtk_events_pending ())
	 gtk_main_iteration ();

     /* need to know so we can update the display when necessary */
     old_soundcheck = track->soundcheck;

#ifdef G_THREADS_ENABLED
     mutex_data = FALSE;
     thread = g_thread_create (th_nm_get_soundcheck, track, TRUE, NULL);
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
	 success = GPOINTER_TO_UINT(g_thread_join (thread));
	 g_mutex_unlock (mutex);
     }
     else
     {
	 g_warning ("Thread creation failed, falling back to default.\n");
	 success = nm_get_soundcheck (track);
     }
#else
     success = nm_get_soundcheck (track);
#endif

     /*normalization part*/
     if(!success)
     {
	 gchar *path = get_file_name_from_source (track, SOURCE_PREFER_LOCAL);
	 gtkpod_warning (
	     _("'%s-%s' (%s) could not be normalized.\n\n"),
	     track->artist, track->title, path? path:"");
	 g_free (path);
     }
     else
     {
	 ++succ_count;
	 if(old_soundcheck != track->soundcheck)
	 {
	     gtkpod_track_updated(track);
	     data_changed (track->itdb);
	 }
     }
     /*end normalization*/

     ++count;
     gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR (progress_bar),
				   (gdouble) count/n);

     diff = time(NULL) - start;
     fullsecs = (diff*n/count)-diff;
     hrs  = fullsecs / 3600;
     mins = (fullsecs % 3600) / 60;
     secs = ((fullsecs % 60) / 5) * 5;
     /* don't bounce up too quickly (>10% change only) */
/*	      left = ((mins < left) || (100*mins >= 110*left)) ? mins : left;*/
     progtext = g_strdup_printf (
	 _("%d%% (%d:%02d:%02d left)"),
	 count*100/n, (int)hrs, (int)mins, (int)secs);
     gtk_progress_bar_set_text(GTK_PROGRESS_BAR (progress_bar),
			       progtext);
     g_free (progtext);

     while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
     list=g_list_next(list);
  } /*end while*/

  g_message("TODO tools:nm_tracks_list - statusbar\n");
//  gtkpod_statusbar_timeout (0);

//  gtkpod_statusbar_message (ngettext ("Normalized %d of %d tracks.",
//				      "Normalized %d of %d tracks.", n),
//			    count, n);

  gtk_widget_destroy (dialog);
  release_widgets();
}



/* ------------------------------------------------------------

	    Synchronize Contacts / Calendar

   ------------------------------------------------------------ */

typedef enum
{
    SYNC_CONTACTS,
    SYNC_CALENDAR,
    SYNC_NOTES
} SyncType;


/* FIXME: tools need to be defined for each itdb separately */
/* replace %i in all strings of argv with prefs_get_ipod_mount() */
static void tools_sync_replace_percent_i (iTunesDB *itdb, gchar **argv)
{
    gchar *ipod_mount;
    gint ipod_mount_len;
    gint offset = 0;

    if (!itdb) return;

    ipod_mount = get_itdb_prefs_string (itdb, KEY_MOUNTPOINT);

    if (!ipod_mount) ipod_mount = g_strdup ("");

    ipod_mount_len = strlen (ipod_mount);

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

    g_free (ipod_mount);
}


/* execute the specified script, giving out error/status messages on
   the way */
static gboolean tools_sync_script (iTunesDB *itdb, SyncType type)
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
	script = get_itdb_prefs_string (itdb, "path_sync_contacts");
	break;
    case SYNC_CALENDAR:
	script = get_itdb_prefs_string (itdb, "path_sync_calendar");
	break;
    case SYNC_NOTES:
	script = get_itdb_prefs_string (itdb, "path_sync_notes");
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

    tools_sync_replace_percent_i (itdb, argv);

    script_path = g_find_program_in_path (argv[0]);
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
    /*then fork*/
    pid=fork();

    /*and cast mp3gain*/
    switch (pid)
    {
    case -1: /* parent and error, now what?*/
	break;
    case 0: /*child*/
	close(fdpipe[READ]);
	dup2(fdpipe[WRITE],fileno(stdout));
	dup2(fdpipe[WRITE],fileno(stderr));
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

gboolean tools_sync_all (iTunesDB *itdb)
{
    gboolean success;

    success = tools_sync_script (itdb, SYNC_CALENDAR);
    if (success)
	success = tools_sync_script (itdb, SYNC_CONTACTS);
    if (success)
	tools_sync_script (itdb, SYNC_NOTES);
    return success;
}

gboolean tools_sync_contacts (iTunesDB *itdb)
{
    return tools_sync_script (itdb, SYNC_CONTACTS);
}

gboolean tools_sync_calendar (iTunesDB *itdb)
{
    return tools_sync_script (itdb, SYNC_CALENDAR);
}

gboolean tools_sync_notes (iTunesDB *itdb)
{
    return tools_sync_script (itdb, SYNC_NOTES);
}
