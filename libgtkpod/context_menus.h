/*
 |  Copyright (C) 2003 Corey Donohoe <atmos at atmos dot org>
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
 |
 |  $Id$
 */
#ifndef CONTEXT_MENUS_H
#define CONTEXT_MENUS_H

#include<gtk/gtk.h>

/**
 * Attach a menu item to your context menu
 *
 * @m - the GtkMenu we're attaching to
 * @str - a gchar* with the menu label
 * @stock - name of the stock icon (or NULL if none is to be used)
 * @func - the callback for when the item is selected (or NULL)
 * @mi - the GtkWidget we're gonna hook into the menu
 */
GtkWidget *hookup_menu_item(GtkWidget *m, const gchar *str, const gchar *stock, GCallback func, gpointer userdata);

GtkWidget *add_sub_menu(GtkWidget *menu, gchar* label, gchar* stockid);

GtkWidget *add_exec_commands(GtkWidget *menu);

/**
 *  Add separator to Menu @m and return pointer to separator widget
 */
GtkWidget *add_separator(GtkWidget *menu);

void context_menu_delete_track_head(GtkMenuItem *mi, gpointer data);

GtkWidget *add_copy_track_to_filesystem (GtkWidget *menu);
GtkWidget *add_create_playlist_file (GtkWidget *menu);
GtkWidget *add_update_tracks_from_file (GtkWidget *menu);
GtkWidget *add_create_new_playlist (GtkWidget *menu);
GtkWidget *add_edit_track_details (GtkWidget *menu);

#endif
