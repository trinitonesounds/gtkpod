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
on_edit_preferences1_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_cfg_song_list_all_toggled           (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_cfg_song_list_artist_toggled        (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_cfg_song_list_album_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_cfg_song_list_genre_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_cfg_song_list_track_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_cfg_song_list_year_toggled          (GtkToggleButton *togglebutton,
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
on_re_init0_menu                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_re_init1_menu                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_alphabetize0_up_menu                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_alphabetize0_down_menu              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_alphabetize1_up_menu                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_alphabetize1_down_menu              (GtkMenuItem     *menuitem,
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
