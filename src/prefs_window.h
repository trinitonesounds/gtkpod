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

void block_prefs_window (void);
void release_prefs_window (void);
void prefs_window_ok(void);
void prefs_window_apply(void);
void prefs_window_cancel(void);
void prefs_window_create(void);
void prefs_window_delete(void);
void prefs_window_set_md5songs(gboolean val);
void prefs_window_set_update_existing(gboolean val);
void prefs_window_set_block_display(gboolean val);
void prefs_window_set_id3_write(gboolean val);
void prefs_window_set_id3_writeall(gboolean val);
void prefs_window_set_mount_point(const gchar *mp);
void prefs_window_set_play_now_path(const gchar *path);
void prefs_window_set_play_enqueue_path(const gchar *path);

void prefs_window_set_keep_backups(gboolean active);
void prefs_window_set_write_extended_info(gboolean active);
void prefs_window_set_delete_playlist(gboolean val);
void prefs_window_set_delete_song_ipod(gboolean val);
void prefs_window_set_delete_song_playlist(gboolean val);
void prefs_window_set_auto_import(gboolean val);
void prefs_window_set_charset (gchar *charset);
void prefs_window_set_mpl_autoselect (gboolean autoselect);
void prefs_window_set_show_duplicates (gboolean val);
void prefs_window_set_show_updated (gboolean val);
void prefs_window_set_show_non_updated (gboolean val);
void prefs_window_set_display_toolbar (gboolean val);
void prefs_window_set_update_charset (gboolean val);
void prefs_window_set_write_charset (gboolean val);
void prefs_window_set_add_recursively (gboolean val);
void prefs_window_set_toolbar_style (GtkToolbarStyle val);
void prefs_window_set_save_sorted_order (gboolean val);
void prefs_window_set_sort_tab_num (gint num);
void prefs_window_set_automount(gboolean val);

#endif
