#include <gtk/gtk.h>


gboolean
on_gtkpod_delete_event                 (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_import_itunes1_activate             (GtkMenuItem     *menuitem,
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
on_cut1_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_copy1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_paste1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_delete1_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_import_itunes1_button               (GtkButton       *button,
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
