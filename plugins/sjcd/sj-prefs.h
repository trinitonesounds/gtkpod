/*
 * Copyright (C) 2003 Ross Burton <ross@burtonini.com>
 *
 * Sound Juicer - sj-prefs.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Ross Burton <ross@burtonini.com>
 */

#ifndef SJ_PREFS_H
#define SJ_PREFS_H

GtkWidget *init_sjcd_preferences();

extern const char* prefs_get_default_device (void);
void show_preferences_dialog (void);
gboolean cd_drive_exists (const char *device);
void show_help (GtkWindow *parent);

const gchar* sj_get_default_file_pattern (void);
const gchar* sj_get_default_path_pattern (void);
void prefs_profile_changed (GtkWidget *widget, gpointer user_data);
G_MODULE_EXPORT void prefs_base_folder_changed (GtkWidget *chooser, gpointer user_data);
void prefs_path_option_changed (GtkComboBox *combo, gpointer user_data);
void prefs_file_option_changed (GtkComboBox *combo, gpointer user_data);

G_MODULE_EXPORT void on_destroy_dialog_content_cb(GtkWidget *widget, gpointer user_data);

#endif /* SJ_PREFS_H */
