/*
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
#include "support.h"
#include <limits.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <math.h>


/**
 * READ_WRITE_BLOCKSIZE - how many bytes we read per fread/fwrite call
 */
#define READ_WRITE_BLOCKSIZE 65536

/**
 * Private Variables for this subsystem
 * @dest_dir - The export directory we're putting the files in
 * @tracks - A GList of Track* references to export
 * @fs - the file selection dialog
 */
static struct {
    GList *tracks;
    gchar *dest_dir;
    GtkWidget *fs;
} file_export;

#ifdef G_THREADS_ENABLED
/* Thread specific */
static  GMutex *mutex = NULL;
static GCond  *cond = NULL;
static gboolean mutex_data = FALSE;
#endif



/*
|  Copyright (C) 2004 Ero Carrera <ero at dkbza.org>
|
|  Placed under GPL in agreement with Ero Carrera. (JCS -- 12 March 2004)
*/

/**
 * Check if supported char and return substitute.
 */
static gchar check_char(gchar c)
{
    gint i;
    static const gchar 
	invalid[]={'"', '*', ':', '<', '>', '?', '\\', '|', '/', 0};
    static const gchar
	replace[]={'_', '_', '-', '_', '_', '_', '-',  '-', '-', 0};
    for(i=0; invalid[i]!=0; i++)
	if(c==invalid[i])  return replace[i];
    return c;
}

/**
 * Process a path. It will substitute all the invalid characters.
 */
gchar *fix_path(const gchar *orig)
{
        gint i, l;
        gchar *new;

        if(orig == NULL)         return NULL;
        new = g_strdup(orig);
        for(i=0, l=strlen(orig); i<l; i++)
	{
	    new[i]=check_char(new[i]);
        }
        return new;
}

/* End of code originally supplied by Ero Carrera */



gchar *get_export_filename_template (Track *track)
{
    const gchar *p = prefs_get_export_template ();
    gchar **templates, **tplp;
    gchar *tname, *ext = NULL;
    gchar *result;

    if (!track) return (strdup (""));

    tname = get_track_name_on_disk (track);
    if (!tname) return (NULL);         /* this should not happen... */
    ext = strrchr (tname, '.');        /* pointer to filename extension */

    templates = g_strsplit (p, ";", 0);
    tplp = templates;
    while (*tplp)
    {
	if (strcmp (*tplp, "%o") == 0)
	{   /* this is only a valid extension if the original filename
	       is present */
	    if (track->pc_path_locale && strlen(track->pc_path_locale))  break;
	}
	else if (strrchr (*tplp, '.') == NULL)
	{   /* this templlate does not have an extension and therefore
	     * matches */
	    if (ext)
	    {   /* if we have an extension, add it */
		gchar *str = g_strdup_printf ("%s%s", *tplp, ext);
		g_free (*tplp);
		*tplp = str;
	    }
	    break;
	}
	else if (ext && (strlen (*tplp) >= strlen (ext)))
	{  /* this template is valid if the extensions match */
	    if (strcmp (&((*tplp)[strlen (*tplp) - strlen (ext)]), ext) == 0)
		break;
	}
	++tplp;
    }
    result = g_strdup (*tplp);
    g_strfreev (templates);
    return result;
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
    GString *result;
    gchar *res_utf8, *res_cs = NULL;
    gchar dummy[100];
    gchar *p, *template, *basename = NULL;
    gboolean original=FALSE; /* indicate whether original filename is
			      * being used */

    if (!track) return NULL; 

    template = get_export_filename_template (track);

    if (!template)
    {
	gchar *fn = get_track_name_on_disk (track);
	gtkpod_warning (_("Output filename template ('%s') does not match the file '%s'\n"), prefs_get_export_template (), fn ? fn:"");
	g_free (fn);
	return NULL;
    }

    result = g_string_new ("");

    /* try to get the original filename */
    if (track->pc_path_utf8)
	basename = g_path_get_basename (track->pc_path_utf8);

    p=template;
    while (*p != '\0') {
	if (*p == '%') {
	    gchar* tmp = NULL;
	    gchar *fixtmp = NULL;
	    p++;
	    switch (*p) {
	    case 'o':
		if (basename)
		{
		    tmp = basename;
		    original = TRUE;
		}
		break;
	    case 'a':
		tmp = track_get_item_utf8 (track, T_ARTIST);
		break;
	    case 'A':
		tmp = track_get_item_utf8 (track, T_ALBUM);
		break;
	    case 't':
		tmp = track_get_item_utf8 (track, T_TITLE);
		break;
	    case 'c':
		tmp = track_get_item_utf8 (track, T_COMPOSER);
		break;
	    case 'g':
	    case 'G':
		tmp = track_get_item_utf8 (track, T_GENRE);
		break;
	    case 'C':
		tmp = dummy;
		if (track->cds == 0)
		    sprintf (tmp, "%.2d", track->cd_nr);
		else if (track->cds < 10)
		    sprintf(tmp, "%.1d", track->cd_nr);
		else if (track->cds < 100)
		    sprintf (tmp, "%.2d", track->cd_nr);
		else if (track->cds < 1000)
		    sprintf (tmp, "%.3d", track->cd_nr);
		else {
		    sprintf (tmp,"%.4d", track->cd_nr);
		}
		break;
	    case 'T':
		tmp = dummy;
		if (track->tracks == 0)
		    sprintf (tmp, "%.2d", track->track_nr);
		else if (track->tracks < 10)
		    sprintf(tmp, "%.1d", track->track_nr);
		else if (track->tracks < 100)
		    sprintf (tmp, "%.2d", track->track_nr);
		else if (track->tracks < 1000)
		    sprintf (tmp, "%.3d", track->track_nr);
		else {
		    sprintf (tmp,"%.4d", track->track_nr);
		}
		break;
	    case 'Y':
		tmp = dummy;
		sprintf (tmp, "%4d", track->year);
		break;
	    case '%':
		tmp = "%";
		break;
	    default:
		gtkpod_warning (_("Unknown token '%%%c' in template '%s'"),
				*p, template);
		break;
	    }
	    if (tmp)
	    {
		if(prefs_get_fix_path())
		{
		    fixtmp = fix_path (tmp);
		    tmp = fixtmp;
		}
		result = g_string_append (result, tmp);
		tmp = NULL;
		g_free (fixtmp);
		fixtmp = NULL;
	    }
	}
	else 
	    result = g_string_append_c (result, *p);
	p++;
    }
    /* get the utf8 version of the filename */
    res_utf8 = g_string_free (result, FALSE);
    /* convert it to the charset */
    if (prefs_get_special_export_charset ())
    {   /* use the specified charset */
	res_cs = charset_from_utf8 (res_utf8);
    }
    else
    {   /* use the charset stored in track->charset */
	res_cs = charset_track_charset_from_utf8 (track, res_utf8);
    }
    g_free (basename);
    g_free (res_utf8);
    g_free (template);
    return res_cs;
}

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
 * file_export_cleanup - Free all data structures used by this subsystem
 * set all relevant pointers back to NULL
 */
static void
file_export_cleanup(void)
{
    if(file_export.dest_dir) g_free(file_export.dest_dir);
    if(file_export.tracks) g_list_free(file_export.tracks);
    if(file_export.fs) gtk_widget_destroy(file_export.fs);
    memset(&file_export, 0, sizeof(file_export));
    release_widgets ();
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
    
    if(prefs_get_export_check_existing() && file_is_ok(file, dest))
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
	gchar *filename = g_build_filename (file_export.dest_dir,
					    dest_file, NULL);
	if (mkdirhier(filename)) {
	    if(copy_file(from_file, filename))
	        result = TRUE;
	}
	g_free(from_file);
	g_free(dest_file);
	g_free(filename);
    }
    return(result);
}



#ifdef G_THREADS_ENABLED
/* Threaded write_track */
static gpointer th_write_track (gpointer s)
{
    gboolean result = write_track ((Track *)s);

    g_mutex_lock (mutex);
    mutex_data = TRUE;   /* signal that thread will end */
    g_cond_signal (cond);
    g_mutex_unlock (mutex);

    return (gpointer)result;
}
#endif

/* This function is called when the user presses the abort button
 * during flush_tracks() */
static void write_tracks_abort (gboolean *abort)
{
    *abort = TRUE;
}


/**
 * file_export_do - if we get the "Ok" from the file selection we wanna
 * write the files out to disk
 */
static void
file_export_do(void)
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

    /* close the file requester */
    if(file_export.fs)
    {
	gtk_widget_destroy (file_export.fs);
	file_export.fs = NULL;
    }


    block_widgets ();

    abort = FALSE;
    n = g_list_length (file_export.tracks);
    /* calculate total length to be copied */
    for(l = file_export.tracks; l && !abort; l = l->next)
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
	time_t diff, start, mins, secs;

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
	for(l = file_export.tracks; l && !abort; l = l->next)
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
	    mins = ((diff/fraction)-diff+5)/60;
	    secs = ((((time_t)(diff/fraction)-diff+5) % 60) / 5) * 5;
	    /* don't bounce up too quickly (>10% change only) */
/*	      left = ((mins < left) || (100*mins >= 110*left)) ? mins : left;*/
	    progtext = g_strdup_printf (_("%.0f%% (%d:%02d) left"),
					100*fraction, (int)mins, (int)secs);
	    gtk_progress_bar_set_text(GTK_PROGRESS_BAR (progress_bar),
				      progtext);
	    g_free (progtext);
	    while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
	}
	gtk_widget_destroy (dialog);
	if (!result || abort)
	    gtkpod_statusbar_message (_("Some tracks were not copied."));
    }
    file_export_cleanup();
    release_widgets ();
}

/**
 * export_files_cancel_button_clicked - when the user aborts the file
 * exporting or the file selection receives a delete_event
 * @w - the button clicked
 * @data - a pointer to the fileselection
 */
static void
export_files_cancel_button_clicked(GtkWidget *w, gpointer data)
{
    file_export_cleanup();
}

/**
 * export_files_ok_button_clicked - when the user clicks the "Ok" button on
 * the export file selection
 * @w - the button clicked
 * @data - a pointer to the fileselection
 */
static void
export_files_ok_button_clicked(GtkWidget *w, gpointer data)
{
    if((w) && (data))
    {
	const gchar *name;
	name = gtk_file_selection_get_filename (GTK_FILE_SELECTION(data));
	if(name)
	{
	    prefs_set_last_dir_export (name);
	    file_export.dest_dir = g_strdup (prefs_get_last_dir_export ());
	    file_export_do();
	}
	else file_export_cleanup ();
    }
}


/**
 * export_file_init - Export files off of your ipod to an arbitrary
 * directory, specified by the file selection dialog
 * @tracks - GList with data of type (Track*) we want to write 
 */
void
file_export_init(GList *tracks)
{
    GtkWidget *w = NULL;

    if(!file_export.fs)
    {
	file_export.tracks = g_list_copy (tracks);
	w = gtk_file_selection_new(_("Select Export Destination Directory"));
	gtk_file_selection_set_select_multiple (GTK_FILE_SELECTION(w),
						FALSE);
	gtk_file_selection_set_filename(GTK_FILE_SELECTION(w),
					prefs_get_last_dir_export ());
	g_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(w)->ok_button),
		"clicked", G_CALLBACK(export_files_ok_button_clicked), w);
	g_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(w)->cancel_button),
		"clicked", G_CALLBACK(export_files_cancel_button_clicked), w);
	g_signal_connect(GTK_OBJECT(w), "delete_event",
		G_CALLBACK(export_files_cancel_button_clicked), w);
	file_export.fs = w;
	gtk_widget_show(w);
	block_widgets ();
    }
}


