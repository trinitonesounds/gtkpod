/*
|  Copyright (C) 2002 Corey Donohoe <atmos at atmos.org>
|                2004 Jorg Schuler <jcsjcs at users.sourceforge.net>
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
|  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
|  USA
| 
|  iTunes and iPod are trademarks of Apple
| 
|  This product is not supported/written/published by Apple!
|
|  $Id$
*/
#ifndef _GTKPOD_MD5_H_
#define _GTKPOD_MD5_H_

#include "display_itdb.h"

gchar *md5_hash_on_filename (gchar *name, gboolean silent);
/* Any calls to the following functions immediately return if md5sums
 * is not on */
Track *md5_file_exists (iTunesDB *itdb, gchar *file, gboolean silent);
Track *md5_md5_exists (iTunesDB *itdb, gchar *md5);
Track *md5_track_exists (iTunesDB *itdb, Track *s);
Track *md5_track_exists_insert (iTunesDB *itdb, Track *s);
void md5_track_remove (Track *s);
void md5_free (iTunesDB *itdb);
void md5_free_eitdb (ExtraiTunesDBData *eitdb);
#endif
