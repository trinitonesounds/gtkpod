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
#ifndef _GTKPOD_CONFIRMATION_WINDOW_H
#define _GTKPOD_CONFIRMATION_WINDOW_H

#include <gtk/gtk.h>

/**
 * CONFIRMATION_WINDOW_TYPES - the different types of delete file
 * confirmation dialogs
 */
enum {
    CONFIRMATION_WINDOW_SONG = 0,
    CONFIRMATION_WINDOW_PLAYLIST,
    CONFIRMATION_WINDOW_SONG_FROM_IPOD,
    CONFIRMATION_WINDOW_SONG_FROM_PLAYLIST,
    CONFIRMATION_WINDOW_CREATE_IPOD_DIRS,
    CONFIRMATION_WINDOW_COUNT
};

void confirmation_window_create(int window_type);
void confirmation_window_ok_clicked(void);
void confirmation_window_cancel_clicked(void);
void confirmation_window_prefs_toggled(gboolean val);

#endif
