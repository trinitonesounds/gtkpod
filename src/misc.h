/*
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

#ifndef __MISC_H__
#define __MISC_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <stdio.h>

#define STATUSBAR_TIMEOUT 4200
/* we receive UTF8 strings which should be translated to the locale
 * before printing */
/* FIXME: write a popup for gtkpod_warning which holds all the
 * warnings in a list */
#define gtkpod_warning(...) do { gchar *utf8=g_strdup_printf (__VA_ARGS__); gchar *loc=g_locale_from_utf8 (utf8, -1, NULL, NULL, NULL); fprintf (stderr, "%s", loc); g_free (loc); g_free (utf8);} while (FALSE)
#define C_FREE(a) if(a) g_free(a); a=NULL


extern GtkWidget *gtkpod_window;

void create_add_files_fileselector (void);
gchar *concat_dir (G_CONST_RETURN gchar *dir, G_CONST_RETURN gchar *file);
void open_about_window (void);
void close_about_window (void);
gboolean parse_ipod_id_from_string(gchar **s, guint32 *id);
void cleanup_backup_and_extended_files (void);
void gtkpod_main_quit(void);
void disable_gtkpod_import_buttons(void);
void gtkpod_main_window_set_active(gboolean active);

void gtkpod_statusbar_init(GtkWidget *);
void gtkpod_statusbar_message(const gchar *message);

void gtkpod_songs_statusbar_init(GtkWidget*);
void gtkpod_songs_statusbar_update(void);
void ipod_directories_head (void);
void delete_playlist_head (void);
void delete_song_head (void);

#endif __MISC_H__
