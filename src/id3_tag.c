/* Time-stamp: <2003-09-25 22:47:20 jcs>
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
#include <id3tag.h>
#include <gtk/gtk.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>

#include "id3_tag.h"
#include "charset.h"
#include "support.h"

static gchar* id3_get_string (struct id3_tag *tag, char *frame_name)
{
    gchar* rtn;
    const id3_ucs4_t *string;
    id3_utf8_t *utf8;
    struct id3_frame *frame;
    union id3_field *field;

    frame = id3_tag_findframe (tag, frame_name, 0);
    if (!frame) return NULL;

    if (frame_name == ID3_FRAME_COMMENT)
        field = id3_frame_field (frame, 3);
    else
        field = id3_frame_field (frame, 1);

    if (!field) return NULL;

    if (frame_name == ID3_FRAME_COMMENT)
        string = id3_field_getfullstring (field);
    else
        string = id3_field_getstrings (field, 0); 

    if (!string) return NULL;

    if (frame_name == ID3_FRAME_GENRE) 
       string = id3_genre_name (string);
    printf("%s\n", string);
    utf8 = id3_ucs4_utf8duplicate (string);
    rtn = charset_from_utf8 (utf8);
    g_free (utf8);
    return rtn;
}
 
static void id3_set_string (struct id3_tag *tag, const char *frame_name, const char *data)
{
    int res;
    struct id3_frame *frame;
    union id3_field *field;
    id3_ucs4_t *ucs4;
    id3_utf8_t *utf8;

    if (data == NULL) 
	return;

    printf ("updating id3: %s: %s\n", frame_name, data);

    /*
     * An empty string removes the frame altogether.
     */
    if (strlen(data) == 0)
    {
        while ((frame = id3_tag_findframe (tag, frame_name, 0)))
 	    id3_tag_detachframe (tag, frame);
	return;
    }

    frame = id3_tag_findframe (tag, frame_name, 0);
    if (!frame) 
    {
	frame = id3_frame_new (frame_name);
	id3_tag_attachframe (tag, frame);
    }

    if (frame_name == ID3_FRAME_COMMENT)
    {
	field = id3_frame_field (frame, 3);
	field->type = ID3_FIELD_TYPE_STRINGFULL;
    }
    else
    {
	field = id3_frame_field (frame, 1);
	field->type = ID3_FIELD_TYPE_STRINGLIST;
    }

    utf8 = charset_to_utf8 (data);
    ucs4 = id3_utf8_ucs4duplicate (utf8);

    if (frame_name == ID3_FRAME_GENRE)
    {
	char *tmp;
 	int index = id3_genre_number (ucs4);
	g_free (ucs4);
	tmp = g_strdup_printf("%d", index);
	ucs4 = id3_latin1_ucs4duplicate (tmp);
    }
    g_free (utf8);

    if (frame_name == ID3_FRAME_COMMENT)
        res = id3_field_setfullstring (field, ucs4);
    else 
        res = id3_field_setstrings (field, 1, &ucs4);

    if (res != 0)
	g_print("error setting id3 field: %s\n", frame_name);
}

/***
 * Reads id3v1.x / id3v2 tag and load data into the File_Tag structure.
 * If a tag entry exists (ex: title), we allocate memory, else value stays to NULL
 * @returns: TRUE on success, else FALSE.
 */
gboolean Id3tag_Read_File_Tag (gchar *filename, File_Tag *FileTag)
{
    struct id3_file *id3file;
    struct id3_tag *tag;
    gchar* string;
    gchar* string2;

    if (!filename || !FileTag)
        return FALSE;

    memset (FileTag, 0, sizeof (File_Tag));

    if (!(id3file = id3_file_open (filename, ID3_FILE_MODE_READONLY)))
    {
	gchar *fbuf = charset_to_utf8 (filename);
        g_print(_("ERROR while opening file: '%s' (%s).\n"),
		fbuf, g_strerror(errno));
	g_free (fbuf);
        return FALSE;
    }

    if ((tag = id3_file_tag(id3file)))
    {
        FileTag->title = id3_get_string (tag, ID3_FRAME_TITLE);
        FileTag->artist = id3_get_string (tag, ID3_FRAME_ARTIST);
        FileTag->album = id3_get_string (tag, ID3_FRAME_ALBUM);
        FileTag->year = id3_get_string (tag, ID3_FRAME_YEAR);
        FileTag->composer = id3_get_string (tag, "TCOM");
        FileTag->comment = id3_get_string (tag, ID3_FRAME_COMMENT);
	FileTag->genre = id3_get_string (tag, ID3_FRAME_GENRE);

	string = id3_get_string (tag, "TLEN");
	if (string) 
	{
            FileTag->songlen = (guint32) strtoul (string, 0, 10);
	    g_free (string);
	}

	string = id3_get_string (tag, ID3_FRAME_TRACK);
	if (string)
	{
	    string2 = strchr(string,'/');
	    if (string2)
	    {
	        FileTag->track_total = g_strdup_printf ("%.2d", atoi (string2+1));
	        *string2 = '\0';
	    }
	    FileTag->track = g_strdup_printf ("%.2d", atoi (string));
            g_free(string);
	}
    }

    id3_file_close (id3file);
    return TRUE;
}


/*
 * Write the ID3 tags to the file. Returns TRUE on success, else 0.
 */
gboolean Id3tag_Write_File_Tag (gchar *filename, File_Tag *FileTag)
{
    struct id3_tag* tag;
    struct id3_file* id3file;
    gint error = 0;

    id3file = id3_file_open (filename, ID3_FILE_MODE_READWRITE);
    if (!id3file)
    {
	gchar *fbuf = charset_to_utf8 (filename);
        g_print(_("ERROR while opening file: '%s' (%s).\n"),
		fbuf, g_strerror(errno));
	g_free (fbuf);
        return FALSE;
    }

    if ((tag = id3_file_tag(id3file)))
    {
	id3_set_string (tag, ID3_FRAME_TITLE, FileTag->title);
	id3_set_string (tag, ID3_FRAME_ARTIST, FileTag->artist);
	id3_set_string (tag, ID3_FRAME_ALBUM, FileTag->album);
	id3_set_string (tag, ID3_FRAME_GENRE, FileTag->genre);
	id3_set_string (tag, ID3_FRAME_COMMENT, FileTag->comment);
	id3_set_string (tag, "TCOM", FileTag->composer);

        if ( FileTag->track_total && strlen(FileTag->track_total)>0 )
	{
	    char* string = g_strconcat(FileTag->track,"/",FileTag->track_total,NULL);
    	    id3_set_string (tag, ID3_FRAME_TRACK, string);
	    g_free(string);
	}
	else
	    id3_set_string (tag, ID3_FRAME_TRACK, FileTag->track);
    }

    if (id3_file_update(id3file) != 0)
    {
	gchar *fbuf = charset_to_utf8 (filename);
        g_print(_("ERROR while writing tag to file: '%s' (%s).\n"),
		fbuf, g_strerror(errno));
	g_free (fbuf);
        return FALSE;
    }

    id3_file_close (id3file);

    if (error) return FALSE;
    else       return TRUE;
}



/*
 * Return the sub version of the ID3v2 tag, for example id3v2.2, id3v2.3
 */
gint Id3tag_Get_Id3v2_Version (gchar *filename)
{
#if 0
    FILE *file;
	guchar tmp[4];

    if ( filename!=NULL && (file=fopen(filename,"r"))!=NULL )
    {
        fseek(file,0,SEEK_SET);
        if (fread(tmp,1,4, file) != 4)
            return -1;

        if (tmp[0] == 'I' && tmp[1] == 'D' && tmp[2] == '3' && tmp[3] < 0xFF)
        {
            fclose(file);
            return (gint)tmp[3];
        }else
        {
            return -1;
        }
    }else
#endif
    {
        return -1;
    }
}
