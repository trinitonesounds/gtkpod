/* Time-stamp: <2005-01-06 00:03:26 jcs>
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

#ifndef __MISC_TRACK_H__
#define __MISC_TRACK_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

void gp_duplicate_remove (Track *oldtrack, Track *track);
Track *gp_track_by_filename (iTunesDB *itdb, gchar *filename);

void add_idlist_to_playlist (Playlist *pl, gchar *str);
void add_text_plain_to_playlist (Playlist *pl, gchar *str, gint position,
				 AddTrackFunc trackaddfunc, gpointer data);
void do_selected_tracks (void (*do_func)(GList *trackids));
void do_selected_entry (void (*do_func)(GList *trackids), gint inst);
void do_selected_playlist (void (*do_func)(GList *trackids));


#endif
