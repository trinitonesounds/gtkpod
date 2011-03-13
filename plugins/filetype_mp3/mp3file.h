/* Time-stamp: <2008-08-17 10:58:51 jcs>
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

#ifndef MP3FILEH_INCLUDED
#define MP3FILEH_INCLUDED 1

#include "plugin.h"
#include "libgtkpod/itdb.h"
#include "libgtkpod/gp_private.h"

gboolean mp3_write_file_info (const gchar *filename, Track *track, GError **error);
Track *mp3_get_file_info (const gchar *name, GError **error);
gboolean mp3_read_soundcheck (const gchar *path, Track *track, GError **error);
gboolean mp3_read_gapless (const gchar *path, Track *track, GError **error);

gboolean id3_read_tags (const gchar *name, Track *track);
gboolean id3_lyrics_read (const gchar *filename, gchar **lyrics, GError **error);
gboolean id3_lyrics_save (const gchar *filename, const gchar *lyrics, GError **error);

gboolean mp3_can_convert();
gchar *mp3_get_conversion_cmd();
gchar *mp3_get_gain_cmd();

#endif
