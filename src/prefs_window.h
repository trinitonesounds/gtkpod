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
#ifndef _GTKPOD_PREFS_WINDOW_H
#define _GTKPOD_PREFS_WINDOW_H

#include <gtk/gtk.h>

void prefs_window_save(void);
void prefs_window_create(void);
void prefs_window_cancel(void);
void prefs_window_set_md5songs_active(gboolean val);
void prefs_window_set_writeid3_active(gboolean val);
void prefs_window_set_mount_point(const gchar *mp);

void prefs_window_set_song_list_all(gboolean val);
void prefs_window_set_keep_backups(gboolean active);
void prefs_window_set_write_extended_info(gboolean active);
void prefs_window_set_song_list_album(gboolean val);
void prefs_window_set_song_list_track(gboolean val);
void prefs_window_set_song_list_genre(gboolean val);
void prefs_window_set_delete_playlist(gboolean val);
void prefs_window_set_delete_song_ipod(gboolean val);
void prefs_window_set_song_list_artist(gboolean val);
void prefs_window_set_delete_song_playlist(gboolean val);
void prefs_window_set_auto_import(gboolean val);
void prefs_window_set_lc_ctype (gchar *lc_ctype);
#endif
