/* Time-stamp: <2003-09-02 22:55:43 jcs>
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
#include "confirmation.h"
#include "time.h"

#define STATUSBAR_TIMEOUT 4200

#define C_FREE(a) {if(a) g_free(a); a=NULL;}

/* I'm sure there was a C-function or macro for this... */
#define SIGN(a) (a<0 ? -1:a>0 ? +1:0)

/* pointer to main window */
extern GtkWidget *gtkpod_window;
/* indicates whether widgets are currently blocked */
extern gboolean widgets_blocked;

void create_add_files_fileselector (void);
void create_add_playlists_fileselector (void);
gchar *concat_dir (G_CONST_RETURN gchar *dir, G_CONST_RETURN gchar *file);
gchar *concat_dir_if_relative (G_CONST_RETURN gchar *base_dir,
			       G_CONST_RETURN gchar *rel_dir);
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
void gtkpod_space_statusbar_init(GtkWidget *w);

S_item SM_to_S (SM_item sm);
S_item ST_to_S (ST_CAT_item st);
gchar *get_song_info (Song *song);

void gtkpod_songs_statusbar_init(GtkWidget*);
void gtkpod_songs_statusbar_update(void);
void ipod_directories_head (void);
void delete_playlist_head (void);
void delete_song_head (void);
void delete_entry_head (gint inst);

void delete_populate_settings (Playlist *pl, GList *selected_songids,
			       gchar **label, gchar **title,
			       gboolean *confirm_again,
			       ConfHandlerOpt *confirm_again_handler,
			       GString **str);

void block_widgets (void);
void release_widgets (void);
void update_blocked_widget (GtkWidget *w, gboolean sens);

void mount_ipod(void);
void unmount_ipod(void);
void call_script (gchar *script);

void do_command_on_entries (gchar *command, gchar *what, GList *selected_songs);
void play_songs (GList *selected_songs);
void enqueue_songs (GList *selected_songs);

void delete_song_ok (gpointer user_data1, gpointer user_data2);

void gtkpod_warning (const gchar *format, ...);

gchar *time_time_to_string (time_t time);
time_t time_get_time (Song *song, SM_item sm_item);
gchar *time_field_to_string (Song *song, SM_item sm_item);
void time_set_time (Song *song, time_t time, SM_item sm_item);

gint compare_string (gchar *str1, gchar *str2);
gint compare_string_case_insensitive (gchar *str1, gchar *str2);

gchar *filename_from_uri (const char *uri,
			  char      **hostname,
			  GError    **error);

void generate_category_playlists (S_item cat);
Playlist *generate_displayed_playlist (void);
Playlist *generate_selected_playlist (void);
Playlist *generate_new_playlist (GList *songs);
Playlist *generate_playlist_with_name (GList *songs, gchar *pl_name);
Playlist *generate_playlist (GList *songs, gchar *pl_name);
Playlist *generate_new_playlist (GList *songs);
void most_listened_pl (void);
void last_listened_pl(void);
void most_rated_pl(void);
void since_last_pl(void);
void rebuild_iTunesDB(void);
gchar *which(const gchar *exe);
#endif
