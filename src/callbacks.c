/* Time-stamp: <2004-02-03 23:21:30 JST jcs>
|
|  Copyright (C) 2002-2003 Jorg Schuler <jcsjcs at users.sourceforge.net>
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

/* Most function prototypes in this file were written by glade2. */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

#include "charset.h"
#include "dirbrowser.h"
#include "display.h"
#include "file.h"
#include "file_export.h"
#include "info.h"
#include "misc.h"
#include "normalize.h"
#include "playlist.h"
#include "prefs.h"
#include "prefs_window.h"
#include "support.h"

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
  dirbrowser_create ();
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
  if (!widgets_blocked) gtkpod_main_quit ();
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
  dirbrowser_create ();
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
    if (!widgets_blocked)
    {
	return gtkpod_main_quit ();
    }
    return TRUE; /* don't quit -- would cause numerous error messages */
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
  add_new_playlist_user_name (NULL, -1);
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
on_playlist_treeview_drag_data_get     (GtkWidget       *widget,
					GdkDragContext  *context,
					GtkSelectionData *data,
					guint            info,
					guint            time,
					gpointer         user_data)
{
    GtkTreeSelection *ts = NULL;
    GString *reply = g_string_sized_new (2000);

    /* printf("sm drag get info: %d\n", info);*/
    if((data) && (ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget))))
    {
	switch (info)
	{
	case DND_GTKPOD_IDLIST:
	    gtk_tree_selection_selected_foreach(ts,
				    on_pm_dnd_get_id_foreach, reply);
	    break;
	case DND_GTKPOD_PM_PATHLIST:
	    gtk_tree_selection_selected_foreach(ts,
				    on_dnd_get_path_foreach, reply);
	    break;
	case DND_TEXT_PLAIN:
	    gtk_tree_selection_selected_foreach(ts,
				    on_pm_dnd_get_file_foreach, reply);
	    break;
	}
    }
    gtk_selection_data_set(data, data->target, 8, reply->str, reply->len);
    g_string_free (reply, TRUE);
}


/* NOTE: if we do it the "right way" by having this function called as
   a 'drag_end' callback, we run into the following problem: This is
   also called with context->action==GDK_ACTION_MOVE if tracks are
   reordered using drag and drop or when dropping into the master
   playlist, irrespective of the arguments of
   'gtk_drop_finish()'. Therefore this function is called directly by
   on_playlist_treeview_drag_data_received() */
static void
tracks_moved_or_copied     (GdkDragContext  *context, gchar *trackids)
{
/*      printf ("ttracks_moved_or_copied ts/dc/action: %p/%d\n", context, context?context->action:0);  */
    if(trackids && context)
    {
	gint n=0;
	gchar *buf = NULL;
	gchar *ptr = trackids;
	Playlist *pl = pm_get_selected_playlist ();

	/* count the number of ids */
	while ((ptr=strchr (ptr, '\n')))
	{
	    ++n;
	    ++ptr;
	}

	if (pl && (pl->type == PL_TYPE_NORM) &&
	    (context->action == GDK_ACTION_MOVE))
	{
	    guint32 id = 0;
	    gchar *str = g_strdup (trackids);

	    while(parse_ipod_id_from_string(&str,&id))
	    {
		remove_trackid_from_playlist (pl, id);
	    }
	    g_free (str);

	    buf = g_strdup_printf (
		ngettext ("Moved one track",
			  "Moved %d tracks", n), n);
	}
	else if (pl)
	{
	    buf = g_strdup_printf (
		ngettext ("Copied one track",
			  "Copied %d tracks", n), n);
	}

	if (buf)
	{
	    gtkpod_statusbar_message (buf);
	    g_free (buf);
	}
    }
}


void
on_playlist_treeview_drag_data_received
					(GtkWidget       *widget,
					GdkDragContext  *context,
					gint             x,
					gint             y,
					GtkSelectionData *data,
					guint            info,
					guint            time,
					gpointer         user_data)
{
    GtkTreeIter i;
    GtkTreePath *path = NULL;
    GtkTreeModel *model = NULL;
    GtkTreeViewDropPosition pos = 0;
    gint position = -1;
    Playlist *pl = NULL;


/*     printf ("treeview received drag data/length/format: %p/%d/%d\n", data, data?data->length:0, data?data->format:0); */
/*     printf ("treeview received drag context/actions/suggested action: %p/%d/%d\n", context, context?context->actions:0, context?context->suggested_action:0); */
    /* sometimes we get empty dnd data, ignore */
    if((!context) || (!data) || (data->length < 0)) return;
    /* yet another check, i think it's an 8 bit per byte check */
    if(data->format != 8) return;
    if(gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(widget),
					 x, y, &path, &pos))
    {
	gboolean del_src;

	if (context && (context->suggested_action & GDK_ACTION_MOVE))
	     del_src = TRUE;
	else del_src = FALSE;
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
	if(gtk_tree_model_get_iter(model, &i, path))
	{
	    gtk_tree_model_get(model, &i, 0, &pl, -1);
	}
	/* get position of current path */
	position = atoi (gtk_tree_path_to_string (path));
	/* adjust position */
	if (pos == GTK_TREE_VIEW_DROP_AFTER)  ++position;
	/* don't allow drop _before_ MPL */
	if (position == 0) ++position;
	switch (info)
	{
	case DND_GTKPOD_IDLIST:
	    if(pl)
	    {
		if ((pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE) ||
		    (pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER))
		{ /* drop into existing playlist */
		    if (pl->type == PL_TYPE_NORM)
		    {
			add_idlist_to_playlist (pl, data->data);
			/* this is a hack -- see comment at
			   tracks_moved_or_copied */
			tracks_moved_or_copied (context, data->data);
			gtk_drag_finish (context, TRUE, del_src, time);
		    }
		    else
		    {
			gtk_drag_finish (context, FALSE, FALSE, time);
		    }
		}
		else
		{ /* drop between playlists */
		    Playlist *plitem;
		    plitem = add_new_playlist_user_name (NULL, position);
		    if (plitem)
		    {
			add_idlist_to_playlist (plitem, data->data);
			/* this is a hack -- see comment at
			   tracks_moved_or_copied */
			tracks_moved_or_copied (context, data->data);
			gtk_drag_finish (context, TRUE, del_src, time);
		    }
		    else
		    {
			gtk_drag_finish (context, FALSE, FALSE, time);
		    }
		}
	    }
	    else gtk_drag_finish (context, FALSE, FALSE, time);
	    break;
	case DND_TEXT_PLAIN:
	    if(pl)
	    {
		if ((pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE) ||
		    (pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER))
		{ /* drop into existing playlist */
		    add_text_plain_to_playlist (pl, data->data, 0, NULL, NULL);
		    gtk_drag_finish (context, TRUE, FALSE, time);
		}
		else
		{ /* drop between playlists */
		    add_text_plain_to_playlist (NULL, data->data, position,
						NULL, NULL);
		    gtk_drag_finish (context, TRUE, del_src, time);
		}
	    }
	    else gtk_drag_finish (context, FALSE, FALSE, time);
	    break;
	case DND_GTKPOD_PM_PATHLIST:
	    /* dont allow moves before MPL */
	    position = atoi (gtk_tree_path_to_string (path));
	    if (position == 0)  pos = GTK_TREE_VIEW_DROP_AFTER;
	    pm_move_pathlist (data->data, path, pos);
	    gtk_drag_finish (context, TRUE, FALSE, time);
	    break;
	default:
	    puts ("not yet implemented");
	    gtk_drag_finish (context, FALSE, FALSE, time);
	    break;
	}
	gtk_tree_path_free(path);
    }
    else   gtk_drag_finish (context, FALSE, FALSE, time);
}



void
on_track_treeview_drag_data_get        (GtkWidget       *widget,
					GdkDragContext  *context,
					GtkSelectionData *data,
					guint            info,
					guint            time,
					gpointer         user_data)
{
    GtkTreeSelection *ts = NULL;
    GString *reply = g_string_sized_new (2000);

/*     printf("tm drag get info: %d\n", info); */
    if((data) && (ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget))))
    {
	switch (info)
	{
	case DND_GTKPOD_IDLIST:
	    gtk_tree_selection_selected_foreach(ts,
				    on_tm_dnd_get_id_foreach, reply);
	    break;
	case DND_GTKPOD_TM_PATHLIST:
	    gtk_tree_selection_selected_foreach(ts,
				    on_dnd_get_path_foreach, reply);
	    break;
	case DND_TEXT_PLAIN:
	    gtk_tree_selection_selected_foreach(ts,
				    on_tm_dnd_get_file_foreach, reply);
	    break;
	}
    }
    gtk_selection_data_set(data, data->target, 8, reply->str, reply->len);
    g_string_free (reply, TRUE);
}


gboolean
on_prefs_window_delete_event           (GtkWidget       *widget,
					GdkEvent        *event,
					gpointer         user_data)
{
  prefs_window_delete ();
  gtkpod_statusbar_message(_("Preferences not updated"));
  return FALSE;
}


void
on_cfg_mount_point_changed             (GtkEditable     *editable,
					gpointer         user_data)
{
    gchar *buf = gtk_editable_get_chars(editable,0, -1);
    prefs_window_set_mount_point(buf);
    g_free (buf);
}


void
on_cfg_md5tracks_toggled                (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    prefs_window_set_md5tracks(gtk_toggle_button_get_active(togglebutton));
}


void
on_cfg_id3_write_toggled                (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    prefs_window_set_id3_write(gtk_toggle_button_get_active(togglebutton));
}


void
on_prefs_ok_clicked                    (GtkButton       *button,
					gpointer         user_data)
{
    prefs_window_ok();
}


void
on_prefs_cancel_clicked                (GtkButton       *button,
					gpointer         user_data)
{
    prefs_window_cancel();
    gtkpod_statusbar_message(_("Preferences not updated"));
}


void
on_prefs_apply_clicked                 (GtkButton       *button,
					gpointer         user_data)
{
    prefs_window_apply ();
    gtkpod_statusbar_message(_("Preferences applied"));
}

void
on_edit_preferences1_activate          (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    if(!widgets_blocked)  prefs_window_create();
}

gboolean
on_playlist_treeview_key_release_event (GtkWidget       *widget,
					GdkEventKey     *event,
					gpointer         user_data)
{
    guint mods;

    mods = event->state;

    if(!widgets_blocked && (mods & GDK_CONTROL_MASK))
    {
	switch(event->keyval)
	{
	    case GDK_u:
		do_selected_playlist (update_trackids);
		break;
	    case GDK_n:
		add_new_playlist_user_name (NULL, -1);
		break;
	    default:
		break;
	}

    }
  return FALSE;
}


gboolean
on_track_treeview_key_release_event     (GtkWidget       *widget,
					GdkEventKey     *event,
					gpointer         user_data)
{
    guint mods;
    mods = event->state;

    if(!widgets_blocked && (mods & GDK_CONTROL_MASK))
    {
	switch(event->keyval)
	{
	    case GDK_d:
		delete_track_head (FALSE);
		break;
	    case GDK_u:
		do_selected_tracks (update_trackids);
		break;
	    default:
		break;
	}
    }
    return FALSE;
}


void
on_cfg_delete_track_from_playlist_toggled
					(GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    prefs_window_set_delete_track_playlist(
	    gtk_toggle_button_get_active(togglebutton));
}


void
on_cfg_delete_track_from_ipod_toggled  (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    prefs_window_set_delete_track_ipod(
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
  display_set_check_ipod_menu ();
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
on_track_treeview_drag_data_received    (GtkWidget       *widget,
					GdkDragContext  *context,
					gint             x,
					gint             y,
					GtkSelectionData *data,
					guint            info,
					guint            time,
					gpointer         user_data)
{
    GtkTreePath *path = NULL;
    GtkTreeModel *model = NULL;
    GtkTreeViewDropPosition pos = 0;
    gboolean result = FALSE;

    /* printf ("sm drop received info: %d\n", info); */

    /* sometimes we get empty dnd data, ignore */
    if(widgets_blocked || (!context) ||
       (!data) || (data->length < 0)) return;
    /* yet another check, i think it's an 8 bit per byte check */
    if(data->format != 8) return;

    model = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
    if(gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(widget),
					 x, y, &path, &pos))
    {
	switch (info)
	{
	case DND_GTKPOD_TM_PATHLIST:
	    result = tm_move_pathlist (data->data, path, pos);
	    break;
	case DND_GTKPOD_IDLIST:
	    /* is disabled in tm_drop_types anyhow (display.c) */
	    printf ("idlist not supported yet\n");
	    break;
	case DND_TEXT_PLAIN:
	    result = tm_add_filelist (data->data, path, pos);
	    break;
	default:
	    g_warning ("Programming error: on track_treeview_drag_data_received: unknown drop: not supported: %d\n", info);
	    break;
	}
	gtk_tree_path_free(path);
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
    g_free (descr);
    g_free (charset);
}

void
on_delete_tracks_activate               (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    delete_track_head (FALSE);
}


void
on_delete_playlist_activate                (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    delete_playlist_head (FALSE);
}

void
on_delete_tab_entry_activate           (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    gint inst = get_sort_tab_number (
	_("Delete selected entry of which sort tab?"));

    if (inst != -1)   delete_entry_head (inst, FALSE);
}

void
on_delete_full_tracks_activate               (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
     delete_track_head (TRUE);
}


void
on_delete_full_playlist_activate                (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    delete_playlist_head (TRUE);
}

void
on_delete_full_tab_entry_activate           (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    gint inst = get_sort_tab_number (
	_("Delete selected entry of which sort tab?"));

    if (inst != -1)
    {
	delete_entry_head (inst, TRUE);
    }
}

void
on_ipod_directories_menu               (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    ipod_directories_head ();
}
void
on_gtkpod_status_realize               (GtkWidget       *widget,
					gpointer         user_data)
{
    gtkpod_statusbar_init(widget);
}


void
on_tracks_statusbar_realize             (GtkWidget       *widget,
					gpointer         user_data)
{
    gtkpod_tracks_statusbar_init(widget);
}

void
on_st_treeview_drag_data_get           (GtkWidget       *widget,
					GdkDragContext  *context,
					GtkSelectionData *data,
					guint            info,
					guint            time,
					gpointer         user_data)
{
    GtkTreeSelection *ts = NULL;

    if((data) && (ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget))))
    {
	if(info == DND_GTKPOD_IDLIST)	/* gtkpod/file */
	{
	    GString *reply = g_string_sized_new (2000);
	    gtk_tree_selection_selected_foreach(ts,
				    on_st_listing_drag_foreach, reply);
	    if(reply->len)
	    {
		gtk_selection_data_set(data, data->target, 8, reply->str,
				       reply->len);
	    }
	    g_string_free (reply, TRUE);
	}
	else if(info == DND_TEXT_PLAIN)
	{
	    /* FIXME: not implemented yet -- must also change
	     * st_drag_types in display.c */
	    g_warning ("Programming error: on_st_treeview_drag_data_get: received file of type 'text/plain'\n");
	}
    }

}

/* delete selected entry in sort tab */
gboolean
on_st_treeview_key_release_event       (GtkWidget       *widget,
					GdkEventKey     *event,
					gpointer         user_data)
{
    guint mods;
    mods = event->state;

    if(!widgets_blocked && (mods & GDK_CONTROL_MASK))
    {
	switch(event->keyval)
	{
	    case GDK_d:
		delete_entry_head (st_get_instance_from_treeview (
				       GTK_TREE_VIEW (widget)), FALSE);
		break;
	    case GDK_u:
		do_selected_entry (update_trackids,
				   st_get_instance_from_treeview (
				       GTK_TREE_VIEW (widget)));
		break;
	    default:
		break;
	}

    }
  return FALSE;
}

void
on_cfg_mpl_autoselect_toggled          (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    prefs_window_set_mpl_autoselect(gtk_toggle_button_get_active(togglebutton));
}


void
on_cfg_block_display_toggled           (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    prefs_window_set_block_display(gtk_toggle_button_get_active(togglebutton));
}

void
on_stop_button_clicked                 (GtkButton       *button,
					gpointer         user_data)
{
    display_stop_update (-1);
}

void
on_add_PL_button_clicked               (GtkButton       *button,
					gpointer         user_data)
{
    create_add_playlists_fileselector ();
}


void
on_add_playlist1_activate              (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    create_add_playlists_fileselector ();
}

void
on_update_playlist_activate (GtkMenuItem     *menuitem,
			     gpointer         user_data)
{
    do_selected_playlist (update_trackids);
}

/* update tracks in tab entry */
void
on_update_tab_entry_activate        (GtkMenuItem     *menuitem,
				     gpointer         user_data)
{
    gint inst = get_sort_tab_number (
	_("Update selected entry of which sort tab?"));

    if (inst != -1) do_selected_entry (update_trackids, inst);
}

void
on_update_tracks_activate            (GtkMenuItem     *menuitem,
				     gpointer         user_data)
{
    do_selected_tracks (update_trackids);
}


void
on_sync_playlist_activate (GtkMenuItem     *menuitem,
			     gpointer         user_data)
{
    do_selected_playlist (sync_trackids);
}

/* sync tracks in tab entry */
void
on_sync_tab_entry_activate        (GtkMenuItem     *menuitem,
				     gpointer         user_data)
{
    gint inst = get_sort_tab_number (
	_("Sync dirs of selected entry in which sort tab?"));

    if (inst != -1) do_selected_entry (sync_trackids, inst);
}

void
on_sync_tracks_activate            (GtkMenuItem     *menuitem,
				     gpointer         user_data)
{
    do_selected_tracks (sync_trackids);
}


void
on_cfg_update_existing_toggled         (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    prefs_window_set_update_existing(
	gtk_toggle_button_get_active(togglebutton));
}

void
on_save_track_order1_activate           (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    tm_rows_reordered ();
    pm_rows_reordered ();
}


void
on_cfg_show_duplicates_toggled         (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    prefs_window_set_show_duplicates(
	gtk_toggle_button_get_active(togglebutton));

}


void
on_cfg_show_updated_toggled            (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    prefs_window_set_show_updated(
	gtk_toggle_button_get_active(togglebutton));

}


void
on_cfg_show_non_updated_toggled        (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    prefs_window_set_show_non_updated(
	gtk_toggle_button_get_active(togglebutton));

}


void
on_cfg_show_sync_dirs_toggled        (GtkToggleButton *togglebutton,
				      gpointer         user_data)
{
    prefs_window_set_show_sync_dirs(
	gtk_toggle_button_get_active(togglebutton));

}


void
on_toolbar_menu_activate               (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    prefs_set_display_toolbar (
	gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)));
}


void
on_cfg_display_toolbar_toggled         (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    prefs_window_set_display_toolbar(
	gtk_toggle_button_get_active(togglebutton));
}


void
on_more_sort_tabs_activate             (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    prefs_set_sort_tab_num (prefs_get_sort_tab_num()+1, TRUE);
}


void
on_less_sort_tabs_activate             (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    prefs_set_sort_tab_num (prefs_get_sort_tab_num()-1, TRUE);
}

void
on_cfg_toolbar_style_both_toggled      (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    if (gtk_toggle_button_get_active(togglebutton))
    {
	prefs_window_set_toolbar_style (GTK_TOOLBAR_BOTH);
    }
}


void
on_cfg_toolbar_style_text_toggled      (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    if (gtk_toggle_button_get_active(togglebutton))
    {
	prefs_window_set_toolbar_style (GTK_TOOLBAR_TEXT);
    }
}


void
on_cfg_toolbar_style_icons_toggled      (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    if (gtk_toggle_button_get_active(togglebutton))
    {
	prefs_window_set_toolbar_style (GTK_TOOLBAR_ICONS);
    }
}

void
on_play_now_path_entry_changed         (GtkEditable     *editable,
					gpointer         user_data)
{
    gchar *buf = gtk_editable_get_chars(editable,0, -1);
    prefs_window_set_play_now_path (buf);
    g_free (buf);
}


void
on_play_enqueue_path_entry_changed     (GtkEditable     *editable,
					gpointer         user_data)
{
    gchar *buf = gtk_editable_get_chars(editable,0, -1);
    prefs_window_set_play_enqueue_path (buf);
    g_free (buf);
}


void
on_mp3gain_entry_changed               (GtkEditable     *editable,
					gpointer         user_data)
{
    gchar *buf = gtk_editable_get_chars(editable,0, -1);
    prefs_window_set_mp3gain_path (buf);
    g_free (buf);
}


void
on_cfg_automount_ipod_toggled          (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    prefs_window_set_automount(gtk_toggle_button_get_active(togglebutton));
}


void
on_export_playlist_activate  (GtkMenuItem     *menuitem,
			      gpointer         user_data)
{
    Playlist *pl = pm_get_selected_playlist ();

    if (!pl)
    {
	gtkpod_statusbar_message (_("No playlist selected"));
	return;
    }
    file_export_init (pl->members);
}


void
on_export_tab_entry_activate (GtkMenuItem     *menuitem,
			      gpointer         user_data)
{
    TabEntry *entry;
    gint inst;

    inst = get_sort_tab_number (_("Export selected entry of which sort tab?"));
    if (inst == -1) return;

    entry = st_get_selected_entry (inst);
    if (!entry)
    {
	gchar *str = g_strdup_printf(_("No entry selected in Sort Tab %d"),
				     inst+1);
	gtkpod_statusbar_message (str);
	g_free (str);
	return;
    }
    file_export_init (entry->members);
}


void
on_export_tracks_activate     (GtkMenuItem     *menuitem,
			      gpointer         user_data)
{
    GList *tracks = tm_get_selected_tracks ();

    if (tracks)
    {
	file_export_init(tracks);
	g_list_free (tracks);
    }
    else
    {
	gtkpod_statusbar_message (_("No tracks selected"));
    }
}


void
on_play_playlist_activate              (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    Playlist *pl = pm_get_selected_playlist ();
    if (pl)
	play_tracks (pl->members);
    else
	gtkpod_statusbar_message (_("No playlist selected"));
}


void
on_play_tab_entry_activate             (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    TabEntry *entry;
    gint inst;

    inst = get_sort_tab_number (_("Play tracks in selected entry of which sort tab?"));
    if (inst == -1) return;

    entry = st_get_selected_entry (inst);
    if (!entry)
    {
	gchar *str = g_strdup_printf(_("No entry selected in Sort Tab %d"),
				     inst+1);
	gtkpod_statusbar_message (str);
	g_free (str);
	return;
    }
    play_tracks (entry->members);
}


void
on_play_tracks_activate                 (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    GList *tracks = tm_get_selected_tracks ();
    if (tracks)
    {
	play_tracks (tracks);
	g_list_free (tracks);
	tracks = NULL;
    }
    else
	gtkpod_statusbar_message (_("No tracks selected"));
}


void
on_enqueue_playlist_activate           (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    Playlist *pl = pm_get_selected_playlist ();
    if (pl)
	enqueue_tracks (pl->members);
    else
	gtkpod_statusbar_message (_("No playlist selected"));
}


void
on_enqueue_tab_entry_activate          (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    TabEntry *entry;
    gint inst;

    inst = get_sort_tab_number (_("Enqueue tracks in selected entry of which sort tab?"));
    if (inst == -1) return;

    entry = st_get_selected_entry (inst);
    if (!entry)
    {
	gchar *str = g_strdup_printf(_("No entry selected in Sort Tab %d"),
				     inst+1);
	gtkpod_statusbar_message (str);
	g_free (str);
	return;
    }
    enqueue_tracks (entry->members);
}


void
on_enqueue_tracks_activate              (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    GList *tracks = tm_get_selected_tracks ();
    if (tracks)
    {
	enqueue_tracks (tracks);
	g_list_free (tracks);
	tracks = NULL;
    }
    else
	gtkpod_statusbar_message (_("No tracks selected"));
}


void
on_arrange_sort_tabs_activate          (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    st_arrange_visible_sort_tabs ();
}


void
on_space_statusbar_realize             (GtkWidget       *widget,
					gpointer         user_data)
{
    gtkpod_space_statusbar_init(widget);
}


void
on_cfg_update_charset_toggled          (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    prefs_window_set_update_charset(
	gtk_toggle_button_get_active(togglebutton));
}


void
on_cfg_write_charset_toggled           (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    prefs_window_set_write_charset(
	gtk_toggle_button_get_active(togglebutton));
}


void
on_cfg_add_recursively_toggled         (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    prefs_window_set_add_recursively(
	gtk_toggle_button_get_active(togglebutton));
}

void
on_cfg_sync_remove_toggled             (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    prefs_window_set_sync_remove(
	gtk_toggle_button_get_active(togglebutton));
}


void
on_cfg_sync_remove_confirm_toggled     (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    prefs_window_set_sync_remove_confirm(
	gtk_toggle_button_get_active(togglebutton));
}


void
on_cfg_time_format_changed             (GtkEditable     *editable,
					gpointer         user_data)
{
    gchar *buf = gtk_editable_get_chars(editable,0, -1);
    prefs_window_set_time_format (buf);
    g_free (buf);
}


void
on_sp_or_button_toggled                (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    guint32 inst = (guint32)user_data & SP_MASK;

    prefs_set_sp_or (inst, gtk_toggle_button_get_active (togglebutton));
    sp_conditions_changed (inst);
}


void
on_sp_cond_button_toggled            (GtkToggleButton *togglebutton,
				      gpointer         user_data)
{
    guint32 inst = (guint32)user_data & SP_MASK;
    T_item cond = (guint32)user_data >> SP_SHIFT;

    prefs_set_sp_cond (inst, cond,
		       gtk_toggle_button_get_active (togglebutton));
    sp_conditions_changed (inst);
}

void
on_sp_rating_n_toggled                 (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    guint32 inst = (guint32)user_data & SP_MASK;
    guint32 n = (guint32)user_data >> SP_SHIFT;

    prefs_set_sp_rating_n (inst, n,
			   gtk_toggle_button_get_active (togglebutton));
    if (prefs_get_sp_cond (inst, T_RATING))
	sp_conditions_changed (inst);
}


void
on_sp_entry_activate             (GtkEditable     *editable,
				  gpointer         user_data)
{
    guint32 inst = (guint32)user_data & SP_MASK;
    T_item item = (guint32)user_data >> SP_SHIFT;
    gchar *buf = gtk_editable_get_chars(editable,0, -1);

/*    printf ("sp_entry_activate inst: %d, item: %d\n", inst, item);*/

    prefs_set_sp_entry (inst, item, buf);
    g_free (buf);
    st_update_date_interval_from_string (inst, item, TRUE);
/*     if (prefs_get_sp_autodisplay (inst))  sp_go (inst); */
    sp_go (inst);
}


void
on_sp_cal_button_clicked        (GtkButton       *button,
				 gpointer         user_data)
{
    guint32 inst = (guint32)user_data & SP_MASK;
    T_item item = (guint32)user_data >> SP_SHIFT;

    cal_open_calendar (inst, item);
}


void
on_sp_go_clicked                       (GtkButton       *button,
					gpointer         user_data)
{
    guint32 inst = (guint32)user_data & SP_MASK;
    sp_go (inst);
}


void
on_sp_go_always_toggled                (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    guint32 inst = (guint32)user_data & SP_MASK;
    gboolean state = gtk_toggle_button_get_active (togglebutton);

    /* display data if autodisplay is turned on */
    if (state)  on_sp_go_clicked (NULL, user_data);
    prefs_set_sp_autodisplay(inst, state);
}

void
on_sp_playcount_low_value_changed      (GtkSpinButton   *spinbutton,
					gpointer         user_data)
{
    guint32 inst = (guint32)user_data & SP_MASK;

    prefs_set_sp_playcount_low (inst,
				gtk_spin_button_get_value (spinbutton));
    if (prefs_get_sp_cond (inst, T_PLAYCOUNT))
	sp_conditions_changed (inst);
}


void
on_sp_playcount_high_value_changed     (GtkSpinButton   *spinbutton,
					gpointer         user_data)
{
    guint32 inst = (guint32)user_data & SP_MASK;

    prefs_set_sp_playcount_high (inst,
				 gtk_spin_button_get_value (spinbutton));
    if (prefs_get_sp_cond (inst, T_PLAYCOUNT))
	sp_conditions_changed (inst);
}

void
on_cfg_sort_tab_num_sb_value_changed   (GtkSpinButton   *spinbutton,
					gpointer         user_data)
{
    prefs_window_set_sort_tab_num (gtk_spin_button_get_value (spinbutton));
}

void
on_tooltips_menu_activate              (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    prefs_set_display_tooltips_main (
	gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)));
}


void
on_cfg_display_tooltips_main_toggled   (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    prefs_window_set_display_tooltips_main (
	gtk_toggle_button_get_active (togglebutton));
}


void
on_cfg_display_tooltips_prefs_toggled  (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    prefs_window_set_display_tooltips_prefs (
	gtk_toggle_button_get_active (togglebutton));
}


void
on_cfg_multi_edit_toggled              (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    prefs_window_set_multi_edit (
	gtk_toggle_button_get_active (togglebutton));
}


void
on_cfg_multi_edit_title_toggled        (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    prefs_window_set_multi_edit_title (
	gtk_toggle_button_get_active (togglebutton));
}


void
on_new_playlist1_activate              (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
  add_new_playlist (NULL, -1);
}

void
on_pl_containing_displayed_tracks_activate (GtkMenuItem     *menuitem,
					    gpointer         user_data)
{
    generate_displayed_playlist ();
}

void
on_pl_containing_selected_tracks_activate (GtkMenuItem     *menuitem,
					    gpointer         user_data)
{
    generate_selected_playlist ();
}

void
on_pl_for_each_artist_activate         (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    generate_category_playlists (T_ARTIST);
}

void
on_pl_for_each_album_activate         (GtkMenuItem     *menuitem,
				       gpointer         user_data)
{
    generate_category_playlists (T_ALBUM);
}

void
on_pl_for_each_genre_activate         (GtkMenuItem     *menuitem,
				       gpointer         user_data)
{
    generate_category_playlists (T_GENRE);
}

void
on_pl_for_each_composer_activate         (GtkMenuItem     *menuitem,
					  gpointer         user_data)
{
    generate_category_playlists (T_COMPOSER);
}


void
on_most_listened_tracks1_activate       (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    most_listened_pl();
}


void
on_all_tracks_never_listened_to1_activate
					(GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    never_listened_pl();
}

void
on_most_rated_tracks_playlist_s1_activate
					(GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    most_rated_pl();
}


void
on_most_recent_played_tracks_activate   (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    last_listened_pl();
}

void
on_played_since_last_time1_activate    (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    since_last_pl();
}


void
on_rebuild_ipod_db1_activate           (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
/*    rebuild_iTunesDB();*/
}


void
on_cfg_misc_track_nr_value_changed      (GtkSpinButton   *spinbutton,
					gpointer         user_data)
{
    prefs_window_set_misc_track_nr (gtk_spin_button_get_value (spinbutton));
}

void
on_cfg_not_played_track_toggled         (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    prefs_window_set_not_played_track (
	gtk_toggle_button_get_active (togglebutton));
}



void
on_cfg_export_template_changed            (GtkEditable     *editable,
					gpointer         user_data)
{
    gchar *buf = gtk_editable_get_chars(editable,0, -1);
    prefs_window_set_export_template(buf);
    g_free (buf);
}


void
on_cfg_normalization_level_changed     (GtkEditable     *editable,
					gpointer         user_data)
{

}


void
on_cfg_write_gaintag_toggled           (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    prefs_window_set_write_gaintag (
	gtk_toggle_button_get_active (togglebutton));
}


void
on_cfg_special_export_charset_toggled  (GtkToggleButton *togglebutton,
					gpointer         user_data)
{

}


void
on_sorting_clicked                     (GtkButton       *button,
					gpointer         user_data)
{
    sort_window_create ();
}


void
on_sorting_activate                    (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    sort_window_create ();
}


void
on_st_ascend_toggled                   (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    if (gtk_toggle_button_get_active(togglebutton))
	sort_window_set_st_sort (SORT_ASCENDING);
}


void
on_st_descend_toggled                  (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    if (gtk_toggle_button_get_active(togglebutton))
	sort_window_set_st_sort (SORT_DESCENDING);
}


void
on_st_none_toggled                     (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    if (gtk_toggle_button_get_active(togglebutton))
	sort_window_set_st_sort (SORT_NONE);
}


void
on_pm_ascend_toggled                   (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    if (gtk_toggle_button_get_active(togglebutton))
	sort_window_set_pm_sort (SORT_ASCENDING);
}


void
on_pm_descend_toggled                  (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    if (gtk_toggle_button_get_active(togglebutton))
	sort_window_set_pm_sort (SORT_DESCENDING);
}


void
on_pm_none_toggled                     (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    if (gtk_toggle_button_get_active(togglebutton))
	sort_window_set_pm_sort (SORT_NONE);
}


void
on_pm_autostore_toggled                (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    sort_window_set_pm_autostore (gtk_toggle_button_get_active(togglebutton));
}



void
on_tm_ascend_toggled                   (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    if (gtk_toggle_button_get_active(togglebutton))
	sort_window_set_tm_sort (SORT_ASCENDING);
}


void
on_tm_descend_toggled                  (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    if (gtk_toggle_button_get_active(togglebutton))
	sort_window_set_tm_sort (SORT_DESCENDING);
}


void
on_tm_none_toggled                     (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    if (gtk_toggle_button_get_active(togglebutton))
	sort_window_set_tm_sort (SORT_NONE);
}

void
on_tm_autostore_toggled                (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    sort_window_set_tm_autostore (gtk_toggle_button_get_active(togglebutton));
}


void
on_sort_case_sensitive_toggled         (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    sort_window_set_case_sensitive(
	gtk_toggle_button_get_active(togglebutton));
}


void
on_sort_apply_clicked                  (GtkButton       *button,
					gpointer         user_data)
{
    sort_window_apply ();
}


void
on_sort_cancel_clicked                 (GtkButton       *button,
					gpointer         user_data)
{
    sort_window_cancel ();
}


void
on_sort_ok_clicked                     (GtkButton       *button,
					gpointer         user_data)
{
    sort_window_ok ();
}


gboolean
on_sort_window_delete_event            (GtkWidget       *widget,
					GdkEvent        *event,
					gpointer         user_data)
{
    sort_window_delete ();
    return FALSE;
}

void
on_normalize_selected_playlist_activate
					(GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    Playlist *pl = pm_get_selected_playlist ();
    if (pl)
	nm_tracks_list (pl->members);
    else
	gtkpod_statusbar_message (_("No playlist selected"));
}


void
on_normalize_selected_tab_entry_activate
					(GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    TabEntry *entry;
    gint inst;

    inst = get_sort_tab_number (_("Normalize tracks in selected entry of which sort tab?"));
    if (inst == -1) return;

    entry = st_get_selected_entry (inst);
    if (entry)
    {
	nm_tracks_list (entry->members);
    }
    else
    {
	gchar *str = g_strdup_printf(_("No entry selected in Sort Tab %d"),
				     inst+1);
	gtkpod_statusbar_message (str);
	g_free (str);
	return;
    }
}


void
on_normalize_selected_tracks_activate   (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
   GList *tracks = tm_get_selected_tracks ();
   nm_tracks_list (tracks);
   g_list_free (tracks);
}


void
on_normalize_displayed_tracks_activate  (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    GList *tracks = tm_get_all_tracks ();
    nm_tracks_list (tracks);
    g_list_free (tracks);
}


void
on_normalize_all_tracks                (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    Playlist *plitem = get_playlist_by_nr (0);
    nm_tracks_list (plitem->members);
}


void
on_normalize_newly_added_tracks        (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    nm_new_tracks ();
}


void
on_info_window1_activate               (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)))
	 info_open_window ();
    else info_close_window ();
}


gboolean
on_gtkpod_info_delete_event            (GtkWidget       *widget,
					GdkEvent        *event,
					gpointer         user_data)
{
    info_close_window ();
    return FALSE;
}


void
on_info_close_clicked                  (GtkButton       *button,
					gpointer         user_data)
{
    info_close_window ();
}


void
on_cfg_write_id3v24_toggled            (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    prefs_window_set_id3_write_id3v24
	(gtk_toggle_button_get_active(togglebutton));
}


void
on_readtags_toggled                    (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    prefs_window_set_readtags
	(gtk_toggle_button_get_active(togglebutton));
}


void
on_parsetags_template_changed             (GtkEditable     *editable,
					gpointer         user_data)
{
    gchar *buf = gtk_editable_get_chars (editable,0, -1);
    prefs_window_set_parsetags_template (buf);
    g_free (buf);
}


void
on_parsetags_overwrite_toggled         (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    prefs_window_set_parsetags_overwrite
	(gtk_toggle_button_get_active(togglebutton));
}


void
on_parsetags_toggled                   (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    prefs_window_set_parsetags
	(gtk_toggle_button_get_active(togglebutton));
}

void
on_check_ipod_files_activate           (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    check_db();
}

void
on_sync_contacts_path_entry_changed    (GtkEditable     *editable,
					gpointer         user_data)
{
    gchar *buf = gtk_editable_get_chars(editable,0, -1);
    prefs_window_set_sync_contacts_path (buf);
    g_free (buf);
}


void
on_sync_calendar_entry_changed         (GtkEditable     *editable,
					gpointer         user_data)
{
    gchar *buf = gtk_editable_get_chars(editable,0, -1);
    prefs_window_set_sync_calendar_path (buf);
    g_free (buf);
}


void
on_sync_calendar_activate              (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    tools_sync_calendar ();
}


void
on_sync_contacts_activate              (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    tools_sync_contacts ();
}


void
on_concal_autosync_toggled             (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    prefs_window_set_concal_autosync
	(gtk_toggle_button_get_active(togglebutton));
}
