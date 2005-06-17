/* Time-stamp: <2005-06-17 22:25:31 jcs>
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

#ifndef __CHARSET_H__
#define __CHARSET_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <stdio.h>
#include "itdb.h"

#define GTKPOD_JAPAN_AUTOMATIC "gtkpod-japan-automatic"

void charset_init_combo (GtkCombo *combo);
gchar *charset_from_description (gchar *descr);
gchar *charset_to_description (gchar *charset);
gchar *charset_to_utf8 (const gchar *str);
gchar *charset_from_utf8 (const gchar *str);
gchar *charset_track_charset_from_utf8 (Track *s, const gchar *str);
gchar *charset_to_charset (const gchar *from_charset,
			   const gchar *to_charset,
			   const gchar *str);
gchar *charset_get_auto (void);
void charset_reset_auto (void);
#endif 
