/* Time-stamp: <2005-06-17 21:01:39 jcs>
|
|  Copyright (C) 2002 Corey Donohoe <atmos at atmos.org>
|  Copyright (C) 2002-2005 Jorg Schuler <jcsjcs at users sourceforge net>
|  Part of the gtkpod project.
| 
|  URL: http://www.gtkpod.org/
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
#ifndef _GTKPOD_PREFS_WINDOW_H
#define _GTKPOD_PREFS_WINDOW_H

#include <gtk/gtk.h>
#include "prefs.h"

extern const gchar *path_entry_names[];

void prefs_window_update_default_sizes (void);
void prefs_window_block (void);
void prefs_window_release (void);
void prefs_window_show_hide_tooltips (void);
void prefs_window_ok (void);
void prefs_window_apply (void);
void prefs_window_cancel (void);
void prefs_window_create (void);
void prefs_window_delete (void);

void prefs_window_set_unused_gboolean3 (gboolean val);

void sort_window_create (void);
void sort_window_block (void);
void sort_window_release (void);
void sort_window_show_hide_tooltips (void);
void sort_window_ok (void);
void sort_window_apply (void);
void sort_window_cancel (void);
void sort_window_create (void);
void sort_window_delete (void);
void sort_window_set_pm_autostore (gboolean val);
void sort_window_set_tm_autostore (gboolean val);
void sort_window_set_pm_sort (gint val);
void sort_window_set_st_sort (gint val);
void sort_window_set_tm_sort (gint val);
void sort_window_set_case_sensitive (gboolean val);
void sort_window_update (void);
#endif
