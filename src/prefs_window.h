/* Time-stamp: <2004-10-02 22:34:27 jcs>
|
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
|
|  $Id$
*/
#ifndef _GTKPOD_PREFS_WINDOW_H
#define _GTKPOD_PREFS_WINDOW_H

#include <gtk/gtk.h>
#include "prefs.h"

extern const gchar *path_button_names[];
extern const gchar *path_entry_names[];

void prefs_window_update_default_sizes (void);
void prefs_window_block (void);
void prefs_window_release (void);
void prefs_window_show_hide_tooltips (void);
void prefs_window_ok (void);
void prefs_window_apply (void);
void prefs_window_cancel (void);
void prefs_window_create (void);
void prefs_window_delete (void);
void prefs_window_set_readtags (gboolean val);
void prefs_window_set_parsetags (gboolean val);
void prefs_window_set_parsetags_overwrite (gboolean val);
void prefs_window_set_parsetags_template (const gchar *mp);
void prefs_window_set_md5tracks (gboolean val);
void prefs_window_set_update_existing (gboolean val);
void prefs_window_set_block_display (gboolean val);
void prefs_window_set_id3_write (gboolean val);
void prefs_window_set_id3_write_id3v24 (gboolean val);
void prefs_window_set_mount_point (const gchar *mp);
void prefs_window_set_time_format (const gchar *format);
void prefs_window_set_write_extended_info (gboolean active);
void prefs_window_set_delete_track_ipod (gboolean val);
void prefs_window_set_delete_track_playlist (gboolean val);
void prefs_window_set_sync_remove_confirm (gboolean val);
void prefs_window_set_autoimport (gboolean val);
void prefs_window_set_charset (gchar *charset);
void prefs_window_set_mpl_autoselect (gboolean autoselect);
void prefs_window_set_show_duplicates (gboolean val);
void prefs_window_set_show_updated (gboolean val);
void prefs_window_set_show_non_updated (gboolean val);
void prefs_window_set_show_sync_dirs (gboolean val);
void prefs_window_set_sync_remove (gboolean val);
void prefs_window_set_display_toolbar (gboolean val);
void prefs_window_set_display_tooltips_main (gboolean val);
void prefs_window_set_display_tooltips_prefs (gboolean val);
void prefs_window_set_multi_edit (gboolean val);
void prefs_window_set_multi_edit_title (gboolean val);
void prefs_window_set_misc_track_nr (gint val);
void prefs_window_set_not_played_track (gboolean val);
void prefs_window_set_update_charset (gboolean val);
void prefs_window_set_write_charset (gboolean val);
void prefs_window_set_add_recursively (gboolean val);
void prefs_window_set_toolbar_style (GtkToolbarStyle val);
void prefs_window_set_sort_tab_num (gint num);
void prefs_window_set_automount (gboolean val);
void prefs_window_set_concal_autosync (gboolean val);
void prefs_window_set_tmp_disable_sort (gboolean val);
void prefs_window_set_mserv_use (gboolean val);
void prefs_window_set_mserv_report_probs (gboolean val);
void prefs_window_set_mserv_music_root (const gchar *val);
void prefs_window_set_mserv_trackinfo_root (const gchar *val);
void prefs_window_set_mserv_username (const gchar *val);
void prefs_window_set_unused_gboolean3 (gboolean val);

void sort_window_create (void);
void sort_window_block (void);
void sort_window_release (void);
void sort_window_show_hide_tooltips (void);
void sort_window_ok (void);
void sort_window_apply (void);
void sort_window_cancel (void);
void sort_window_create (void);
void sort_window_delete (void);
void sort_window_set_pm_autostore (gboolean val);
void sort_window_set_tm_autostore (gboolean val);
void sort_window_set_pm_sort (gint val);
void sort_window_set_st_sort (gint val);
void sort_window_set_tm_sort (gint val);
void sort_window_set_case_sensitive (gboolean val);
void sort_window_update (void);
#endif
