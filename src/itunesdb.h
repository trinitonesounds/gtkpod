/* Time-stamp: <2003-11-29 12:08:57 jcs>
|
|  Copyright (C) 2002-2003 Jorg Schuler <jcsjcs at users.sourceforge.net>
|  Part of the gtkpod project.
|
|  URL: http://gtkpod.sourceforge.net/
| 
|  Most of the code in this file has been ported from the perl
|  script "mktunes.pl" (part of the gnupod-tools collection) written
|  by Adrian Ulrich <pab at blinkenlights.ch>.
|
|  gnupod-tools: http://www.blinkenlights.ch/cgi-bin/fm.pl?get=ipod
| 
|  The code contained in this file is free software; you can redistribute
|  it and/or modify it under the terms of the GNU Lesser General Public
|  License as published by the Free Software Foundation; either version
|  2.1 of the License, or (at your option) any later version.
|  
|  This file is distributed in the hope that it will be useful,
|  but WITHOUT ANY WARRANTY; without even the implied warranty of
|  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
|  Lesser General Public License for more details.
|  
|  You should have received a copy of the GNU Lesser General Public
|  License along with this code; if not, write to the Free Software
|  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
| 
|  iTunes and iPod are trademarks of Apple
| 
|  This product is not supported/written/published by Apple!
|
|  $Id$
*/

#ifndef __ITUNESDB_H__
#define __ITUNESDB_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "song.h"
#include "playlist.h"

enum {
  MHOD_ID_TITLE = 1,
  MHOD_ID_PATH = 2,
  MHOD_ID_ALBUM = 3,
  MHOD_ID_ARTIST = 4,
  MHOD_ID_GENRE = 5,
  MHOD_ID_FDESC = 6,
  MHOD_ID_COMMENT = 8,
  MHOD_ID_COMPOSER = 12,
  MHOD_ID_PLAYLIST = 100
};

#define ITUNESDB_COPYBLK 65536      /* blocksize for cp () */

gboolean itunesdb_parse (const gchar *path);
gboolean itunesdb_parse_file (const gchar *filename);
gboolean itunesdb_write (const gchar *path);
gboolean itunesdb_write_to_file (const gchar *filename);
gboolean itunesdb_copy_track_to_ipod (const gchar *path, Track *track,
				      const gchar *pcfile);
gchar *itunesdb_get_track_name_on_ipod (const gchar *path, Track *s);
gboolean itunesdb_cp (const gchar *from_file, const gchar *to_file);
guint32 itunesdb_time_get_mac_time (void);
time_t itunesdb_time_mac_to_host (guint32 mactime);
guint32 itunesdb_time_host_to_mac (time_t time);
#endif 
