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
#include "file_export.h"
#include "prefs.h"
#include <limits.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>


/**
 * READ_WRITE_BLOCKSIZE - how many bytes we read per fread/fwrite call
 */
#define READ_WRITE_BLOCKSIZE (PATH_MAX * 16)

/**
 * Private Variables for this subsystem
 * @title - The title of the export directory
 * @songs - A GList of Song* references to export
 */
static struct {
    gchar *title;
    GList *songs;
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
	}
	result = g_strdup(buf);
    }
    return(result);
}

/**
 * file_export_private_data_reinit - Initialize the file_export members
 * @title - full path to the output folder
 * @songs - the GList we need to save to iterate over the songs
 */
static void
file_export_private_data_reinit(gchar *title, GList *songs)
{
    file_export.title = g_strdup(title);
    file_export.songs = songs;
}
/**
 * file_export_cleanup - Free all data structures used by this subsystem
 */
static void
file_export_cleanup(void)
{
    if(file_export.title) g_free(file_export.title);
    if(file_export.songs) g_list_free(file_export.songs);
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
    gchar *dest_file = NULL;
    gchar buf[PATH_MAX];
    gchar *from_file = NULL;
    gboolean result = FALSE;
    
    if((tmp = get_preferred_song_name_format(s)))
    {
	dest_file = chop_filename(tmp);
	g_free(tmp);
	from_file = get_song_name_on_disk(s);
	snprintf(buf, PATH_MAX, "%s%s", file_export.title, dest_file); 
	if(copy_file_from_file_to_dest(from_file, buf))
	    result = TRUE;
	g_free(from_file);
	g_free(dest_file);
    }
    return(result);
}

/**
 * export_file_init - Export files off of your ipod to an arbitrary
 * directory
 * @dir - the directory the export folder resides in, with final '/' 
 * @title - the folder name in dir, with final '/'
 * @songs - GList with data of type (Song*)
 * Returns - TRUE on successful write, FALSE otherwise
 */
gboolean
file_export_init(gchar *dir, gchar *title,GList *songs)
{
    Song *s = NULL;
    GList *l = NULL;
    gboolean result = FALSE;
    gchar buf[PATH_MAX];

    file_export.title = NULL;
    file_export.songs = NULL;
    
    if(!title) 
	title = g_strdup("gtkpod_export/");
    if(!dir) 
	snprintf(buf, PATH_MAX, "%s%s", cfg->last_dir.file_export, title);
    else
	snprintf(buf, PATH_MAX, "%s%s", dir, title);
    
    file_export_private_data_reinit(buf, songs);
    if(!g_file_test(buf, G_FILE_TEST_IS_DIR))
    {
	if((mkdir(buf, 0755)) == 0)
	{
	    fprintf(stderr, "Created dir %s\n", buf);
	}
	else
	{
	    switch(errno)
	    {
		case EPERM:
		    fprintf(stderr, "Bad perms for mkdir\n");
		    break;
		default:
		    fprintf(stderr, "Creation error %d\n", errno);
		    break;
	    }
	    return(FALSE);
	}
    }
    if(songs)
    {
	for(l = file_export.songs; l; l = l->next)
	{
	    s = (Song*)l->data;
	    if(write_song(s))
	    {
		fprintf(stderr, "Wrote %s-%s\n", s->artist, s->title);	
	    }
	    else
	    {
		fprintf(stderr, "Failed to write %s-%s\n", s->artist, s->title);	
	    }
	    l->data = NULL;
	}
	result = TRUE;
    }
    file_export_cleanup();
    return(result);
}
