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
}


/* Read Preferences and initialise the cfg-struct */
gboolean read_prefs (int argc, char *argv[])
{
  gint j;
  gint32 val;
  gchar *string;
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
      { 0, 0, 0, 0 }
    };
  
  if (cfg != NULL) discard_prefs ();
  cfg = g_malloc0 (sizeof (struct cfg));
  cfg->ipod_mount = g_strdup ("/mnt/ipod");
  cfg->last_dir = g_strdup ("~/");
  cfg->writeid3 = FALSE;

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
  if (cfg != NULL)
    {
      g_free (cfg->ipod_mount);
      g_free (cfg->last_dir);
      g_free (cfg);
      cfg = NULL;
    }
}
