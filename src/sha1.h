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
#ifndef _GTKPOD_MD5_H_
#define _GTKPOD_MD5_H_

#include "song.h"
#include <glib.h>

/* Any calls to these functions immediately return if md5sums is not on */

gchar *song_exists_on_ipod(Song *s);
void unique_file_repository_init(GList *songlist);
void unique_file_repository_free(void);
void song_removed_from_ipod(Song *s);
gchar *do_hash_on_file(FILE *fp);

#endif
