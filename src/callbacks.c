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

#include <gdk/gdkkeysyms.h>
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
#include "prefs_window.h"
#include "md5.h"
#include "delete_window.h"
#include "file_export.h"
#include "charset.h"

void
on_add_files1_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    create_add_files_fileselector ();
}


void
on_add_directory1_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  create_dir_browser ();
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
  gtkpod_main_quit ();
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
on_add_files1_button                   (GtkButton       *button,
                                        gpointer         user_data)
{
  create_add_files_fileselector ();
}


void
on_add_directory1_button               (GtkButton       *button,
                                        gpointer         user_data)
{
  create_dir_browser ();
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
  gtkpod_main_quit ();
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
		if((pl) && (pl->type == PL_TYPE_NORM))
		{
		    gchar *str = g_strdup(data->data);
		    guint32 id = 0;
		    while(parse_ipod_id_from_string(&str,&id))
		    {
			add_songid_to_playlist(pl, id);
		    }
		    data_changed();
		}
	    }
	    gtk_tree_path_free(tp);
	}
	else
	{
	    fprintf(stderr, "Unknown treepath droppage or badly "
			    "positioned drop\n");
	}
    }
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
    }
}


gboolean
on_prefs_window_delete_event           (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  prefs_window_cancel ();
  return FALSE;
}


void
on_cfg_mount_point_changed             (GtkEditable     *editable,
                                        gpointer         user_data)
{
    prefs_window_set_mount_point(gtk_editable_get_chars(editable,0, -1));
}


void
on_cfg_md5songs_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    prefs_window_set_md5songs_active(gtk_toggle_button_get_active(togglebutton));
}


void
on_cfg_writeid3_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    prefs_window_set_writeid3_active(gtk_toggle_button_get_active(togglebutton));
}


void
on_prefs_ok_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
    gboolean md5;

    md5 = cfg->md5songs;
    prefs_window_save();
    write_prefs();
    if(!md5 && cfg->md5songs)
	md5_unique_file_init(get_song_list());
    if(md5 && !cfg->md5songs)
	md5_unique_file_free();
    sm_show_preferred_columns();
}


void
on_prefs_cancel_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
    prefs_window_cancel();
}


void
on_edit_preferences1_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    prefs_window_create(); 
}

void
on_cfg_song_list_all_toggled           (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    prefs_window_set_song_list_all(gtk_toggle_button_get_active(togglebutton));
}

void
on_cfg_song_list_artist_toggled        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    prefs_window_set_song_list_artist(gtk_toggle_button_get_active(togglebutton));
}

void
on_cfg_song_list_album_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    prefs_window_set_song_list_album(gtk_toggle_button_get_active(togglebutton));
}

void
on_cfg_song_list_genre_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    prefs_window_set_song_list_genre(gtk_toggle_button_get_active(togglebutton));
}

void
on_cfg_song_list_track_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    prefs_window_set_song_list_track(gtk_toggle_button_get_active(togglebutton));
}

gboolean
on_playlist_treeview_key_release_event (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{
    guint mods;

    mods = event->state;

    if(mods & GDK_CONTROL_MASK)
    {
	switch(event->keyval)
	{
	    case GDK_d:
		confirmation_window_create(CONFIRMATION_WINDOW_PLAYLIST);
		break;
	    case GDK_n:
		add_new_playlist ();
		break;
	    default:
		break;
	}

    }
  return FALSE;
}


gboolean
on_song_treeview_key_release_event     (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{
    guint mods;
    mods = event->state;

    if(mods & GDK_CONTROL_MASK)
    {
	switch(event->keyval)
	{
	    case GDK_d:
		confirmation_window_create(CONFIRMATION_WINDOW_SONG);
		break;
	    default:
		break;
	}

    }
  return FALSE;
}


void
on_prefs_request_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    confirmation_window_prefs_toggled(gtk_toggle_button_get_active(togglebutton));
}


void
on_delete_ok_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
    confirmation_window_ok_clicked();
}


void
on_delete_cancel_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
    confirmation_window_cancel_clicked();

}


gboolean
on_delete_confirmation_delete_event    (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    confirmation_window_cancel_clicked();
    return FALSE;
}


void
on_export_files_to_disk_activate       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    file_export_init(get_currently_selected_songs());
}


void
on_cfg_delete_playlist_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    prefs_window_set_delete_playlist(
	    gtk_toggle_button_get_active(togglebutton));
}


void
on_cfg_delete_track_from_playlist_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    prefs_window_set_delete_song_playlist(
	    gtk_toggle_button_get_active(togglebutton));
}


void
on_cfg_delete_track_from_ipod_toggled  (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    prefs_window_set_delete_song_ipod(
	    gtk_toggle_button_get_active(togglebutton));
}


void
on_cfg_autoimport_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    prefs_window_set_auto_import(
	    gtk_toggle_button_get_active(togglebutton));
}


void
on_cfg_keep_backups_toggled            (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    prefs_window_set_keep_backups(
	    gtk_toggle_button_get_active(togglebutton));
}


void
on_cfg_write_extended_info_toggled     (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    prefs_window_set_write_extended_info(
	    gtk_toggle_button_get_active(togglebutton));
}


void
on_offline1_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  prefs_set_offline (
     gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)));
}

void
on_import_itunes_mi_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  handle_import ();
}


void
on_import_button_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
  handle_import ();
}


void
on_song_treeview_drag_data_received    (GtkWidget       *widget,
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
    /* allow us to drag only onto ourselves =) */
    w = gtk_drag_get_source_widget(drag_context);
    if(w != widget) return;
    /* yet another check, i think it's an 8 bit per byte check */
    if(data->format != 8) return;

    if(gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(widget), x, y, &tp,
		&pos))
    {
	/* 
	 * ensure a valid tree path, and that we're dropping BEFORE or AFTER
	 * a track not onto
	 */
	if((tp) && ((pos == GTK_TREE_VIEW_DROP_BEFORE) ||
	    (pos == GTK_TREE_VIEW_DROP_AFTER))) 
	{
	    tm = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
	    if(gtk_tree_model_get_iter(tm, &i, tp))
	    {
		Song *s = NULL;
		gtk_tree_model_get(tm, &i, 0, &s, -1);
		if(s)
		{
		    guint32 id = 0;
		    Song *new_song = NULL;
		    Playlist *current_pl = NULL;
		    gchar *str = g_strdup(data->data);
		   
		    if((current_pl = get_currently_selected_playlist()) &&
			    (current_pl->type != PL_TYPE_MPL))
		    {
			guint insert_index = -1;
			GList *new_list = NULL, *members = NULL, *l = NULL;

			members = g_list_copy(current_pl->members);
			/* build a list of songs to append */
			while(parse_ipod_id_from_string(&str,&id))
			{
			    if((new_song = get_song_by_id(id)))
			    {
				members = g_list_remove(members, new_song);
				new_list = g_list_append(new_list, new_song);
			    }
			}

			/* find where we wanna put the new tracks */
			if((insert_index = g_list_index(members, s)) != -1)
			{
			    switch(pos)
			    {
				case GTK_TREE_VIEW_DROP_AFTER:
				    insert_index++;
				case GTK_TREE_VIEW_DROP_BEFORE:
				    for(l = new_list; l; l = l->next)
				    {
					new_song = (Song*)l->data;
					members = g_list_insert(members,
						new_song, insert_index++);
				    }
				    break;
				default:
				    break;
			    }
			}
			if(new_list) 
			    g_list_free(new_list);
			if(current_pl->members) 
			    g_list_free(current_pl->members);
			current_pl->members = members;
			pm_select_playlist_reinit(current_pl);
			data_changed();
		    }
		    else
		    {
			g_free(str);
		    }
		}
	    }
	    gtk_tree_path_free(tp);
	}
    }
}


void
on_charset_combo_entry_changed          (GtkEditable     *editable,
                                        gpointer         user_data)
{
    gchar *descr, *charset;

    descr = gtk_editable_get_chars (editable, 0, -1);
    charset = charset_from_description (descr);
    prefs_window_set_charset (charset);
    C_FREE (descr);
    C_FREE (charset);
}

void
on_delete_song_menu                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    confirmation_window_create(CONFIRMATION_WINDOW_SONG);
}


void
on_delete_playlist_menu                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    confirmation_window_create(CONFIRMATION_WINDOW_PLAYLIST);
}


void
on_ipod_directories_menu               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_alphabetize0_up_menu                   (GtkMenuItem     *menuitem,
					   gpointer         user_data)
{
    st_sort (0, GTK_SORT_ASCENDING);
}


void
on_alphabetize0_down_menu                   (GtkMenuItem     *menuitem,
					     gpointer         user_data)
{
    st_sort (0, GTK_SORT_DESCENDING);
}


void
on_re_init0_menu                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    st_redisplay (0);
}


void
on_alphabetize1_up_menu                   (GtkMenuItem     *menuitem,
					   gpointer         user_data)
{
    st_sort (1, GTK_SORT_ASCENDING);
}


void
on_alphabetize1_down_menu                   (GtkMenuItem     *menuitem,
					     gpointer         user_data)
{
    st_sort (1, GTK_SORT_DESCENDING);
}


void
on_re_init1_menu                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    st_redisplay (1);
}


void
on_cfg_st_autoselect_toggled           (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    prefs_window_set_st_autoselect(
	(guint32)user_data,
	gtk_toggle_button_get_active(togglebutton));
}
