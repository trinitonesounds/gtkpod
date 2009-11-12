/* Time-stamp: <2006-05-10 00:01:02 jcs>
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

#ifndef __NORMALIZE_H__
#define __NORMALIZE_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib.h>

/* appears to be missing prior to glib 2.4 */
#ifndef G_MININT32
#define G_MININT32	((gint32)  0x80000000)
#endif

#define TRACKVOLERROR G_MININT32

void nm_new_tracks (iTunesDB *itdb);
void nm_tracks_list (GList *list);

gboolean tools_sync_all (iTunesDB *itdb);
gboolean tools_sync_contacts (iTunesDB *itdb);
gboolean tools_sync_calendar (iTunesDB *itdb);
gboolean tools_sync_notes (iTunesDB *itdb);

void do_command_on_entries (const gchar *command, const gchar *what,
			    GList *selected_tracks);
void tools_play_tracks (GList *selected_tracks);
void tools_enqueue_tracks (GList *selected_tracks);
#endif
