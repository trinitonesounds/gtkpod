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
*/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "file_export.h"
#include "prefs.h"
#include "support.h"
#include <limits.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

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

/** chop_filename - Remove non-unix friendly characters from the fielname
 * @filename - the filename we're going to mangle
 * Returns - An allocated string with the new filename, you must free it
 */
static gchar*
chop_filename(const gchar *filename)
{
    int i = 0;
    gchar buf[PATH_MAX];
    gchar *result = NULL;
    
    if(filename)
    {
	snprintf(buf, PATH_MAX, "%s", filename);
	for(i = 0; i < PATH_MAX && buf[i] != '\0'; i++)
	{
	    if(buf[i] == ' ')
		buf[i] = '_';
	    else if(buf[i] == '/')
		buf[i] = '_';
	}
	result = g_strdup(buf);
    }
    return(result);
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
copy_file_from_fd_to_fd(FILE *from, FILE *to)
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
			fprintf(stderr, "%d is errno writing\n", errno);
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
 * copy_file_from_file_to_dest - copy the filename on disk named file, to
 * the destination file dest.  Both names are FULL pathnames to the file
 * @file - the filename to copy 
 * @dest - the filename we copy to
 * Returns TRUE on successful copying
 */
static gboolean 
copy_file_from_file_to_dest(gchar *file, gchar *dest)
{
    gboolean result = FALSE;
    FILE *from = NULL, *to = NULL;
    
    if((from = fopen(file, "r")))
    {
	if((to = fopen(dest, "w")))
	{
	    result = copy_file_from_fd_to_fd(from, to);
	    fclose(to);
	}
	else
	{
	    switch(errno)
	    {
		case EPERM:
		    fprintf(stderr, "Permission Error\n");
		default:
		    fprintf(stderr, "Unable to open %s for writing\n", dest);
		    break;
	    }
	}
	fclose(from);
    }
    else
    {
	fprintf(stderr, "Unable to open %s for reading\n", file);
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
    gchar *tmp = NULL;
    gchar buf[PATH_MAX];
    gchar *dest_file = NULL;
    gchar *from_file = NULL;
    gboolean result = FALSE;
    
    if((tmp = get_preferred_song_name_format(s)))
    {
	dest_file = chop_filename(tmp);
	g_free(tmp);
	from_file = get_song_name_on_disk(s);
	snprintf(buf, PATH_MAX, "%s/%s", file_export.dest_dir, dest_file); 
	if(copy_file_from_file_to_dest(from_file, buf))
	    result = TRUE;
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
	    if(!write_song(s))
		fprintf(stderr, "Failed to write %s-%s\n", s->artist, s->title);	
	    l->data = NULL;
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
	gchar **name = NULL;
	if((name = gtk_file_selection_get_selections(GTK_FILE_SELECTION(data))))
	{
	    if(name[0])
	    {
		if(g_file_test(name[0], G_FILE_TEST_IS_DIR))
		    file_export.dest_dir = g_strdup(name[0]);
		else
		    file_export.dest_dir = g_path_get_dirname(name[0]);

		g_strfreev(name);
		file_export_do();
	    }
	}
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
	file_export.songs = songs;
	w = gtk_file_selection_new(_("Select Export Destination Directory"));
	gtk_file_selection_set_filename(GTK_FILE_SELECTION(w),
					cfg->last_dir.export);
	g_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(w)->ok_button),
		"clicked", G_CALLBACK(export_files_ok_button_clicked), w);
	g_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(w)->cancel_button),
		"clicked", G_CALLBACK(export_files_cancel_button_clicked), w);
	g_signal_connect(GTK_OBJECT(w), "delete_event",
		G_CALLBACK(export_files_cancel_button_clicked), w);
	gtk_widget_show(w);
	file_export.fs = w;
    }
    else if(songs) 
    {
	    g_list_free(songs);
    }
}
