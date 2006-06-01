/* Time-stamp: <2006-05-25 00:56:55 jcs>
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

#ifndef __PREFS_H__
#define __PREFS_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include "prefs_window.h"
#include "display.h"



/* End-of-list marker for variable-length lists */
#define LIST_END_MARKER "----++++----"

/* Different paths that can be set in the prefs window */
typedef enum
{
    PATH_PLAY_NOW = 0,
    PATH_PLAY_ENQUEUE,
    PATH_MP3GAIN,
    PATH_SYNC_CONTACTS,
    PATH_SYNC_CALENDAR,
    PATH_MSERV_MUSIC_ROOT,
    PATH_MSERV_TRACKINFO_ROOT,
    PATH_SYNC_NOTES,
    PATH_NUM
} PathType;


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


/* Not sure where to put these (maybe prefkeys.h?: Playlist-Autosync
 * options */
enum
{
    /* no auto-sync */
    PLAYLIST_AUTOSYNC_MODE_NONE = 0,
    /* use dirs from filenames in playlist */
    PLAYLIST_AUTOSYNC_MODE_AUTOMATIC = 1,
    /* use specified dir */
    PLAYLIST_AUTOSYNC_MODE_MANUAL = 2
};


struct win_size {
    gint x;
    gint y;
};

struct cfg
{
  gchar    *charset;        /* CHARSET to use with file operations */
  gboolean md5tracks;	    /* don't allow track duplication on your ipod */
  gboolean block_display;   /* block display during change of selection? */
  gboolean tmp_disable_sort;/* tmp. disable sorting during change of slctn? */
  gboolean startup_messages;/* show startup messages/warnings? */
  gboolean autoimport;	    /* whether or not to automatically import files */
  gboolean autoimport_commandline;/* whether or not to automatically
				   * import files as set by commandline */
  struct
  {
    guint    category;       /* which category was selected last? */
    /* the following fields are for the "special" tab */
    gboolean sp_or;          /* logic operation: OR? (FALSE: AND) */
    gboolean sp_rating;      /* consider rating? */
    guint32  sp_rating_state;/* pass which rating? (1 star: bit 1...) */
    gboolean sp_playcount;   /* consider playcount? */
    guint sp_playcount_low;  /* lower limit for playcounts */
    guint sp_playcount_high; /* higher limit for playcounts */
    gboolean sp_played;      /* consider last played? */
    gchar *sp_played_state;  /* current "played" string */
    gboolean sp_modified;    /* consider last modified? */
    gchar *sp_modified_state;/* current "modified" string */
    gboolean sp_added;       /* consider added? */
    gchar *sp_added_state;   /* current "added" string */
    gboolean sp_autodisplay; /* display automatically? */
  } st[SORT_TAB_MAX];
  struct sortcfg
  {         /* sort type: SORT_ASCENDING, SORT_DESCENDING, SORT_NONE */
    gint pm_sort;            /* sort type for playlists           */
    gint st_sort;            /* sort type for sort tabs           */
    gint tm_sort;            /* sort type for tracks              */
    TM_item tm_sortcol;      /* sort column for tracks            */
    gboolean pm_autostore;   /* save sort order automatically?    */
    gboolean tm_autostore;   /* save sort order automatically?    */
    gboolean case_sensitive; /* Should sorting be case-sensitive? */
    GList *tmp_sort_ign_fields; /* used in prefs_window.c only     */
    gchar *tmp_sort_ign_strings;/* used in prefs_window.c only     */
  } sortcfg;
  gboolean info_window;   /* is info window open (will then open on restart */
  gboolean mpl_autoselect;/* select mpl automatically? */
  gboolean offline;       /* are we working offline, i.e. without iPod? */
  gboolean write_extended_info; /* write additional file with PC
				   filenames etc? */
  struct
  {
      gboolean track, ipod_file, local_file, database;
  } deletion;
  struct win_size size_gtkpod;  /* last size of gtkpod main window */
  struct win_size size_cal;     /* last size of calendar window */
  struct win_size size_conf_sw; /* last size of conf window (scrolled) */
  struct win_size size_conf;    /* last size of conf window */
  struct win_size size_dirbr;   /* last size of dirbrowser window */
  struct win_size size_prefs;   /* last size of prefs window */
  struct win_size size_info;    /* last size of info window */
  gint tm_col_width[TM_NUM_COLUMNS];    /* width colums in track model */
  gboolean col_visible[TM_NUM_COLUMNS]; /* displayed track model colums */
  TM_item col_order[TM_NUM_COLUMNS];    /* order of columns */
  gboolean readtags;            /* Read tags from file contents? */
  gboolean parsetags;           /* Get tags by parsing filename? */
  gchar   *parsetags_template;  /* template for parsing tags */
  gboolean parsetags_overwrite; /* Overwrite tags already set? */
  gboolean coverart;            /* Read coverart when adding files? */
  gchar *coverart_template;     /* template for coverart filename   */
  gboolean autosettags[TM_NUM_TAGS_PREFS]; /* autoset empty tags to filename?*/
  gint paned_pos[PANED_NUM];    /* position of the GtkPaned elements */

  gboolean display_toolbar;     /* should toolbar be displayed */
  GtkToolbarStyle toolbar_style;/* style of toolbar */
  gboolean display_tooltips_main; /* should tooltips be displayed (main) */
  gboolean display_tooltips_prefs;/* should toolbar be displayed (prefs) */
  gboolean update_charset;      /* Update charset when updating track? */
  gboolean write_charset;       /* Use selected charset when writing track? */
  gboolean add_recursively;     /* Add directories recursively? */
  gint sort_tab_num;            /* number of sort tabs displayed */
  gboolean group_compilations;  /* group compilations when browsing */
  guint32 statusbar_timeout;    /* timeout for statusbar messages */
  gint last_prefs_page;         /* last page selected in prefs window */
  gboolean automount;		/* whether we should mount/unmount the ipod */
  gboolean multi_edit;          /* multi edit enabled? */
  gboolean multi_edit_title;    /* multi edit also enabled for title field? */
  gboolean not_played_track;    /* not played track in Highest rated playlist?*/
  gint misc_track_nr;            /* track's nr in the Highest rated, most played and most recently played pl*/
  gboolean mserv_use;           /* use mserv data (rating)? */
  gchar *mserv_username;        /* username for mserv? */
  gboolean unused_gboolean3;    /* unused */
  float version;                /* version of gtkpod writing the cfg file */
};


/* types for sortcfg.xx_sort */
enum
{
    SORT_ASCENDING = GTK_SORT_ASCENDING,
    SORT_DESCENDING = GTK_SORT_DESCENDING,
    SORT_NONE = 10*(GTK_SORT_ASCENDING+GTK_SORT_DESCENDING),
};

/* New prefs backend. Will replace the stuff above */

/* 
 * Wrapper data types for temp prefrences
 */

/* A wrapper around a GTree for regular temporary prefrences */
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
void init_prefs(int argc, char *argv[]);
void cleanup_prefs();

/*
 * Functions that are used to manipulate prefrences.
 * The prefrences table shouldn't be modified directly.
 */

/* Functions that set prefrence values */

void prefs_set_string(const gchar *key, const gchar *value);
void prefs_set_int(const gchar *key, const gint value);
void prefs_set_int64(const gchar *key, const gint64 value);

/* The index parameter is used for numbered preference keys.
 * (i.e. pref0, pref1, etc) */
void prefs_set_string_index(const gchar *key, const guint index,
			    const gchar *value);
void prefs_set_int_index(const gchar *key, const guint index,
			 const gint value);
void prefs_set_int64_index(const gchar *key, guint index,
			   const gint64 value);

/* Functions that get prefrence values */
gchar *prefs_get_string(const gchar *key);
gboolean prefs_get_string_value(const gchar *key, gchar **value);
gint prefs_get_int(const gchar *key);
gboolean prefs_get_int_value(const gchar *key, gint *value);
gint64 prefs_get_int64(const gchar *key);
gboolean prefs_get_int64_value(const gchar *key, gint64 *value);

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
TempPrefs *prefs_create_subset (const gchar *subkey);
TempPrefs *temp_prefs_create_subset (TempPrefs *temp_prefs,
				     const gchar *subkey);
void temp_prefs_destroy (TempPrefs *temp_prefs);
void temp_prefs_apply (TempPrefs *temp_prefs);
void temp_prefs_flush(TempPrefs *temp_prefs);
gint temp_prefs_size (TempPrefs *temp_prefs);

/*
 * Functions that add various types of info to the temp prefs tree.
 */
void temp_prefs_set_string(TempPrefs *temp_prefs, const gchar *key,
			   const gchar *value);
void temp_prefs_set_int(TempPrefs *temp_prefs, const gchar *key,
			const gint value);
void temp_prefs_set_int64(TempPrefs *temp_prefs, const gchar *key,
			  const gint64 value);

/*
 * Functions that retrieve various types of info from the temp prefs tree.
 */
gchar *temp_prefs_get_string(TempPrefs *temp_prefs, const gchar *key);
gboolean temp_prefs_get_string_value(TempPrefs *temp_prefs,
				     const gchar *key, gchar **value);
gint temp_prefs_get_int(TempPrefs *temp_prefs, const gchar *key);
gboolean temp_prefs_get_int_value(TempPrefs *temp_prefs,
				  const gchar *key, gint *value);


/* Numbered prefrences functions */
void temp_prefs_set_string_index(TempPrefs *temp_prefs, const gchar *key,
				 const guint index, const gchar *value);
void temp_prefs_set_int_index(TempPrefs *temp_prefs, const gchar *key,
			      const guint index, const gint value);
void temp_prefs_set_int64_index(TempPrefs *temp_prefs, const gchar *key,
				const guint index, const gint64 value);


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
void prefs_print(void);
void cfg_free(struct cfg *c);
void sortcfg_free(struct sortcfg *c);
void write_prefs (void);
void discard_prefs (void);
struct cfg* clone_prefs(void);
struct sortcfg* clone_sortprefs(void);
gboolean read_prefs_old (GtkWidget *gtkpod, int argc, char *argv[]);
gchar *prefs_validate_path (const gchar *path, const gchar *allowed);

void prefs_set_offline(gboolean active);
void prefs_set_pm_sort (gint type);
void prefs_set_tm_sort (gint type);
void prefs_set_st_sort (gint type);
void prefs_set_tm_sortcol (TM_item col);
void prefs_set_tm_autostore (gboolean active);
void prefs_set_pm_autostore (gboolean active);
void prefs_set_write_extended_info(gboolean active);
void prefs_set_autoimport(gboolean val);
void prefs_set_mpl_autoselect (gboolean autoselect);
void prefs_set_md5tracks(gboolean active);
void prefs_set_block_display(gboolean active);
void prefs_set_readtags (gboolean active);
gboolean prefs_get_readtags(void);
void prefs_set_parsetags (gboolean active);
gboolean prefs_get_parsetags(void);
void prefs_set_parsetags_overwrite (gboolean active);
gboolean prefs_get_parsetags_overwrite(void);
void prefs_set_autosettags (gint category, gboolean autoset);
gboolean prefs_get_autosettags (gint category);
void prefs_set_parsetags_template (const gchar *tpl);
const gchar *prefs_get_parsetags_template (void);
void prefs_set_coverart (gboolean active);
gboolean prefs_get_coverart(void);
void prefs_set_coverart_template (const gchar *tpl);
const gchar *prefs_get_coverart_template (void);
void prefs_set_charset (gchar *charset);
void prefs_cfg_set_charset (struct cfg *cfg, gchar *charset);
void prefs_set_size_gtkpod (gint x, gint y);
void prefs_set_size_cal (gint x, gint y);
void prefs_set_size_conf_sw (gint x, gint y);
void prefs_set_size_conf (gint x, gint y);
void prefs_set_size_dirbr (gint x, gint y);
void prefs_set_size_prefs (gint x, gint y);
void prefs_set_size_info (gint x, gint y);
void prefs_set_tm_col_width (gint col, gint width);
void prefs_set_col_visible (TM_item tm_item, gboolean visible);
void prefs_set_col_order (gint pos, TM_item col);
void prefs_set_paned_pos (gint i, gint pos);
void prefs_set_statusbar_timeout (guint32 val);
void prefs_set_automount(gboolean val);
void prefs_set_info_window(gboolean val);

gboolean prefs_get_offline(void);
gint prefs_get_pm_sort (void);
gint prefs_get_st_sort (void);
gint prefs_get_tm_sort (void);
TM_item prefs_get_tm_sortcol (void);
gboolean prefs_get_tm_autostore (void);
gboolean prefs_get_pm_autostore (void);
gboolean prefs_get_write_extended_info(void);
gboolean prefs_get_autoimport(void);
gboolean prefs_get_mpl_autoselect (void);
gchar * prefs_get_charset (void);
void prefs_get_size_gtkpod (gint *x, gint *y);
void prefs_get_size_cal (gint *x, gint *y);
void prefs_get_size_conf_sw (gint *x, gint *y);
void prefs_get_size_conf (gint *x, gint *y);
void prefs_get_size_dirbr (gint *x, gint *y);
void prefs_get_size_prefs (gint *x, gint *y);
void prefs_get_size_info (gint *x, gint *y);
gint prefs_get_tm_col_width (gint col);
gboolean prefs_get_col_visible (TM_item tm_item);
TM_item prefs_get_col_order (gint pos);
gboolean prefs_get_md5tracks(void);
gboolean prefs_get_block_display(void);
gint prefs_get_paned_pos (gint i);
guint32 prefs_get_statusbar_timeout (void);

gboolean prefs_get_display_toolbar (void);
void prefs_set_display_toolbar (gboolean val);
gboolean prefs_get_update_charset (void);
void prefs_set_update_charset (gboolean val);
gboolean prefs_get_write_charset (void);
void prefs_set_write_charset (gboolean val);
gboolean prefs_get_add_recursively (void);
void prefs_set_add_recursively (gboolean val);
gboolean prefs_get_case_sensitive (void);
void prefs_set_case_sensitive (gboolean val);
gint prefs_get_sort_tab_num (void);
void prefs_set_sort_tab_num (gint i, gboolean update_display);
gboolean prefs_get_group_compilations (void);
void prefs_set_group_compilations (gboolean val, gboolean update_display);
GtkToolbarStyle prefs_get_toolbar_style (void);
void prefs_set_toolbar_style (GtkToolbarStyle i);
gint prefs_get_last_prefs_page (void);
void prefs_set_last_prefs_page (gint i);
gboolean prefs_get_automount (void);
gboolean prefs_get_info_window (void);
void prefs_set_sp_or (guint32 inst, gboolean state);
gboolean prefs_get_sp_or (guint32 inst);
void prefs_set_sp_cond (guint32 inst, T_item t_item, gboolean state);
gboolean prefs_get_sp_cond (guint32 inst, T_item t_item);
void prefs_set_sp_rating_n (guint32 inst, gint n, gboolean state);
gboolean prefs_get_sp_rating_n (guint32 inst, gint n);
void prefs_set_sp_rating_state (guint32 inst, guint32 state);
guint32 prefs_get_sp_rating_state (guint32 inst);
void prefs_set_sp_entry (guint32 inst, T_item t_item, const gchar *str);
gchar *prefs_get_sp_entry (guint32 inst, T_item t_item);
void prefs_set_sp_autodisplay (guint32 inst, gboolean state);
gboolean prefs_get_sp_autodisplay (guint32 inst);
gint32 prefs_get_sp_playcount_low (guint32 inst);
gint32 prefs_get_sp_playcount_high (guint32 inst);
void prefs_set_sp_playcount_low (guint32 inst, gint32 limit);
void prefs_set_sp_playcount_high (guint32 inst, gint32 limit);
void prefs_set_display_tooltips_main (gboolean state);
gboolean prefs_get_display_tooltips_main (void);
void prefs_set_display_tooltips_prefs (gboolean state);
gboolean prefs_get_display_tooltips_prefs (void);
void prefs_set_multi_edit (gboolean state);
gboolean prefs_get_multi_edit (void);
void prefs_set_misc_track_nr (gint state);
gint prefs_get_misc_track_nr (void);
void prefs_set_not_played_track (gboolean state);
gboolean prefs_get_not_played_track (void);
void prefs_set_multi_edit_title (gboolean state);
gboolean prefs_get_multi_edit_title (void);
void prefs_set_unused_gboolean3(gboolean val);
gboolean prefs_get_unused_gboolean3(void);
void prefs_set_tmp_disable_sort(gboolean val);
gboolean prefs_get_tmp_disable_sort(void);
void prefs_set_startup_messages(gboolean val);
gboolean prefs_get_startup_messages(void);
gboolean prefs_get_mserv_use(void);
void prefs_set_mserv_use(gboolean val);
const gchar *prefs_get_mserv_username(void);
void prefs_set_mserv_username(const gchar *root);
gboolean prefs_get_autoimport_commandline(void);
void prefs_set_autoimport_commandline(gboolean val);

/* Podcast preferences */
void prefs_set_pc_dir(const gchar *str);
gchar *prefs_get_pc_dir(void);
void prefs_set_pc_del_age(gboolean val);
gboolean prefs_get_pc_del_age(void);
void prefs_set_pc_del_age_val(gint val);
gint prefs_get_pc_del_age_val(void);
void prefs_set_pc_del_copied(gboolean val);
gboolean prefs_get_pc_del_copied(void);
void prefs_set_pc_auto_fetch(gboolean val);
gboolean prefs_get_pc_auto_fetch(void);
void prefs_set_pc_log(gboolean val);
gboolean prefs_get_pc_log(void);
void prefs_set_pc_log_file(const gchar *str);
gchar *prefs_get_pc_log_file(void);
void prefs_set_pc_auto_sync(gboolean val);
gboolean prefs_get_pc_auto_sync(void);
void prefs_set_pc_ipod_del_age(gboolean val);
gboolean prefs_get_pc_ipod_del_age(void);
void prefs_set_pc_ipod_del_age_val(gint val);
gint prefs_get_pc_ipod_del_age_val(void);
void prefs_set_pc_ipod_del_played(gboolean val);
gboolean prefs_get_pc_ipod_del_played(void);
void prefs_set_pc_ipod_inc_date(gboolean val);
gboolean prefs_get_pc_ipod_inc_date(void);
void prefs_set_pc_change_genre(gboolean val);
gboolean prefs_get_pc_change_genre(void);

gboolean prefs_get_disable_sorting(void);

#endif
