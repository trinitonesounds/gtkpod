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
on_new_playlist1_activate              (GtkMenuItem     *menuitem,
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
on_export_files_to_disk_activate       (GtkMenuItem     *menuitem,
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
on_delete_song_menu                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_delete_playlist_menu                (GtkMenuItem     *menuitem,
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
on_songs_in_selected_playlist1_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_selected_songs1_activate            (GtkMenuItem     *menuitem,
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
on_cfg_save_sorted_order_toggled       (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_sort_tab_num_combo_entry_changed    (GtkEditable     *editable,
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
on_tab_entry_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_delete_tab_entry_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_redraw_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);
