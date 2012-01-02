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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors: Ross Burton <ross@burtonini.com>
 */

#ifndef SJ_PREFS_H
#define SJ_PREFS_H

extern const char* prefs_get_default_device ();
void on_edit_preferences_cb (GtkMenuItem *item, gpointer user_data);
gboolean cd_drive_exists (const char *device);
void show_help (GtkWindow *parent);

void prefs_profile_changed (GtkWidget *widget, gpointer user_data);
G_MODULE_EXPORT void prefs_base_folder_changed (GtkWidget *chooser, gpointer user_data);
void prefs_path_option_changed (GtkComboBox *combo, gpointer user_data);
void prefs_file_option_changed (GtkComboBox *combo, gpointer user_data);
G_MODULE_EXPORT void on_edit_preferences_cb (GtkMenuItem *item, gpointer user_data);

#endif /* SJ_PREFS_H */
