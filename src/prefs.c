/* Time-stamp: <2003-11-29 13:23:30 jcs>
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
|
|  $Id$
*/

/* -------------------------------------------------------------------
 *
 * HOWTO add a new_option to the prefs dialog
 *
 * - add the desired option to the prefs window using glade-2
 *
 * - modify the cfg structure in prefs.h accordingly
 *
 * - set the default value of new_option in cfg_new() in prefs.c
 *
 * - add function prefs_get_new_option() and
 *   prefs_set_new_option() to prefs.[ch]. These functions are
 *   called from within gtkpod to query/set the state of the option.
 *   prefs_set_new_option() should verify that the value passed is
 *   valid. 
 *
 * - add a function prefs_window_set_new_option to
 *   prefs_window.[ch]. This function is called from the callback
 *   functions in callback.c to set the state of the option. The value
 *   is applied to the actual prefs when pressing the "OK" or "Apply"
 *   button in the prefs window.
 *
 * - if your option is a pointer to data, make sure the data is copied
 *   in clone_prefs() in prefs.c
 *
 * - add code to prefs_window_create() in prefs_window.c to set the
 *   correct state of the option in the prefs window.
 *
 * - add code to prefs_window_set() in prefs_window.c to actually take
 *   over the new state when closing the prefs window
 *
 * - add code to write_prefs_to_file_desc() to write the state of
 *   new_option to the prefs file
 *
 * - add code to read_prefs_from_file_desc() to read the
 *   new_option from the prefs file
 *
 * - if you want new_option to be a command line option as well, add
 *   code to usage() and read_prefs()
 *
 * ---------------------------------------------------------------- */

/* FIXME: simplify code to make adding of new options easier:
                  prefs_window_create()
                  write_prefs_to_file_desc()
                  read_prefs_from_file_desc()
*/

/* This tells Alpha OSF/1 not to define a getopt prototype in <stdio.h>.
   Ditto for AIX 3.2 and <stdlib.h>.  */
#ifndef _NO_PROTO
# define _NO_PROTO
#endif

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef HAVE_GETOPT_LONG_ONLY
#  include <getopt.h>
#else
#  include "getopt.h"
#endif
#include "display.h"
#include "info.h"
#include "md5.h"
#include "misc.h"
#include "prefs.h"
#include "support.h"

/* global config struct */
static struct cfg *cfg = NULL;

/* enum for reading of options */
enum {
  GP_HELP,
  GP_MOUNT,
  GP_ID3_WRITE,
  GP_MD5TRACKS,
  GP_AUTO,
  GP_OFFLINE
};


/* need to convert to locale charset before printing to console */
#define usage_fpf(file, ...) do { gchar *utf8=g_strdup_printf (__VA_ARGS__); gchar *loc=g_locale_from_utf8 (utf8, -1, NULL, NULL, NULL); fprintf (file, "%s", loc); g_free (loc); g_free (utf8);} while (FALSE)

static void usage (FILE *file)
{
  usage_fpf(file, _("gtkpod version %s usage:\n"), VERSION);
  usage_fpf(file, _("  -h, --help:   display this message\n"));
  usage_fpf(file, _("  -m path:      define the mountpoint of your iPod\n"));
  usage_fpf(file, _("  --mountpoint: same as '-m'.\n"));
  usage_fpf(file, _("  -w:           write changed ID3 tags to file\n"));
  usage_fpf(file, _("  --id3_write:   same as '-w'.\n"));
  usage_fpf(file, _("  -c:           check files automagically for duplicates\n"));
  usage_fpf(file, _("  --md5:        same as '-c'.\n"));
  usage_fpf(file, _("  -a:           import database automatically after start.\n"));
  usage_fpf(file, _("  --auto:       same as '-a'.\n"));
  usage_fpf(file, _("  -o:           use offline mode. No changes are exported to the iPod,\n                but to ~/.gtkpod/ instead. iPod is updated if 'Sync' is\n                used with 'Offline' deactivated.\n"));
  usage_fpf(file, _("  --offline:    same as '-o'.\n"));
}

struct cfg *cfg_new(void)
{
    struct cfg *mycfg = NULL;
    gchar buf[PATH_MAX], *str;
    gint i;

    mycfg = g_malloc0 (sizeof (struct cfg));
    if(getcwd(buf, PATH_MAX))
    {
	mycfg->last_dir.browse = g_strdup_printf ("%s/", buf);
	mycfg->last_dir.export = g_strdup_printf ("%s/", buf);
    }
    else
    {
	mycfg->last_dir.browse = g_strdup ("~/");
	mycfg->last_dir.export = g_strdup ("~/");
    }
    if((str = getenv("IPOD_MOUNTPOINT")))
    {
	snprintf(buf, PATH_MAX, "%s", str);
	mycfg->ipod_mount = g_strdup_printf("%s/", buf);
    }
    else
    {
	mycfg->ipod_mount = g_strdup ("/mnt/ipod");
    }
    mycfg->charset = NULL;
    mycfg->deletion.track = TRUE;
    mycfg->deletion.ipod_file = TRUE;
    mycfg->deletion.syncing = TRUE;
    mycfg->md5tracks = FALSE;
    mycfg->update_existing = FALSE;
    mycfg->block_display = FALSE;
    mycfg->autoimport = FALSE;
    for (i=0; i<SORT_TAB_MAX; ++i)
    {
	mycfg->st[i].autoselect = TRUE;
	mycfg->st[i].category = (i<ST_CAT_NUM ? i:0);
	mycfg->st[i].sp_or = FALSE;
	mycfg->st[i].sp_rating = FALSE;
	mycfg->st[i].sp_rating_state = 0;
	mycfg->st[i].sp_playcount = FALSE;
	mycfg->st[i].sp_playcount_low = 0;
	mycfg->st[i].sp_playcount_high = -1;
	mycfg->st[i].sp_played = FALSE;
	mycfg->st[i].sp_played_state = g_strdup (">4w");
	mycfg->st[i].sp_modified = FALSE;
	mycfg->st[i].sp_modified_state = g_strdup ("<1d");
	mycfg->st[i].sp_autodisplay = FALSE;
    }
    mycfg->mpl_autoselect = TRUE;
    mycfg->offline = FALSE;
    mycfg->keep_backups = TRUE;
    mycfg->write_extended_info = TRUE;
    mycfg->id3_write = FALSE;
    mycfg->id3_writeall = FALSE;
    mycfg->size_gtkpod.x = 600;
    mycfg->size_gtkpod.y = 500;
    mycfg->size_cal.x = 500;
    mycfg->size_cal.y = 350;
    mycfg->size_conf_sw.x = 300;
    mycfg->size_conf_sw.y = 300;
    mycfg->size_conf.x = 300;
    mycfg->size_conf.y = -1;
    mycfg->size_dirbr.x = 300;
    mycfg->size_dirbr.y = 400;
    mycfg->size_prefs.x = -1;
    mycfg->size_prefs.y = 480;
    for (i=0; i<TM_NUM_COLUMNS; ++i)
    {
	mycfg->tm_col_width[i] = 80;
	mycfg->col_visible[i] = FALSE;
	mycfg->col_order[i] = i;
    }
    mycfg->col_visible[TM_COLUMN_ARTIST] = TRUE;
    mycfg->col_visible[TM_COLUMN_ALBUM] = TRUE;
    mycfg->col_visible[TM_COLUMN_TITLE] = TRUE;
    mycfg->col_visible[TM_COLUMN_GENRE] = TRUE;
    mycfg->col_visible[TM_COLUMN_PLAYCOUNT] = TRUE;
    mycfg->col_visible[TM_COLUMN_RATING] = TRUE;
    for (i=0; i<TM_NUM_TAGS_PREFS; ++i)
    {
	mycfg->tag_autoset[i] = FALSE;
    }
    mycfg->tag_autoset[TM_COLUMN_TITLE] = TRUE;
    for (i=0; i<PANED_NUM; ++i)
    {
	mycfg->paned_pos[i] = -1;  /* -1 means: let gtk worry about position */
    }
    mycfg->show_duplicates = TRUE;
    mycfg->show_updated = TRUE;
    mycfg->show_non_updated = TRUE;
    mycfg->show_sync_dirs = TRUE;
    mycfg->sync_remove = TRUE;
    mycfg->display_toolbar = TRUE;
    mycfg->toolbar_style = GTK_TOOLBAR_BOTH;
    mycfg->display_tooltips_main = TRUE;
    mycfg->display_tooltips_prefs = TRUE;
    mycfg->update_charset = FALSE;
    mycfg->write_charset = FALSE;
    mycfg->add_recursively = TRUE;
    mycfg->sort_tab_num = 2;
    mycfg->last_prefs_page = 0;
    mycfg->statusbar_timeout = STATUSBAR_TIMEOUT;
    mycfg->play_now_path = g_strdup ("xmms -p %s");
    mycfg->play_enqueue_path = g_strdup ("xmms -e %s");
    mycfg->mp3gain_path = g_strdup ("");
    mycfg->time_format = g_strdup ("%k:%M %d %b %g");
    mycfg->filename_format = g_strdup ("%A/%d/%t - %n.mp3");
    mycfg->automount = FALSE;
    mycfg->info_window = FALSE;
    mycfg->multi_edit = FALSE;
    mycfg->multi_edit_title = TRUE;
    mycfg->not_played_track = TRUE;
    mycfg->misc_track_nr = 25;
    mycfg->sortcfg.pm_sort = SORT_NONE;
    mycfg->sortcfg.st_sort = SORT_NONE;
    mycfg->sortcfg.tm_sort = SORT_NONE;
    mycfg->sortcfg.tm_sortcol = TM_COLUMN_TITLE;
    mycfg->sortcfg.pm_autostore = FALSE;
    mycfg->sortcfg.tm_autostore = FALSE;
    mycfg->sortcfg.case_sensitive = FALSE;
    return(mycfg);
}

static void
read_prefs_from_file_desc(FILE *fp)
{
    gchar buf[PATH_MAX];
    gchar *line, *arg, *bufp;
    gint len;

    if(fp)
    {
      while (fgets (buf, PATH_MAX, fp))
	{
	  /* allow comments */
	  if ((buf[0] == ';') || (buf[0] == '#')) continue;
	  arg = strchr (buf, '=');
	  if (!arg || (arg == buf))
	    {
	      gtkpod_warning (_("Error while reading prefs: %s\n"), buf);
	      continue;
	    }
	  /* skip whitespace */
	  bufp = buf;
	  while (g_ascii_isspace(*bufp)) ++bufp;
	  line = g_strndup (buf, arg-bufp);
	  ++arg;
	  len = strlen (arg); /* remove newline */
	  if((len>0) && (arg[len-1] == 0x0a))  arg[len-1] = 0;
	  /* skip whitespace */
	  while (g_ascii_isspace(*arg)) ++arg;
	  if(g_ascii_strcasecmp (line, "version") == 0)
	  {
	      cfg->version = g_ascii_strtod (arg, NULL);
	  }
	  else if(g_ascii_strcasecmp (line, "mountpoint") == 0)
	  {
	      prefs_set_mount_point (arg);
	  }
	  else if(g_ascii_strcasecmp (line, "play_now_path") == 0)
	  {
	      prefs_set_play_now_path (arg);
	  }
	  else if(g_ascii_strcasecmp (line, "play_enqueue_path") == 0)
	  {
	      prefs_set_play_enqueue_path (arg);
	  }
	  else if(g_ascii_strcasecmp (line, "mp3gain_path") == 0)
	  {
	      prefs_set_mp3gain_path (arg);
	  }
	  else if(g_ascii_strcasecmp (line, "time_format") == 0)
	  {
	      prefs_set_time_format (arg);
	  }
	  else if(g_ascii_strcasecmp (line, "filename_format") == 0)
	  {
	      prefs_set_filename_format (arg);
	  }
	  else if(g_ascii_strcasecmp (line, "charset") == 0)
	  {
		if(strlen (arg))      prefs_set_charset(arg);
	  }
	  else if(g_ascii_strcasecmp (line, "id3") == 0)
	  {
	      prefs_set_id3_write((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "id3_all") == 0)
	  {
	      prefs_set_id3_writeall((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "md5") == 0)
	  {
	      prefs_set_md5tracks((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "update_existing") == 0)
	  {
	      prefs_set_update_existing((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "block_display") == 0)
	  {
	      prefs_set_block_display((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "delete_file") == 0)
	  {
	      prefs_set_track_playlist_deletion((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "delete_playlist") == 0)
	  {
	      /* ignore -- no longer supported as of 0.61-CVS */
	  }
	  else if(g_ascii_strcasecmp (line, "delete_ipod") == 0)
	  {
	      prefs_set_track_ipod_file_deletion((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "sync_remove_confirm") == 0)
	  {
	      prefs_set_sync_remove_confirm((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "auto_import") == 0)
	  {
	      prefs_set_auto_import((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strncasecmp (line, "st_autoselect", 13) == 0)
	  {
	      gint i = atoi (line+13);
	      prefs_set_st_autoselect (i, atoi (arg));
	  }      
	  else if(g_ascii_strncasecmp (line, "st_category", 11) == 0)
	  {
	      gint i = atoi (line+11);
	      prefs_set_st_category (i, atoi (arg));
	  }      
	  else if(g_ascii_strncasecmp (line, "sp_or", 4) == 0)
	  {
	      gint i = atoi (line+4);
	      prefs_set_sp_or (i, atoi (arg));
	  }      
	  else if(g_ascii_strncasecmp (line, "sp_rating_cond", 14) == 0)
	  {
	      gint i = atoi (line+14);
	      prefs_set_sp_cond (i, T_RATING, atoi (arg));
	  }      
	  else if(g_ascii_strncasecmp (line, "sp_playcount_cond", 17) == 0)
	  {
	      gint i = atoi (line+17);
	      prefs_set_sp_cond (i, T_PLAYCOUNT, atoi (arg));
	  }      
	  else if(g_ascii_strncasecmp (line, "sp_played_cond", 14) == 0)
	  {
	      gint i = atoi (line+14);
	      prefs_set_sp_cond (i, T_TIME_PLAYED, atoi (arg));
	  }      
	  else if(g_ascii_strncasecmp (line, "sp_modified_cond", 16) == 0)
	  {
	      gint i = atoi (line+16);
	      prefs_set_sp_cond (i, T_TIME_MODIFIED, atoi (arg));
	  }      
	  else if(g_ascii_strncasecmp (line, "sp_rating_state", 15) == 0)
	  {
	      gint i = atoi (line+15);
	      prefs_set_sp_rating_state (i, atoi (arg));
	  }      
	  else if(g_ascii_strncasecmp (line, "sp_playcount_low", 16) == 0)
	  {
	      gint i = atoi (line+16);
	      prefs_set_sp_playcount_low (i, atoi (arg));
	  }      
	  else if(g_ascii_strncasecmp (line, "sp_playcount_high", 17) == 0)
	  {
	      gint i = atoi (line+17);
	      prefs_set_sp_playcount_high (i, atoi (arg));
	  }      
	  else if(g_ascii_strncasecmp (line, "sp_played_state", 15) == 0)
	  {
	      gint i = atoi (line+15);
	      prefs_set_sp_entry (i, T_TIME_PLAYED, arg);
	  }      
	  else if(g_ascii_strncasecmp (line, "sp_modified_state", 17) == 0)
	  {
	      gint i = atoi (line+17);
	      prefs_set_sp_entry (i, T_TIME_MODIFIED, arg);
	  }      
	  else if(g_ascii_strncasecmp (line, "sp_autodisplay", 14) == 0)
	  {
	      gint i = atoi (line+14);
	      prefs_set_sp_autodisplay (i, atoi (arg));
	  }
	  else if(g_ascii_strcasecmp (line, "mpl_autoselect") == 0)
	  {
	      prefs_set_mpl_autoselect((gboolean)atoi(arg));
	  }
	  else if((g_ascii_strncasecmp (line, "tm_col_width", 12) == 0) ||
		  (g_ascii_strncasecmp (line, "sm_col_width", 12) == 0))
	  {
	      gint i = atoi (line+12);
	      prefs_set_tm_col_width (i, atoi (arg));
	  }      
	  else if(g_ascii_strncasecmp (line, "tag_autoset", 11) == 0)
	  {
	      gint i = atoi (line+11);
	      prefs_set_tag_autoset (i, atoi (arg));
	  }      
	  else if(g_ascii_strncasecmp (line, "col_visible", 11) == 0)
	  {
	      gint i = atoi (line+11);
	      prefs_set_col_visible (i, atoi (arg));
	  }      
	  else if(g_ascii_strncasecmp (line, "col_order", 9) == 0)
	  {
	      gint i = atoi (line+9);
	      prefs_set_col_order (i, atoi (arg));
	  }      
	  else if(g_ascii_strncasecmp (line, "paned_pos_", 10) == 0)
	  {
	      gint i = atoi (line+10);
	      prefs_set_paned_pos (i, atoi (arg));
	  }      
	  else if(g_ascii_strcasecmp (line, "offline") == 0)
	  {
	      prefs_set_offline((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "sort_tab_num") == 0)
	  {
	      prefs_set_sort_tab_num(atoi(arg), FALSE);
	  }
	  else if(g_ascii_strcasecmp (line, "toolbar_style") == 0)
	  {
	      prefs_set_toolbar_style(atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "pm_autostore") == 0)
	  {
	      prefs_set_pm_autostore((gboolean)atoi(arg));
	  }
	  else if((g_ascii_strcasecmp (line, "tm_autostore") == 0) ||
		  (g_ascii_strcasecmp (line, "sm_autostore") == 0))
	  {
	      prefs_set_tm_autostore((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "pm_sort") == 0)
	  {
	      prefs_set_pm_sort(atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "st_sort") == 0)
	  {
	      prefs_set_st_sort(atoi(arg));
	  }
	  else if((g_ascii_strcasecmp (line, "tm_sort_") == 0) ||
		  (g_ascii_strcasecmp (line, "sm_sort_") == 0))
	  {
	      prefs_set_tm_sort(atoi(arg));
	  }
	  else if((g_ascii_strcasecmp (line, "tm_sortcol") == 0) ||
		  (g_ascii_strcasecmp (line, "sm_sortcol") == 0))
	  {
	      prefs_set_tm_sortcol(atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "last_prefs_page") == 0)
	  {
	      prefs_set_last_prefs_page(atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "backups") == 0)
	  {
	      prefs_set_keep_backups((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "extended_info") == 0)
	  {
	      prefs_set_write_extended_info((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "dir_browse") == 0)
	  {
	      prefs_set_last_dir_browse(strdup(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "dir_export") == 0)
	  {
	      prefs_set_last_dir_export(strdup(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "show_duplicates") == 0)
	  {
	      prefs_set_show_duplicates((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "show_updated") == 0)
	  {
	      prefs_set_show_updated((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "show_non_updated") == 0)
	  {
	      prefs_set_show_non_updated((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "show_sync_dirs") == 0)
	  {
	      prefs_set_show_sync_dirs((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "sync_remove") == 0)
	  {
	      prefs_set_sync_remove((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "display_toolbar") == 0)
	  {
	      prefs_set_display_toolbar((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "display_tooltips_main") == 0)
	  {
	      prefs_set_display_tooltips_main((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "display_tooltips_prefs") == 0)
	  {
	      prefs_set_display_tooltips_prefs((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "multi_edit") == 0)
	  {
	      prefs_set_multi_edit((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "multi_edit_title") == 0)
	  {
	      prefs_set_multi_edit_title((gboolean)atoi(arg));
	  }
	  else if((g_ascii_strcasecmp (line, "not_played_track") == 0) ||
		  (g_ascii_strcasecmp (line, "not_played_song") == 0))
	  {
	      prefs_set_not_played_track((gboolean)atoi(arg));
	  }
       	  else if((g_ascii_strcasecmp (line, "misc_track_nr") == 0) ||
		  (g_ascii_strcasecmp (line, "misc_song_nr") == 0))
       	  {
       	      prefs_set_misc_track_nr(atoi(arg));
       	  }
	  else if(g_ascii_strcasecmp (line, "update_charset") == 0)
	  {
	      prefs_set_update_charset((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "write_charset") == 0)
	  {
	      prefs_set_write_charset((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "add_recursively") == 0)
	  {
	      prefs_set_add_recursively((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "case_sensitive") == 0)
	  {
	      prefs_set_case_sensitive((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "save_sorted_order") == 0)
	  {
	      /* ignore option -- has been deleted with 0.53 */
	  }
	  else if(g_ascii_strcasecmp (line, "size_gtkpod.x") == 0)
	  {
	      prefs_set_size_gtkpod (atoi (arg), -2);
	  }
	  else if(g_ascii_strcasecmp (line, "size_gtkpod.y") == 0)
	  {
	      prefs_set_size_gtkpod (-2, atoi (arg));
	  }
	  else if(g_ascii_strcasecmp (line, "size_cal.x") == 0)
	  {
	      prefs_set_size_cal (atoi (arg), -2);
	  }
	  else if(g_ascii_strcasecmp (line, "size_cal.y") == 0)
	  {
	      prefs_set_size_cal (-2, atoi (arg));
	  }
	  else if(g_ascii_strcasecmp (line, "size_conf_sw.x") == 0)
	  {
	      prefs_set_size_conf_sw (atoi (arg), -2);
	  }
	  else if(g_ascii_strcasecmp (line, "size_conf_sw.y") == 0)
	  {
	      prefs_set_size_conf_sw (-2, atoi (arg));
	  }
	  else if(g_ascii_strcasecmp (line, "size_conf.x") == 0)
	  {
	      prefs_set_size_conf (atoi (arg), -2);
	  }
	  else if(g_ascii_strcasecmp (line, "size_conf.y") == 0)
	  {
	      prefs_set_size_conf (-2, atoi (arg));
	  }
	  else if(g_ascii_strcasecmp (line, "size_dirbr.x") == 0)
	  {
	      prefs_set_size_dirbr (atoi (arg), -2);
	  }
	  else if(g_ascii_strcasecmp (line, "size_dirbr.y") == 0)
	  {
	      prefs_set_size_dirbr (-2, atoi (arg));
	  }
	  else if(g_ascii_strcasecmp (line, "size_prefs.x") == 0)
	  {
	      prefs_set_size_prefs (atoi (arg), -2);
	  }
	  else if(g_ascii_strcasecmp (line, "size_prefs.y") == 0)
	  {
	      prefs_set_size_prefs (-2, atoi (arg));
	  }
	  else if(g_ascii_strcasecmp (line, "automount") == 0)
	  {
	      prefs_set_automount (atoi (arg));
	  }
	  else if(g_ascii_strcasecmp (line, "info_window") == 0)
	  {
	      prefs_set_info_window (atoi (arg));
	  }
	  else if(g_ascii_strcasecmp (line, "write_gaintag") == 0)
	  {
	      prefs_set_write_gaintag ((gboolean)atoi(arg));
	  }                                                                
	  else if(g_ascii_strcasecmp (line, "special_export_charset") == 0)
	  {
	      prefs_set_special_export_charset ((gboolean)atoi(arg));
	  }                                                                
	  else
	  {
	      gtkpod_warning (_("Error while reading prefs: %s\n"), buf);
	  }	      
	  g_free(line);
	}
    }
}


/* we first read from /etc/gtkpod/prefs and then overwrite the
   settings with ~/.gtkpod/prefs */
void
read_prefs_defaults(void)
{
  gchar *cfgdir = NULL;
  gchar filename[PATH_MAX+1];
  FILE *fp = NULL;
  gboolean have_prefs = FALSE;

  cfgdir = prefs_get_cfgdir ();
  if (cfgdir)
  {
      snprintf(filename, PATH_MAX, "%s/prefs", cfgdir);
      filename[PATH_MAX] = 0;
      if(g_file_test(filename, G_FILE_TEST_EXISTS))
      {
	  if((fp = fopen(filename, "r")))
	  {
	      read_prefs_from_file_desc(fp);
	      fclose(fp);
	      have_prefs = TRUE; /* read prefs */
	  }
	  else
	  {
	      gtkpod_warning(_("Unable to open config file '%s' for reading\n"), filename);
	  }
      }
  }
  if (!have_prefs)
  {
      snprintf (filename, PATH_MAX, "/etc/gtkpod/prefs");
      if (g_file_test (filename, G_FILE_TEST_EXISTS))
      {
	  if((fp = fopen(filename, "r")))
	  {
	      read_prefs_from_file_desc(fp);
	      fclose(fp);
	  }
      }
  }
  C_FREE (cfgdir);
  /* handle version changes in prefs */
  if (cfg->version == 0.0)
  {
      /* most likely prefs file written by V0.50 */
      /* I added two new PANED elements since V0.50 --> shift */
      gint i;
      for (i=PANED_NUM_ST-1; i>=0; --i)
      {
	  prefs_set_paned_pos (PANED_NUM_GLADE + i,
			       prefs_get_paned_pos (PANED_NUM_GLADE + i - 2));
      }
      prefs_set_paned_pos (PANED_STATUS1, -1);
      prefs_set_paned_pos (PANED_STATUS2, -1);
  }
  /* ... */
  /* set statusbar paned to a decent value if unset */
  if (prefs_get_paned_pos (PANED_STATUS1) == -1)
  {
      gint x,y;
      prefs_get_size_gtkpod (&x, &y);
      /* set to about 2/3 of the window width */
      if (x>0)   prefs_set_paned_pos (PANED_STATUS1, 20*x/30);
  }
  /* set statusbar paned to a decent value if unset */
  if (prefs_get_paned_pos (PANED_STATUS2) == -1)
  {
      gint x,y,p;
      prefs_get_size_gtkpod (&x, &y);
      p = prefs_get_paned_pos (PANED_STATUS1);
      /* set to about half of the remaining window */
      if (x>0)   prefs_set_paned_pos (PANED_STATUS2, (x-p)/2 );
  }
}

/* Read Preferences and initialise the cfg-struct */
gboolean read_prefs (GtkWidget *gtkpod, int argc, char *argv[])
{
  GtkCheckMenuItem *menu;
  int opt;
  int option_index;
  struct option const options[] =
    {
      { "h",           no_argument,	NULL, GP_HELP },
      { "help",        no_argument,	NULL, GP_HELP },
      { "m",           required_argument,	NULL, GP_MOUNT },
      { "mountpoint",  required_argument,	NULL, GP_MOUNT },
      { "w",           no_argument,	NULL, GP_ID3_WRITE },
      { "id3_write",   no_argument,	NULL, GP_ID3_WRITE },
      { "c",           no_argument,	NULL, GP_MD5TRACKS },
      { "md5",	       no_argument,	NULL, GP_MD5TRACKS },
      { "o",           no_argument,	NULL, GP_OFFLINE },
      { "offline",     no_argument,	NULL, GP_OFFLINE },
      { "a",           no_argument,	NULL, GP_AUTO },
      { "auto",        no_argument,	NULL, GP_AUTO },
      { 0, 0, 0, 0 }
    };
  
  if (cfg != NULL) discard_prefs ();
  
  cfg = cfg_new();
  read_prefs_defaults();

  while((opt=getopt_long_only(argc, argv, "", options, &option_index)) != -1) {
    switch(opt) 
      {
      case GP_HELP:
	usage(stdout);
	exit(0);
	break;
      case GP_MOUNT:
	g_free (cfg->ipod_mount);
	cfg->ipod_mount = g_strdup (optarg);
	break;
      case GP_ID3_WRITE:
	cfg->id3_write = TRUE;
	break;
      case GP_MD5TRACKS:
	cfg->md5tracks = TRUE;
	break;
      case GP_AUTO:
	cfg->autoimport = TRUE;
	break;
      case GP_OFFLINE:
	cfg->offline = TRUE;
	break;
      default:
	usage_fpf(stderr, _("Unknown option: %s\n"), argv[optind]);
	usage(stderr);
	exit(1);
      }
  }
  menu = GTK_CHECK_MENU_ITEM (lookup_widget (gtkpod, "offline_menu"));
  gtk_check_menu_item_set_active (menu, prefs_get_offline ());
  return TRUE;
}

static void
write_prefs_to_file_desc(FILE *fp)
{
    gint i;

    if(!fp)
	fp = stderr;

    /* update column widths, x,y-size of main window and GtkPaned
     * positions */
    display_update_default_sizes ();
    /* update order of track view columns */
    tm_store_col_order ();

    fprintf(fp, "version=%s\n", VERSION);
    fprintf(fp, "mountpoint=%s\n", cfg->ipod_mount);
    fprintf(fp, "play_now_path=%s\n", cfg->play_now_path);
    fprintf(fp, "play_enqueue_path=%s\n", cfg->play_enqueue_path);
    fprintf(fp, "mp3gain_path=%s\n", cfg->mp3gain_path);
    fprintf(fp, "time_format=%s\n", cfg->time_format);
    fprintf(fp, "filename_format=%s\n", cfg->filename_format);
    if (cfg->charset)
    {
	fprintf(fp, "charset=%s\n", cfg->charset);
    } else {
	fprintf(fp, "charset=\n");
    }
    fprintf(fp, "id3=%d\n", prefs_get_id3_write ());
    fprintf(fp, "id3_all=%d\n", prefs_get_id3_writeall ());
    fprintf(fp, "md5=%d\n",prefs_get_md5tracks ());
    fprintf(fp, "update_existing=%d\n",prefs_get_update_existing ());
    fprintf(fp, "block_display=%d\n",prefs_get_block_display());
    fprintf(fp, _("# delete confirmation\n"));
    fprintf(fp, "delete_file=%d\n",prefs_get_track_playlist_deletion());
    fprintf(fp, "delete_ipod=%d\n",prefs_get_track_ipod_file_deletion());
    fprintf(fp, "sync_remove_confirm=%d\n",prefs_get_sync_remove_confirm());
    fprintf(fp, "auto_import=%d\n",prefs_get_auto_import());
    fprintf(fp, _("# sort tab: select 'All', last selected page (=category)\n"));
    for (i=0; i<SORT_TAB_MAX; ++i)
    {
	fprintf(fp, "st_autoselect%d=%d\n", i, prefs_get_st_autoselect (i));
	fprintf(fp, "st_category%d=%d\n", i, prefs_get_st_category (i));
	fprintf(fp, "sp_or%d=%d\n", i, prefs_get_sp_or (i));
	fprintf(fp, "sp_rating_cond%d=%d\n", i, prefs_get_sp_cond (i, T_RATING));
	fprintf(fp, "sp_rating_state%d=%d\n", i, prefs_get_sp_rating_state(i));
	fprintf(fp, "sp_playcount_cond%d=%d\n", i, prefs_get_sp_cond (i, T_PLAYCOUNT));
	fprintf(fp, "sp_playcount_low%d=%d\n", i, prefs_get_sp_playcount_low (i));
	fprintf(fp, "sp_playcount_high%d=%d\n", i, prefs_get_sp_playcount_high (i));
	fprintf(fp, "sp_played_cond%d=%d\n", i, prefs_get_sp_cond (i, T_TIME_PLAYED));
	fprintf(fp, "sp_played_state%d=%s\n", i, prefs_get_sp_entry (i, T_TIME_PLAYED));
	fprintf(fp, "sp_modified_cond%d=%d\n", i, prefs_get_sp_cond (i, T_TIME_MODIFIED));
	fprintf(fp, "sp_modified_state%d=%s\n", i, prefs_get_sp_entry (i, T_TIME_MODIFIED));
	fprintf(fp, "sp_autodisplay%d=%d\n", i, prefs_get_sp_autodisplay (i));
    }
    fprintf(fp, _("# autoselect master playlist?\n"));
    fprintf(fp, "mpl_autoselect=%d\n", prefs_get_mpl_autoselect ());
    fprintf(fp, _("# title=0, artist, album, genre, composer\n"));
    fprintf(fp, _("# track_nr=5, ipod_id, pc_path, transferred\n"));
    fprintf(fp, _("# autoset: set empty tag to filename?\n"));
    for (i=0; i<TM_NUM_COLUMNS; ++i)
    {
	fprintf(fp, "tm_col_width%d=%d\n", i, prefs_get_tm_col_width (i));
	fprintf(fp, "col_visible%d=%d\n",  i, prefs_get_col_visible (i));
	fprintf(fp, "col_order%d=%d\n",  i, prefs_get_col_order (i));
	if (i < TM_NUM_TAGS_PREFS)
	    fprintf(fp, "tag_autoset%d=%d\n", i, prefs_get_tag_autoset (i));
    }	
    fprintf(fp, _("# position of sliders (paned): playlists, above tracks,\n# between sort tabs, and in statusbar.\n"));
    for (i=0; i<PANED_NUM; ++i)
    {
	fprintf(fp, "paned_pos_%d=%d\n", i, prefs_get_paned_pos (i));
    }
    fprintf(fp, "sort_tab_num=%d\n",prefs_get_sort_tab_num());
    fprintf(fp, "last_prefs_page=%d\n",prefs_get_last_prefs_page());
    fprintf(fp, "offline=%d\n",prefs_get_offline());
    fprintf(fp, "backups=%d\n",prefs_get_keep_backups());
    fprintf(fp, "extended_info=%d\n",prefs_get_write_extended_info());
    fprintf(fp, "dir_browse=%s\n",cfg->last_dir.browse);
    fprintf(fp, "dir_export=%s\n",cfg->last_dir.export);
    fprintf(fp, "show_duplicates=%d\n",prefs_get_show_duplicates());
    fprintf(fp, "show_updated=%d\n",prefs_get_show_updated());
    fprintf(fp, "show_non_updated=%d\n",prefs_get_show_non_updated());
    fprintf(fp, "show_sync_dirs=%d\n",prefs_get_show_sync_dirs());
    fprintf(fp, "sync_remove=%d\n",prefs_get_sync_remove());
    fprintf(fp, "display_toolbar=%d\n",prefs_get_display_toolbar());
    fprintf(fp, "toolbar_style=%d\n",prefs_get_toolbar_style());
    fprintf(fp, "pm_autostore=%d\n",prefs_get_pm_autostore());
    fprintf(fp, "tm_autostore=%d\n",prefs_get_tm_autostore());
    fprintf(fp, "pm_sort=%d\n",prefs_get_pm_sort());
    fprintf(fp, "st_sort=%d\n",prefs_get_st_sort());
    fprintf(fp, "tm_sort_=%d\n",prefs_get_tm_sort());
    fprintf(fp, "tm_sortcol=%d\n",prefs_get_tm_sortcol());
    fprintf(fp, "display_tooltips_main=%d\n",
	    prefs_get_display_tooltips_main());
    fprintf(fp, "display_tooltips_prefs=%d\n",
	    prefs_get_display_tooltips_prefs());
    fprintf(fp, "multi_edit=%d\n", prefs_get_multi_edit());
    fprintf(fp, "multi_edit_title=%d\n", prefs_get_multi_edit_title());
    fprintf(fp, "misc_track_nr=%d\n", prefs_get_misc_track_nr());
    fprintf(fp, "not_played_track=%d\n", prefs_get_not_played_track());
    fprintf(fp, "update_charset=%d\n",prefs_get_update_charset());
    fprintf(fp, "write_charset=%d\n",prefs_get_write_charset());
    fprintf(fp, "add_recursively=%d\n",prefs_get_add_recursively());
    fprintf(fp, "case_sensitive=%d\n",prefs_get_case_sensitive());
    fprintf(fp, _("# window sizes: main window, confirmation scrolled,\n#               confirmation non-scrolled, dirbrowser, prefs\n"));
    fprintf (fp, "size_gtkpod.x=%d\n", cfg->size_gtkpod.x);
    fprintf (fp, "size_gtkpod.y=%d\n", cfg->size_gtkpod.y);
    fprintf (fp, "size_cal.x=%d\n", cfg->size_cal.x);
    fprintf (fp, "size_cal.y=%d\n", cfg->size_cal.y);
    fprintf (fp, "size_conf_sw.x=%d\n", cfg->size_conf_sw.x);
    fprintf (fp, "size_conf_sw.y=%d\n", cfg->size_conf_sw.y);
    fprintf (fp, "size_conf.x=%d\n", cfg->size_conf.x);
    fprintf (fp, "size_conf.y=%d\n", cfg->size_conf.y);
    fprintf (fp, "size_dirbr.x=%d\n", cfg->size_dirbr.x);
    fprintf (fp, "size_dirbr.y=%d\n", cfg->size_dirbr.y);
    fprintf (fp, "size_prefs.x=%d\n", cfg->size_prefs.x);
    fprintf (fp, "size_prefs.y=%d\n", cfg->size_prefs.y);
    fprintf (fp, "automount=%d\n", cfg->automount);
    fprintf (fp, "info_window=%d\n", cfg->info_window);
    fprintf (fp, "write_gaintag=%d\n", cfg->write_gaintag);
    fprintf (fp, "special_export_charset=%d\n", cfg->special_export_charset);
}

void 
write_prefs (void)
{
    gchar filename[PATH_MAX+1];
    gchar *cfgdir;
    FILE *fp = NULL;

    cfgdir = prefs_get_cfgdir ();
    if(!cfgdir)
      {
	gtkpod_warning (_("Settings are not saved.\n"));
      }
    else
      {
	snprintf(filename, PATH_MAX, "%s/prefs", cfgdir);
	filename[PATH_MAX] = 0;
	if((fp = fopen(filename, "w")))
	  {
	    write_prefs_to_file_desc(fp);
	    fclose(fp);
	  }
	else
	  {
	    gtkpod_warning (_("Unable to open '%s' for writing\n"),
			    filename);
	  }
      }
    C_FREE (cfgdir);
}



/* Free all memory including the cfg-struct itself. */
void discard_prefs ()
{
    cfg_free(cfg);
    cfg = NULL;
}

void cfg_free(struct cfg *c)
{
    if(c)
    {
      g_free (c->ipod_mount);
      g_free (c->charset);
      g_free (c->last_dir.browse);
      g_free (c->last_dir.export);
      g_free (c->play_now_path);
      g_free (c->play_enqueue_path);
      g_free (c->mp3gain_path);
      g_free (c->time_format);
      g_free (c->filename_format);
      g_free (c);
    }
}

void sortcfg_free(struct sortcfg *c)
{
    g_free (c);
}


static gchar *
get_dirname_of_filename(const gchar *file)
{
    gint len;
    gchar *buf, *result = NULL;

    if (!file) return NULL;

    if (g_file_test(file, G_FILE_TEST_IS_DIR))
	buf = g_strdup (file);
    else
	buf = g_path_get_dirname (file);
	
    len = strlen (buf);
    if (len && (buf[len-1] == '/'))	result = buf;
    else
    {
	result = g_strdup_printf ("%s/", buf);
	g_free (buf);
    }
    return result;
}


void prefs_set_offline(gboolean active)
{
  cfg->offline = active;
}
void prefs_set_keep_backups(gboolean active)
{
  cfg->keep_backups = active;
}
void prefs_set_write_extended_info(gboolean active)
{
  cfg->write_extended_info = active;
}
void prefs_set_last_dir_browse(const gchar *file)
{
    if (file)
    {
	C_FREE(cfg->last_dir.browse);
	cfg->last_dir.browse = get_dirname_of_filename(file);
    }
}
const gchar *prefs_get_last_dir_browse(void)
{
    return cfg->last_dir.browse;
}

void prefs_set_last_dir_export(const gchar *file)
{
    if (file)
    {
	g_free(cfg->last_dir.export);
	cfg->last_dir.export = get_dirname_of_filename(file);
    }
}
const char *prefs_get_last_dir_export(void)
{
    return cfg->last_dir.export;
}

void prefs_set_mount_point(const gchar *mp)
{
    if(cfg->ipod_mount) g_free(cfg->ipod_mount);
    /* if new mount point starts with "~/", we replace it with the
       home directory */
    if (strncmp ("~/", mp, 2) == 0)
      cfg->ipod_mount = g_build_filename (g_get_home_dir (), mp+2, NULL);
    else cfg->ipod_mount = g_strdup(mp);
}


/* If the status of md5 hash flag changes, free or re-init the md5
   hash table */
void prefs_set_md5tracks(gboolean active)
{
    if (cfg->md5tracks && !active)
    { /* md5 checksums have been turned off */
	cfg->md5tracks = active;
	md5_unique_file_free ();
    }
    if (!cfg->md5tracks && active)
    { /* md5 checksums have been turned on */
	cfg->md5tracks = active; /* must be set before calling
				   hash_tracks() */
	hash_tracks ();
    }
}

gboolean prefs_get_md5tracks(void)
{
    return cfg->md5tracks;
}

void prefs_set_update_existing(gboolean active)
{
    cfg->update_existing = active;
}

gboolean prefs_get_update_existing(void)
{
    return cfg->update_existing;
}

/* Should the display be blocked (be insenstive) while it is updated
   after a selection (playlist, tab entry) change? */
void prefs_set_block_display(gboolean active)
{
    cfg->block_display = active;
}

gboolean prefs_get_block_display(void)
{
    return cfg->block_display;
}

void prefs_set_id3_write(gboolean active)
{
    cfg->id3_write = active;
}

gboolean prefs_get_id3_write(void)
{
    return cfg->id3_write;
}

void prefs_set_id3_writeall(gboolean active)
{
    cfg->id3_writeall = active;
}

gboolean prefs_get_id3_writeall(void)
{
    return cfg->id3_writeall;
}

gboolean prefs_get_offline(void)
{
  return cfg->offline;
}

gboolean prefs_get_keep_backups(void)
{
  return cfg->keep_backups;
}

gboolean prefs_get_write_extended_info(void)
{
  return cfg->write_extended_info;
}

void prefs_set_track_playlist_deletion(gboolean val)
{
    cfg->deletion.track = val;
}

gboolean prefs_get_track_playlist_deletion(void)
{
    return(cfg->deletion.track);
}

void prefs_set_track_ipod_file_deletion(gboolean val)
{
    cfg->deletion.ipod_file = val;
}

gboolean prefs_get_track_ipod_file_deletion(void)
{
    return(cfg->deletion.ipod_file);
}

void prefs_set_sync_remove_confirm(gboolean val)
{
    cfg->deletion.syncing = val;
}

gboolean prefs_get_sync_remove_confirm(void)
{
    return(cfg->deletion.syncing);
}

void prefs_set_charset (gchar *charset)
{
    prefs_cfg_set_charset (cfg, charset);
}


void prefs_cfg_set_charset (struct cfg *cfgd, gchar *charset)
{
    C_FREE (cfgd->charset);
    if (charset && strlen (charset))
	cfgd->charset = g_strdup (charset);
/*     printf ("set_charset: '%s'\n", charset);	 */
}


gchar *prefs_get_charset (void)
{
    return cfg->charset;
/*     printf ("get_charset: '%s'\n", cfg->charset); */
}

struct cfg *clone_prefs(void)
{
    struct cfg *result = NULL;

    if(cfg)
    {
	result = g_memdup (cfg, sizeof (struct cfg));
	if (cfg->ipod_mount)
	    result->ipod_mount = g_strdup(cfg->ipod_mount);
	if (cfg->charset)
	    result->charset = g_strdup(cfg->charset);
	if(cfg->last_dir.browse)
	    result->last_dir.browse = g_strdup(cfg->last_dir.browse);
	if(cfg->last_dir.browse)
	    result->last_dir.browse = g_strdup(cfg->last_dir.browse);
	if(cfg->last_dir.export)
	    result->last_dir.export = g_strdup(cfg->last_dir.export);
	if(cfg->play_now_path)
	    result->play_now_path = g_strdup(cfg->play_now_path);
	if(cfg->play_enqueue_path)
	    result->play_enqueue_path = g_strdup(cfg->play_enqueue_path);
	if(cfg->mp3gain_path)
	    result->mp3gain_path = g_strdup(cfg->mp3gain_path);
	if(cfg->time_format)
	    result->time_format = g_strdup(cfg->time_format);
	if (cfg->filename_format)
	    result->filename_format = g_strdup(cfg->filename_format);
    }
    return(result);
}

struct sortcfg *clone_sortprefs(void)
{
    struct sortcfg *result = NULL;

    if(cfg)
    {
	result = g_memdup (&cfg->sortcfg, sizeof (struct sortcfg));
    }
    return(result);
}

gboolean prefs_get_auto_import(void)
{
    return(cfg->autoimport);
}

void prefs_set_auto_import(gboolean val)
{
    cfg->autoimport = val;
}

/* "inst": the instance of the sort tab */
gboolean prefs_get_st_autoselect (guint32 inst)
{
    if (inst < SORT_TAB_MAX)
    {
	return cfg->st[inst].autoselect;
    }
    else
    {
	return TRUE;
    }
}

/* "inst": the instance of the sort tab, "autoselect": should "All" be
 * selected automatically? */
void prefs_set_st_autoselect (guint32 inst, gboolean autoselect)
{
    if (inst < SORT_TAB_MAX)
    {
	cfg->st[inst].autoselect = autoselect;
    }
}

/* "inst": the instance of the sort tab */
guint prefs_get_st_category (guint32 inst)
{
    if (inst < SORT_TAB_MAX)
    {
	return cfg->st[inst].category;
    }
    else
    {
	return TRUE; /* hmm.... this should not happen... */
    }
}

/* "inst": the instance of the sort tab, "category": one of the
   ST_CAT_... */
void prefs_set_st_category (guint32 inst, guint category)
{
    if ((inst < SORT_TAB_MAX) && (category < ST_CAT_NUM))
    {
	cfg->st[inst].category = category;
    }
    else
    {
	gtkpod_warning (_(" Preferences: Category nr (%d<%d?) or sorttab nr (%d<%d?) out of range.\n"), category, ST_CAT_NUM, inst, SORT_TAB_MAX);
    }
}

gboolean prefs_get_mpl_autoselect (void)
{
    return cfg->mpl_autoselect;
}

/* Should the MPL be selected automatically? */
void prefs_set_mpl_autoselect (gboolean autoselect)
{
    cfg->mpl_autoselect = autoselect;
}


/* retrieve the width of the track display columns. "col": one of the
   TM_COLUMN_... */
gint prefs_get_tm_col_width (gint col)
{
    if (col < TM_NUM_COLUMNS && (cfg->tm_col_width[col] > 0))
	return cfg->tm_col_width[col];
    return 80;  /* default -- col should be smaller than
		   TM_NUM_COLUMNS) */
}

/* set the width of the track display columns. "col": one of the
   TM_COLUMN_..., "width": current width */
void prefs_set_tm_col_width (gint col, gint width)
{
    if (col < TM_NUM_COLUMNS && width > 0)
	cfg->tm_col_width[col] = width;
}


/* Returns "$HOME/.gtkpod" or NULL if dir does not exist and cannot be
   created. You must g_free the string after use */
gchar *prefs_get_cfgdir (void)
{
  G_CONST_RETURN gchar *str;
  gchar *cfgdir=NULL;

  if((str = g_get_home_dir ()))
    {
      cfgdir = g_build_filename (str, ".gtkpod", NULL);
      if(!g_file_test(cfgdir, G_FILE_TEST_IS_DIR))
	{
	  if(mkdir(cfgdir, 0755) == -1)
	    {
	      gtkpod_warning(_("Unable to 'mkdir %s'\n"), cfgdir);
	      C_FREE (cfgdir); /*defined in misc.h*/
	    }
	}
    }
  return cfgdir;
}

/* Returns the ipod_mount. Don't modify it! */
const gchar *prefs_get_ipod_mount (void)
{
    return cfg->ipod_mount;
}

/* Sets the default size for the gtkpod window. -2 means: don't change
 * the current size */
void prefs_set_size_gtkpod (gint x, gint y)
{
    if (x != -2) cfg->size_gtkpod.x = x;
    if (y != -2) cfg->size_gtkpod.y = y;
}

/* Sets the default size for the calendar window. -2 means: don't change
 * the current size */
void prefs_set_size_cal (gint x, gint y)
{
    if (x != -2) cfg->size_cal.x = x;
    if (y != -2) cfg->size_cal.y = y;
}

/* Sets the default size for the scrolled conf window. -2 means: don't
 * change the current size */
void prefs_set_size_conf_sw (gint x, gint y)
{
    if (x != -2) cfg->size_conf_sw.x = x;
    if (y != -2) cfg->size_conf_sw.y = y;
}

/* Sets the default size for the non-scrolled conf window. -2 means:
 * don't change the current size */
void prefs_set_size_conf (gint x, gint y)
{
    if (x != -2) cfg->size_conf.x = x;
    if (y != -2) cfg->size_conf.y = y;
}

/* Sets the default size for the dirbrowser window. -2 means:
 * don't change the current size */
void prefs_set_size_dirbr (gint x, gint y)
{
    if (x != -2) cfg->size_dirbr.x = x;
    if (y != -2) cfg->size_dirbr.y = y;
}

/* Sets the default size for the prefs window. -2 means:
 * don't change the current size */
void prefs_set_size_prefs (gint x, gint y)
{
    if (x != -2) cfg->size_prefs.x = x;
    if (y != -2) cfg->size_prefs.y = y;
}

/* Writes the current default size for the gtkpod window in "x" and
   "y" */
void prefs_get_size_gtkpod (gint *x, gint *y)
{
    *x = cfg->size_gtkpod.x;
    *y = cfg->size_gtkpod.y;
}

/* Writes the current default size for the gtkpod window in "x" and
   "y" */
void prefs_get_size_cal (gint *x, gint *y)
{
    *x = cfg->size_cal.x;
    *y = cfg->size_cal.y;
}

/* Writes the current default size for the scrolled conf window in "x"
   and "y" */
void prefs_get_size_conf_sw (gint *x, gint *y)
{
    *x = cfg->size_conf_sw.x;
    *y = cfg->size_conf_sw.y;
}

/* Writes the current default size for the non-scrolled conf window in
   "x" and "y" */
void prefs_get_size_conf (gint *x, gint *y)
{
    *x = cfg->size_conf.x;
    *y = cfg->size_conf.y;
}


/* Writes the current default size for the dirbrowser window in
   "x" and "y" */
void prefs_get_size_dirbr (gint *x, gint *y)
{
    *x = cfg->size_dirbr.x;
    *y = cfg->size_dirbr.y;
}


/* Writes the current default size for the prefs window in
   "x" and "y" */
void prefs_get_size_prefs (gint *x, gint *y)
{
    *x = cfg->size_prefs.x;
    *y = cfg->size_prefs.y;
}


/* Should empty tags be set to filename? -- "category": one of the
   TM_COLUMN_..., "autoset": new value */
void prefs_set_tag_autoset (gint category, gboolean autoset)
{
    if (category < TM_NUM_TAGS_PREFS)
	cfg->tag_autoset[category] = autoset;
}


/* Should empty tags be set to filename? -- "category": one of the
   TM_COLUMN_... */
gboolean prefs_get_tag_autoset (gint category)
{
    if (category < TM_NUM_TAGS_PREFS)
	return cfg->tag_autoset[category];
    return FALSE;
}

/* Display column tm_item @visible: new value */
void prefs_set_col_visible (TM_item tm_item, gboolean visible)
{
    if (tm_item < TM_NUM_COLUMNS)
	cfg->col_visible[tm_item] = visible;
}


/* Display column tm_item? */
gboolean prefs_get_col_visible (TM_item tm_item)
{
    if (tm_item < TM_NUM_COLUMNS)
	return cfg->col_visible[tm_item];
    return FALSE;
}

/* Display which column at nr @pos? */
void prefs_set_col_order (gint pos, TM_item tm_item)
{
    if (pos < TM_NUM_COLUMNS)
	cfg->col_order[pos] = tm_item;
}


/* Display column nr @pos? */
TM_item prefs_get_col_order (gint pos)
{
    if (pos < TM_NUM_COLUMNS)
	return cfg->col_order[pos];
    return -1;
}

/* get position of GtkPaned element nr. "i" */
/* return value: -1: don't change position */
gint prefs_get_paned_pos (gint i)
{
    if (i < PANED_NUM)
	return cfg->paned_pos[i];
    else
    {
	g_warning ("Programming error: prefs_get_paned_pos: arg out of range (%d)\n", i);
	return 100; /* something reasonable? */
    }
}

/* set position of GtkPaned element nr. "i" */
void prefs_set_paned_pos (gint i, gint pos)
{
    if (i < PANED_NUM)
	cfg->paned_pos[i] = pos;
}


/* A value of "0" will set the default defined in misc.c */
void prefs_set_statusbar_timeout (guint32 val)
{
    if (val == 0)  val = STATUSBAR_TIMEOUT;
    cfg->statusbar_timeout = val;
}

guint32 prefs_get_statusbar_timeout (void)
{
    return cfg->statusbar_timeout;
}

gboolean prefs_get_show_duplicates (void)
{
    return cfg->show_duplicates;
}

void prefs_set_show_duplicates (gboolean val)
{
    cfg->show_duplicates = val;
}

gboolean prefs_get_show_updated (void)
{
    return cfg->show_updated;
}

void prefs_set_show_updated (gboolean val)
{
    cfg->show_updated = val;
}

gboolean prefs_get_show_non_updated (void)
{
    return cfg->show_non_updated;
}

void prefs_set_show_non_updated (gboolean val)
{
    cfg->show_non_updated = val;
}

gboolean prefs_get_show_sync_dirs (void)
{
    return cfg->show_sync_dirs;
}

void prefs_set_show_sync_dirs (gboolean val)
{
    cfg->show_sync_dirs = val;
}

gboolean prefs_get_sync_remove (void)
{
    return cfg->sync_remove;
}

void prefs_set_sync_remove (gboolean val)
{
    cfg->sync_remove = val;
}

gboolean prefs_get_display_toolbar (void)
{
    return cfg->display_toolbar;
}

void prefs_set_display_toolbar (gboolean val)
{
    cfg->display_toolbar = val;
    display_show_hide_toolbar ();
}

gboolean prefs_get_update_charset (void)
{
    return cfg->update_charset;
}

void prefs_set_update_charset (gboolean val)
{
    cfg->update_charset = val;
}

gboolean prefs_get_write_charset (void)
{
    return cfg->write_charset;
}

void prefs_set_write_charset (gboolean val)
{
    cfg->write_charset = val;
}

gboolean prefs_get_add_recursively (void)
{
    return cfg->add_recursively;
}

void prefs_set_add_recursively (gboolean val)
{
    cfg->add_recursively = val;
}

gboolean prefs_get_case_sensitive (void)
{
    return cfg->sortcfg.case_sensitive;
}

void prefs_set_case_sensitive (gboolean val)
{
    cfg->sortcfg.case_sensitive = val;
}

gint prefs_get_sort_tab_num (void)
{
    return cfg->sort_tab_num;
}

void prefs_set_sort_tab_num (gint i, gboolean update_display)
{
    if ((i>=0) && (i<=SORT_TAB_MAX))
    {
	if (cfg->sort_tab_num != i)
	{
	    cfg->sort_tab_num = i;
	    if (update_display) st_show_visible ();
	}
    }
}

GtkToolbarStyle prefs_get_toolbar_style (void)
{
    return cfg->toolbar_style;
}

void prefs_set_toolbar_style (GtkToolbarStyle i)
{
    switch (i)
    {
    case GTK_TOOLBAR_ICONS:
    case GTK_TOOLBAR_TEXT:
    case GTK_TOOLBAR_BOTH:
	break;
    case GTK_TOOLBAR_BOTH_HORIZ:
	i = GTK_TOOLBAR_BOTH;
	break;
    default:  /* illegal -- ignore */
	gtkpod_warning (_("prefs_set_toolbar_style: illegal style '%d' ignored\n"), i);
	return;
    }

    cfg->toolbar_style = i;
    display_show_hide_toolbar ();
}

gboolean prefs_get_pm_autostore (void)
{
    return cfg->sortcfg.pm_autostore;
}

void prefs_set_pm_autostore (gboolean val)
{
    cfg->sortcfg.pm_autostore = val;
}

gboolean prefs_get_tm_autostore (void)
{
    return cfg->sortcfg.tm_autostore;
}

void prefs_set_tm_autostore (gboolean val)
{
    cfg->sortcfg.tm_autostore = val;
}

gint prefs_get_pm_sort (void)
{
    return cfg->sortcfg.pm_sort;
}

void prefs_set_pm_sort (gint i)
{
    switch (i)
    {
    case SORT_ASCENDING:
    case SORT_DESCENDING:
    case SORT_NONE:
	break;
    default:  /* illegal -- ignore */
	gtkpod_warning (_("prefs_set_pm_sort: illegal type '%d' ignored\n"), i);
	return;
    }

    cfg->sortcfg.pm_sort = i;
}

gint prefs_get_st_sort (void)
{
    return cfg->sortcfg.st_sort;
}

void prefs_set_st_sort (gint i)
{
    switch (i)
    {
    case SORT_ASCENDING:
    case SORT_DESCENDING:
    case SORT_NONE:
	break;
    default:  /* illegal -- ignore */
	gtkpod_warning (_("prefs_set_st_sort: illegal type '%d' ignored\n"), i);
	return;
    }

    cfg->sortcfg.st_sort = i;
}

gint prefs_get_tm_sort (void)
{
    return cfg->sortcfg.tm_sort;
}

void prefs_set_tm_sort (gint i)
{
    switch (i)
    {
    case SORT_ASCENDING:
    case SORT_DESCENDING:
    case SORT_NONE:
	break;
    default:  /* illegal -- ignore */
	gtkpod_warning (_("prefs_set_tm_sort: illegal type '%d' ignored\n"), i);
	return;
    }

    cfg->sortcfg.tm_sort = i;
}

TM_item prefs_get_tm_sortcol (void)
{
    return cfg->sortcfg.tm_sortcol;
}

void prefs_set_tm_sortcol (TM_item i)
{
    if (i < TM_NUM_COLUMNS)
	cfg->sortcfg.tm_sortcol = i;
}

void prefs_set_display_tooltips_main (gboolean state)
{
    cfg->display_tooltips_main = state;
    display_show_hide_tooltips ();
}

gboolean prefs_get_display_tooltips_main (void)
{
    return cfg->display_tooltips_main;
}

void prefs_set_display_tooltips_prefs (gboolean state)
{
    cfg->display_tooltips_prefs = state;
    display_show_hide_tooltips ();
}

gboolean prefs_get_display_tooltips_prefs (void)
{
    return cfg->display_tooltips_prefs;
}

void prefs_set_multi_edit (gboolean state)
{
    cfg->multi_edit = state;
}

gboolean prefs_get_multi_edit (void)
{
    return cfg->multi_edit;
}

void prefs_set_not_played_track (gboolean state)
{
    cfg->not_played_track = state;
}

gboolean prefs_get_not_played_track (void)
{
    return cfg->not_played_track;
}

void prefs_set_misc_track_nr (gint state)
{
    cfg->misc_track_nr = state;
}

gint prefs_get_misc_track_nr (void)
{
    return cfg->misc_track_nr;
}

void prefs_set_multi_edit_title (gboolean state)
{
    cfg->multi_edit_title = state;
}

gboolean prefs_get_multi_edit_title (void)
{
    return cfg->multi_edit_title;
}

gint prefs_get_last_prefs_page (void)
{
    return cfg->last_prefs_page;
}

void prefs_set_last_prefs_page (gint i)
{
    cfg->last_prefs_page = i;
}

/* validate the the play_path @path and return a valid copy that has
 * to be freed with g_free when it's not needed any more. */
/* Rules: - only one '%'
          - must be '%s'
          - removes all invalid '%' */
gchar *prefs_validate_play_path (const gchar *path)
{
    const gchar *pp;
    gchar *npp, *npath=NULL;
    gint num;

    if ((!path) || (strlen (path) == 0)) return g_strdup ("");

    npath = g_malloc0 (strlen (path)+1); /* new path can only be shorter
					    than old path */
    pp = path;
    npp = npath;
    num = 0; /* number of '%' */
    while (*pp)
    {
	if (*pp == '%')
	{
	    if (num != 0)
	    {
		gtkpod_warning (_("'%s': only one '%%s' allowed.\n"), path);
		++pp; /* skip '%.' */
	    }
	    else
	    {
		if (*(pp+1) != 's')
		{
		    gtkpod_warning (_("'%s': only one '%%s' allowed.\n"), path);
		    ++pp; /* skip '%s' */
		}
		else
		{   /* copy '%s' */
		    *npp++ = *pp++;
		    *npp++ = *pp;
		}
	    }
	    ++num;
	}
	else
	{
	    *npp++ = *pp;
	}
	if (*pp) ++pp; /* increment if we are not at the end */
    }
    return npath;
}


void prefs_set_play_now_path (const gchar *path)
{
    C_FREE (cfg->play_now_path);
    cfg->play_now_path = prefs_validate_play_path (path);
}

const gchar *prefs_get_play_now_path (void)
{
    return cfg->play_now_path;
}

void prefs_set_play_enqueue_path (const gchar *path)
{
    C_FREE (cfg->play_enqueue_path);
    cfg->play_enqueue_path = prefs_validate_play_path (path);
}

const gchar *prefs_get_play_enqueue_path (void)
{
    return cfg->play_enqueue_path;
}

void prefs_set_mp3gain_path (const gchar *path)
{
    C_FREE (cfg->mp3gain_path);
    if (path)
    {
	cfg->mp3gain_path = g_strstrip (g_strdup (path));
    }
    else
    {
	cfg->mp3gain_path = g_strdup ("");
    }
}

const gchar *prefs_get_mp3gain_path (void)
{
    return cfg->mp3gain_path;
}

void prefs_set_time_format (const gchar *format)
{
    if (format)
    {
	C_FREE (cfg->time_format);
	cfg->time_format = g_strdup (format);
    }
}

gchar *prefs_get_time_format (void)
{
    return cfg->time_format;
}

gboolean 
prefs_get_automount (void)
{
    return cfg->automount;
}
void

prefs_set_automount(gboolean val)
{
    cfg->automount = val;
}

gboolean 
prefs_get_info_window(void)
{
    return cfg->info_window;
}
void

prefs_set_info_window(gboolean val)
{
    cfg->info_window = val;
}

void prefs_set_sp_or (guint32 inst, gboolean state)
{
    if (inst < SORT_TAB_MAX)	cfg->st[inst].sp_or = state;
}

gboolean prefs_get_sp_or (guint32 inst)
{
    if (inst < SORT_TAB_MAX)	return cfg->st[inst].sp_or;
    return FALSE;
}

/* Set whether condition @t_item in sort tab @inst is activated or not */
void prefs_set_sp_cond (guint32 inst, T_item t_item, gboolean state)
{
    if (inst < SORT_TAB_MAX)
    {
	switch (t_item)
	{
	case T_RATING:
	    cfg->st[inst].sp_rating = state;
	    break;
	case T_PLAYCOUNT:
	    cfg->st[inst].sp_playcount = state;
	    break;
	case T_TIME_PLAYED:
	    cfg->st[inst].sp_played = state;
	    break;
	case T_TIME_MODIFIED:
	    cfg->st[inst].sp_modified = state;
	    break;
	default:
	    /* programming error */
	    fprintf (stderr, "prefs_set_sp_cond(): inst=%d, !t_item=%d!\n",
		     inst, t_item);
	    break;
	}
    }
    else
    {
	/* programming error */
	fprintf (stderr, "prefs_set_sp_cond(): !inst=%d! t_item=%d\n",
		 inst, t_item);
    }
}


/* Set whether condition @t_item in sort tab @inst is activated or not */
gboolean prefs_get_sp_cond (guint32 inst, T_item t_item)
{
    if (inst < SORT_TAB_MAX)
    {
	switch (t_item)
	{
	case T_RATING:
	    return cfg->st[inst].sp_rating;
	case T_PLAYCOUNT:
	    return cfg->st[inst].sp_playcount;
	case T_TIME_PLAYED:
	    return cfg->st[inst].sp_played;
	case T_TIME_MODIFIED:
	    return cfg->st[inst].sp_modified;
	default:
	    /* programming error */
	    fprintf (stderr, "prefs_get_sp_cond(): inst=%d !t_item=%d!\n",
		     inst, t_item);
	    break;
	}
    }
    else
    {
	/* programming error */
	fprintf (stderr, "prefs_get_sp_cond(): !inst=%d! t_item=%d\n",
		 inst, t_item);
    }
    return FALSE;
}


void prefs_set_sp_rating_n (guint32 inst, gint n, gboolean state)
{
    if ((inst < SORT_TAB_MAX) && (n <=RATING_MAX))
    {
	if (state)
	    cfg->st[inst].sp_rating_state |= (1<<n);
	else
	    cfg->st[inst].sp_rating_state &= ~(1<<n);
    }
    else
	fprintf (stderr, "prefs_set_sp_rating_n(): inst=%d, n=%d\n", inst, n);
}


gboolean prefs_get_sp_rating_n (guint32 inst, gint n)
{
    if ((inst < SORT_TAB_MAX) && (n <=RATING_MAX))
    {
	if ((cfg->st[inst].sp_rating_state & (1<<n)) != 0)
	    return TRUE;
	else
	    return FALSE;
    }
    fprintf (stderr, "prefs_get_sp_rating_n(): inst=%d, n=%d\n", inst, n);
    return FALSE;
}


void prefs_set_sp_rating_state (guint32 inst, guint32 state)
{
    if (inst < SORT_TAB_MAX)
	/* only keep the 'RATING_MAX+1' lowest bits */
        cfg->st[inst].sp_rating_state = (state & ((1<<(RATING_MAX+1))-1));
    else
        fprintf (stderr, "prefs_set_sp_rating_state(): inst=%d\n", inst);
}


guint32 prefs_get_sp_rating_state (guint32 inst)
{
    if (inst < SORT_TAB_MAX)
        return cfg->st[inst].sp_rating_state;
    fprintf (stderr, "prefs_get_sp_rating_state(): inst=%d\n", inst);
    return 0;
}


void prefs_set_sp_entry (guint32 inst, T_item t_item, const gchar *str)
{
/*    printf("psse: %d, %d, %s\n", inst, t_item, str);*/
    if (inst < SORT_TAB_MAX)
    {
	gchar *cstr = NULL;

	if (str)  cstr = g_strdup (str);

	switch (t_item)
	{
	case T_TIME_PLAYED:
	    g_free (cfg->st[inst].sp_played_state);
	    cfg->st[inst].sp_played_state = cstr;
	    break;
	case T_TIME_MODIFIED:
	    g_free (cfg->st[inst].sp_modified_state);
	    cfg->st[inst].sp_modified_state = cstr;
	    break;
	default:
	    /* programming error */
	    g_free (cstr);
	    fprintf (stderr, "prefs_set_sp_entry(): inst=%d !t_item=%d!\n",
		     inst, t_item);
	    break;
	}
    }
    else
    {
	/* programming error */
	fprintf (stderr, "prefs_set_sp_entry(): !inst=%d! t_item=%d\n",
		 inst, t_item);
    }
}


/* Returns the current default or a pointer to "". Guaranteed to never
   return NULL */
gchar *prefs_get_sp_entry (guint32 inst, T_item t_item)
{
    gchar *result = NULL;

    if (inst < SORT_TAB_MAX)
    {
	switch (t_item)
	{
	case T_TIME_PLAYED:
	    result = cfg->st[inst].sp_played_state;
	    break;
	case T_TIME_MODIFIED:
	    result = cfg->st[inst].sp_modified_state;
	    break;
	default:
	    /* programming error */
	    fprintf (stderr, "prefs_get_sp_entry(): !t_item=%d!\n", t_item);
	    break;
	}
    }
    else
    {
	/* programming error */
	fprintf (stderr, "prefs_get_sp_entry(): !inst=%d!\n", inst);
    }
    if (result == NULL)    result = "";
    return result;
}


void prefs_set_sp_autodisplay (guint32 inst, gboolean state)
{
    if (inst < SORT_TAB_MAX)
	cfg->st[inst].sp_autodisplay = state;
    else
	fprintf (stderr, "prefs_set_sp_autodisplay(): !inst=%d!\n", inst);
}


gboolean prefs_get_sp_autodisplay (guint32 inst)
{
    if (inst < SORT_TAB_MAX)
	return cfg->st[inst].sp_autodisplay;
    else
	fprintf (stderr, "prefs_get_sp_autodisplay(): !inst=%d!\n", inst);
    return FALSE;
}


gint32 prefs_get_sp_playcount_low (guint32 inst)
{
    if (inst < SORT_TAB_MAX)
	return cfg->st[inst].sp_playcount_low;
    else
	fprintf (stderr, "prefs_get_sp_playcount_low(): !inst=%d!\n", inst);
    return 0;
}


gint32 prefs_get_sp_playcount_high (guint32 inst)
{
    if (inst < SORT_TAB_MAX)
	return cfg->st[inst].sp_playcount_high;
    else
	fprintf (stderr, "prefs_get_sp_playcount_high(): !inst=%d!\n", inst);
    return (guint32)-1;
}


void prefs_set_sp_playcount_low (guint32 inst, gint32 limit)
{
    if (inst < SORT_TAB_MAX)
	cfg->st[inst].sp_playcount_low = limit;
    else
	fprintf (stderr, "prefs_set_sp_playcount_low(): !inst=%d!\n", inst);
}


void prefs_set_sp_playcount_high (guint32 inst, gint32 limit)
{
    if (inst < SORT_TAB_MAX)
	cfg->st[inst].sp_playcount_high = limit;
    else
	fprintf (stderr, "prefs_set_sp_playcount_high(): !inst=%d!\n", inst);
}

char* prefs_get_filename_format(void)
{
    return(cfg->filename_format);
}

void prefs_set_filename_format(char* val)
{
    g_free(cfg->filename_format);
    cfg->filename_format = g_strdup(val);
}

gboolean prefs_get_write_gaintag(void)
{
    return(cfg->write_gaintag);
}

void prefs_set_write_gaintag(gboolean val)
{
    cfg->write_gaintag = val;
}

gboolean prefs_get_special_export_charset(void)
{
    return(cfg->special_export_charset);
}

void prefs_set_special_export_charset(gboolean val)
{
    cfg->special_export_charset = val;
}
