/* Time-stamp: <2004-06-13 21:58:57 JST jcs>
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
on_track_treeview_drag_data_get         (GtkWidget       *widget,
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
on_cfg_md5tracks_toggled                (GtkToggleButton *togglebutton,
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
on_track_treeview_key_release_event     (GtkWidget       *widget,
					GdkEventKey     *event,
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
on_track_treeview_drag_data_received    (GtkWidget       *widget,
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
on_delete_tracks_activate                    (GtkMenuItem     *menuitem,
					gpointer         user_data);

void
on_delete_playlist_activate                (GtkMenuItem     *menuitem,
					gpointer         user_data);

void
on_delete_tab_entry_activate           (GtkMenuItem     *menuitem,
					gpointer         user_data);

void
on_delete_full_tracks_activate                    (GtkMenuItem     *menuitem,
					gpointer         user_data);

void
on_delete_full_playlist_activate                (GtkMenuItem     *menuitem,
					gpointer         user_data);

void
on_delete_full_tab_entry_activate           (GtkMenuItem     *menuitem,
					gpointer         user_data);

void
on_ipod_directories_menu               (GtkMenuItem     *menuitem,
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
on_update_tracks_activate            (GtkMenuItem     *menuitem,
				     gpointer         user_data);

void
on_sync_playlist_activate            (GtkMenuItem     *menuitem,
					gpointer         user_data);

void
on_sync_tab_entry_activate                  (GtkMenuItem     *menuitem,
					       gpointer         user_data);

void
on_sync_tracks_activate            (GtkMenuItem     *menuitem,
				     gpointer         user_data);

void
on_cfg_update_existing_toggled         (GtkToggleButton *togglebutton,
					gpointer         user_data);

void
on_save_track_order1_activate           (GtkMenuItem     *menuitem,
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
on_play_now_path_entry_changed         (GtkEditable     *editable,
					gpointer         user_data);

void
on_play_enqueue_path_entry_changed     (GtkEditable     *editable,
					gpointer         user_data);

void
on_mp3gain_entry_changed               (GtkEditable     *editable,
					gpointer         user_data);

void
on_cfg_export_check_existing_toggled          (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_cfg_fix_path_toggled          (GtkToggleButton *togglebutton,
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
on_export_tracks_activate     (GtkMenuItem     *menuitem,
			      gpointer         user_data);

void
on_play_playlist_activate              (GtkMenuItem     *menuitem,
					gpointer         user_data);

void
on_play_tab_entry_activate             (GtkMenuItem     *menuitem,
					gpointer         user_data);

void
on_play_tracks_activate                 (GtkMenuItem     *menuitem,
					gpointer         user_data);

void
on_enqueue_playlist_activate           (GtkMenuItem     *menuitem,
					gpointer         user_data);

void
on_enqueue_tab_entry_activate          (GtkMenuItem     *menuitem,
					gpointer         user_data);

void
on_enqueue_tracks_activate              (GtkMenuItem     *menuitem,
					gpointer         user_data);

void
on_arrange_sort_tabs_activate          (GtkMenuItem     *menuitem,
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
on_pl_containing_displayed_tracks_activate (GtkMenuItem     *menuitem,
					   gpointer         user_data);
void
on_pl_containing_selected_tracks_activate (GtkMenuItem     *menuitem,
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
on_most_listened_tracks1_activate       (GtkMenuItem     *menuitem,
					gpointer         user_data);

void
on_most_rated_tracks_playlist_s1_activate
					(GtkMenuItem     *menuitem,
					gpointer         user_data);

void
on_most_recent_played_tracks_activate   (GtkMenuItem     *menuitem,
					gpointer         user_data);

void
on_rebuild_ipod_db1_activate           (GtkMenuItem     *menuitem,
					gpointer         user_data);

void
on_cfg_misc_track_nr_value_changed      (GtkSpinButton   *spinbutton,
					gpointer         user_data);

void
on_cfg_not_played_track_toggled         (GtkToggleButton *togglebutton,
					gpointer         user_data);

void
on_played_since_last_time1_activate    (GtkMenuItem     *menuitem,
					gpointer         user_data);

void
on_cfg_export_template_changed            (GtkEditable     *editable,
					gpointer         user_data);

void
on_cfg_normalization_level_changed     (GtkEditable     *editable,
					gpointer         user_data);

void
on_cfg_special_export_charset_toggled  (GtkToggleButton *togglebutton,
					gpointer         user_data);

void
on_sort_case_sensitive_toggled         (GtkToggleButton *togglebutton,
					gpointer         user_data);

void
on_sorting_clicked                     (GtkButton       *button,
					gpointer         user_data);

void
on_sorting_activate                    (GtkMenuItem     *menuitem,
					gpointer         user_data);

void
on_st_ascend_toggled                   (GtkToggleButton *togglebutton,
					gpointer         user_data);

void
on_st_descend_toggled                  (GtkToggleButton *togglebutton,
					gpointer         user_data);

void
on_st_none_toggled                     (GtkToggleButton *togglebutton,
					gpointer         user_data);

void
on_pm_none_toggled                     (GtkToggleButton *togglebutton,
					gpointer         user_data);

void
on_pm_descend_toggled                  (GtkToggleButton *togglebutton,
					gpointer         user_data);

void
on_pm_ascend_toggled                   (GtkToggleButton *togglebutton,
					gpointer         user_data);

void
on_pm_autostore_toggled                (GtkToggleButton *togglebutton,
					gpointer         user_data);

void
on_sort_apply_clicked                  (GtkButton       *button,
					gpointer         user_data);

void
on_sort_cancel_clicked                 (GtkButton       *button,
					gpointer         user_data);

void
on_sort_ok_clicked                     (GtkButton       *button,
					gpointer         user_data);

gboolean
on_sort_window_delete_event            (GtkWidget       *widget,
					GdkEvent        *event,
					gpointer         user_data);

void
on_tm_ascend_toggled                   (GtkToggleButton *togglebutton,
					gpointer         user_data);

void
on_tm_descend_toggled                  (GtkToggleButton *togglebutton,
					gpointer         user_data);

void
on_tm_none_toggled                     (GtkToggleButton *togglebutton,
					gpointer         user_data);
void
on_tm_autostore_toggled                (GtkToggleButton *togglebutton,
					gpointer         user_data);


void
on_normalize_selected_playlist_activate
					(GtkMenuItem     *menuitem,
					gpointer         user_data);

void
on_normalize_selected_tab_entry_activate
					(GtkMenuItem     *menuitem,
					gpointer         user_data);

void
on_normalize_selected_tracks_activate   (GtkMenuItem     *menuitem,
					gpointer         user_data);

void
on_normalize_displayed_tracks_activate  (GtkMenuItem     *menuitem,
					gpointer         user_data);

void
on_normalize_all_tracks                (GtkMenuItem     *menuitem,
					gpointer         user_data);

void
on_normalize_newly_added_tracks        (GtkMenuItem     *menuitem,
					gpointer         user_data);

void
on_all_tracks_never_listened_to1_activate
					(GtkMenuItem     *menuitem,
					gpointer         user_data);

void
on_info_window1_activate               (GtkMenuItem     *menuitem,
					gpointer         user_data);

gboolean
on_gtkpod_info_delete_event            (GtkWidget       *widget,
					GdkEvent        *event,
					gpointer         user_data);

void
on_info_close_clicked                  (GtkButton       *button,
					gpointer         user_data);


void
on_cfg_write_id3v24_toggled            (GtkToggleButton *togglebutton,
					gpointer         user_data);

void
on_readtags_toggled                    (GtkToggleButton *togglebutton,
					gpointer         user_data);

void
on_parsetags_template_changed             (GtkEditable     *editable,
					gpointer         user_data);

void
on_parsetags_overwrite_toggled         (GtkToggleButton *togglebutton,
					gpointer         user_data);

void
on_parsetags_toggled                   (GtkToggleButton *togglebutton,
					gpointer         user_data);

void
on_check_ipod_files_activate           (GtkMenuItem     *menuitem,
					gpointer         user_data);

void
on_sync_contacts_path_entry_changed    (GtkEditable     *editable,
					gpointer         user_data);

void
on_sync_calendar_entry_changed         (GtkEditable     *editable,
					gpointer         user_data);

void
on_sync_calendar_activate              (GtkMenuItem     *menuitem,
					gpointer         user_data);

void
on_sync_contacts_activate              (GtkMenuItem     *menuitem,
					gpointer         user_data);

void
on_concal_autosync_toggled             (GtkToggleButton *togglebutton,
					gpointer         user_data);

void
on_pl_for_each_year_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_all_tracks_not_listed_in_any_playlist1_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_cfg_export_check_existing_toggled   (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_cfg_fix_path                       (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_random_playlist_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_randomize_current_playlist_activate (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

