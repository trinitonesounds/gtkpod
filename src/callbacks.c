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
#include "display.h"

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

/* parse a bunch of ipod ids delimited by \n
 * @s - address of the character string we're parsing
 * @id - pointer the ipod id parsed from the string
 * returns FALSE when the string is empty, TRUE when the string can still be
 * 	parsed
 */
static gboolean
parse_ipod_id_from_string(gchar **s, guint32 *id)
{
    if((s) && (*s))
    {
	int i = 0;
	gchar buf[4096];
	gchar *new = NULL;
	gchar *str = *s;
	guint max = strlen(str);

	for(i = 0; i < max; i++)
	{
	    if(str[i] == '\n')
	    {
		snprintf(buf, 4096, "%s", str);
		buf[i] = '\0';
		*id = (guint32)atoi(buf);
		if((i+1) < max)
		    new = g_strdup(&str[i+1]);
		break;
	    }
	}
	g_free(str);
	*s = new;
	return(TRUE);
    }
    return(FALSE);
}

void
on_playlist_treeview_drag_data_received
                                        (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data)
{
    GtkTreeIter i;
    GtkWidget *w = NULL;
    GtkTreePath *tp = NULL;
    GtkTreeModel *tm = NULL;
    GtkTreeViewDropPosition pos = 0;

    /* sometimes we get empty dnd data, ignore */
    if((!data) || (data->length < 0)) return;
    /* don't allow us to drag onto ourselves =) */
    w = gtk_drag_get_source_widget(drag_context);
    if(w == widget) return;
    /* yet another check, i think it's an 8 bit per byte check */
    if(data->format != 8) return;

    if(gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(widget), x, y, &tp,
		&pos))
    {
	/* 
	 * ensure a valid tree path, and that we're dropping ON a playlist
	 * not between 
	 */
	if((tp) && ((pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE) ||
	    (pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER))) 
	{
	    tm = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
	    if(gtk_tree_model_get_iter(tm, &i, tp))
	    {
		Playlist *pl = NULL;
		gtk_tree_model_get(tm, &i, 0, &pl, -1);
		if(pl)
		{
		    gchar *str = g_strdup(data->data);
		    guint32 id = 0;
		    while(parse_ipod_id_from_string(&str,&id))
		    {
			add_songid_to_playlist(pl, id);
		    }
		}
	    }
	    gtk_tree_path_free(tp);
	}
	else
	{
	    fprintf(stderr, "Unknown treepath droppage or badly positioned
				drop\n");
	}
    }
    fprintf(stderr, "Playlist dnd receieved\n");
}



void
on_song_treeview_drag_data_get         (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data)
{
    GtkTreeSelection *ts = NULL;
    
    if((data) && (ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget))))
    {
	gchar *reply = NULL;
	if(info == 1000)	/* gtkpod/file */
	{
	    gtk_tree_selection_selected_foreach(ts,
				    on_song_listing_drag_foreach, &reply);
	    if(reply)
	    {
		gtk_selection_data_set(data, data->target, 8, reply,
					strlen(reply));
		g_free(reply);
	    }
	}
	else if(info == 1001)
	{
	    fprintf(stderr, "received file of type \"text/plain\"\n");
	}
	else
	{
	    fprintf(stderr, "Unknown info %d\n", info);
	}
    }
}

