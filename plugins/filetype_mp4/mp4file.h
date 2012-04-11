/* Time-stamp: <2006-06-11 14:39:42 jcs>
|
|  Copyright (C) 2002-2005 Jorg Schuler <jcsjcs at users sourceforge net>
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

#ifndef MP4FILEH_INCLUDED
#define MP4FILEH_INCLUDED 1

#include "libgtkpod/itdb.h"

gboolean mp4_write_file_info (const gchar *filename, Track *track, GError **error);
Track *mp4_get_file_info (const gchar *name, GError **error);
gboolean mp4_read_lyrics(const gchar *filename, gchar **lyrics, GError **error);
gboolean mp4_write_lyrics(const gchar *filename, const gchar *lyrics, GError **error);
#endif
