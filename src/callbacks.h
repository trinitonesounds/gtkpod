/* Time-stamp: <2003-08-03 15:51:49 jcs>
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
*/


/* Most function definitions in this file were written by glade 2 */

#include <gtk/gtk.h>

gboolean
on_gtkpod_delete_event                 (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_add_files1_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_add_directory1_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_export_itunes1_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_quit1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_add_files1_button                   (GtkButton       *button,
                                        gpointer         user_data);

void
on_add_directory1_button               (GtkButton       *button,
                                        gpointer         user_data);

void
on_export_itunes1_button               (GtkButton       *button,
                                        gpointer         user_data);

void
on_new_playlist_button                 (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_about_window_close                  (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_about_window_close_button           (GtkButton       *button,
                                        gpointer         user_data);

void
on_new_playlist_button                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_sorttab_switch_page                 (GtkNotebook     *notebook,
                                        GtkNotebookPage *page,
                                        guint            page_num,
                                        gpointer         user_data);

void
on_playlist_treeview_drag_data_get     (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data);

void
on_playlist_treeview_drag_data_received
                                        (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data);

void
on_song_treeview_drag_data_get         (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data);

gboolean
on_prefs_window_delete_event           (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_cfg_mount_point_changed             (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_cfg_md5songs_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_cfg_id3_write_toggled                (GtkToggleButton *togglebutton,
                                         gpointer         user_data);

void
on_prefs_ok_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_prefs_cancel_clicked                (GtkButton       *button,
                                        gpointer         user_data);


void
on_prefs_apply_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_edit_preferences1_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

gboolean
on_playlist_treeview_key_release_event (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

gboolean
on_song_treeview_key_release_event     (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

void
on_cfg_delete_playlist_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_cfg_delete_track_from_playlist_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_cfg_delete_track_from_ipod_toggled  (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_cfg_autoimport_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_cfg_keep_backups_toggled            (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_cfg_write_extended_info_toggled     (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_offline1_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_import_itunes_mi_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_import_button_clicked               (GtkButton       *button,
                                        gpointer         user_data);

void
on_song_treeview_drag_data_received    (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data);
void
on_charset_combo_entry_changed         (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_delete_songs_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_delete_playlist_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_ipod_directories_menu               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_gtkpod_status_realize               (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_songs_statusbar_realize             (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_cfg_id3_writeall_toggled            (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_st_treeview_drag_data_get           (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data);

gboolean
on_st_treeview_key_release_event       (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

void
on_cfg_mpl_autoselect_toggled          (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_cfg_block_display_toggled           (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_stop_button_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_add_PL_button_clicked               (GtkButton       *button,
                                        gpointer         user_data);

void
on_add_playlist1_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_update_playlist_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_update_tab_entry_activate                  (GtkMenuItem     *menuitem,
					       gpointer         user_data);

void
on_update_songs_activate            (GtkMenuItem     *menuitem,
				     gpointer         user_data);

void
on_sync_playlist_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_sync_tab_entry_activate                  (GtkMenuItem     *menuitem,
					       gpointer         user_data);

void
on_sync_songs_activate            (GtkMenuItem     *menuitem,
				     gpointer         user_data);

void
on_cfg_update_existing_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_save_song_order1_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_cfg_show_duplicates_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_cfg_show_updated_toggled            (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_cfg_show_non_updated_toggled        (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_cfg_show_sync_dirs_toggled        (GtkToggleButton *togglebutton,
				      gpointer         user_data);

void
on_cfg_save_sorted_order_toggled       (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_toolbar_menu_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_cfg_display_toolbar_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_more_sort_tabs_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_less_sort_tabs_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_cfg_toolbar_display_text_toggled    (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_cfg_toolbar_style_both_toggled      (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_cfg_toolbar_style_text_toggled      (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_cfg_toolbar_style_icons_toggled     (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_alpha_playlists0_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_alpha_sort_tab0_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_alpha_playlist1_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_alpha_sort_tab1_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_delete_tab_entry_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_reset_sorting_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_play_now_path_entry_changed         (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_play_enqueue_path_entry_changed     (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_cfg_automount_ipod_toggled          (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_export_playlist_activate  (GtkMenuItem     *menuitem,
			      gpointer         user_data);

void
on_export_tab_entry_activate (GtkMenuItem     *menuitem,
			      gpointer         user_data);

void
on_export_songs_activate     (GtkMenuItem     *menuitem,
			      gpointer         user_data);

void
on_play_playlist_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_play_tab_entry_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_play_songs_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_enqueue_playlist_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_enqueue_tab_entry_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_enqueue_songs_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_arrange_sort_tabs_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_space_statusbar_realize             (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_cfg_update_charset_toggled          (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_cfg_write_charset_toggled           (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_cfg_add_recursively_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_cfg_sync_remove_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_cfg_sync_remove_confirm_toggled     (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_cfg_time_format_changed             (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_cfg_case_sensitive_toggled          (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_sp_or_button_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_sp_rating_n_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_sp_entry_activate          (GtkEditable     *editable,
			      gpointer         user_data);

void
on_sp_cal_button_clicked       (GtkButton       *button,
				gpointer         user_data);

void
on_sp_cond_button_toggled         (GtkToggleButton *togglebutton,
				   gpointer         user_data);

void
on_sp_go_clicked                       (GtkButton       *button,
                                        gpointer         user_data);

void
on_sp_go_always_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_sp_playcount_high_value_changed     (GtkSpinButton   *spinbutton,
                                        gpointer         user_data);

void
on_sp_playcount_low_value_changed      (GtkSpinButton   *spinbutton,
                                        gpointer         user_data);

void
on_cfg_sort_tab_num_sb_value_changed   (GtkSpinButton   *spinbutton,
                                        gpointer         user_data);

void
on_tooltips_menu_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_cfg_display_tooltips_main_toggled   (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_cfg_display_tooltips_prefs_toggled  (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_cfg_multi_edit_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_cfg_multi_edit_title_toggled        (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_new_playlist1_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_pl_containing_displayed_songs_activate (GtkMenuItem     *menuitem,
					   gpointer         user_data);
void
on_pl_containing_selected_songs_activate (GtkMenuItem     *menuitem,
					  gpointer         user_data);
void
on_pl_for_each_artist_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_pl_for_each_album_activate         (GtkMenuItem     *menuitem,
				       gpointer         user_data);

void
on_pl_for_each_genre_activate         (GtkMenuItem     *menuitem,
				       gpointer         user_data);

void
on_pl_for_each_composer_activate         (GtkMenuItem     *menuitem,
					  gpointer         user_data);


void
on_add_most_played_songs__pl1_activate (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_most_listened_songs1_activate       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_most_rated_songs_playlist_s1_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_most_recent_played_songs_activate   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_rebuild_ipod_db1_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_cfg_misc_song_nr_value_changed      (GtkSpinButton   *spinbutton,
                                        gpointer         user_data);

void
on_cfg_not_played_song_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_played_since_last_time1_activate    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_normalize_all_the_ipod_s_songs1_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_normalize_the_newly_inserted_songs1_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_cfg_filename_format_changed            (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_cfg_normalization_level_changed     (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_cfg_write_gaintag_toggled           (GtkToggleButton *togglebutton,
                                        gpointer         user_data);
