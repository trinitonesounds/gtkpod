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

#include "file_export.h"
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
#include <glib/gprintf.h>

/**
 * READ_WRITE_BLOCKSIZE - how many bytes we read per fread/fwrite call
 */
#define READ_WRITE_BLOCKSIZE 65536

/**
 * Private Variables for this subsystem
 * @dest_dir - The export directory we're putting the files in
 * @songs - A GList of Song* references to export
 * @fs - the file selection dialog
 */
static struct {
    GList *songs;
    gchar *dest_dir;
    GtkWidget *fs;
} file_export;


/**
 * get_preferred_filename - useful for generating the preferred
 * @param Song the song
 * @return the file filename (including directories) for this Song
 * based on the users preferences.  The returned char* must be freed
 * by the caller.
 */
static gchar *
song_get_export_filename (Song *song)
{
    GString *result;
    char *res_utf8, *res_cs = NULL;
    char dummy[5];
    char *p, *str, *extension=NULL, *basename = NULL;
    gboolean original=FALSE; /* indicate whether original filename is
			      * being used */

    if (!song) return NULL; 
    result = g_string_new ("");

    p = prefs_get_filename_format ();

    /* try to get an extension */
    str = get_track_name_on_disk (song);
    if (str)
    {
	gchar *s = strrchr (str, '.');
	if (s)     extension = g_strdup (s);
	C_FREE (str);
    }
    /* well... we need some default */
    /* if (!extension)   extension = g_strdup (".mp3");*/

    /* try to get the original filename */
    if (song->pc_path_utf8)
	basename = g_path_get_basename (song->pc_path_utf8);

    while (*p != '\0') {
	if (*p == '%') {
	    char* tmp = NULL;
	    p++;
	    switch (*p) {
	    case 'o':
		if (basename)
		{
		    tmp = basename;
		    original = TRUE;
		}
		break;
	    case 'A':
		tmp = song_get_item_utf8 (song, S_ARTIST);
		break;
	    case 'd':
		tmp = song_get_item_utf8 (song, S_ALBUM);
		break;
	    case 'n':
		tmp = song_get_item_utf8 (song, S_TITLE);
		break;
	    case 't':
		tmp = dummy;
		if (song->tracks == 0)
		    sprintf (tmp, "%.2d", song->track_nr);
		else if (song->tracks < 10)
		    sprintf(tmp, "%.1d", song->track_nr);
		else if (song->tracks < 100)
		    sprintf (tmp, "%.2d", song->track_nr);
		else if (song->tracks < 1000)
		    sprintf (tmp, "%.3d", song->track_nr);
		else {
		    g_print ("wow, more that 1000 tracks!");
		    sprintf (tmp,"%.4d", song->track_nr);
		}
		break;
	    default:
		/* 
		 * [FIXME] add more cases here and an error message for 
		 * the default case
		 */
		break;
	    }
	    if (tmp) {
		result = g_string_append (result, tmp);
		tmp = NULL;
	    }
	}
	else 
	    result = g_string_append_c (result, *p);
	p++;
    }
    C_FREE (basename);
    /* add the extension */
    if (!original && extension)
	result = g_string_append (result, extension);
    C_FREE (extension);
    /* get the utf8 version of the filename */
    res_utf8 = g_string_free (result,FALSE);
    /* convert it to the charset */
    if (prefs_get_special_export_charset ())
    {   /* use the specified charset */
	res_cs = charset_from_utf8 (res_utf8);
    }
    else
    {   /* use the charset stored in song->charset */
	res_cs = charset_song_charset_from_utf8 (song, res_utf8);
    }
    C_FREE (res_utf8);
    return res_cs;
}

/**
 * Recursively make directories in the given filename.
 * @return FALSE is this is not possible.
 */
gboolean
make_dirs(char* filename)
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
    if(file_export.songs) g_list_free(file_export.songs);
    if(file_export.fs) gtk_widget_destroy(file_export.fs);
    memset(&file_export, 0, sizeof(file_export));
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
 * write_song_to_dir - copy the Song* to the desired output file
 * @s - The Song reference we're manipulating
 * Returns - TRUE on success, FALSE on failure
 */
static gboolean
write_song(Song *s)
{
    gchar *from_file = NULL;
    gchar *dest_file = NULL;
    gchar buf[PATH_MAX];
    gboolean result = FALSE;
    
    if((dest_file = song_get_export_filename(s)))
    {
	from_file = get_track_name_on_disk(s);
	g_snprintf(buf, PATH_MAX, "%s/%s", file_export.dest_dir, dest_file); 
	if (make_dirs(buf)) {
	    if(copy_file(from_file, buf))
	        result = TRUE;
	}
	g_free(from_file);
	g_free(dest_file);
    }
    return(result);
}

/**
 * file_export_do - if we get the "Ok" from the file selection we wanna
 * write the files out to disk
 */
static void
file_export_do(void)
{
    Song *s = NULL;
    GList *l = NULL;
    
    if(file_export.songs)
    {
	for(l = file_export.songs; l; l = l->next)
	{
	    s = (Song*)l->data;
	    if (song_is_valid (s))
	    {
		if(!write_song(s))
		    gtkpod_warning (_("Failed to write %s-%s\n"), s->artist, s->title);	
		l->data = NULL;
	    }
	}
    }
    file_export_cleanup();
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
	    file_export.dest_dir = g_strdup (cfg->last_dir.export);
	    file_export_do();
	}
	else file_export_cleanup ();
    }
}


/**
 * export_file_init - Export files off of your ipod to an arbitrary
 * directory, specified by the file selection dialog
 * @songs - GList with data of type (Song*) we want to write 
 */
void
file_export_init(GList *songs)
{
    GtkWidget *w = NULL;

    if(!file_export.fs)
    {
	/* FIXME: we should besser use an ID list since songs might be
	   removed during the procedure of selecting a directory */
	file_export.songs = g_list_copy (songs);
	w = gtk_file_selection_new(_("Select Export Destination Directory"));
	gtk_file_selection_set_select_multiple (GTK_FILE_SELECTION(w),
						FALSE);
	gtk_file_selection_set_filename(GTK_FILE_SELECTION(w),
					cfg->last_dir.export);
	g_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(w)->ok_button),
		"clicked", G_CALLBACK(export_files_ok_button_clicked), w);
	g_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(w)->cancel_button),
		"clicked", G_CALLBACK(export_files_cancel_button_clicked), w);
	g_signal_connect(GTK_OBJECT(w), "delete_event",
		G_CALLBACK(export_files_cancel_button_clicked), w);
	file_export.fs = w;
	gtk_widget_show(w);
    }
}
