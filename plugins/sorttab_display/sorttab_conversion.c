/*
|  Copyright (C) 2002-2007 Jorg Schuler <jcsjcs at users sourceforge net>
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

#include "sorttab_conversion.h"

/* translates a ST_CAT_... (defined in display.h) into a
 * T_... (defined in display.h). Returns -1 in case a translation is not
 * possible */
T_item ST_to_T (ST_CAT_item st)
{
    switch (st)
    {
    case ST_CAT_ARTIST:      return T_ARTIST;
    case ST_CAT_ALBUM:       return T_ALBUM;
    case ST_CAT_GENRE:       return T_GENRE;
    case ST_CAT_COMPOSER:    return T_COMPOSER;
    case ST_CAT_TITLE:       return T_TITLE;
    case ST_CAT_YEAR:        return T_YEAR;
    case ST_CAT_SPECIAL:     g_return_val_if_reached (-1);
    default:                 g_return_val_if_reached (-1);
    }
}
