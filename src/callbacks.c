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

#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#include "misc.h"
#include "prefs.h"
#include "itunesdb.h"
#include "dirbrowser.h"
#include "song.h"
#include "playlist.h"


void
on_import_itunes1_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  handle_import_itunes ();
}


void
on_add_files1_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  create_add_files_fileselector (cfg->last_dir);
}


void
on_add_directory1_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkWidget *browser;

  browser = xmms_create_dir_browser (_("Select directory to add recursively"),
				     cfg->last_dir,
				     GTK_SELECTION_SINGLE,
				     add_dir_selected);
  gtk_widget_show (browser);
}


void
on_export_itunes1_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  handle_export ();
}


void
on_quit1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  remove_all_songs ();
  gtk_main_quit ();
}


void
on_new_playlist1_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  add_new_playlist ();
}


void
on_cut1_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_copy1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_paste1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_delete1_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  open_about_window (); /* in misc.c */
}


void
on_import_itunes1_button               (GtkButton       *button,
                                        gpointer         user_data)
{
  handle_import_itunes ();
}


void
on_add_files1_button                   (GtkButton       *button,
                                        gpointer         user_data)
{
  create_add_files_fileselector (cfg->last_dir);
}


void
on_add_directory1_button               (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *browser;

  browser = xmms_create_dir_browser (_("Select directory to add recursively"),
				     cfg->last_dir,
				     GTK_SELECTION_SINGLE,
				     add_dir_selected);
  gtk_widget_show (browser);
}
void
on_export_itunes1_button               (GtkButton       *button,
                                        gpointer         user_data)
{
  handle_export ();
}

gboolean
on_gtkpod_delete_event                 (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  remove_all_playlists ();
  remove_all_songs ();
  gtk_main_quit ();
  return FALSE;
}

gboolean
on_about_window_close                  (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  close_about_window (); /* in misc.c */
  return FALSE;
}


void
on_about_window_close_button           (GtkButton       *button,
                                        gpointer         user_data)
{
  close_about_window (); /* in misc.c */
}

void
on_new_playlist_button                 (GtkButton       *button,
                                        gpointer         user_data)
{
  add_new_playlist ();
}

void
on_sorttab_switch_page                 (GtkNotebook     *notebook,
                                        GtkNotebookPage *page,
                                        guint            page_num,
                                        gpointer         user_data)
{
  st_page_selected (notebook, page_num);
}
