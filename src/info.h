/* Time-stamp: <2005-01-03 22:33:15 jcs>
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

#ifndef __INFO_H__
#define __INFO_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
 
#include <glib.h>

/* info window */
void info_open_window (void);
void info_close_window (void);
void info_update_default_sizes (void);
void info_update (void);
void info_update_track_view (void);
void info_update_track_view_selected (void);
void info_update_playlist_view (void);
void info_update_totals_view (void);
void info_update_totals_view_space (void);

/* statusbar stuff */
#define STATUSBAR_TIMEOUT 4200
void gtkpod_statusbar_init(void);
void gtkpod_statusbar_message(const gchar *message);
void gtkpod_statusbar_reset_timeout (void);
void gtkpod_space_statusbar_init(void);
void gtkpod_tracks_statusbar_init(void);
void gtkpod_tracks_statusbar_update(void);

/* space stuff */
#define SPACE_TIMEOUT 4000
void space_set_ipod_mount (const gchar *mp);
void space_data_update (void);
gboolean ipod_connected (void);
#endif
