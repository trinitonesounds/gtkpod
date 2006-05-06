/* Time-stamp: <2006-05-03 01:15:30 jcs>
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

#ifndef __REPOSITORY_H__
#define __REPOSITORY_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
 
#include <gtk/gtk.h>
#include "itdb.h"

/* Not sure where to put these:
   Playlist-Autosync options */
enum
{
    /* no auto-sync */
    PLAYLIST_AUTOSYNC_MODE_NONE = 0,
    /* use dirs from filenames in playlist */
    PLAYLIST_AUTOSYNC_MODE_AUTOMATIC = 1,
    /* use specified dir */
    PLAYLIST_AUTOSYNC_MODE_MANUAL = 2
};


/* repository window */
void repository_edit (iTunesDB *itdb, Playlist *playlist);
void repository_update_default_sizes (void);
/*void repository_remove_playlist (Playlist *playlist);*/
#endif
