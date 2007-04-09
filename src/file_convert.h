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

#include <gtk/gtk.h>
#include <itdb.h>
#include <file.h>

typedef struct
{
    Track   *track;                /* Track to convert */
    gchar   *converted_file;       /* PC filename of the "mp3" file 
                                      != NULL if the file exists   */
    gint32   old_size;             /* size of the original file    */
    FileType type;                 /* type of the original file    */
    GPid     child_pid;            /* PID of conversion process    */
    gint     child_stdout;         /* STDOUT of conversion process */
    gchar    *command_line;        /* used for conversion */
    gint     source_id;
    gboolean aborted;
} TrackConv;


GError *file_convert_pre_copy (TrackConv *converter);
GError *file_convert_post_copy (TrackConv *converter);
GError *file_convert_wait_for_conversion (TrackConv *converter);

/* extern gchar **cmdline_to_argv(const gchar *cmdline, Track *track); */

void conversion_init (void);
void conversion_shutdown (void);

#endif
