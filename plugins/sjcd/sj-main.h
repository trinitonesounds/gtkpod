/*
 * Sound Juicer - sj-main.h
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

#ifndef SJ_MAIN_H
#define SJ_MAIN_H

#include <gtk/gtk.h>
#include "sj-structures.h"

GtkWidget *sj_create_sound_juicer();

void set_action_enabled (const char *name, gboolean enabled);

G_MODULE_EXPORT void on_destroy_activate (GtkMenuItem *item, gpointer user_data);

AlbumDetails* multiple_album_dialog (GList* albums);

const char* prefs_get_default_device (void);

G_MODULE_EXPORT void on_genre_edit_changed(GtkEditable *widget, gpointer user_data);
G_MODULE_EXPORT void on_year_edit_changed(GtkEditable *widget, gpointer user_data);

GtkWidget* sj_make_volume_button (void);

#endif /* SJ_MAIN_H */
