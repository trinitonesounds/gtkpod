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

#ifndef __PREFS_H__
#define __PREFS_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

struct cfg
{
  gchar    *ipod_mount;   /* mount point of iPod */
  gchar    *last_dir;     /* last directory selected */
  gboolean writeid3;      /* should changes to ID3 tags be written to file */
};

/* enum for reading of options */
enum {
  GP_HELP,
  GP_MOUNT,
  GP_WRITEID3
};

extern struct cfg *cfg;

gboolean read_prefs (int argc, char *argv[]);
void write_prefs (void);
void discard_prefs (void);

#endif __PREFS_H__
