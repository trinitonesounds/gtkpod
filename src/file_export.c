/* Time-stamp: <2004-08-22 13:45:06 jcs>
|
|  Copyright (C) 2002 Corey Donohoe <atmos at atmos.org>
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

#include "misc.h"
#include "prefs.h"
#include "file.h"
#include "charset.h"
#include "interface.h"
#include "support.h"
#include <limits.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <math.h>


/* Structure to keep all necessary information */
struct fcd
{
    GList *tracks;  /* tracks to be written */
    GtkWidget *fc;  /* file chooser */
    GtkWidget *win; /* glade widget to lookup widgets by name */
    gpointer user_data; 
};



/*------------------------------------------------------------------

  Code to export tracks

  ------------------------------------------------------------------*/

/* Strings for prefs settings */
/* use original charset or specified one? */
const gchar *EXPORT_FILES_SPECIAL_CHARSET="export_files_special_charset";
/* whether we check for existing files on export or not */
const gchar *EXPORT_FILES_CHECK_EXISTING="export_files_check_existing";
const gchar *EXPORT_FILES_PATH="export_files_path";
const gchar *EXPORT_FILES_TPL="export_files_template";
/* Default prefs settings */
const gchar *EXPORT_FILES_TPL_DFLT="%o;%a - %t.mp3;%t.wav";




/**
 * READ_WRITE_BLOCKSIZE - how many bytes we read per fread/fwrite call
 */
#define READ_WRITE_BLOCKSIZE 65536


#ifdef G_THREADS_ENABLED
/* Thread specific */
static  GMutex *mutex = NULL;
static GCond  *cond = NULL;
static gboolean mutex_data = FALSE;
#endif



/**
 * Recursively make directories in the given filename.
 * @return FALSE is this is not possible.
 */
gboolean
mkdirhier(char* filename)
{
	char* p = filename;
	if (*p == G_DIR_SEPARATOR) p++;
	while ((p = index(p, G_DIR_SEPARATOR)) != NULL) {
		*p = '\0';
		if (!g_file_test(filename, G_FILE_TEST_EXISTS)) {
			if (mkdir(filename, 0755) == -1) {
				gtkpod_warning (_("Error creating %s: %s\n"), filename, g_strerror(errno));
				return FALSE;
			}
		}
		*p = G_DIR_SEPARATOR;
		p++;
	}
	return TRUE;
}

/**
 * copy_file_from_fd_to_fd - given two open file descriptors, read from one
 * 	and write the data to the other
 * @from - the file descriptor we're reading from
 * @to - the file descriptor we're writing to
 * Returns TRUE on write success, FALSE on write failure
 */
static gboolean
copy_file_fd(FILE *from, FILE *to)
{
    gboolean result = FALSE;
    gchar data[READ_WRITE_BLOCKSIZE];

    if((from) && (to))
    {
	int read_bytes = 0, write_bytes = 0;
	
	do
	{
	    if((read_bytes = 
		    fread(data, sizeof(char), READ_WRITE_BLOCKSIZE, from)))
	    {
		if((write_bytes = 
			 fwrite(data, sizeof(char), read_bytes, to)))
		{
		    result = TRUE;
		}
		else
		{
		    if((ferror(to) != 0))
		    {
/*			fprintf(stderr, "%d is errno writing\n", errno);*/
			result = FALSE;
		    }
		    break;
		}
	    }
	    else
	    {
		if(!feof(from))
		    result = FALSE;
		else 
		    result = TRUE;
		break;

	    }
	    
	} while(!(feof(from)));
    }
    return(result);
}


/* Return TRUE if the file @dest exists and is of same size as @from */
static gboolean
file_is_ok(gchar *from, gchar *dest)
{
    struct stat st_from, st_dest;

    if(stat(dest, &st_dest) == -1)    	      return FALSE;
    if(stat(from, &st_from) == -1)            return FALSE;

    if(st_from.st_size == st_dest.st_size)    return TRUE;

    return FALSE;
}


/**
 * copy_file - copy the filename on disk named file, to
 * the destination file dest.  Both names are FULL pathnames to the file
 * @file - the filename to copy 
 * @dest - the filename we copy to
 * Returns TRUE on successful copying
 */
static gboolean 
copy_file(gchar *file, gchar *dest)
{
    gboolean result = FALSE;
    FILE *from = NULL, *to = NULL;
    gboolean check_existing;

    prefs_get_int_value (EXPORT_FILES_CHECK_EXISTING, &check_existing);
    
    if(check_existing && file_is_ok(file, dest))
    	return TRUE;
	
    if((from = fopen(file, "r")))
    {
	if((to = fopen(dest, "w")))
	{
	    result = copy_file_fd(from, to);
	    fclose(to);
	}
	else
	{
	    switch(errno)
	    {
		case EPERM:
		    gtkpod_warning (_("Error copying '%s' to '%s': Permission Error (%s)\n"), file, dest, g_strerror (errno));
		default:
		    gtkpod_warning (_("Error copying '%s' to '%s' (%s)\n"), file, dest, g_strerror (errno));
		    break;
	    }
	}
	fclose(from);
    }
    else
    {
	gtkpod_warning (_("Unable to open '%s' for reading\n"), file);
    }
    return(result);
}




/* This function is called when the user presses the abort button
 * during flush_tracks() */
static void write_tracks_abort (gboolean *abort)
{
    *abort = TRUE;
}



/**
 * get_preferred_filename - useful for generating the preferred
 * @param Track the track
 * @return the file filename (including directories) for this Track
 * based on the users preferences.  The returned char* must be freed
 * by the caller.
 */
static gchar *
track_get_export_filename (Track *track)
{
    gchar *res_utf8, *res_cs = NULL;
    gchar *template;
    gboolean special_charset;

    g_return_val_if_fail (track, NULL);

    prefs_get_string_value (EXPORT_FILES_TPL, &template);
    res_utf8 = get_string_from_template (track, template, TRUE);
    C_FREE (template);

    prefs_get_int_value (EXPORT_FILES_SPECIAL_CHARSET, &special_charset);

    /* convert it to the charset */
    if (special_charset)
    {   /* use the specified charset */
	res_cs = charset_from_utf8 (res_utf8);
    }
    else
    {   /* use the charset stored in track->charset */
	res_cs = charset_track_charset_from_utf8 (track, res_utf8);
    }
    g_free (res_utf8);
    return res_cs;
}


/**
 * write_track_to_dir - copy the Track* to the desired output file
 * @s - The Track reference we're manipulating
 * Returns - TRUE on success, FALSE on failure
 */
static gboolean
write_track(Track *s)
{
    gchar *dest_file = NULL;
    gboolean result = FALSE;
    
    if((dest_file = track_get_export_filename(s)))
    {
	gchar *from_file = get_track_name_on_disk(s);
	gchar *filename, *dest_dir;

	prefs_get_string_value (EXPORT_FILES_PATH, &dest_dir);
	filename = g_build_filename (dest_dir, dest_file, NULL);

	if (mkdirhier(filename)) {
	    if(copy_file(from_file, filename))
	        result = TRUE;
	}
	g_free(from_file);
	g_free(dest_file);
	g_free(dest_dir);
	g_free(filename);
    }
    return(result);
}

#ifdef G_THREADS_ENABLED
/* Threaded write_track */
static gpointer th_write_track (gpointer s)
{
    gboolean result = FALSE;

    result = write_track ((Track *)s);

    g_mutex_lock (mutex);
    mutex_data = TRUE;   /* signal that thread will end */
    g_cond_signal (cond);
    g_mutex_unlock (mutex);

    return (gpointer)result;
}
#endif


/******************************************************************
   export_files_write - copy the specified tracks to the selected
   directory.
 ******************************************************************/
static void export_files_write (struct fcd *fcd)
{
    GList *l = NULL;
    gint n;
    static gboolean abort;
    gdouble total = 0;
#ifdef G_THREADS_ENABLED
    GThread *thread = NULL;
    GTimeVal gtime;
    if (!mutex) mutex = g_mutex_new ();
    if (!cond) cond = g_cond_new ();
#endif

    g_return_if_fail (fcd && fcd->win && fcd->fc);

    abort = FALSE;
    n = g_list_length (fcd->tracks);
    /* calculate total length to be copied */
    for(l = fcd->tracks; l && !abort; l = l->next)
    {
	Track *s = (Track*)l->data;
	total += s->size;
    }

    if(n != 0)
    {
	/* create the dialog window */
	GtkWidget *dialog, *progress_bar, *label, *image, *hbox;
	gchar *progtext = NULL;
	gint count = 0;     /* number of tracks copied */
	gdouble copied = 0;  /* number of bytes copied */
	gdouble fraction;    /* fraction copied (copied/total) */
	gboolean result = TRUE;
	time_t diff, start, fullsecs, hrs, mins, secs;

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
	progtext = g_strdup (_("copying..."));
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR (progress_bar), progtext);
	g_free (progtext);

	/* Indicate that user wants to abort */
	g_signal_connect_swapped (GTK_OBJECT (dialog), "response",
				  G_CALLBACK (write_tracks_abort),
				  &abort);

	/* Add the image/label + progress bar to dialog */
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
			    hbox, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
			    progress_bar, FALSE, FALSE, 0);
	gtk_widget_show_all (dialog);

	while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
	start = time(NULL);
	for(l = fcd->tracks; l && !abort; l = l->next)
	{
	    Track *s = (Track*)l->data;
	    gchar *buf;

	    copied += s->size;
#ifdef G_THREADS_ENABLED
	    mutex_data = FALSE;
	    thread = g_thread_create (th_write_track, s, TRUE, NULL);
	    if (thread)
	    {
		g_mutex_lock (mutex);
		do
		{
		    while (widgets_blocked && gtk_events_pending ())
			gtk_main_iteration ();
		    /* wait a maximum of 20 ms */
		    g_get_current_time (&gtime);
		    g_time_val_add (&gtime, 20000);
		    g_cond_timed_wait (cond, mutex, &gtime);
		} while(!mutex_data);
		g_mutex_unlock (mutex);
		result &= (gboolean)g_thread_join (thread);
	    }
	    else {
		g_warning ("Thread creation failed, falling back to default.\n");
		result &= write_track (s);
	    }
#else
	    result &= write_track (s);
#endif
	    if (!result)
	    {
		gtkpod_warning (_("Failed to write '%s-%s'\n"), s->artist, s->title);	
	    }

	    ++count;
	    if (count == 1)  /* we need longer timeout */
		prefs_set_statusbar_timeout (3*STATUSBAR_TIMEOUT);
	    if (count == n)  /* we need to reset timeout */
		prefs_set_statusbar_timeout (0);
	    buf = g_strdup_printf (ngettext ("Copied %d of %d track.",
					     "Copied %d of %d tracks.", n),
				   count, n);
	    gtkpod_statusbar_message(buf);
	    g_free (buf);

	    fraction = copied/total;

	    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR (progress_bar),
					  fraction);
	    diff = time(NULL) - start;
	    fullsecs = (diff/fraction)-diff+5;
	    hrs  = fullsecs / 3600;
	    mins = (fullsecs % 3600) / 60;
	    secs = ((fullsecs % 60) / 5) * 5;
	    /* don't bounce up too quickly (>10% change only) */
/*	      left = ((mins < left) || (100*mins >= 110*left)) ? mins : left;*/
	    progtext = g_strdup_printf (
		_("%d%% (%d:%02d:%02d left)"),
		(int)(100*fraction), (int)hrs, (int)mins, (int)secs);
	    gtk_progress_bar_set_text(GTK_PROGRESS_BAR (progress_bar),
				      progtext);
	    g_free (progtext);
	    while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
	}
	gtk_widget_destroy (dialog);
	if (!result || abort)
	    gtkpod_statusbar_message (_("Some tracks were not copied."));
    }
}

/******************************************************************
   export_files_cleanup - free memory taken up by the fcd structure.
 ******************************************************************/
static void export_files_cleanup (struct fcd *fcd)
{
    g_return_if_fail (fcd);
    g_list_free (fcd->tracks);
    if (fcd->win)  gtk_widget_destroy (fcd->win);
    g_free (fcd);
    release_widgets ();
}


/******************************************************************
   export_files_retrieve_options - retrieve options and store
   them in the prefs.
 ******************************************************************/
static void export_files_retrieve_options (struct fcd *fcd)
{
    g_return_if_fail (fcd && fcd->win && fcd->fc);

    option_get_toggle_button (fcd->win,
			      EXPORT_FILES_SPECIAL_CHARSET);
    option_get_toggle_button (fcd->win,
			      EXPORT_FILES_CHECK_EXISTING);
    option_get_string (fcd->win, EXPORT_FILES_TPL, NULL);
    option_get_filename (GTK_FILE_CHOOSER (fcd->fc),
			 EXPORT_FILES_PATH, NULL);
}


/******************************************************************
   export_files_response - handle the response codes accordingly.
 ******************************************************************/
static void export_files_response (GtkDialog *fc,
				   gint response,
				   struct fcd *fcd)
{
/*     printf ("received response code: %d\n", response); */
    switch (response)
    {
    case RESPONSE_APPLY:
	export_files_retrieve_options (fcd);
	break;
    case GTK_RESPONSE_ACCEPT:
	export_files_retrieve_options (fcd);
	export_files_write (fcd);
	export_files_cleanup (fcd);
	gtk_widget_destroy (GTK_WIDGET (fc));
	break;
    case GTK_RESPONSE_CANCEL:
	export_files_cleanup (fcd);
	gtk_widget_destroy (GTK_WIDGET (fc));
	break;
    case GTK_RESPONSE_DELETE_EVENT:
	export_files_cleanup (fcd);
	break;
    default:
	fprintf (stderr, "Programming error: export_files_response(): unknown response '%d'\n", response);
	break;
    }
}


/******************************************************************
   export_files_init - Export files off of your ipod to an arbitrary
   directory, specified by the file chooser dialog
   @tracks - GList with data of type (Track*) we want to write 
 ******************************************************************/
void export_files_init (GList *tracks)
{
    GtkWidget *win = create_export_files_options ();
    GtkWidget *options = lookup_widget (win, "options_frame");
    struct fcd *fcd = g_malloc0 (sizeof (struct fcd));
    GtkWidget *fc = gtk_file_chooser_dialog_new (
	_("Select Export Destination Directory"),
	NULL,
	GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
	GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	GTK_STOCK_APPLY, RESPONSE_APPLY,
	GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
	NULL);

    /* Information needed to clean up later */
    fcd->tracks = g_list_copy (tracks);
    fcd->win = win;
    fcd->fc = fc;

    /* according to GTK FAQ: move a widget to a new parent */
    gtk_widget_ref (options);
    gtk_container_remove (GTK_CONTAINER (win), options);
    gtk_widget_unref (options);

    /* set extra options */
    gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (fc),
				       options);

    /* set last folder */
    option_set_folder (GTK_FILE_CHOOSER (fc),
		       EXPORT_FILES_PATH);
    /* set toggle button "charset" */
    option_set_toggle_button (win, EXPORT_FILES_SPECIAL_CHARSET, FALSE);
    /* set toggle button "check for existing files" */
    option_set_toggle_button (win, EXPORT_FILES_CHECK_EXISTING, TRUE);

    /* set last template */
    option_set_string (win, EXPORT_FILES_TPL, EXPORT_FILES_TPL_DFLT);

    /* catch response codes */
    g_signal_connect (fc, "response",
		      G_CALLBACK (export_files_response),
		      fcd);

    gtk_widget_show (fc);
    block_widgets ();
}






/*------------------------------------------------------------------

  Code to export a playlist file

  ------------------------------------------------------------------*/


/* Options for the export playlist file file chooser */
typedef enum
{
    EXPORT_PLAYLIST_FILE_TYPE_M3U = 0,
    EXPORT_PLAYLIST_FILE_TYPE_PLS
} ExportPlaylistFileType;

/* Strings for the widgets involved */
const gchar *ExportPlaylistFileTypeW[] = {
    "type_m3u", "type_pls", NULL };
/* order must be identical to 'FileSource' enum in file.h */
const gchar *ExportPlaylistFileSourceW[] = {
    "source_prefer_local", "source_local", "source_ipod", NULL };

/* Strings for prefs settings */
const gchar *EXPORT_PLAYLIST_FILE_TYPE="export_playlist_file_type";
const gchar *EXPORT_PLAYLIST_FILE_SOURCE="export_playlist_file_source";
const gchar *EXPORT_PLAYLIST_FILE_PATH="export_playlist_file_path";
const gchar *EXPORT_PLAYLIST_FILE_TPL="export_playlist_file_template";

/* Default prefs settings */
const gchar *EXPORT_PLAYLIST_FILE_TPL_DFLT="%a - %t";


/******************************************************************
   export_playlist_file_cleanup - free memory taken up by the fcd
   structure.
 ******************************************************************/
static void export_playlist_file_cleanup (struct fcd *fcd)
{
    g_return_if_fail (fcd);
    g_list_free (fcd->tracks);
    if (fcd->win)  gtk_widget_destroy (fcd->win);
    g_free (fcd);
    release_widgets ();
}


/******************************************************************
   export_playlist_file_retrieve_options - retrieve options and store
   them in the prefs.
 ******************************************************************/
static void export_playlist_file_retrieve_options (struct fcd *fcd)
{
    g_return_if_fail (fcd && fcd->win && fcd->fc);

    option_get_radio_button (fcd->win,
			     EXPORT_PLAYLIST_FILE_TYPE,
			     ExportPlaylistFileTypeW);
    option_get_radio_button (fcd->win,
			     EXPORT_PLAYLIST_FILE_SOURCE,
			     ExportPlaylistFileSourceW);
    option_get_string (fcd->win, EXPORT_PLAYLIST_FILE_TPL, NULL);
    option_get_folder (GTK_FILE_CHOOSER (fcd->fc),
		       EXPORT_PLAYLIST_FILE_PATH, NULL);
}


/******************************************************************
   export_playlist_file_write - write out a playlist file (filename is
   retrieved from the file chooser dialog, options are retrieved from
   the prefs system
 ******************************************************************/
static void export_playlist_file_write (struct fcd *fcd)
{
    guint num;
    gint type, source;
    gchar *template, *fname;
    FILE *file;

    g_return_if_fail (fcd && fcd->fc);

    num = g_list_length (fcd->tracks);

    prefs_get_int_value (EXPORT_PLAYLIST_FILE_TYPE, &type);
    prefs_get_int_value (EXPORT_PLAYLIST_FILE_SOURCE, &source);
    prefs_get_string_value (EXPORT_PLAYLIST_FILE_TPL, &template);
    if (!template)
	template = g_strdup (EXPORT_PLAYLIST_FILE_TPL_DFLT);

    fname = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(fcd->fc));

    file = fopen (fname, "w");

    if (file)
    {
	guint i,n;
	gchar *buf;

	switch (type)
	{
	case EXPORT_PLAYLIST_FILE_TYPE_M3U:
	    fprintf (file, "#EXTM3U\n");
	    break;
	case EXPORT_PLAYLIST_FILE_TYPE_PLS:
	    fprintf (file, "[playlist]\n");
	    break;
	}

	for (n=0,i=0; i<num; ++i)
	{
	    Track *track = g_list_nth_data (fcd->tracks, i);
	    gchar *infotext_utf8 = get_string_from_template (track,
							     template,
							     FALSE);
	    gchar *filename = get_track_name_from_source (track,
							  source);
	    gchar *infotext;

	    if (infotext_utf8)
	    {
		infotext = charset_from_utf8 (infotext_utf8);
		C_FREE (infotext_utf8);
	    }
	    else
	    {
		infotext = g_strdup ("");
	    }

	    if (filename)
	    {
		++n;  /* number of verified tracks */

		switch (type)
		{
		case EXPORT_PLAYLIST_FILE_TYPE_M3U:
		    fprintf (file, "#EXTINF:%d,%s\n",
			     (track->tracklen+990)/1000, infotext);
		    fprintf (file, "%s\n", filename);
		    break;
		case EXPORT_PLAYLIST_FILE_TYPE_PLS:
		    fprintf (file, "File%d=%s\n", n, filename);
		    fprintf (file, "Title%d=%s\n", n, infotext);
		    fprintf (file, "Length%d=%d\n", n,
			     (track->tracklen+990)/1000);
		    break;
		}
	    }
	    else
	    {
		gtkpod_warning (_("No valid filename for: %s\n\n"), infotext);
	    }
	    g_free (infotext);
	    g_free (filename);
	}
	switch (type)
	{
	case EXPORT_PLAYLIST_FILE_TYPE_M3U:
	    break;
	case EXPORT_PLAYLIST_FILE_TYPE_PLS:
	    fprintf (file, "NumberOfEntries=%d\n", n);
	    fprintf (file, "Version=2\n");
	    break;
	}
	fclose (file);
	buf = g_strdup_printf (
	    ngettext ("Created playlist with one track.",
		      "Created playlist with %d tracks.", n), n);
	gtkpod_statusbar_message (buf);
	g_free (buf);
    }
    else
    {
	gtkpod_warning (_("Could not open '%s' for writing (%s).\n\n"),
			fname, g_strerror (errno));
    }
    g_free (template);
    g_free (fname);
}
    


/******************************************************************
   export_playlist_file_response - handle the response codes
   accordingly.
 ******************************************************************/
static void export_playlist_file_response (GtkDialog *fc,
					   gint response,
					   struct fcd *fcd)
{
/*     printf ("received response code: %d\n", response); */
    switch (response)
    {
    case RESPONSE_APPLY:
	export_playlist_file_retrieve_options (fcd);
	break;
    case GTK_RESPONSE_ACCEPT:
	export_playlist_file_retrieve_options (fcd);
	export_playlist_file_write (fcd);
	export_playlist_file_cleanup (fcd);
	gtk_widget_destroy (GTK_WIDGET (fc));
	break;
    case GTK_RESPONSE_CANCEL:
	export_playlist_file_cleanup (fcd);
	gtk_widget_destroy (GTK_WIDGET (fc));
	break;
    case GTK_RESPONSE_DELETE_EVENT:
	export_playlist_file_cleanup (fcd);
	break;
    default:
	fprintf (stderr, "Programming error: export_playlist_file_response(): unknown response '%d'\n", response);
	break;
    }
}


/******************************************************************
   export_playlist_file_init - Create a playlist file to a location
   specified by the file selection dialog.
   @tracks: GList with tracks to be in playlist file.
 ******************************************************************/
void export_playlist_file_init (GList *tracks)
{
    GtkWidget *win = create_export_playlist_file_options ();
    GtkWidget *options = lookup_widget (win, "options_frame");
    struct fcd *fcd = g_malloc0 (sizeof (struct fcd));
    GtkWidget *fc = gtk_file_chooser_dialog_new (
	_("Create Playlist File"),
	NULL,
	GTK_FILE_CHOOSER_ACTION_SAVE,
	GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	GTK_STOCK_APPLY, RESPONSE_APPLY,
	GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
	NULL);

    /* Information needed to clean up later */
    fcd->tracks = g_list_copy (tracks);
    fcd->win = win;
    fcd->fc = fc;

    /* according to GTK FAQ: move a widget to a new parent */
    gtk_widget_ref (options);
    gtk_container_remove (GTK_CONTAINER (win), options);
    gtk_widget_unref (options);

    /* set extra options */
    gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (fc),
				       options);

    /* set last folder */
    option_set_folder (GTK_FILE_CHOOSER (fc),
		       EXPORT_PLAYLIST_FILE_PATH);
    /* set last type */
    option_set_radio_button (win, EXPORT_PLAYLIST_FILE_TYPE,
			     ExportPlaylistFileTypeW, 0);
    /* set last source */
    option_set_radio_button (win, EXPORT_PLAYLIST_FILE_SOURCE,
			     ExportPlaylistFileSourceW, 0);
    /* set last template */
    option_set_string (win, EXPORT_PLAYLIST_FILE_TPL,
		       EXPORT_PLAYLIST_FILE_TPL_DFLT);

    /* catch response codes */
    g_signal_connect (fc, "response",
		      G_CALLBACK (export_playlist_file_response),
		      fcd);

    gtk_widget_show (fc);
    block_widgets ();
}


/*
Playlists examples:

	Simple M3U playlist:  
	 
	
	C:\My Music\Pink Floyd\1979---The_Wall_CD1\1.In_The_Flesh.mp3  
	C:\My Music\Pink Floyd\1979---The_Wall_CD1\10.One_Of_My_Turns.mp3  
	

	Extended M3U playlist:  
	

	#EXTM3U  
	#EXTINF:199,Pink Floyd - In The Flesh  
	R:\Music\Pink Floyd\1979---The_Wall_CD1\1.In_The_Flesh.mp3  
	#EXTINF:217,Pink Floyd - One Of My Turns  
	R:\Music\Pink Floyd\1979---The_Wall_CD1\10.One_Of_My_Turns.mp3  
	

	PLS playlist:  
	 
	
	[playlist]  
	File1=C:\My Music\Pink Floyd\1979---The_Wall_CD1\1.In_The_Flesh.mp3  
	Title1=Pink Floyd - In The Flesh  
	Length1=199  
	File2=C:\My Music\Pink Floyd\1979---The_Wall_CD1\10.One_Of_My_Turns.mp3  
	Title2=Pink Floyd - One Of My Turns  
	Length2=217  
	NumberOfEntries=2  
	Version=2  
*/
