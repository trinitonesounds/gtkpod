/*
|  File conversion started by Simon Naunton <snaunton gmail.com> in 2007
|
|  Copyright (C) 2002-2007 Jorg Schuler <jcsjcs at users.sourceforge.net>
|  Part of the gtkpod project.
|
|  URL: http://gtkpod.sourceforge.net/
|  URL: http://www.gtkpod.org
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


#ifndef __FILE_CONVERT_H_
#define __FILE_CONVERT_H_

#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif

#include <itdb.h>

extern const gchar *FILE_CONVERT_CACHEDIR;
extern const gchar *FILE_CONVERT_MAXDIRSIZE;
extern const gchar *FILE_CONVERT_TEMPLATE;
extern const gchar *FILE_CONVERT_MAX_THREADS_NUM;
extern const gchar *FILE_CONVERT_DISPLAY_LOG;

typedef enum
{
    FILE_CONVERT_INACTIVE = 0,
    FILE_CONVERT_REQUIRED,
    FILE_CONVERT_SCHEDULED,
    FILE_CONVERT_PROCESSING,
    FILE_CONVERT_FAILED,
    FILE_CONVERT_REQUIRED_FAILED,
    FILE_CONVERT_KILLED,
    FILE_CONVERT_CONVERTED
} FileConvertStatus;

void file_convert_init (void);
void file_convert_shutdown (void);
void file_convert_prefs_changed (void);
void file_convert_update_default_sizes (void);
gboolean file_convert_add_track (Track *track);
void file_convert_itdb_first (iTunesDB *itdb);
void file_convert_cancel_itdb (iTunesDB *itdb);
void file_convert_cancel_track (Track *track);
Track *file_convert_timed_wait (iTunesDB* itdb, gint ms);

#endif
