/* Time-stamp: 
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
#ifndef __ID3_TAG_H__
#define __ID3_TAG_H__

#include <glib.h>

typedef struct 
{
    gchar *title;          /* Title of track */
    gchar *artist;         /* Artist name */
    gchar *album;          /* Album name */
    gchar *year;           /* Year of track */
    gchar *trackstring;    /* Position of track in the album */
    gchar *track_total;    /* The number of tracks for the album (ex: 12/20) */
    gchar *genre;          /* Genre of track */
    gchar *comment;        /* Comment */
    gchar *composer;	   /* Composer */
    guint32 size;          /* Size of file in bytes */
    guint32 tracklen;      /* Length of file in ms */
    GList *other;          /* List of unsupported fields (used for ogg only) */
    gchar *auto_charset;   /* in case of auto-detection: which charset
			      was used? */
} Id3tag;

gboolean id3_tag_read	(gchar *filename, Id3tag *tag);
gboolean id3_tag_write	(gchar *filename, Id3tag *tag);

#endif /* !__ID3TAG_H__ */
