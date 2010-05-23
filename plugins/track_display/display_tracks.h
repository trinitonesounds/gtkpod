/*
 |  Copyright (C) 2002-2010 Jorg Schuler <jcsjcs at users sourceforge net>
 |                                          Paul Richardson <phantom_sf at users.sourceforge.net>
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

#ifndef DISPLAY_TRACKS_H_
#define DISPLAY_TRACKS_H_

#include "plugin.h"
#include "libgtkpod/gtkpod_app_iface.h"
#include "libgtkpod/misc_conversion.h"

void tm_create_track_display(GtkWidget *parent);
void tm_destroy_widgets(void);
void tm_rows_reordered(void);
void tm_stop_editing(gboolean cancel);
gboolean tm_add_filelist(gchar *data, GtkTreePath *path, GtkTreeViewDropPosition pos);
void tm_update_default_sizes (void);
void tm_store_col_order (void);
void tm_show_preferred_columns(void);
GList *tm_get_selected_tracks(void);

void display_show_hide_searchbar(void);

void track_display_set_tracks_cb(GtkPodApp *app, gpointer tks, gpointer data);
void track_display_set_playlist_cb(GtkPodApp *app, gpointer pl, gpointer data);
void track_display_set_sort_enablement(GtkPodApp *app, gboolean flag, gpointer data);
void track_display_track_removed_cb(GtkPodApp *app, gpointer tk, gint32 pos, gpointer data);
void track_display_track_updated_cb(GtkPodApp *app, gpointer tk, gpointer data);
void track_display_preference_changed_cb(GtkPodApp *app, gpointer pfname, gint32 value, gpointer data);

#endif /* DISPLAY_TRACKS_H_ */
