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

#define ISSPACE(a) (((a)==9) || ((a)==' '))    /* TAB, SPACE */

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
    mycfg->autoimport = FALSE;
    for (i=0; i<SORT_TAB_NUM; ++i)
    {
	mycfg->st[i].autoselect = TRUE;
	mycfg->st[i].category = (i<ST_CAT_NUM ? i:0);
    }
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
    for (i=0; i<SM_NUM_COLUMNS_PREFS; ++i)
    {
	mycfg->sm_col_width[i] = 80;
    }
    for (i=0; i<SM_NUM_TAGS_PREFS; ++i)
    {
	mycfg->tag_autoset[i] = FALSE;
    }
    for (i=0; i<PANED_NUM; ++i)
    {
	mycfg->paned_pos[i] = -1;  /* -1 means: let gtk worry about position */
    }
    mycfg->tag_autoset[SM_COLUMN_ARTIST] = TRUE;
    mycfg->tag_autoset[SM_COLUMN_ALBUM] = TRUE;
    mycfg->tag_autoset[SM_COLUMN_TITLE] = TRUE;
    mycfg->statusbar_timeout = STATUSBAR_TIMEOUT;
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
	  while (ISSPACE(*bufp)) ++bufp;
	  line = g_strndup (buf, arg-bufp);
	  ++arg;
	  len = strlen (arg); /* remove newline */
	  if((len>0) && (arg[len-1] == 0x0a))  arg[len-1] = 0;
	  /* skip whitespace */
	  while (ISSPACE(*arg)) ++arg;
	  if(g_ascii_strcasecmp (line, "mountpoint") == 0)
	  {
	      gchar mount_point[PATH_MAX];
	      snprintf(mount_point, PATH_MAX, "%s", arg);
	      prefs_set_mount_point(mount_point);
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
	  else if(g_ascii_strcasecmp (line, "album") == 0)
	  {
	      prefs_set_song_list_show_album((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "track") == 0)
	  {
	      prefs_set_song_list_show_track((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "genre") == 0)
	  {
	      prefs_set_song_list_show_genre((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "artist") == 0)
	  {
	      prefs_set_song_list_show_artist((gboolean)atoi(arg));
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
	  else if(g_ascii_strncasecmp (line, "paned_pos", 9) == 0)
	  {
	      gint i = atoi (line+9);
	      prefs_set_paned_pos (i, atoi (arg));
	  }      
	  else if(g_ascii_strcasecmp (line, "offline") == 0)
	  {
	      prefs_set_offline((gboolean)atoi(arg));
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
	  else
	  {
	      gtkpod_warning (_("Error while reading prefs: %s\n"), buf);
	  }	      
	  g_free(line);
	}
    }
}


void
read_prefs_defaults(void)
{
  gchar *cfgdir = NULL;
  gchar filename[PATH_MAX+1];
  FILE *fp = NULL;
  
  if ((cfgdir = prefs_get_cfgdir ())) {
    {
      snprintf(filename, PATH_MAX, "%s/prefs", cfgdir);
      filename[PATH_MAX] = 0;
      if(g_file_test(filename, G_FILE_TEST_EXISTS))
	{
	  if((fp = fopen(filename, "r")))
	    {
	      read_prefs_from_file_desc(fp);
	      fclose(fp);
	    }
	  else
	    {
	      gtkpod_warning(_("Unable to open config file \"%s\" for reading\n"), filename);
	    }
	}
    }
  }
  C_FREE (cfgdir);
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

    fprintf(fp, "mountpoint=%s\n", cfg->ipod_mount);
    if (cfg->charset)
    {
	fprintf(fp, "charset=%s\n", cfg->charset);
    } else {
	fprintf(fp, "charset=\n");
    }
    fprintf(fp, "id3=%d\n", prefs_get_id3_write ());
    fprintf(fp, "id3_all=%d\n", prefs_get_id3_writeall ());
    fprintf(fp, "md5=%d\n",cfg->md5songs);
    fprintf(fp, "album=%d\n",prefs_get_song_list_show_album());
    fprintf(fp, "track=%d\n",prefs_get_song_list_show_track());
    fprintf(fp, "genre=%d\n",prefs_get_song_list_show_genre());
    fprintf(fp, "artist=%d\n",prefs_get_song_list_show_artist());
    fprintf(fp, "delete_file=%d\n",prefs_get_song_playlist_deletion());
    fprintf(fp, "delete_playlist=%d\n",prefs_get_playlist_deletion());
    fprintf(fp, "delete_ipod=%d\n",prefs_get_song_ipod_file_deletion());
    fprintf(fp, "auto_import=%d\n",prefs_get_auto_import());
    for (i=0; i<SORT_TAB_NUM; ++i)
    {
	fprintf(fp, "st_autoselect%d=%d\n", i, prefs_get_st_autoselect (i));
	fprintf(fp, "st_category%d=%d\n", i, prefs_get_st_category (i));
    }
    for (i=0; i<SM_NUM_COLUMNS_PREFS; ++i)
    {
	fprintf(fp, "sm_col_width%d=%d\n", i, prefs_get_sm_col_width (i));
    }	
    for (i=0; i<SM_NUM_TAGS_PREFS; ++i)
    {
	fprintf(fp, "tag_autoset%d=%d\n", i, prefs_get_tag_autoset (i));
    }	
    for (i=0; i<PANED_NUM; ++i)
    {
	fprintf(fp, "paned_pos%d=%d\n", i, prefs_get_paned_pos (i));
    }	
    fprintf(fp, "offline=%d\n",prefs_get_offline());
    fprintf(fp, "backups=%d\n",prefs_get_keep_backups());
    fprintf(fp, "extended_info=%d\n",prefs_get_write_extended_info());
    fprintf(fp, "dir_browse=%s\n",cfg->last_dir.browse);
    fprintf(fp, "dir_export=%s\n",cfg->last_dir.export);
    fprintf (fp, "size_gtkpod.x=%d\n", cfg->size_gtkpod.x);
    fprintf (fp, "size_gtkpod.y=%d\n", cfg->size_gtkpod.y);
    fprintf (fp, "size_conf_sw.x=%d\n", cfg->size_conf_sw.x);
    fprintf (fp, "size_conf_sw.y=%d\n", cfg->size_conf_sw.y);
    fprintf (fp, "size_conf.x=%d\n", cfg->size_conf.x);
    fprintf (fp, "size_conf.y=%d\n", cfg->size_conf.y);
    
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
      C_FREE (c);
    }
}

static gchar *
get_dirname_of_filename(gchar *file)
{
  if (g_file_test(file, G_FILE_TEST_IS_DIR))
    return g_strdup (file);
  else return g_path_get_dirname (file);
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



/* song list opts */
void prefs_set_song_list_show_all(gboolean val)
{

    if(val)
    {
	cfg->song_list_show.artist = TRUE;
	cfg->song_list_show.album = TRUE;
	cfg->song_list_show.track = TRUE;
	cfg->song_list_show.genre = TRUE;
    }
    else
    {
	cfg->song_list_show.artist = FALSE;
	cfg->song_list_show.album = FALSE;
	cfg->song_list_show.track = FALSE;
	cfg->song_list_show.genre = FALSE;
    }
}

void prefs_set_song_list_show_artist(gboolean val)
{
    cfg->song_list_show.artist = val;
}

void prefs_set_song_list_show_album(gboolean val)
{
    cfg->song_list_show.album = val;
}

void prefs_set_song_list_show_track(gboolean val)
{
    cfg->song_list_show.track = val;
}

void prefs_set_song_list_show_genre(gboolean val)
{
    cfg->song_list_show.genre = val;
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

gboolean prefs_get_song_list_show_all(void)
{
    if((cfg->song_list_show.artist == FALSE) &&
	(cfg->song_list_show.album == FALSE) &&
	(cfg->song_list_show.track == FALSE) &&
	(cfg->song_list_show.genre == FALSE))
	return(TRUE);
    else
	return(FALSE);
}

gboolean prefs_get_song_list_show_artist(void)
{
    return(cfg->song_list_show.artist);
}

gboolean prefs_get_song_list_show_album(void)
{
    return(cfg->song_list_show.album);
}

gboolean prefs_get_song_list_show_track(void)
{
    return(cfg->song_list_show.track); 
}

gboolean prefs_get_song_list_show_genre(void)
{
    return(cfg->song_list_show.genre); 
}



void prefs_print(void)
{
    FILE *fp = stderr;
    gchar *on = "On";
    gchar *off = "Off";
    
    fprintf(fp, "GtkPod Preferences\n");
    fprintf(fp, "Mount Point:\t%s\n", cfg->ipod_mount);
    fprintf(fp, "Interactive ID3:\t");
    if(cfg->id3_write)
	fprintf(fp, "%s\n", on);
    else
	fprintf(fp, "%s\n", off);
    
    fprintf(fp, "MD5 Songs:\t");
    if(cfg->md5songs)
	fprintf(fp, "%s\n", on);
    else
	fprintf(fp, "%s\n", off);
    
    fprintf(fp, "Auto Import:\t");
    if(cfg->autoimport)
	fprintf(fp, "%s\n", on);
    else
	fprintf(fp, "%s\n", off);
    
    fprintf(fp, "Song List Options:\n");
    fprintf(fp, "  Show All Attributes: ");
    if(prefs_get_song_list_show_all())
	fprintf(fp, "%s\n", on);
    else
	fprintf(fp, "%s\n", off);
    fprintf(fp, "  Show Album: ");
    if(prefs_get_song_list_show_album())
	fprintf(fp, "%s\n", on);
    else
	fprintf(fp, "%s\n", off);
    fprintf(fp, "  Show Track: ");
    if(prefs_get_song_list_show_track())
	fprintf(fp, "%s\n", on);
    else
	fprintf(fp, "%s\n", off);
    fprintf(fp, "  Show Genre: ");
    if(prefs_get_song_list_show_genre())
	fprintf(fp, "%s\n", on);
    else
	fprintf(fp, "%s\n", off);
    fprintf(fp, "  Show Artist: ");
    if(prefs_get_song_list_show_artist())
	fprintf(fp, "%s\n", on);
    else
	fprintf(fp, "%s\n", off);
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
    if (cfgd->charset)
    {
	g_free (cfgd->charset);
	cfgd->charset = NULL;
    }
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
    if (inst < SORT_TAB_NUM)
    {
	return cfg->st[inst].autoselect;
    }
    else
    {
	return TRUE; /* hmm.... this should not happen... */
    }
}

/* "inst": the instance of the sort tab, "autoselect": should "All" be
 * selected automatically? */
void prefs_set_st_autoselect (guint32 inst, gboolean autoselect)
{
    if (inst < SORT_TAB_NUM)
    {
	cfg->st[inst].autoselect = autoselect;
    }
}

/* "inst": the instance of the sort tab */
guint prefs_get_st_category (guint32 inst)
{
    if (inst < SORT_TAB_NUM)
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
    if ((inst < SORT_TAB_NUM) && (category < ST_CAT_NUM))
    {
	cfg->st[inst].category = category;
    }
    else
    {
	gtkpod_warning ("Category nr (%d) or sorttab nr (%d) out of range.\n", category, inst);
    }
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
	  if(!mkdir(cfgdir, 0755))
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

/* get position of GtkPaned element nr. "i" */
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
