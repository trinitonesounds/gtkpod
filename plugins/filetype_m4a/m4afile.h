/* Time-stamp: <2005-01-07 23:51:33 jcs>
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

#ifndef M4AFILEH_INCLUDED
#define M4AFILEH_INCLUDED 1

#include "libgtkpod/itdb.h"

Track *m4a_get_file_info (const gchar *m4aFileName, GError **error);
gboolean m4a_write_file_info (const gchar *filename, Track *track, GError **error);
gboolean m4a_can_convert();
gchar *m4a_get_conversion_cmd();
gchar *m4a_get_gain_cmd();

#endif
