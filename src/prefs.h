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

/* m
enum {
    OPT_SHOW_DEFAULT=	0,
    OPT_SHOW_ARTIST = (1 << 0),
    OPT_SHOW_ALBUM  = (1 << 1),
    OPT_SHOW_YEAR   = (1 << 2),
    OPT_SHOW_TRACK  = (1 << 3),
    OPT_SHOW_GENRE  = (1 << 4),
    OPT_SHOW_ALL    = (1 << 5)	
};
*/

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
  gboolean autoimport;	  /* whether or not to automatically import files */
  struct {
    gboolean autoselect;  /* automatically select "All" in sort tab? */
    guint    category;    /* which category was selected last? */
  } st[SORT_TAB_NUM];
  gboolean offline;       /* are we working offline, i.e. without iPod? */
  gboolean keep_backups;  /* write backups of iTunesDB etc to ~/.gtkpod? */
  gboolean write_extended_info; /* write additional file with PC filenames etc? */
  struct {
      gboolean artist, album, track, genre;
  } song_list_show;       /* what columns are displayed in the song list */
  struct {
      gchar *browse, *export;
  } last_dir;	          /* last directories used by the fileselections */
  struct {
      gboolean song, playlist, ipod_file;
  } deletion;
  struct win_size size_gtkpod;  /* last size of gtkpod main window */
  struct win_size size_conf_sw; /* last size of gtkpod main window */
  struct win_size size_conf;    /* last size of gtkpod main window */
  gint sm_col_width[SM_NUM_COLUMNS_PREFS]; /* width colums in song model */
  gboolean tag_autoset[SM_NUM_TAGS_PREFS]; /* autoset empty tags to filename?*/
  gint paned_pos[PANED_NUM];    /* position of the GtkPaned elements
				 * */
  guint32 statusbar_timeout;    /* timeout for statusbar messages */
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
void prefs_set_st_category (guint32 inst, guint category);
void prefs_set_playlist_deletion(gboolean val);
void prefs_set_song_list_show_all(gboolean val);
void prefs_set_song_list_show_track(gboolean val);
void prefs_set_song_list_show_genre(gboolean val);
void prefs_set_song_list_show_album(gboolean val);
void prefs_set_song_list_show_artist(gboolean val);
void prefs_set_song_playlist_deletion(gboolean val);
void prefs_set_song_ipod_file_deletion(gboolean val);
void prefs_set_md5songs(gboolean active);
void prefs_set_id3_write(gboolean active);
void prefs_set_id3_writeall(gboolean active);
void prefs_set_last_dir_browse(gchar * dir);
void prefs_set_last_dir_export(gchar * dir);
void prefs_set_charset (gchar *charset);
void prefs_cfg_set_charset (struct cfg *cfg, gchar *charset);
void prefs_set_size_gtkpod (gint x, gint y);
void prefs_set_size_conf_sw (gint x, gint y);
void prefs_set_size_conf (gint x, gint y);
void prefs_set_sm_col_width (gint col, gint width);
void prefs_set_tag_autoset (gint category, gboolean autoset);
void prefs_set_paned_pos (gint i, gint pos);
void prefs_set_statusbar_timeout (guint32 val);

gboolean prefs_get_offline(void);
gboolean prefs_get_keep_backups(void);
gboolean prefs_get_write_extended_info(void);
gboolean prefs_get_auto_import(void);
gboolean prefs_get_st_autoselect (guint32 inst);
guint prefs_get_st_category (guint32 inst);
gboolean prefs_get_playlist_deletion(void);
gboolean prefs_get_song_list_show_all(void);
gboolean prefs_get_song_list_show_album(void);
gboolean prefs_get_song_list_show_track(void);
gboolean prefs_get_song_list_show_genre(void);
gboolean prefs_get_song_list_show_artist(void);
gboolean prefs_get_song_playlist_deletion(void);
gboolean prefs_get_song_ipod_file_deletion(void);
gboolean prefs_get_id3_write(void);
gboolean prefs_get_id3_writeall(void);
gchar *prefs_get_ipod_mount (void);
gchar * prefs_get_charset (void);
void prefs_get_size_gtkpod (gint *x, gint *y);
void prefs_get_size_conf_sw (gint *x, gint *y);
void prefs_get_size_conf (gint *x, gint *y);
gint prefs_get_sm_col_width (gint col);
gboolean prefs_get_tag_autoset (gint category);
gboolean prefs_get_md5songs(void);
gint prefs_get_paned_pos (gint i);
guint32 prefs_get_statusbar_timeout (void);

#endif __PREFS_H__
