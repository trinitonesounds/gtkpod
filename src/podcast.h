/* Time-stamp: <2005-07-01 23:16:54 asd>
|
|  Copyright (C) 2002-2005 Alexander Dutton <alexdutton at f2s dot com>
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

#ifndef __PODCAST_H__
#define __PODCAST_H__

struct podcast
{
    gchar *name;                 /* of the podcast */
    gchar *url;                  /* of XML file */
};

struct podcast_file
{
    gchar *title;                 /* of each individual audio file */
    gchar *url;                  /* where the audio file is to be found */
    gchar *desc;                 /* description of audio file (from XML) */
    gchar *artist;               /* itunes:author in actuality */
    gchar pubdate[14];           /* "YYYYMMDDHHMMSS" in UTC/GMT*/
    gchar fetchdate[14];         /* same as above */
    glong size;                  /* filesize */
    gchar *local;                /* where the file is kept on the local system */
    gboolean fetched;            /* whether or not this podcast has been fetched */
    gboolean tofetch;            /* whether we should fetch this podcast */
};

enum
{
  PC_SUBS_NAME = 0,
  PC_SUBS_URL,
  PC_SUBS_NUM_COLS
};

enum
{
  PCL_TITLE = 0,
  PCL_URL,
  PCL_SIZE,
  PCL_PROGRESS,
  PCL_NUM_COLS
};

enum
{
  ABORT_SELECTED = 0,
  ABORT_CURRENT,
  ABORT_ALL
};

gboolean podcast_fetch_in_progress;

void podcast_write_from_store (GtkListStore *store);
void podcast_read_into_store (GtkListStore *store);
void podcast_read_from_file ();

void podcast_file_add (gchar *title, gchar *url, 
                       gchar *desc, gchar *artist, 
                       gchar pubdate[14], gchar fetchdate[14], 
                       glong size, gchar *local,
                       gboolean fetched, gboolean tofetch);
void podcast_file_delete_by_url (gchar *url);
gboolean podcast_already_have_url (gchar *url);
GList *podcast_file_find_to_fetch ();
void podcast_fetch ();
void podcast_fetch_thread ();

gchar *podcast_get_tag_attr(gchar *attrs, gchar *req);

void podcast_set_status(gchar *status);
void podcast_set_cur_file_name(gchar *text);

int update_progress(gpointer *data,
                    double t, /* dltotal */
                    double d, /* dlnow */
                    double ultotal,
                    double ulnow);

#endif
