/*
 |  Copyright (C) 2002-2011 Jorg Schuler <jcsjcs at users sourceforge net>
 |                                             Paul Richardson <phantom_sf at users.sourceforge.net>
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
 */

#ifndef NEW_SORTTABS_H_
#define NEW_SORTTABS_H_

#include <gtk/gtk.h>
#include "libgtkpod/gtkpod_app_iface.h"
#include "date_parser.h"
#include "sorttab_conversion.h"
#include "sorttab_widget.h"

/* Total number */
#define PANED_NUM (PANED_NUM_GLADE + PANED_NUM_ST)

void sorttab_display_new(GtkPaned *sort_tab_parent, gchar *glade_path);

SortTabWidget *sorttab_display_get_sort_tab_widget(gchar *text);

void sorttab_display_append_widget();

void sorttab_display_remove_widget();

/* Callbacks for signals received from core gtkpod */
void sorttab_display_select_playlist_cb(GtkPodApp *app, gpointer pl, gpointer data);
void sorttab_display_track_removed_cb(GtkPodApp *app, gpointer tk, gint32 pos, gpointer data);
void sorttab_display_track_updated_cb(GtkPodApp *app, gpointer tk, gpointer data);
void sorttab_display_preference_changed_cb(GtkPodApp *app, gpointer pfname, gpointer value, gpointer data);

#endif /* NEW_SORTTABS_H_ */
