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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "prefs.h"
#include "support.h"
#include "misc.h"
#include "display.h"
#include "md5.h"

/* global config struct */
/* FIXME: make me static */
struct cfg *cfg = NULL;

/* enum for reading of options */
enum {
  GP_HELP,
  GP_MOUNT,
  GP_ID3_WRITE,
  GP_MD5SONGS,
  GP_AUTO,
  GP_OFFLINE
};

static void usage (FILE *file)
{
  fprintf(file, _("gtkpod version %s usage:\n"), VERSION);
  fprintf(file, _("  -h, --help:   display this message\n"));
  fprintf(file, _("  -m path:      define the mountpoint of your iPod\n"));
  fprintf(file, _("  --mountpoint: same as \"-m\".\n"));
  fprintf(file, _("  -w:           write changed ID3 tags to file\n"));
  fprintf(file, _("  --id3_write:   same as \"-w\".\n"));
  fprintf(file, _("  -c:           check files automagically for duplicates\n"));
  fprintf(file, _("  --md5:        same as \"-c\".\n"));
  fprintf(file, _("  -a:           import database automatically after start.\n"));
  fprintf(file, _("  --auto:       same as \"-a\".\n"));
  fprintf(file, _("  -o:           use offline mode. No changes are exported to the iPod,\n                but to ~/.gtkpod/ instead. iPod is updated if \"Export\" is\n                used with \"Offline\" deactivated.\n"));
  fprintf(file, _("  --offline:    same as \"-o\".\n"));
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
    mycfg->deletion.song = TRUE;
    mycfg->deletion.playlist = TRUE;
    mycfg->deletion.ipod_file = TRUE;
    mycfg->md5songs = FALSE;
    mycfg->update_existing = FALSE;
    mycfg->block_display = FALSE;
    mycfg->autoimport = FALSE;
    for (i=0; i<SORT_TAB_MAX; ++i)
    {
	mycfg->st[i].autoselect = TRUE;
	mycfg->st[i].category = (i<ST_CAT_NUM ? i:0);
    }
    mycfg->mpl_autoselect = TRUE;
    mycfg->offline = FALSE;
    mycfg->keep_backups = TRUE;
    mycfg->write_extended_info = TRUE;
    mycfg->id3_write = FALSE;
    mycfg->id3_writeall = FALSE;
    mycfg->size_gtkpod.x = 600;
    mycfg->size_gtkpod.y = 500;
    mycfg->size_conf_sw.x = 300;
    mycfg->size_conf_sw.y = 300;
    mycfg->size_conf.x = 300;
    mycfg->size_conf.y = -1;
    mycfg->size_dirbr.x = 300;
    mycfg->size_dirbr.y = 400;
    for (i=0; i<SM_NUM_COLUMNS_PREFS; ++i)
    {
	mycfg->sm_col_width[i] = 80;
	mycfg->col_visible[i] = FALSE;
	mycfg->col_order[i] = i;
    }
    mycfg->col_visible[SM_COLUMN_ARTIST] = TRUE;
    mycfg->col_visible[SM_COLUMN_ALBUM] = TRUE;
    mycfg->col_visible[SM_COLUMN_TITLE] = TRUE;
    mycfg->col_visible[SM_COLUMN_GENRE] = TRUE;
    for (i=0; i<SM_NUM_TAGS_PREFS; ++i)
    {
	mycfg->tag_autoset[i] = FALSE;
    }
    mycfg->tag_autoset[SM_COLUMN_TITLE] = TRUE;
    for (i=0; i<PANED_NUM; ++i)
    {
	mycfg->paned_pos[i] = -1;  /* -1 means: let gtk worry about position */
    }
    mycfg->show_duplicates = TRUE;
    mycfg->show_updated = TRUE;
    mycfg->show_non_updated = TRUE;
    mycfg->display_toolbar = TRUE;
    mycfg->update_charset = FALSE;
    mycfg->toolbar_style = GTK_TOOLBAR_BOTH;
    mycfg->save_sorted_order = FALSE;
    mycfg->sort_tab_num = 2;
    mycfg->last_prefs_page = 0;
    mycfg->statusbar_timeout = STATUSBAR_TIMEOUT;
    mycfg->play_now_path = g_strdup ("xmms -p %s");
    mycfg->play_enqueue_path = g_strdup ("xmms -e %s");
    mycfg->automount = FALSE;
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
	  if(g_ascii_strcasecmp (line, "mountpoint") == 0)
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
	      prefs_set_md5songs((gboolean)atoi(arg));
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
	      prefs_set_song_playlist_deletion((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "delete_playlist") == 0)
	  {
	      prefs_set_playlist_deletion((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "delete_ipod") == 0)
	  {
	      prefs_set_song_ipod_file_deletion((gboolean)atoi(arg));
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
	  else if(g_ascii_strcasecmp (line, "mpl_autoselect") == 0)
	  {
	      prefs_set_mpl_autoselect((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strncasecmp (line, "sm_col_width", 12) == 0)
	  {
	      gint i = atoi (line+12);
	      prefs_set_sm_col_width (i, atoi (arg));
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
	  else if(g_ascii_strncasecmp (line, "paned_pos", 9) == 0)
	  {
	      gint i = atoi (line+9);
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
	  else if(g_ascii_strcasecmp (line, "display_toolbar") == 0)
	  {
	      prefs_set_display_toolbar((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "update_charset") == 0)
	  {
	      prefs_set_update_charset((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "save_sorted_order") == 0)
	  {
	      prefs_set_save_sorted_order((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "size_gtkpod.x") == 0)
	  {
	      prefs_set_size_gtkpod (atoi (arg), -2);
	  }
	  else if(g_ascii_strcasecmp (line, "size_gtkpod.y") == 0)
	  {
	      prefs_set_size_gtkpod (-2, atoi (arg));
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
	  else if(g_ascii_strcasecmp (line, "automount") == 0)
	  {
	      prefs_set_automount (atoi (arg));
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
	      gtkpod_warning(_("Unable to open config file \"%s\" for reading\n"), filename);
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
  /* set statusbar paned to a decent value if unset */
  if (prefs_get_paned_pos (PANED_STATUS) == -1)
  {
      gint x,y;
      prefs_get_size_gtkpod (&x, &y);
      /* set to about 3/4 of the window width */
      if (x>0)   prefs_set_paned_pos (PANED_STATUS, 22*x/30);
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
      { "id3_write",    no_argument,	NULL, GP_ID3_WRITE },
      { "c",           no_argument,	NULL, GP_MD5SONGS },
      { "md5",	       no_argument,	NULL, GP_MD5SONGS },
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
      case GP_MD5SONGS:
	cfg->md5songs = TRUE;
	break;
      case GP_AUTO:
	cfg->autoimport = TRUE;
	break;
      case GP_OFFLINE:
	cfg->offline = TRUE;
	break;
      default:
	fprintf(stderr, _("Unknown option: %s\n"), argv[optind]);
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
    /* update order of song view columns */
    sm_store_col_order ();

    fprintf(fp, "mountpoint=%s\n", cfg->ipod_mount);
    fprintf(fp, "play_now_path=%s\n", cfg->play_now_path);
    fprintf(fp, "play_enqueue_path=%s\n", cfg->play_enqueue_path);
    if (cfg->charset)
    {
	fprintf(fp, "charset=%s\n", cfg->charset);
    } else {
	fprintf(fp, "charset=\n");
    }
    fprintf(fp, "id3=%d\n", prefs_get_id3_write ());
    fprintf(fp, "id3_all=%d\n", prefs_get_id3_writeall ());
    fprintf(fp, "md5=%d\n",prefs_get_md5songs ());
    fprintf(fp, "update_existing=%d\n",prefs_get_update_existing ());
    fprintf(fp, "block_display=%d\n",prefs_get_block_display());
    fprintf(fp, _("# delete confirmation\n"));
    fprintf(fp, "delete_file=%d\n",prefs_get_song_playlist_deletion());
    fprintf(fp, "delete_playlist=%d\n",prefs_get_playlist_deletion());
    fprintf(fp, "delete_ipod=%d\n",prefs_get_song_ipod_file_deletion());
    fprintf(fp, "auto_import=%d\n",prefs_get_auto_import());
    fprintf(fp, _("# sort tab: select 'All', last selected page (=category)\n"));
    for (i=0; i<SORT_TAB_MAX; ++i)
    {
	fprintf(fp, "st_autoselect%d=%d\n", i, prefs_get_st_autoselect (i));
	fprintf(fp, "st_category%d=%d\n", i, prefs_get_st_category (i));
    }
    fprintf(fp, _("# autoselect master playlist?\n"));
    fprintf(fp, "mpl_autoselect=%d\n", prefs_get_mpl_autoselect ());
    fprintf(fp, _("# title=0, artist, album, genre, composer\n"));
    fprintf(fp, _("# track_nr=5, ipod_id, pc_path, transferred\n"));
    fprintf(fp, _("# autoset: set empty tag to filename?\n"));
    for (i=0; i<SM_NUM_COLUMNS_PREFS; ++i)
    {
	fprintf(fp, "sm_col_width%d=%d\n", i, prefs_get_sm_col_width (i));
	fprintf(fp, "col_visible%d=%d\n",  i, prefs_get_col_visible (i));
	fprintf(fp, "col_order%d=%d\n",  i, prefs_get_col_order (i));
	if (i < SM_NUM_TAGS_PREFS)
	    fprintf(fp, "tag_autoset%d=%d\n", i, prefs_get_tag_autoset (i));
    }	
    fprintf(fp, _("# position of sliders (paned): playlists, above songs,\n# between sort tabs, and in statusbar.\n"));
    for (i=0; i<PANED_NUM; ++i)
    {
	fprintf(fp, "paned_pos%d=%d\n", i, prefs_get_paned_pos (i));
    }
    fprintf(fp, "sort_tab_num=%d\n",prefs_get_sort_tab_num());
    fprintf(fp, "toolbar_style=%d\n",prefs_get_toolbar_style());
    fprintf(fp, "last_prefs_page=%d\n",prefs_get_last_prefs_page());
    fprintf(fp, "offline=%d\n",prefs_get_offline());
    fprintf(fp, "backups=%d\n",prefs_get_keep_backups());
    fprintf(fp, "extended_info=%d\n",prefs_get_write_extended_info());
    fprintf(fp, "dir_browse=%s\n",cfg->last_dir.browse);
    fprintf(fp, "dir_export=%s\n",cfg->last_dir.export);
    fprintf(fp, "show_duplicates=%d\n",prefs_get_show_duplicates());
    fprintf(fp, "show_updated=%d\n",prefs_get_show_updated());
    fprintf(fp, "show_non_updated=%d\n",prefs_get_show_non_updated());
    fprintf(fp, "display_toolbar=%d\n",prefs_get_display_toolbar());
    fprintf(fp, "update_charset=%d\n",prefs_get_update_charset());
    fprintf(fp, "save_sorted_order=%d\n",prefs_get_save_sorted_order());
    fprintf(fp, _("# window sizes: main window, confirmation scrolled,\n#               confirmation non-scrolled, dirbrowser\n"));
    fprintf (fp, "size_gtkpod.x=%d\n", cfg->size_gtkpod.x);
    fprintf (fp, "size_gtkpod.y=%d\n", cfg->size_gtkpod.y);
    fprintf (fp, "size_conf_sw.x=%d\n", cfg->size_conf_sw.x);
    fprintf (fp, "size_conf_sw.y=%d\n", cfg->size_conf_sw.y);
    fprintf (fp, "size_conf.x=%d\n", cfg->size_conf.x);
    fprintf (fp, "size_conf.y=%d\n", cfg->size_conf.y);
    fprintf (fp, "size_dirbr.x=%d\n", cfg->size_dirbr.x);
    fprintf (fp, "size_dirbr.y=%d\n", cfg->size_dirbr.y);
    fprintf (fp, "automount=%d\n", cfg->automount);
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
	    gtkpod_warning (_("Unable to open \"%s\" for writing\n"),
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
    { /* C_FREE defined in misc.h */
      C_FREE (c->ipod_mount);
      C_FREE (c->charset);
      C_FREE (c->last_dir.browse);
      C_FREE (c->last_dir.export);
      C_FREE (c->play_now_path);
      C_FREE (c->play_enqueue_path);
      C_FREE (c);
    }
}

static gchar *
get_dirname_of_filename(gchar *file)
{
    gint len;
    gchar *buf, *result = NULL;

    if (!file) return NULL;

    if (g_file_test(file, G_FILE_TEST_IS_DIR))
	buf = g_strdup (file);
    else
	buf = g_path_get_dirname (file);
	
    len = strlen (buf);
    if (len && (buf[len-1] == '/'))
	result = buf;
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
void prefs_set_last_dir_browse(gchar *file)
{
    if (file)
    {
	C_FREE(cfg->last_dir.browse);
	cfg->last_dir.browse = get_dirname_of_filename(file);
    }
}

void prefs_set_last_dir_export(gchar *file)
{
    if (file)
    {
	if(cfg->last_dir.export) g_free(cfg->last_dir.export);
	cfg->last_dir.export = get_dirname_of_filename(file);
    }
}

void prefs_set_mount_point(const gchar *mp)
{
    if(cfg->ipod_mount) g_free(cfg->ipod_mount);
    /* if new mount point starts with "~/", we replace it with the
       home directory */
    if (strncmp ("~/", mp, 2) == 0)
      cfg->ipod_mount = concat_dir (g_get_home_dir (), mp+2);
    else cfg->ipod_mount = g_strdup(mp);
}


/* If the status of md5 hash flag changes, free or re-init the md5
   hash table */
void prefs_set_md5songs(gboolean active)
{
    if (cfg->md5songs && !active)
    { /* md5 checksums have been turned off */
	cfg->md5songs = active;
	md5_unique_file_free ();
    }
    if (!cfg->md5songs && active)
    { /* md5 checksums have been turned on */
	cfg->md5songs = active; /* must be set before calling
				   hash_songs() */
	hash_songs ();
    }
}

gboolean prefs_get_md5songs(void)
{
    return cfg->md5songs;
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

void prefs_set_playlist_deletion(gboolean val)
{
    cfg->deletion.playlist = val;
}

gboolean prefs_get_playlist_deletion(void)
{
    return(cfg->deletion.playlist);
}

void prefs_set_song_playlist_deletion(gboolean val)
{
    cfg->deletion.song = val;
}

gboolean prefs_get_song_playlist_deletion(void)
{
    return(cfg->deletion.song);
}

void prefs_set_song_ipod_file_deletion(gboolean val)
{
    cfg->deletion.ipod_file = val;
}

gboolean prefs_get_song_ipod_file_deletion(void)
{
    return(cfg->deletion.ipod_file);
}

void prefs_set_charset (gchar *charset)
{
    prefs_cfg_set_charset (cfg, charset);
}


void prefs_cfg_set_charset (struct cfg *cfgd, gchar *charset)
{
    C_FREE (cfgd->charset);
    if (charset && strlen (charset))  cfgd->charset = g_strdup (charset);
}


gchar *prefs_get_charset (void)
{
    return cfg->charset;
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
	gtkpod_warning ("Category nr (%d) or sorttab nr (%d) out of range.\n", category, inst);
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


/* retrieve the width of the song display columns. "col": one of the
   SM_COLUMN_... */
gint prefs_get_sm_col_width (gint col)
{
    if (col < SM_NUM_COLUMNS_PREFS && (cfg->sm_col_width[col] > 0))
	return cfg->sm_col_width[col];
    return 80;  /* default -- col should be smaller than
		   SM_NUM_COLUMNS_PREFS) */
}

/* set the width of the song display columns. "col": one of the
   SM_COLUMN_..., "width": current width */
void prefs_set_sm_col_width (gint col, gint width)
{
    if (col < SM_NUM_COLUMNS_PREFS && width > 0)
	cfg->sm_col_width[col] = width;
}


/* Returns "$HOME/.gtkpod" or NULL if dir does not exist and cannot be
   created. You must g_free the string after use */
gchar *prefs_get_cfgdir (void)
{
  G_CONST_RETURN gchar *str;
  gchar *cfgdir=NULL;

  if((str = g_get_home_dir ()))
    {
      cfgdir = concat_dir (str, ".gtkpod");
      if(!g_file_test(cfgdir, G_FILE_TEST_IS_DIR))
	{
	  if(mkdir(cfgdir, 0755) == -1)
	    {
	      gtkpod_warning(_("Unable to \"mkdir %s\"\n"), cfgdir);
	      C_FREE (cfgdir); /*defined in misc.h*/
	    }
	}
    }
  return cfgdir;
}

/* Returns the ipod_mount. You must g_free the string after use */
gchar *prefs_get_ipod_mount (void)
{
    if (cfg->ipod_mount)  return g_strdup (cfg->ipod_mount);
    else                  return NULL;
}

/* Sets the default size for the gtkpod window. -2 means: don't change
 * the current size */
void prefs_set_size_gtkpod (gint x, gint y)
{
    if (x != -2) cfg->size_gtkpod.x = x;
    if (y != -2) cfg->size_gtkpod.y = y;
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

/* Writes the current default size for the gtkpod window in "x" and
   "y" */
void prefs_get_size_gtkpod (gint *x, gint *y)
{
    *x = cfg->size_gtkpod.x;
    *y = cfg->size_gtkpod.y;
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


/* Should empty tags be set to filename? -- "category": one of the
   SM_COLUMN_..., "autoset": new value */
void prefs_set_tag_autoset (gint category, gboolean autoset)
{
    if (category < SM_NUM_TAGS_PREFS)
	cfg->tag_autoset[category] = autoset;
}


/* Should empty tags be set to filename? -- "category": one of the
   SM_COLUMN_... */
gboolean prefs_get_tag_autoset (gint category)
{
    if (category < SM_NUM_TAGS_PREFS)
	return cfg->tag_autoset[category];
    return FALSE;
}

/* Display column nr @pos? @visible: new value */
void prefs_set_col_visible (gint pos, gboolean visible)
{
    if (pos < SM_NUM_COLUMNS_PREFS)
	cfg->col_visible[pos] = visible;
}


/* Display column nr @pos? */
gboolean prefs_get_col_visible (gint pos)
{
    if (pos < SM_NUM_COLUMNS_PREFS)
	return cfg->col_visible[pos];
    return FALSE;
}

/* Display which column at nr @pos? */
void prefs_set_col_order (gint pos, SM_item sm_item)
{
    if (pos < SM_NUM_COLUMNS_PREFS)
	cfg->col_order[pos] = sm_item;
}


/* Display column nr @pos? */
SM_item prefs_get_col_order (gint pos)
{
    if (pos < SM_NUM_COLUMNS_PREFS)
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
	fprintf (stderr, "Programming error: prefs_get_paned_pos: arg out of range (%d)\n", i);
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

gboolean prefs_get_save_sorted_order (void)
{
    return cfg->save_sorted_order;
}

void prefs_set_save_sorted_order (gboolean val)
{
    cfg->save_sorted_order = val;
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
	printf (_("prefs_set_toolbar_style: illegal style '%d' ignored\n"), i);
	return;
    }

    cfg->toolbar_style = i;
    display_show_hide_toolbar ();
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

    if ((!path) || (strlen (path) == 0)) return NULL;

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

gchar *prefs_get_play_now_path (void)
{
    return cfg->play_now_path;
}

void prefs_set_play_enqueue_path (const gchar *path)
{
    C_FREE (cfg->play_enqueue_path);
    cfg->play_enqueue_path = prefs_validate_play_path (path);
}

gchar *prefs_get_play_enqueue_path (void)
{
    return cfg->play_enqueue_path;
}

gboolean 
prefs_get_automount(void)
{
    return(cfg->automount);
}
void
prefs_set_automount(gboolean val)
{
    cfg->automount = val;
}
