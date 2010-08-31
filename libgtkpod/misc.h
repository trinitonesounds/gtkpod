/*
|  Copyright (C) 2002-2007 Jorg Schuler <jcsjcs at users sourceforge net>
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

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <stdio.h>
#include <libxml/xmlversion.h>
#include <libxml/xmlmemory.h>
#include "file.h"
#include "gp_itdb.h"
#include "time.h"

#if !GTK_CHECK_VERSION(2, 20, 0)
    #define gtk_widget_get_realized GTK_WIDGET_REALIZED
#endif

#define C_FREE(a) {g_free(a); a=NULL;}

/* where to find the scripts */
extern const gchar *SCRIPTDIR;

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

/* Some symbols not necessarily defined */

gchar *utf8_strcasestr (const gchar *haystack, const gchar *needle);

gchar *get_user_string (gchar *title, gchar *message, gchar *dflt,
			gchar *opt_msg, gboolean *opt_state, const gchar *accept_button);
Playlist *add_new_pl_user_name (iTunesDB *itdb, gchar *dflt, gint32 pos);
void add_new_pl_or_spl_user_name (iTunesDB *itdb, gchar *dflt, gint32 pos);
void create_add_files_fileselector (void);
void create_add_playlists_fileselector (void);
gchar *concat_dir (G_CONST_RETURN gchar *dir, G_CONST_RETURN gchar *file);
gchar *concat_dir_if_relative (G_CONST_RETURN gchar *base_dir,
			       G_CONST_RETURN gchar *rel_dir);
float get_ms_since (GTimeVal *old_time, gboolean update);
gint get_sort_tab_number (gchar *text);
gboolean parse_tracks_from_string (gchar **s, Track **track);
gboolean parse_artwork_from_string(gchar **s, Artwork **artwork);
void gtkpod_init (int argc, char *argv[]);
void gtkpod_shutdown (void);

gchar *get_allowed_percent_char (void);

void add_blocked_widget(GtkWidget *w);
void block_widgets (void);
void release_widgets (void);
void update_blocked_widget (GtkWidget *w, gboolean sens);

/*void mount_ipod(void);
  void unmount_ipod(void);*/
void call_script (gchar *script, ...);

gchar **build_argv_from_strings (const gchar *first_arg, ...);

gchar *time_time_to_string (time_t t);
gchar *time_fromtime_to_string (time_t t);
gchar *time_totime_to_string (time_t t);
time_t time_string_to_time (const gchar *str);
time_t time_string_to_fromtime (const gchar *str);
time_t time_string_to_totime (const gchar *str);

gchar *get_filesize_as_string (double size);

gchar *make_sortkey (const gchar *name);
gint compare_string (const gchar *str1, const gchar *str2);
void compare_string_fuzzy_generate_keys (void);
gint compare_string_fuzzy (const gchar *str1, const gchar *str2);
const gchar *fuzzy_skip_prefix (const gchar *sortkey);
gint compare_string_case_insensitive (const gchar *str1,
				      const gchar *str2);
gint compare_string_start_case_insensitive (const gchar *haystack,
					    const gchar *needle);

gchar *filename_from_uri (const char *uri,
			  char      **hostname,
			  GError    **error);

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

gboolean mkdirhier(const gchar *dirname, gboolean silent);
gboolean mkdirhierfile(const gchar *filename);
gint64 get_size_of_directory (const gchar *dir);
gchar *convert_filename (const gchar *filename);

guint32 replaygain_to_soundcheck (gdouble gain);
gdouble soundcheck_to_replaygain (guint32 soundcheck);


void option_set_radio_button (GladeXML *win_xml,
			      const gchar *prefs_string,
			      const gchar **widgets,
			      gint dflt);
gint option_get_radio_button (GladeXML *win_xml,
			      const gchar *prefs_string,
			      const gchar **widgets);
void option_set_radio_button_gb (GtkBuilder *win_xml,
			      const gchar *prefs_string,
			      const gchar **widgets,
			      gint dflt);
gint option_get_radio_button_gb (GtkBuilder *win_xml,
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
void option_set_string_gb (GtkBuilder *win_xml,
			const gchar *name,
			const gchar *dflt);
void option_get_string_gb (GtkBuilder *win_xml,
			const gchar *name,
			gchar **value);
void option_set_toggle_button (GladeXML *win_xml,
			       const gchar *name,
			       gboolean dflt);
gboolean option_get_toggle_button (GladeXML *win_xml,
				   const gchar *name);
void option_set_toggle_button_gb (GtkBuilder *win_xml,
			       const gchar *name,
			       gboolean dflt);
gboolean option_get_toggle_button_gb (GtkBuilder *win_xml,
				   const gchar *name);

gchar *get_string_from_template (Track *track,
				 const gchar *template,
				 gboolean is_filename,
				 gboolean silent);
gchar *get_string_from_full_template (Track *track,
				      const gchar *full_template,
				      gboolean is_filename);

gchar *which (const gchar *exe);

GladeXML *gtkpod_xml_new (const gchar *gtkpod_xml_file, const gchar *name);
GtkWidget *gtkpod_xml_get_widget (GladeXML *xml, const gchar *name);

gchar *get_itdb_prefs_key (gint index, const gchar *subkey);
gchar *get_playlist_prefs_key (gint index, Playlist *pl, const gchar *subkey);
gint get_itdb_index (iTunesDB *itdb);
gchar *get_itdb_prefs_string (iTunesDB *itdb, const gchar *subkey);
gchar *get_playlist_prefs_string (Playlist *playlist, const gchar *subkey);
gint get_itdb_prefs_int (iTunesDB *itdb, const gchar *subkey);
gint get_playlist_prefs_int (Playlist *playlist, const gchar *subkey);
gboolean get_itdb_prefs_string_value (iTunesDB *itdb, const gchar *subkey,
				      gchar **value);
gboolean get_itdb_prefs_int_value (iTunesDB *itdb, const gchar *subkey,
				   gint *value);
void set_itdb_prefs_string (iTunesDB *itdb,
			    const gchar *subkey, const gchar *value);
void set_itdb_index_prefs_string (gint index,
				  const gchar *subkey, const gchar *value);
void set_itdb_prefs_int (iTunesDB *itdb, const gchar *subkey, gint value);
void set_itdb_index_prefs_int (gint index,
			       const gchar *subkey, gint value);
void remove_itdb_prefs (iTunesDB *itdb);
void load_ipod_prefs (iTunesDB *itdb, const gchar *mountpoint);
gboolean save_ipod_prefs (iTunesDB *itdb, const gchar *mountpoint);

gboolean get_offline (iTunesDB *itdb);

void delete_populate_settings(struct DeleteData *dd, gchar **label, gchar **title, gboolean *confirm_again, gchar **confirm_again_key, GString **str);

void message_sb_no_itdb_selected ();
void message_sb_no_playlist_selected ();
void message_sb_no_tracks_selected ();
void message_sb_no_ipod_itdb_selected ();

void gtkpod_shutdown ();

#endif
