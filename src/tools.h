/* Time-stamp: <2004-07-19 22:25:45 jcs>
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

#ifndef __NORMALIZE_H__
#define __NORMALIZE_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include "track.h"

#define TRACKVOLERROR G_MININT32

void nm_new_tracks (void);
void nm_tracks_list (GList *list);

gboolean tools_sync_contacts (void);
gboolean tools_sync_calendar (void);

void do_command_on_entries (const gchar *command, const gchar *what,
			    GList *selected_tracks);
void tools_play_tracks (GList *selected_tracks);
void tools_enqueue_tracks (GList *selected_tracks);
#endif
