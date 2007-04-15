/*
|  Copyright (C) 2002-2007 Jorg Schuler <jcsjcs at users sourceforge net>
|  Copyright (C) 2006 James Liggett <jrliggett at cox.net>
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

#ifndef __PREFS_H__
#define __PREFS_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include "prefs_window.h"
#include "display.h"



/* Not sure where to put these (maybe prefkeys.h?): prefs keys used */
/* repository.c */
extern const gchar *KEY_CONCAL_AUTOSYNC;
extern const gchar *KEY_SYNC_DELETE_TRACKS;
extern const gchar *KEY_SYNC_CONFIRM_DIRS;
extern const gchar *KEY_SYNC_CONFIRM_DELETE;
extern const gchar *KEY_SYNC_SHOW_SUMMARY;
extern const gchar *KEY_MOUNTPOINT;
extern const gchar *KEY_IPOD_MODEL;
extern const gchar *KEY_FILENAME;
extern const gchar *KEY_PATH_SYNC_CONTACTS;
extern const gchar *KEY_PATH_SYNC_CALENDAR;
extern const gchar *KEY_PATH_SYNC_NOTES;
extern const gchar *KEY_SYNCMODE;
extern const gchar *KEY_MANUAL_SYNCDIR;
extern const gchar *KEY_DISPLAY_COVERART;

/* New prefs backend. Will replace the stuff above */

/* 
 * Wrapper data types for temp preferences
 */

/* A wrapper around a GTree for regular temporary preferences */
typedef struct 
{
	GTree *tree;
} TempPrefs;

/* A wrapper around a GTree for variable-length list */
typedef struct 
{
	GTree *tree;
} TempLists;

/* Prefrences setup and cleanup */
void prefs_init (int argc, char *argv[]);
void prefs_save (void);
void prefs_shutdown (void);

/*
 * Functions that are used to manipulate preferences.
 * The preferences table shouldn't be modified directly.
 */

/* Functions that set preference values */

void prefs_set_string(const gchar *key, const gchar *value);
void prefs_set_int(const gchar *key, const gint value);
void prefs_set_int64(const gchar *key, const gint64 value);
void prefs_set_double(const gchar *key, gdouble value);

/* The index parameter is used for numbered preference keys.
 * (i.e. pref0, pref1, etc) */
void prefs_set_string_index(const gchar *key, const guint index,
			    const gchar *value);
void prefs_set_int_index(const gchar *key, const guint index,
			 const gint value);
void prefs_set_int64_index(const gchar *key, guint index,
			   const gint64 value);
void prefs_set_double_index(const gchar *key, guint index,
			    gdouble value);

/* Functions that get preference values */
gchar *prefs_get_string(const gchar *key);
gboolean prefs_get_string_value(const gchar *key, gchar **value);
gint prefs_get_int(const gchar *key);
gboolean prefs_get_int_value(const gchar *key, gint *value);
gint64 prefs_get_int64(const gchar *key);
gboolean prefs_get_int64_value(const gchar *key, gint64 *value);
gdouble prefs_get_double(const gchar *key);
gboolean prefs_get_double_value(const gchar *key, gdouble *value);

/* Numbered prefs functions */
gchar *prefs_get_string_index(const gchar *key, const guint index);
gboolean prefs_get_string_value_index(const gchar *key,
				      const guint index, gchar **value);
gint prefs_get_int_index(const gchar *key, const guint index);
gboolean prefs_get_int_value_index(const gchar *key, const guint index,
				   gint *value);
gint64 prefs_get_int64_index(const gchar *key, const guint index);
gboolean prefs_get_int64_value_index(const gchar *key,
				     const guint index, gint64 *value);
gdouble prefs_get_double_index(const gchar *key,
			       guint index);
gboolean prefs_get_double_value_index(const gchar *key, guint index,
				      gdouble *value);
/* Special functions */
void prefs_flush_subkey (const gchar *subkey);
void prefs_rename_subkey (const gchar *subkey_old, const gchar *subkey_new);
void temp_prefs_rename_subkey (TempPrefs *temp_prefs,
			       const gchar *subkey_old,
			       const gchar *subkey_new);
gboolean temp_prefs_subkey_exists (TempPrefs *temp_prefs,
				   const gchar *subkey);

/* 
 * Temp prefs functions
 */
TempPrefs *temp_prefs_create (void);
TempPrefs *temp_prefs_create_subset (TempPrefs *temp_prefs,
				     const gchar *subkey);
void temp_prefs_destroy (TempPrefs *temp_prefs);
void temp_prefs_apply (TempPrefs *temp_prefs);
void temp_prefs_flush(TempPrefs *temp_prefs);
gint temp_prefs_size (TempPrefs *temp_prefs);

/*
 * Functions that add various types of info to the temp prefs tree.
 */
void temp_prefs_remove_key (TempPrefs *temp_prefs, const gchar *key);
void temp_prefs_set_string(TempPrefs *temp_prefs, const gchar *key,
			   const gchar *value);
void temp_prefs_set_int(TempPrefs *temp_prefs, const gchar *key,
			const gint value);
void temp_prefs_set_int64(TempPrefs *temp_prefs, const gchar *key,
			  const gint64 value);
void temp_prefs_set_double(TempPrefs *temp_prefs, const gchar *key,
			   gdouble value);

/*
 * Functions that retrieve various types of info from the temp prefs tree.
 */
gchar *temp_prefs_get_string(TempPrefs *temp_prefs, const gchar *key);
gboolean temp_prefs_get_string_value(TempPrefs *temp_prefs,
				     const gchar *key, gchar **value);
gint temp_prefs_get_int(TempPrefs *temp_prefs, const gchar *key);
gboolean temp_prefs_get_int_value(TempPrefs *temp_prefs,
				  const gchar *key, gint *value);
gdouble temp_prefs_get_double(TempPrefs *temp_prefs, const gchar *key);
gboolean temp_prefs_get_double_value(TempPrefs *temp_prefs, const gchar *key,
				    gdouble *value);

/* Numbered preferences functions */
void temp_prefs_set_string_index(TempPrefs *temp_prefs, const gchar *key,
				 const guint index, const gchar *value);
void temp_prefs_set_int_index(TempPrefs *temp_prefs, const gchar *key,
			      const guint index, const gint value);
void temp_prefs_set_int64_index(TempPrefs *temp_prefs, const gchar *key,
				const guint index, const gint64 value);
void temp_prefs_set_double_index(TempPrefs *temp_prefs, const gchar *key,
				 guint index, gdouble value);


/* 
 * Functions for variable-length lists
 */
 
TempLists *temp_lists_create (void);
void temp_lists_destroy(TempLists *temp_lists);
void temp_list_add(TempLists *temp_lists, const gchar *key, GList *list);
void temp_lists_apply(TempLists *temp_lists);
void prefs_apply_list(gchar *key, GList *list);
GList *prefs_get_list(const gchar *key);
void prefs_free_list(GList *list);
GList *get_list_from_buffer(GtkTextBuffer *buffer);

gchar *prefs_get_cfgdir (void);
#endif
