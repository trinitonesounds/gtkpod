/*
|  Changed by Jorg Schuler <jcsjcs at users.sourceforge.net> to
|  compile "standalone" with the gtkpod project. 2002/11/24
|  Changed by Jorg Schuler to also determine size of file and
|  length of song in ms. 2002/11/28.
|  Modified character conversion handling. Jan 2003
*/


/* id3tag.c - 2001/02/16 */
/*
 *  EasyTAG - Tag editor for MP3 and OGG files
 *  Copyright (C) 2001-2002  Jerome Couderc <j.couderc@ifrance.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <id3.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "id3_tag.h"
#include "genres.h"

#include "charset.h"
#include "support.h"


/****************
 * Declarations *
 ****************/
#define ID3V2_MAX_STRING_LEN 4096
#define CONVERT_OLD_ID3V2_TAG_VERSION FALSE
 /* USE_CHARACTER_SET_TRANSLATION impemented by charset_from/to_utf8 () */
#define USE_CHARACTER_SET_TRANSLATION TRUE
#define NUMBER_TRACK_FORMATED FALSE
#define STRIP_TAG_WHEN_EMPTY_FIELDS FALSE
#define WRITE_ID3V1_TAG TRUE
#define WRITE_ID3V2_TAG TRUE

/**************
 * Prototypes *
 **************/
gchar *Id3tag_Get_Error_Message(ID3_Err error);
gint   Id3tag_Get_Id3v2_Version (gchar *filename);
ID3_C_EXPORT size_t ID3Tag_Link_1       (ID3Tag *id3tag, const char *filename);
ID3_C_EXPORT size_t ID3Field_GetASCII_1 (const ID3Field *field, char *buffer, size_t maxChars, size_t itemNum);

/*************
 * Functions *
 *************/


static gchar *convert_from_file_to_user(gchar *s)
{
    return charset_to_utf8 (s);
}

static gchar *convert_from_user_to_file(gchar *s)
{
    return charset_from_utf8 (s);
}

/*
 * Delete spaces at the end and the beginning of the string 
 */
static void Strip_String (gchar *string)
{
    if (!string) return;
    string = g_strstrip(string);
}


/*
 * Read id3v1.x / id3v2 tag and load data into the File_Tag structure using id3lib functions.
 * Returns TRUE on success, else FALSE.
 * If a tag entry exists (ex: title), we allocate memory, else value stays to NULL
 */
gboolean Id3tag_Read_File_Tag (gchar *filename, File_Tag *FileTag)
{
    FILE   *file;
    ID3Tag *id3_tag = NULL;    /* Tag defined by the id3lib */
    gchar  *string, *string1, *string2;


    if (!filename || !FileTag)
        return FALSE;

    if ( (file=fopen(filename,"r"))==NULL )
    {
	gchar *fbuf = charset_to_utf8 (filename);
        g_print(_("ERROR while opening file: '%s' (%s).\n"),
		fbuf, g_strerror(errno));
	g_free (fbuf);
        return FALSE;
    }
    fseek (file, 0, SEEK_END);
    FileTag->size = ftell (file); /* get the filesize in bytes */
    fclose(file); /* We close it cause id3lib opens/closes file itself */


    /* Get data from tag */
    if ( (id3_tag = ID3Tag_New()) )
    {
        ID3Frame *id3_frame;
        ID3Field *id3_field;
        size_t offset;
        luint num_chars;
        guint field_num = 0; /* First field */

        /* Link the file to the tag */
        offset = ID3Tag_Link_1(id3_tag,filename);
        if ( offset && CONVERT_OLD_ID3V2_TAG_VERSION )
        {
            gint id3v2_version = Id3tag_Get_Id3v2_Version(filename);
            /* If it's an old tag version, we force to change it to an id3v2.3 one */
            if (id3v2_version>0 && id3v2_version<3)
                FileTag->saved = FALSE;
        }

        /* Protection against invalid ID3v2 tag, for example ID3v2.4 in
         * id3lib-3.8.0 : if offset is not nul and the tag contains no
         * frame, may be an invalid id3v2 tag. So we read the id3v1 tag */
#       if ( (ID3LIB_MAJOR >= 3) && (ID3LIB_MINOR >= 8)  )
            if ( offset!=0 && ID3Tag_NumFrames(id3_tag)==0 )
                offset = ID3Tag_LinkWithFlags(id3_tag,filename,ID3TT_ID3V1);
#       endif*/

        string = g_malloc(ID3V2_MAX_STRING_LEN+1);


        /*********
         * Title *
         *********/
        if ( (id3_frame = ID3Tag_FindFrameWithID(id3_tag,ID3FID_TITLE)) )
        {
            if ( (id3_field = ID3Frame_GetField(id3_frame,ID3FN_TEXT)) )
            {
	      /* FIX ME : 'ID3FrameInfo_NumFields' would be better...
                 Note: if 'num_chars' is equal to 0, then the field is empty or corrupted! */
                if ( (num_chars=ID3Field_GetASCII_1(id3_field,string,ID3V2_MAX_STRING_LEN,field_num)) > 0
                     && string != NULL )
                {
                    if (USE_CHARACTER_SET_TRANSLATION)
                    {
		        string1 = convert_from_file_to_user(string);
                        /* Strip_String(string1);*/
                        FileTag->title = g_strdup(string1);
                        g_free(string1);
			if (!FileTag->auto_charset)
			    FileTag->auto_charset = 
				charset_check_auto (string);
                    }else
                    {
		        /* Strip_String(string);*/
                        FileTag->title = g_strdup(string);
                    }
                }
            }
        }


        /**********
         * Artist *
         **********/
        if ( (id3_frame = ID3Tag_FindFrameWithID(id3_tag,ID3FID_LEADARTIST)) )
        {
            if ( (id3_field = ID3Frame_GetField(id3_frame,ID3FN_TEXT)) )
            {
                if ( (num_chars=ID3Field_GetASCII_1(id3_field,string,ID3V2_MAX_STRING_LEN,field_num)) > 0
                     && string != NULL )
                {
                    if (USE_CHARACTER_SET_TRANSLATION)
                    {
		        string1 = convert_from_file_to_user(string);
                        /* Strip_String(string1);*/
                        FileTag->artist = g_strdup(string1);
                        g_free(string1);
			if (!FileTag->auto_charset)
			    FileTag->auto_charset = 
				charset_check_auto (string);
                    }else
                    {
		        /* Strip_String(string); */
                        FileTag->artist = g_strdup(string);
                    }
                }
            }
        }


        /*********
         * Album *
         *********/
        if ( (id3_frame = ID3Tag_FindFrameWithID(id3_tag,ID3FID_ALBUM)) )
        {
            if ( (id3_field = ID3Frame_GetField(id3_frame,ID3FN_TEXT)) )
            {
                if ( (num_chars=ID3Field_GetASCII_1(id3_field,string,ID3V2_MAX_STRING_LEN,field_num)) > 0
                     && string != NULL )
                {
                    if (USE_CHARACTER_SET_TRANSLATION)
                    {
                        string1 = convert_from_file_to_user(string);
                        /* Strip_String(string1);*/
                        FileTag->album = g_strdup(string1);
                        g_free(string1);
			if (!FileTag->auto_charset)
			    FileTag->auto_charset = 
				charset_check_auto (string);
                    }else
                    {
		        /* Strip_String(string); */
                        FileTag->album = g_strdup(string);
                    }
                }
            }
        }


        /***********
         * SONGLEN *
         ***********/
        if ( (id3_frame = ID3Tag_FindFrameWithID(id3_tag,ID3FID_SONGLEN)) )
        {
            if ( (id3_field = ID3Frame_GetField(id3_frame,ID3FN_TEXT)) )
            {
                if ( (num_chars=ID3Field_GetASCII_1(id3_field,string,ID3V2_MAX_STRING_LEN,field_num)) > 0
                     && string != NULL )
                {
		  FileTag->songlen = (guint32)strtoul (string, NULL, 10);
                }
            }
        }


        /********
         * Year *
         ********/
        if ( (id3_frame = ID3Tag_FindFrameWithID(id3_tag,ID3FID_YEAR)) )
        {
            if ( (id3_field = ID3Frame_GetField(id3_frame,ID3FN_TEXT)) )
            {
                if ( (num_chars=ID3Field_GetASCII_1(id3_field,string,ID3V2_MAX_STRING_LEN,field_num)) > 0
                     && string != NULL )
                {
                    gchar *tmp_str;

                    Strip_String(string);

                    /* Fix for id3lib 3.7.x: if the id3v1.x tag was filled with spaces
                     * instead of zeroes, then the year field contains garbages! */
                    tmp_str = string;
                    while (isdigit(*tmp_str)) tmp_str++;
                    *tmp_str = 0;
                    /* End of fix for id3lib 3.7.x */

                    if (USE_CHARACTER_SET_TRANSLATION)
                    {
                        string1 = convert_from_file_to_user(string);
                        /* Strip_String(string1);*/
                        FileTag->year = g_strdup(string1);
                        g_free(string1);
                    }else
                    {
		        /* Strip_String(string);*/
                        FileTag->year = g_strdup(string);
                    }
                }
            }
        }


        /*************************
         * Track and Total Track *
         *************************/
        if ( (id3_frame = ID3Tag_FindFrameWithID(id3_tag,ID3FID_TRACKNUM)) )
        {
            if ( (id3_field = ID3Frame_GetField(id3_frame,ID3FN_TEXT)) )
            {
                if ( (num_chars=ID3Field_GetASCII_1(id3_field,string,ID3V2_MAX_STRING_LEN,field_num)) > 0
                     && string != NULL )
                {
                    
		  /* Strip_String(string);*/

                    if (USE_CHARACTER_SET_TRANSLATION)
                    {
                        string1 = convert_from_file_to_user(string);
                        string2 = strchr(string1,'/');
                        if (NUMBER_TRACK_FORMATED)
                        {
                            if (string2)
                            {
			    FileTag->track_total = g_strdup_printf("%.2d",atoi(string2+1)); /* Just to have numbers like this : '01', '05', '12', ...*/
                                *string2 = '\0';
                            }
                            FileTag->track = g_strdup_printf("%.2d",atoi(string1)); /* Just to have numbers like this : '01', '05', '12', ... */
                        }else
                        {
                            if (string2)
                            {
                                FileTag->track_total = g_strdup(string2+1);
                                *string2 = '\0';
                            }
                            FileTag->track = g_strdup(string1);
                        }
                        g_free(string1);
                    }else
                    {
                        string2 = strchr(string,'/');
                        if (NUMBER_TRACK_FORMATED)
                        {
                            if (string2)
                            {
			    FileTag->track_total = g_strdup_printf("%.2d",atoi(string2+1)); /* Just to have numbers like this : '01', '05', '12', ...*/
                                *string2 = '\0';
                            }
                            FileTag->track = g_strdup_printf("%.2d",atoi(string)); /* Just to have numbers like this : '01', '05', '12', ...*/
                        }else
                            {
                            if (string2)
                            {
                                FileTag->track_total = g_strdup(string2+1);
                                *string2 = '\0';
                            }
                            FileTag->track = g_strdup(string);
                        }
                    }
                }
            }
        }


        /*********
         * Genre *
         *********/
        if ( (id3_frame = ID3Tag_FindFrameWithID(id3_tag,ID3FID_CONTENTTYPE)) )
        {
            if ( (id3_field = ID3Frame_GetField(id3_frame,ID3FN_TEXT)) )
            {
                /*
                 * We manipulate only the name of the genre
                 */
                if ( (num_chars=ID3Field_GetASCII_1(id3_field,string,ID3V2_MAX_STRING_LEN,field_num)) > 0
                     && string != NULL )
                {
                    gchar *tmp;

                    /*Strip_String(string);*/

                    if ( (string[0]=='(') && (tmp=strchr(string,')')) && (strlen((tmp+1))>0) )
                    {
    
                        /* Convert a genre written as '(3)Dance' into 'Dance' */
                        if (USE_CHARACTER_SET_TRANSLATION)
                        {
                            string1 = convert_from_file_to_user(tmp+1);
                            FileTag->genre = g_strdup(string1);
                            g_free(string1);
                        }else
                        {
                            FileTag->genre = g_strdup(tmp+1);
                        }

                    }else if ( (string[0]=='(') && (tmp=strchr(string,')')) )
                    {
    
                        /* Convert a genre written as '(3)' into 'Dance' */
                        *tmp = 0;
                        if (USE_CHARACTER_SET_TRANSLATION)
                        {
                            string1 = convert_from_file_to_user(Id3tag_Genre_To_String(atoi(string+1)));
                            FileTag->genre = g_strdup(string1);
                            g_free(string1);
                        }else
                        {
                            FileTag->genre = g_strdup(Id3tag_Genre_To_String(atoi(string+1)));
                        }

                    }else
                    {

                        /* Genre is already written as 'Dance' */
                            if (USE_CHARACTER_SET_TRANSLATION)
                        {
                            string1 = convert_from_file_to_user(string);
                            FileTag->genre = g_strdup(string1);
                            g_free(string1);
                        }else
                        {
                            FileTag->genre = g_strdup(string);
                        }

                    }
                }
            }
        }



        /***********
         * Comment *
         ***********/
        if ( (id3_frame = ID3Tag_FindFrameWithID(id3_tag,ID3FID_COMMENT)) )
        {
            if ( (id3_field = ID3Frame_GetField(id3_frame,ID3FN_TEXT)) )
            {
                if ( (num_chars=ID3Field_GetASCII_1(id3_field,string,ID3V2_MAX_STRING_LEN,field_num)) > 0
                     && string != NULL )
                {
                    if (USE_CHARACTER_SET_TRANSLATION)
                    {
                        string1 = convert_from_file_to_user(string);
                        /*Strip_String(string1);*/
                        FileTag->comment = g_strdup(string1);
                        g_free(string1);
			if (!FileTag->auto_charset)
			    FileTag->auto_charset = 
				charset_check_auto (string);
                    }else
                    {
		        /*Strip_String(string);*/
                        FileTag->comment = g_strdup(string);
                    }
                }
            }
            /*if ( (id3_field = ID3Frame_GetField(id3_frame,ID3FN_DESCRIPTION)) )
            {
                gchar *comment1 = g_malloc0(MAX_STRING_LEN+1);
                num_chars = ID3Field_GetASCII(id3_field,comment1,MAX_STRING_LEN,Item_Num);
                g_free(comment1);
            }
            if ( (id3_field = ID3Frame_GetField(id3_frame,ID3FN_LANGUAGE)) )
            {
                gchar *comment2 = g_malloc0(MAX_STRING_LEN+1);
                num_chars = ID3Field_GetASCII(id3_field,comment2,MAX_STRING_LEN,Item_Num);
                g_free(comment2);
            }*/
        }
        g_free(string);

        /* Free allocated data */
        ID3Tag_Delete(id3_tag);
    }

    return TRUE;
}


/*
 * Write the ID3 tags to the file. Returns TRUE on success, else 0.
 */
gboolean Id3tag_Write_File_Tag (gchar *filename, File_Tag *FileTag)
{
    FILE     *file;
    ID3Tag   *id3_tag = NULL;
    ID3_Err   error_strip_id3v1  = ID3E_NoError;
    ID3_Err   error_strip_id3v2  = ID3E_NoError;
    ID3_Err   error_update_id3v1 = ID3E_NoError;
    ID3_Err   error_update_id3v2 = ID3E_NoError;
    gint error = 0;
    gint number_of_frames;
    gboolean has_title   = 1;
    gboolean has_artist  = 1;
    gboolean has_album   = 1;
    gboolean has_year    = 1;
    gboolean has_track   = 1;
    gboolean has_genre   = 1;
    gboolean has_comment = 1;


    /* Test to know if we can write into the file */
    if ( (file=fopen(filename,"r+"))==NULL )
    {
	gchar *fbuf = charset_to_utf8 (filename);
	g_print(_("ERROR while opening file: '%s' (%s).\n"),
		fbuf, g_strerror(errno));
	g_free (fbuf);
	return FALSE;
    }
    fclose(file);

    /* We get again the tag from the file to keep also unused data (by EasyTAG), then
     * we replace the changed data */
    if ( (id3_tag = ID3Tag_New()) )
    {
        ID3Frame *id3_frame;
        ID3Field *id3_field;
        gchar *string, *string1;

        ID3Tag_Link(id3_tag,filename);

        /*********
         * Title *
         *********/
        if (FileTag->title)
	{
	    if (g_utf8_strlen(FileTag->title, -1)>0 )
	    {
		/* To avoid problem with a corrupted field, we remove it before to creat a new one. */
		if ( (id3_frame = ID3Tag_FindFrameWithID(id3_tag,ID3FID_TITLE)) )
		    ID3Tag_RemoveFrame(id3_tag,id3_frame);
		id3_frame = ID3Frame_NewID(ID3FID_TITLE);
		ID3Tag_AttachFrame(id3_tag,id3_frame);
		
		if ((id3_field = ID3Frame_GetField(id3_frame,ID3FN_TEXT)))
		{
		    if (USE_CHARACTER_SET_TRANSLATION)
		    {
			string = convert_from_user_to_file(FileTag->title);
			ID3Field_SetASCII(id3_field,string);
			g_free(string);
		    }else
		    {
			ID3Field_SetASCII(id3_field,FileTag->title);
		    }
		}
	    } else
	    {
		if ( (id3_frame = ID3Tag_FindFrameWithID(id3_tag,ID3FID_TITLE)) )
		    ID3Tag_RemoveFrame(id3_tag,id3_frame);
		has_title = 0;
	    }
	}

        /**********
         * Artist *
         **********/
        if (FileTag->artist)
	{
	    if (g_utf8_strlen(FileTag->artist, -1)>0 )
	    {
		if ( (id3_frame = ID3Tag_FindFrameWithID(id3_tag,ID3FID_LEADARTIST)) )
		    ID3Tag_RemoveFrame(id3_tag,id3_frame);
		id3_frame = ID3Frame_NewID(ID3FID_LEADARTIST);
		ID3Tag_AttachFrame(id3_tag,id3_frame);
		
		if ((id3_field = ID3Frame_GetField(id3_frame,ID3FN_TEXT)))
		{
		    if (USE_CHARACTER_SET_TRANSLATION)
		    {
			string = convert_from_user_to_file(FileTag->artist);
			ID3Field_SetASCII(id3_field,string);
			g_free(string);
		    }else
		    {
			ID3Field_SetASCII(id3_field,FileTag->artist);
		    }
		}
	    } else
	    {
		if ( (id3_frame = ID3Tag_FindFrameWithID(id3_tag,ID3FID_LEADARTIST)) )
		    ID3Tag_RemoveFrame(id3_tag,id3_frame);
		has_artist = 0;
	    }
	}

        /*********
         * Album *
         *********/
        if (FileTag->album)
	{
	    if (g_utf8_strlen(FileTag->album, -1)>0 )
	    {
		if ( (id3_frame = ID3Tag_FindFrameWithID(id3_tag,ID3FID_ALBUM)) )
		    ID3Tag_RemoveFrame(id3_tag,id3_frame);
		id3_frame = ID3Frame_NewID(ID3FID_ALBUM);
		ID3Tag_AttachFrame(id3_tag,id3_frame);
		
		if ((id3_field = ID3Frame_GetField(id3_frame,ID3FN_TEXT)))
		{
		    if (USE_CHARACTER_SET_TRANSLATION)
		    {
			string = convert_from_user_to_file(FileTag->album);
			ID3Field_SetASCII(id3_field,string);
			g_free(string);
		    }else
		    {
			ID3Field_SetASCII(id3_field,FileTag->album);
		    }
		}
	    } else
	    {
		if ( (id3_frame = ID3Tag_FindFrameWithID(id3_tag,ID3FID_ALBUM)) )
		    ID3Tag_RemoveFrame(id3_tag,id3_frame);
		has_album = 0;
	    }
	}

        /*************************
         * Track and Total Track *
         *************************/
        if (FileTag->track)
	{
	    if (strlen(FileTag->track)>0 )
	    {
		if ( (id3_frame = ID3Tag_FindFrameWithID(id3_tag,ID3FID_TRACKNUM)) )
		    ID3Tag_RemoveFrame(id3_tag,id3_frame);
		id3_frame = ID3Frame_NewID(ID3FID_TRACKNUM);
		ID3Tag_AttachFrame(id3_tag,id3_frame);
		
		if ((id3_field = ID3Frame_GetField(id3_frame,ID3FN_TEXT)))
		{
		    if ( FileTag->track_total && strlen(FileTag->track_total)>0 )
			string1 = g_strconcat(FileTag->track,"/",FileTag->track_total,NULL);
		    else
			string1 = g_strdup(FileTag->track);
		    
		    if (USE_CHARACTER_SET_TRANSLATION)
		    {
			string = convert_from_user_to_file(string1);
			ID3Field_SetASCII(id3_field,string);
			g_free(string);
		    }else
		    {
			ID3Field_SetASCII(id3_field,string1);
		    }
		    g_free(string1);
		}
	    } else
	    {
		if ( (id3_frame = ID3Tag_FindFrameWithID(id3_tag,ID3FID_TRACKNUM)) )
		    ID3Tag_RemoveFrame(id3_tag,id3_frame);
		has_track = 0;
	    }
	}

        /*********
         * Genre *
         *********/
        if (FileTag->genre)
	{
	    if (g_utf8_strlen(FileTag->genre, -1)>0 )
	    {
		if ( (id3_frame = ID3Tag_FindFrameWithID(id3_tag,ID3FID_CONTENTTYPE)) )
		    ID3Tag_RemoveFrame(id3_tag,id3_frame);
		id3_frame = ID3Frame_NewID(ID3FID_CONTENTTYPE);
		ID3Tag_AttachFrame(id3_tag,id3_frame);
		
		if ((id3_field = ID3Frame_GetField(id3_frame,ID3FN_TEXT)))
		{
		    gchar *tmp_genre;
		    
		    if (USE_CHARACTER_SET_TRANSLATION)
		    {
			string = convert_from_user_to_file(FileTag->genre);
			tmp_genre = g_strdup_printf("(%d)%s",Id3tag_String_To_Genre(string),string);
			ID3Field_SetASCII(id3_field,tmp_genre);
			g_free(string);
		    }else
		    {
			tmp_genre = g_strdup_printf("(%d)%s",Id3tag_String_To_Genre(FileTag->genre),FileTag->genre);
			ID3Field_SetASCII(id3_field,tmp_genre);
		    }
		    g_free(tmp_genre);
		}
		
	    } else
	    {
		if ( (id3_frame = ID3Tag_FindFrameWithID(id3_tag,ID3FID_CONTENTTYPE)) )
		    ID3Tag_RemoveFrame(id3_tag,id3_frame);
		has_genre = 0;
	    }
	}


        /***********
         * Comment *
         ***********/
        if (FileTag->comment)
	{
	    if (g_utf8_strlen(FileTag->comment, -1)>0 )
	    {
		if ( (id3_frame = ID3Tag_FindFrameWithID(id3_tag,ID3FID_COMMENT)) )
		    ID3Tag_RemoveFrame(id3_tag,id3_frame);
		id3_frame = ID3Frame_NewID(ID3FID_COMMENT);
		ID3Tag_AttachFrame(id3_tag,id3_frame);
		
		if ((id3_field = ID3Frame_GetField(id3_frame,ID3FN_TEXT)))
		{
		    if (USE_CHARACTER_SET_TRANSLATION)
		    {
			string = convert_from_user_to_file(FileTag->comment);
			ID3Field_SetASCII(id3_field,string);
			g_free(string);
		    }else
		    {
			ID3Field_SetASCII(id3_field,FileTag->comment);
		    }
		}
		/* These 2 following fields allow synchronisation between id3v2 and id3v1 tags with id3lib */
		if ((id3_field = ID3Frame_GetField(id3_frame,ID3FN_DESCRIPTION)))
		{
		    ID3Field_SetASCII(id3_field,"ID3v1 Comment");
		}
		if ((id3_field = ID3Frame_GetField(id3_frame,ID3FN_LANGUAGE)))
		{
		    ID3Field_SetASCII(id3_field,"XXX");
		}
	    } else
	    {
		while ( (id3_frame = ID3Tag_FindFrameWithID(id3_tag,ID3FID_COMMENT)) )  /* Delete all comment fields*/
		    ID3Tag_RemoveFrame(id3_tag,id3_frame);
		has_comment = 0;
	    }
	}


        /* Set padding when tag was changed, for faster writing */
        ID3Tag_SetPadding(id3_tag,TRUE);
        /* FIXME!! : How to add padding during the first writing of tag */

        /*********************************
         * Update id3v1.x and id3v2 tags *
         *********************************/
         /* Get the number of frames into the tag, cause if it is
          * equal to 0, id3lib-3.7.12 doesn't update the tag */
         number_of_frames = ID3Tag_NumFrames(id3_tag);

        /* If all fields (managed in the UI) are empty and option STRIP_TAG_WHEN_EMPTY_FIELDS
         * is set to 1, we strip the ID3v1.x and ID3v2 tags. Else let see... :)
         */
        if ( STRIP_TAG_WHEN_EMPTY_FIELDS && !has_title && !has_artist && !has_album
             && !has_year && !has_track && !has_genre && !has_comment )/*&& !has_song_len ) */
        {
            error_strip_id3v1 = ID3Tag_Strip(id3_tag,ID3TT_ID3V1);
            error_strip_id3v2 = ID3Tag_Strip(id3_tag,ID3TT_ID3V2);
            /* Check error messages */
            if (error_strip_id3v1 == ID3E_NoError && error_strip_id3v2 == ID3E_NoError)
            {
                g_print(_("Removed tag of '%s'\n"),g_basename(filename));
            }else
            {
                if (error_strip_id3v1 != ID3E_NoError)
                    g_print(_("Error while removing ID3v1 tag of '%s' (%s)\n"),g_basename(filename),Id3tag_Get_Error_Message(error_strip_id3v1));
                if (error_strip_id3v2 != ID3E_NoError)
                    g_print(_("Error while removing ID3v2 tag of '%s' (%s)\n"),g_basename(filename),Id3tag_Get_Error_Message(error_strip_id3v2));
                error++;
            }
            
        }else
        {
            /* It's better to remove the id3v1 tag before, to synchronize it with the
             * id3v2 tag (else id3lib doesn't do it correctly)
             */
            error_strip_id3v1 = ID3Tag_Strip(id3_tag,ID3TT_ID3V1);

            /* ID3v1 tag */
            if (WRITE_ID3V1_TAG && number_of_frames!=0)
            {
                error_update_id3v1 = ID3Tag_UpdateByTagType(id3_tag,ID3TT_ID3V1);
                if (error_update_id3v1 != ID3E_NoError)
                {
                    g_print(_("Error while updating ID3v1 tag of '%s' (%s)\n"),g_basename(filename),Id3tag_Get_Error_Message(error_update_id3v1));
                    error++;
                }
            }else
            {
                error_strip_id3v1 = ID3Tag_Strip(id3_tag,ID3TT_ID3V1);
                if (error_strip_id3v1 != ID3E_NoError)
                {
                    g_print(_("Error while removing ID3v1 tag of '%s' (%s)\n"),g_basename(filename),Id3tag_Get_Error_Message(error_strip_id3v1));
                    error++;
                }
            }

            /* ID3v2 tag */
            if (WRITE_ID3V2_TAG && number_of_frames!=0)
            {
                error_update_id3v2 = ID3Tag_UpdateByTagType(id3_tag,ID3TT_ID3V2);
                if (error_update_id3v2 != ID3E_NoError)
                {
                    g_print(_("Error while updating ID3v2 tag of '%s' (%s)\n"),g_basename(filename),Id3tag_Get_Error_Message(error_update_id3v2));
                    error++;
                }
            }else
            {
                error_strip_id3v2 = ID3Tag_Strip(id3_tag,ID3TT_ID3V2);
                if (error_strip_id3v2 != ID3E_NoError)
                {
                    g_print(_("Error while removing ID3v2 tag of '%s' (%s)\n"),g_basename(filename),Id3tag_Get_Error_Message(error_strip_id3v2));
                    error++;
                }
            }
            
	    /*            if (error == 0)
			  g_print(_("Updated tag of '%s'\n"),g_basename(filename));*/
            
        }

        /* Free allocated data */
        ID3Tag_Delete(id3_tag);
    }

    if (error) return FALSE;
    else       return TRUE;
    
}



/*
 * Return the sub version of the ID3v2 tag, for example id3v2.2, id3v2.3
 */
gint Id3tag_Get_Id3v2_Version (gchar *filename)
{
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
    {
        return -1;
    }
}


gchar *Id3tag_Get_Error_Message(ID3_Err error)
{
    switch (error)
    {
        case ID3E_NoError:
            return _("No error reported");
        case ID3E_NoMemory:
            return _("No available memory");
        case ID3E_NoData:
            return _("No data to parse");
        case ID3E_BadData:
            return _("Improperly formatted data");
        case ID3E_NoBuffer:
            return _("No buffer to write to");
        case ID3E_SmallBuffer:
            return _("Buffer is too small");
        case ID3E_InvalidFrameID:
            return _("Invalid frame ID");
        case ID3E_FieldNotFound:
            return _("Requested field not found");
        case ID3E_UnknownFieldType:
            return _("Unknown field type");
        case ID3E_TagAlreadyAttached:
            return _("Tag is already attached to a file");
        case ID3E_InvalidTagVersion:
            return _("Invalid tag version");
        case ID3E_NoFile:
            return _("No file to parse");
        case ID3E_ReadOnly:
            return _("Attempting to write to a read-only file");
        case ID3E_zlibError:
            return _("Error in compression/uncompression");
        default:
            return _("Unknown error message!");
    }
    
}



/*
 * Returns the corresponding genre value of the input string (for ID3v1.x),
 * else returns 0xFF(unknown genre, but not invalid).
 */
guchar Id3tag_String_To_Genre (gchar *genre)
{
    gint i;

    for (i=0; i<GENRE_MAX; i++)
        if (strcasecmp(genre,id3_genres[i])==0)
            return (guchar)i;
    return (guchar)0xFF;
}


/*
 * Returns the name of a genre code if found
 * Three states for genre code :
 *    - defined (0 to GENRE_MAX)
 *    - undefined/unknown (GENRE_MAX+1 to ID3_INVALID_GENRE-1)
 *    - invalid (>ID3_INVALID_GENRE)
 */
gchar *Id3tag_Genre_To_String (unsigned char genre_code)
{
    if (genre_code>=ID3_INVALID_GENRE)    /* empty */
        return "";
    else if (genre_code>GENRE_MAX)        /* unknown tag */
        return "Unknown";
    else                                  /* known tag */
        return id3_genres[genre_code];
}



/*
 * As the ID3Tag_Link function of id3lib-3.8.0pre2 returns the ID3v1 tags
 * when a file has both ID3v1 and ID3v2 tags, we first try to explicitely
 * get the ID3v2 tags with ID3Tag_LinkWithFlags and, if we cannot get them,
 * fall back to the ID3v1 tags.
 * (Written by Holger Schemel).
 */
ID3_C_EXPORT size_t ID3Tag_Link_1 (ID3Tag *id3tag, const char *filename)
{
    size_t offset;

#   if ( (ID3LIB_MAJOR >= 3) && (ID3LIB_MINOR >= 8) )
        /* First, try to get the ID3v2 tags */
        offset = ID3Tag_LinkWithFlags(id3tag,filename,ID3TT_ID3V2);
        if (offset == 0)
        {
            /* No ID3v2 tags available => try to get the ID3v1 tags */
            offset = ID3Tag_LinkWithFlags(id3tag,filename,ID3TT_ID3V1);
        }
#   else
        /* Function 'ID3Tag_LinkWithFlags' is not defined up to id3lib-.3.7.13 */
        offset = ID3Tag_Link(id3tag,filename);
#   endif
	/*g_print("ID3 TAG SIZE: %d\t%s\n",offset,g_basename(filename)); */
    return offset;
}


/*
 * As the ID3Field_GetASCII function differs with the version of id3lib, we must redefine it.
 */
ID3_C_EXPORT size_t ID3Field_GetASCII_1(const ID3Field *field, char *buffer, size_t maxChars, size_t itemNum)
{

    /* Defined by id3lib:   ID3LIB_MAJOR_VERSION, ID3LIB_MINOR_VERSION, ID3LIB_PATCH_VERSION
     * Defined by autoconf: ID3LIB_MAJOR,         ID3LIB_MINOR,         ID3LIB_PATCH
     *
     * <= 3.7.12 : first item num is 1 for ID3Field_GetASCII
     *  = 3.7.13 : first item num is 0 for ID3Field_GetASCII
     * >= 3.8.0  : doesn't need item num for ID3Field_GetASCII
     */
  /*g_print("id3lib version: %d.%d.%d\n",ID3LIB_MAJOR,ID3LIB_MINOR,ID3LIB_PATCH);*/
#    if (ID3LIB_MAJOR >= 3)
         /* (>= 3.x.x) */
#        if (ID3LIB_MINOR <= 7)
             /* (3.0.0 to 3.7.x) */
#            if (ID3LIB_PATCH >= 13)
                 /* (>= 3.7.13) */
                 return ID3Field_GetASCII(field,buffer,maxChars,itemNum);
#            else
                 return ID3Field_GetASCII(field,buffer,maxChars,itemNum+1);
#            endif
#        else
		 /* (>= to 3.8.0) */
		 /*return ID3Field_GetASCII(field,buffer,maxChars); */
             return ID3Field_GetASCIIItem(field,buffer,maxChars,itemNum);
#        endif
#    else
	     /* Not tested (< 3.x.x) */
         return ID3Field_GetASCII(field,buffer,maxChars,itemNum+1);
#    endif
}
