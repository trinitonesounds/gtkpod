/*
|  Copyright (C) 2002 Jorg Schuler <jcsjcs at sourceforge.net>
|  Part of the gtkpod project.
| 
|  URL: 
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

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include "prefs.h"
#include "support.h"

struct cfg *cfg = NULL;


static void usage (FILE *file)
{
  fprintf(file, _("gtkpod version %s usage:\n"), VERSION);
  fprintf(file, _("  -h, --help:   display this message\n"));
  fprintf(file, _("  -m path:      define the mountpoint of your iPod\n"));
  fprintf(file, _("  --mountpoint: same as \"-m\".\n"));
  fprintf(file, _("  -w:           write changed ID3 tags to file\n"));
  fprintf(file, _("  --writeid3:   same as \"-w\".\n"));
  fprintf(file, _("  -c:           check files automagically for duplicates\n"));
  fprintf(file, _("  --md5:   same as \"-c\".\n"));
}

struct cfg*
cfg_new(void)
{
    struct cfg *mycfg = NULL;
    gchar buf[PATH_MAX];

    mycfg = g_malloc0 (sizeof (struct cfg));
    memset(mycfg, 0, sizeof(struct cfg));
    if(getcwd(buf, PATH_MAX))
    {
	mycfg->last_dir.dir_browse = g_strdup_printf ("%s/", buf);
	mycfg->last_dir.file_browse = g_strdup_printf ("%s/", buf);
    }
    else
    {
	mycfg->last_dir.dir_browse = g_strdup ("~/");
	mycfg->last_dir.file_browse = g_strdup ("~/");
    }
    mycfg->ipod_mount = g_strdup ("/mnt/ipod");
    mycfg->writeid3 = mycfg->md5songs = FALSE;
    return(mycfg);
}

/* Read Preferences and initialise the cfg-struct */
gboolean read_prefs (int argc, char *argv[])
{
  int opt;
  int option_index;
  struct option const options[] =
    {
      { "h",           no_argument,	NULL, GP_HELP },
      { "help",        no_argument,	NULL, GP_HELP },
      { "m",           required_argument,	NULL, GP_MOUNT },
      { "mountpoint",  required_argument,	NULL, GP_MOUNT },
      { "w",           no_argument,	NULL, GP_WRITEID3 },
      { "writeid3",    no_argument,	NULL, GP_WRITEID3 },
      { "c",           no_argument,	NULL, GP_MD5SONGS },
      { "md5",		no_argument,	NULL, GP_MD5SONGS },
      { 0, 0, 0, 0 }
    };
  
  if (cfg != NULL) discard_prefs ();
  
  cfg = cfg_new();

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
      case GP_WRITEID3:
	cfg->writeid3 = TRUE;
	break;
      case GP_MD5SONGS:
	cfg->md5songs = TRUE;
	break;
      default:
	fprintf(stderr, _("Unknown option: %s\n"), argv[optind]);
	usage(stderr);
	exit(1);
      }
  }
  return TRUE;
}



void write_prefs (void)
{
  
}


/* Free all memory including the cfg-struct itself. */
void discard_prefs ()
{
    cfg_free(cfg);
}

void cfg_free(struct cfg *c)
{
    if(c)
    {
      g_free (c->ipod_mount);
      g_free (c->last_dir.dir_browse);
      g_free (c->last_dir.file_browse);
      g_free (c);
    }
}

static gchar *
get_dirname_of_filename(gchar *file)
{
   char *tok = NULL, *buf = NULL, filename[PATH_MAX];
   gchar *result, dump[PATH_MAX];

   snprintf(filename, PATH_MAX, "%s", file);
   buf = (gchar *) malloc((sizeof(gchar) * PATH_MAX) + 1);

   memset(buf, 0, sizeof(buf));
   memset(dump, 0, PATH_MAX);

   if ((tok = strtok(filename, "/")))
   {
      do
      {
         buf = strncat(buf, dump, PATH_MAX);
         snprintf(dump, PATH_MAX, "/%s", tok);
      }
      while ((tok = strtok(NULL, "/")));
   }
   result = g_strdup_printf("%s/",buf);
   g_free(buf);
   return (result);
}

void prefs_set_last_dir_file_browse_for_filename(gchar *file)
{
    if(cfg->last_dir.file_browse) g_free(cfg->last_dir.file_browse);
    cfg->last_dir.file_browse = get_dirname_of_filename(file);
}

void prefs_set_last_dir_dir_browse_for_filename(gchar *file)
{
    if(cfg->last_dir.dir_browse) g_free(cfg->last_dir.dir_browse);
    cfg->last_dir.dir_browse = get_dirname_of_filename(file);
}


void prefs_set_mount_point(const gchar *mp)
{
    if(cfg->ipod_mount) g_free(cfg->ipod_mount);
    cfg->ipod_mount = g_strdup(mp);
}

void prefs_set_md5songs_active(gboolean active)
{
    cfg->md5songs = active;
}

void prefs_set_writeid3_active(gboolean active)
{
    cfg->writeid3 = active;
}
