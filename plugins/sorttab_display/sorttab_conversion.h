/* Time-stamp: <2007-03-19 23:11:13 jcs>
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

#ifndef __SORTTAB_CONVERSION_H__
#define __SORTTAB_CONVERSION_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "libgtkpod/misc_conversion.h"

/* Categories in each sort tab (page numbers) */
typedef enum {
  ST_CAT_ARTIST = 0,
  ST_CAT_ALBUM,
  ST_CAT_GENRE,
  ST_CAT_COMPOSER,
  ST_CAT_TITLE,
  ST_CAT_YEAR,
  ST_CAT_SPECIAL,
  ST_CAT_NUM
} ST_CAT_item;

T_item ST_to_T (ST_CAT_item st);

#endif /* __SORTTAB_CONVERSION_H__ */
