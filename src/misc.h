/* Time-stamp: <2005-07-09 14:32:12 jcs>
|
|  Copyright (C) 2002-2005 Jorg Schuler <jcsjcs at users sourceforge net>
|  Part of the gtkpod project.
| 
|  URL: http://www.gtkpod.org/
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

#ifndef __MISC_H__
#define __MISC_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <stdio.h>
#include "file.h"
#include "display_itdb.h"
#include "itdb.h"
#include "display.h"
#include "confirmation.h"
#include "time.h"

#define C_FREE(a) {g_free(a); a=NULL;}

/* version of GTK_CHECK_VERSION that uses the runtime variables
   gtk_*_version... instead of the compile-time constants
   GTK_*_VERSION */
#define	RUNTIME_GTK_CHECK_VERSION(major,minor,micro)	\
    (gtk_major_version > (major) || \
     (gtk_major_version == (major) && gtk_minor_version > (minor)) || \
     (gtk_major_version == (major) && gtk_minor_version == (minor) && \
      gtk_micro_version >= (micro)))

/* compare a and b, return sign (a-b) -- it has to be this way rather
   than just calculate a-b, because a and b might be unsigned... */
#define COMP(a,b) (a<b ? -1:a>b ? +1:0)

/* Response for the Apply button (we cannot use GTK_RESPONSE_APPLY)
 * because that is only emitted if a filename is present */
enum { RESPONSE_APPLY = 5 };

/* indicates whether widgets are currently blocked */
extern gboolean widgets_blocked;
/* full path to 'gtkpod.glade', defined in main.c */
extern gchar *xml_file;

gchar *get_user_string (gchar *title, gchar *message, gchar *dflt,
			gchar *opt_msg, gboolean *opt_state);
Playlist *add_new_pl_user_name (iTunesDB *itdb, gchar *dflt, gint32 pos);
void add_new_pl_or_spl_user_name (iTunesDB *itdb, gchar *dflt, gint32 pos);
void create_add_files_fileselector (void);
void create_add_playlists_fileselector (void);
gchar *concat_dir (G_CONST_RETURN gchar *dir, G_CONST_RETURN gchar *file);
gchar *concat_dir_if_relative (G_CONST_RETURN gchar *base_dir,
			       G_CONST_RETURN gchar *rel_dir);
float get_ms_since (GTimeVal *old_time, gboolean update);
gint get_sort_tab_number (gchar *text);
void open_about_window (void);
void close_about_window (void);
gboolean parse_tracks_from_string(gchar **s, Track **track);
void cleanup_backup_and_extended_files (void);
gboolean gtkpod_main_quit(void);
void gtkpod_main_window_set_active(gboolean active);

T_item TM_to_T (TM_item sm);
T_item ST_to_T (ST_CAT_item st);
const gchar *get_tm_string (TM_item tm);
const gchar *get_tm_tooltip (TM_item tm);
const gchar *get_t_string (T_item t);
const gchar *get_t_tooltip (T_item t);

gchar *get_track_info (Track *track, gboolean prefer_filename);

void ipod_directories_head (const gchar *mountpoint);
void delete_playlist_head (DeleteAction deleteaction);
void delete_track_head (DeleteAction deleteaction);
void delete_entry_head (gint entry_inst, DeleteAction deleteaction);

void delete_populate_settings (struct DeleteData *dd,
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

void delete_track_ok (struct DeleteData *dd);

void gtkpod_warning (const gchar *format, ...);

gchar *time_time_to_string (time_t t);
gchar *time_fromtime_to_string (time_t t);
gchar *time_totime_to_string (time_t t);
time_t time_string_to_time (const gchar *str);
time_t time_string_to_fromtime (const gchar *str);
time_t time_string_to_totime (const gchar *str);
time_t time_get_time (Track *track, TM_item tm_item);
gchar *time_field_to_string (Track *track, TM_item tm_item);
void time_set_time (Track *track, time_t t, TM_item tm_item);

gchar *get_filesize_as_string (double size);

gint compare_string (gchar *str1, gchar *str2);
void compare_string_fuzzy_generate_keys (void);
gint compare_string_fuzzy (gchar *str1, gchar *str2);
gint compare_string_case_insensitive (gchar *str1, gchar *str2);

gchar *filename_from_uri (const char *uri,
			  char      **hostname,
			  GError    **error);

void generate_category_playlists (iTunesDB *itdb, T_item cat);
Playlist *generate_displayed_playlist (void);
Playlist *generate_selected_playlist (void);
void randomize_current_playlist (void);
Playlist *generate_random_playlist (iTunesDB *itdb);
Playlist *generate_not_listed_playlist (iTunesDB *itdb);
Playlist *generate_playlist_with_name (iTunesDB *itdb, GList *tracks,
				       gchar *pl_name, gboolean del_old);
Playlist *generate_new_playlist (iTunesDB *itdb, GList *tracks);
void most_listened_pl (iTunesDB *itdb);
void never_listened_pl (iTunesDB *itdb);
void last_listened_pl(iTunesDB *itdb);
void most_rated_pl(iTunesDB *itdb);
void since_last_pl(iTunesDB *itdb);
void each_rating_pl (iTunesDB *itdb);

guint32 utf16_strlen (gunichar2 *utf16);
gunichar2 *utf16_strdup (gunichar2 *utf16);

void check_db (iTunesDB *db);

guint32 replaygain_to_soundcheck (gdouble gain);
gdouble soundcheck_to_replaygain (guint32 soundcheck);


void option_set_radio_button (GladeXML *win_xml,
			      const gchar *prefs_string,
			      const gchar **widgets,
			      gint dflt);
gint option_get_radio_button (GladeXML *win_xml,
			      const gchar *prefs_string,
			      const gchar **widgets);
void option_set_folder (GtkFileChooser *fc,
			const gchar *prefs_string);
void option_get_folder (GtkFileChooser *fc,
			const gchar *prefs_string,
			gchar **value);
void option_set_filename (GtkFileChooser *fc,
			  const gchar *prefs_string);
void option_get_filename (GtkFileChooser *fc,
			  const gchar *prefs_string,
			  gchar **value);
void option_set_string (GladeXML *win_xml,
			const gchar *name,
			const gchar *dflt);
void option_get_string (GladeXML *win_xml,
			const gchar *name,
			gchar **value);
void option_set_toggle_button (GladeXML *win_xml,
			       const gchar *name,
			       gboolean dflt);
gboolean option_get_toggle_button (GladeXML *win_xml,
				   const gchar *name);

gchar *get_string_from_template (Track *track,
				 const gchar *full_template,
				 gboolean is_filename);

GList *glist_duplicate (GList *list);

gchar *which (const gchar *exe);
#endif
