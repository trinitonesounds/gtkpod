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

#ifndef __PREFS_H__
#define __PREFS_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include "display.h"

struct win_size {
    gint x;
    gint y;
};

struct cfg
{
  gchar    *ipod_mount;   /* mount point of iPod */
  gchar    *charset;      /* CHARSET to use with file operations */
  gboolean id3_write;     /* should changes to ID3 tags be written to file */
  gboolean id3_writeall;  /* should all ID3 tags be updated */
  gboolean md5songs;	  /* don't allow song duplication on your ipod */
  gboolean update_existing;/* when adding song, update existing song */
  gboolean block_display; /* block display during change of selection? */
  gboolean autoimport;	  /* whether or not to automatically import files */
  struct {
    gboolean autoselect;  /* automatically select "All" in sort tab? */
    guint    category;    /* which category was selected last? */
  } st[SORT_TAB_MAX];
  gboolean mpl_autoselect;/* select mpl automatically? */
  gboolean offline;       /* are we working offline, i.e. without iPod? */
  gboolean keep_backups;  /* write backups of iTunesDB etc to ~/.gtkpod? */
  gboolean write_extended_info; /* write additional file with PC
				   filenames etc? */
  struct {
      gchar *browse, *export;
  } last_dir;	          /* last directories used by the fileselections */
  struct {
      gboolean song, playlist, ipod_file;
  } deletion;
  struct win_size size_gtkpod;  /* last size of gtkpod main window */
  struct win_size size_conf_sw; /* last size of conf window (scrolled) */
  struct win_size size_conf;    /* last size of conf window */
  struct win_size size_dirbr;   /* last size of dirbrowser window */
  gint sm_col_width[SM_NUM_COLUMNS_PREFS]; /* width colums in song model */
  gboolean col_visible[SM_NUM_COLUMNS_PREFS]; /* displayed song model colums */
  SM_item col_order[SM_NUM_COLUMNS_PREFS]; /* order of columns */
  gboolean tag_autoset[SM_NUM_TAGS_PREFS]; /* autoset empty tags to filename?*/
  gint paned_pos[PANED_NUM];    /* position of the GtkPaned elements */

  gboolean show_duplicates;     /* show duplicate notification ?*/
  gboolean show_updated;        /* show update notification ?*/
  gboolean show_non_updated;    /* show update notification ?*/
  gboolean save_sorted_order;   /* save order after sort automatically? */
  gboolean display_toolbar;     /* should toolbar be displayed */
  gboolean update_charset;      /* Update charset when updating song? */
  gboolean write_charset;       /* Add directories recursively? */
  gboolean add_recursively;       /* Update charset when writing song? */
  GtkToolbarStyle toolbar_style;/* style of toolbar */
  gint sort_tab_num;            /* number of sort tabs displayed */
  guint32 statusbar_timeout;    /* timeout for statusbar messages */
  gint last_prefs_page;         /* last page selected in prefs window */
  gchar *play_now_path;         /* path for 'Play Now' */
  gchar *play_enqueue_path;     /* path for 'Play', i.e. 'Enqueue' */
  gboolean automount;		/* whether we should mount/unmount the ipod */
};

/* FIXME: make the global struct obsolete! */
extern struct cfg *cfg;

gchar *prefs_get_cfgdir (void);
void prefs_print(void);
void cfg_free(struct cfg *c);
void write_prefs (void);
void discard_prefs (void);
struct cfg* clone_prefs(void);
void prefs_set_mount_point(const gchar *mp);
gboolean read_prefs (GtkWidget *gtkpod, int argc, char *argv[]);

void prefs_set_offline(gboolean active);
void prefs_set_keep_backups(gboolean active);
void prefs_set_write_extended_info(gboolean active);
void prefs_set_auto_import(gboolean val);
void prefs_set_st_autoselect (guint32 inst, gboolean autoselect);
void prefs_set_mpl_autoselect (gboolean autoselect);
void prefs_set_st_category (guint32 inst, guint category);
void prefs_set_playlist_deletion(gboolean val);
void prefs_set_song_playlist_deletion(gboolean val);
void prefs_set_song_ipod_file_deletion(gboolean val);
void prefs_set_md5songs(gboolean active);
void prefs_set_update_existing(gboolean active);
void prefs_set_block_display(gboolean active);
void prefs_set_id3_write(gboolean active);
void prefs_set_id3_writeall(gboolean active);
void prefs_set_last_dir_browse(gchar * dir);
void prefs_set_last_dir_export(gchar * dir);
void prefs_set_charset (gchar *charset);
void prefs_cfg_set_charset (struct cfg *cfg, gchar *charset);
void prefs_set_size_gtkpod (gint x, gint y);
void prefs_set_size_conf_sw (gint x, gint y);
void prefs_set_size_conf (gint x, gint y);
void prefs_set_size_dirbr (gint x, gint y);
void prefs_set_sm_col_width (gint col, gint width);
void prefs_set_tag_autoset (gint category, gboolean autoset);
void prefs_set_col_visible (gint pos, gboolean visible);
void prefs_set_col_order (gint pos, SM_item col);
void prefs_set_paned_pos (gint i, gint pos);
void prefs_set_statusbar_timeout (guint32 val);
void prefs_set_automount(gboolean val);

gboolean prefs_get_offline(void);
gboolean prefs_get_keep_backups(void);
gboolean prefs_get_write_extended_info(void);
gboolean prefs_get_auto_import(void);
gboolean prefs_get_st_autoselect (guint32 inst);
gboolean prefs_get_mpl_autoselect (void);
guint prefs_get_st_category (guint32 inst);
gboolean prefs_get_playlist_deletion(void);
gboolean prefs_get_song_playlist_deletion(void);
gboolean prefs_get_song_ipod_file_deletion(void);
gboolean prefs_get_id3_write(void);
gboolean prefs_get_id3_writeall(void);
gchar *prefs_get_ipod_mount (void);
gchar * prefs_get_charset (void);
void prefs_get_size_gtkpod (gint *x, gint *y);
void prefs_get_size_conf_sw (gint *x, gint *y);
void prefs_get_size_conf (gint *x, gint *y);
void prefs_get_size_dirbr (gint *x, gint *y);
gint prefs_get_sm_col_width (gint col);
gboolean prefs_get_tag_autoset (gint category);
gboolean prefs_get_col_visible (gint pos);
SM_item prefs_get_col_order (gint pos);
gboolean prefs_get_md5songs(void);
gboolean prefs_get_update_existing(void);
gboolean prefs_get_block_display(void);
gint prefs_get_paned_pos (gint i);
guint32 prefs_get_statusbar_timeout (void);
gboolean prefs_get_show_duplicates (void);
void prefs_set_show_duplicates (gboolean val);
gboolean prefs_get_show_updated (void);
void prefs_set_show_updated (gboolean val);
gboolean prefs_get_show_non_updated (void);
void prefs_set_show_non_updated (gboolean val);
gboolean prefs_get_display_toolbar (void);
void prefs_set_display_toolbar (gboolean val);
gboolean prefs_get_update_charset (void);
void prefs_set_update_charset (gboolean val);
gboolean prefs_get_write_charset (void);
void prefs_set_write_charset (gboolean val);
gboolean prefs_get_add_recursively (void);
void prefs_set_add_recursively (gboolean val);
gboolean prefs_get_save_sorted_order (void);
void prefs_set_save_sorted_order (gboolean val);
gint prefs_get_sort_tab_num (void);
void prefs_set_sort_tab_num (gint i, gboolean update_display);
GtkToolbarStyle prefs_get_toolbar_style (void);
void prefs_set_toolbar_style (GtkToolbarStyle i);
gint prefs_get_last_prefs_page (void);
void prefs_set_last_prefs_page (gint i);
gchar *prefs_validate_play_path (const gchar *path);
void prefs_set_play_now_path (const gchar *path);
gchar *prefs_get_play_now_path (void);
void prefs_set_play_enqueue_path (const gchar *path);
gchar *prefs_get_play_enqueue_path (void);
gboolean prefs_get_automount(void);

#endif __PREFS_H__
