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
#include "file.h"
#include "song.h"
#include "playlist.h"
#include "display.h"

#define STATUSBAR_TIMEOUT 4200

/* we receive UTF8 strings which should be translated to the locale
 * before printing */
/* FIXME: write a popup for gtkpod_warning which holds all the
 * warnings in a list */
#define gtkpod_warning(...) do { gchar *utf8=g_strdup_printf (__VA_ARGS__); gchar *loc=g_locale_from_utf8 (utf8, -1, NULL, NULL, NULL); fprintf (stderr, "%s", loc); g_free (loc); g_free (utf8);} while (FALSE)
#define C_FREE(a) if(a) g_free(a); a=NULL

/* pointer to main window */
extern GtkWidget *gtkpod_window;
/* indicates whether widgets are currently blocked */
extern gboolean widgets_blocked;

void create_add_files_fileselector (void);
void create_add_playlists_fileselector (void);
gchar *concat_dir (G_CONST_RETURN gchar *dir, G_CONST_RETURN gchar *file);
float get_ms_since (GTimeVal *old_time, gboolean update);
gint get_sort_tab_number (gchar *text);
void open_about_window (void);
void close_about_window (void);
gboolean parse_ipod_id_from_string(gchar **s, guint32 *id);
void add_idlist_to_playlist (Playlist *pl, gchar *str);
void add_text_plain_to_playlist (Playlist *pl, gchar *str, gint position,
				 AddSongFunc songaddfunc, gpointer data);
void cleanup_backup_and_extended_files (void);
gboolean gtkpod_main_quit(void);
void disable_gtkpod_import_buttons(void);
void gtkpod_main_window_set_active(gboolean active);

void gtkpod_statusbar_init(GtkWidget *);
void gtkpod_statusbar_message(const gchar *message);

S_item SM_to_S (SM_item sm);
S_item ST_to_S (ST_CAT_item st);
gchar *get_song_info (Song *song);

void gtkpod_songs_statusbar_init(GtkWidget*);
void gtkpod_songs_statusbar_update(void);
void ipod_directories_head (void);
void delete_playlist_head (void);
void delete_song_head (void);
void delete_entry_head (gint inst);

void block_widgets (void);
void release_widgets (void);
void update_blocked_widget (GtkWidget *w, gboolean sens);

void mount_ipod(void);
void unmount_ipod(void);
void call_script (gchar *script);

void do_command_on_entries (gchar *command, gchar *what, GList *selected_songs);
void play_songs (GList *selected_songs);
void enqueue_songs (GList *selected_songs);

#endif __MISC_H__
