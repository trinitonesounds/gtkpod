/* mp3file.h
|
|  Changed by Jorg Schuler <jcsjcs at users.sourceforge.net> to
|  compile "standalone" with the gtkpod project. 2003/05/11
|
|  $Id$
*/

/* This code is taken from the mp3info code. Only the code needed for
 * the playlength calculation has been extracted */

/*
    mp3tech.c - Functions for handling MP3 files and most MP3 data
                structure manipulation.

    Copyright (C) 2000-2001  Cedric Tefft <cedric@earthling.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

  ***************************************************************************

  This file is based in part on:

	* MP3Info 0.5 by Ricardo Cerqueira <rmc@rccn.net>
	* MP3Stat 0.9 by Ed Sweetman <safemode@voicenet.com> and 
			 Johannes Overmann <overmann@iname.com>

*/

#ifndef MP3FILEH_INCLUDED
#define MP3FILEH_INCLUDED 1

#include "song.h"

/*
|  Changed by Jorg Schuler <jcsjcs at users.sourceforge.net> to
|  compile "standalone" with the gtkpod project. 2002/11/24.
|  Changed by Jorg Schuler to also determine size of file and
|  length of song in ms. 2002/11/28.
*/

/* id3tag.h - 2001/02/16 */
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


/****************
 * Declarations *
 ****************/

/*
 * Description of each item of the TagList list
 */
typedef struct _File_Tag File_Tag;
struct _File_Tag
{
    gchar *title;          /* Title of track */
    gchar *artist;         /* Artist name */
    gchar *album;          /* Album name */
    gchar *year;           /* Year of track */
    gchar *trackstring;    /* Position of track in the album */
    gchar *track_total;    /* The number of tracks for the album (ex: 12/20) */
    gchar *genre;          /* Genre of song */
    gchar *comment;        /* Comment */
    gchar *composer;	   /* Composer */
    guint32 songlen;       /* Length of file in ms */
};


/**************
 * Prototypes *
 **************/

gboolean Id3tag_Read_File_Tag  (gchar *filename, File_Tag *FileTag);
gboolean Id3tag_Write_File_Tag (gchar *filename, File_Tag *FileTag);
Track *file_get_mp3_info (gchar *name);
#endif
