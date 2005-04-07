/*
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>

#include "display_private.h"
#include "prefs.h"
#include "misc.h"
#include "misc_track.h"
#include "info.h"
#include "support.h"
#include "context_menus.h"
#include "callbacks.h"

/* pointer to the treeview for the playlist display */
static GtkTreeView *playlist_treeview = NULL;
/* pointer to the currently selected playlist */
static Playlist *current_playlist = NULL;
/* flag set if selection changes to be ignored temporarily */
static gboolean pm_selection_blocked = FALSE;


/* Drag and drop definitions */
static GtkTargetEntry pm_drag_types [] = {
    { DND_GTKPOD_PLAYLISTLIST_TYPE, 0, DND_GTKPOD_PLAYLISTLIST },
    { DND_GTKPOD_TRACKLIST_TYPE, 0, DND_GTKPOD_TRACKLIST },
    { "text/plain", 0, DND_TEXT_PLAIN },
    { "STRING", 0, DND_TEXT_PLAIN }
};
static GtkTargetEntry pm_drop_types [] = {
    { DND_GTKPOD_PLAYLISTLIST_TYPE, 0, DND_GTKPOD_PLAYLISTLIST },
    { DND_GTKPOD_TRACKLIST_TYPE, 0, DND_GTKPOD_TRACKLIST },
    { "text/plain", 0, DND_TEXT_PLAIN },
    { "STRING", 0, DND_TEXT_PLAIN }
};


GtkTreePath *pm_get_path (Playlist *pl);



/* ---------------------------------------------------------------- */
/* Section for playlist display                                     */
/* drag and drop                                                    */
/* ---------------------------------------------------------------- */


/* ----------------------------------------------------------------
 *
 * For drag and drop within the playlist view the following rules apply:
 *
 * 1) Drags between different itdbs: playlists are copied (moved with
 *    CTRL pressed)
 *
 * 2) Drags within the same itdb: playlist is moved (copied with CTRL
 *    pressed)
 *
 * ---------------------------------------------------------------- */


static void pm_drag_begin (GtkWidget *widget,
			   GdkDragContext *drag_context,
			   gpointer user_data)
{
    puts ("drag_begin");
}


static void pm_drag_data_delete (GtkWidget *widget,
			   GdkDragContext *drag_context,
			   gpointer user_data)
{
    puts ("drag_data_delete");
}

static gboolean pm_drag_drop (GtkWidget *widget,
			      GdkDragContext *drag_context,
			      gint x,
			      gint y,
			      guint time,
			      gpointer user_data)
{
    GdkAtom target;

    puts ("drag_data_drop");

    target = gtk_drag_dest_find_target (widget, drag_context, NULL);

    if (target != GDK_NONE)
    {
	gtk_drag_get_data (widget, drag_context, target, time);
	return TRUE;
    }
    return FALSE;
}

static void pm_drag_end (GtkWidget *widget,
			 GdkDragContext *drag_context,
			 gpointer user_data)
{
    puts ("drag_end");
}

static void pm_drag_leave (GtkWidget *widget,
			   GdkDragContext *drag_context,
			   guint time,
			   gpointer user_data)
{
    puts ("drag_leave");
}

static gboolean pm_drag_motion (GtkWidget *widget,
				GdkDragContext *drag_context,
				gint x,
				gint y,
				guint time,
				gpointer user_data)
{
    GtkTreePath *path;
    GtkTreeViewDropPosition pos;
    gboolean result;

    printf ("drag_motion: %d %d %d\n", x, y, pos);

    g_return_val_if_fail (GTK_IS_TREE_VIEW (widget), FALSE);

    result = gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (widget),
						x, y,
						&path,
						&pos);

    gtk_tree_view_set_drag_dest_row (GTK_TREE_VIEW (widget), path, pos);

    if (result == TRUE)
    {
	GdkAtom target;
	target = gtk_drag_dest_find_target (widget, drag_context, NULL);
	if (target == GDK_NONE)
	{
	    gdk_drag_status (drag_context, 0, time);
	}
	else 
	{
	    g_object_set_data (G_OBJECT (widget), "drag_data_by_motion", "set");
	    gtk_drag_get_data (widget, drag_context, target, time);
	}
    }

    return result;
}



/*
 * utility function for appending file for track view (DND)
 */
static void 
on_pm_dnd_get_file_foreach(GtkTreeModel *tm, GtkTreePath *tp, 
			   GtkTreeIter *iter, gpointer data)
{
    Playlist *pl;
    GList *gl;
    GString *filelist = (GString *)data;

    g_return_if_fail (tm);
    g_return_if_fail (iter);
    g_return_if_fail (data);

    /* get current playlist */
    gtk_tree_model_get(tm, iter, PM_COLUMN_PLAYLIST, &pl, -1); 
    g_return_if_fail (pl);

    for (gl=pl->members; gl; gl=gl->next)
    {
	Track *track = gl->data;
	gchar *name = get_track_name_on_disk_verified (track);
	if (name)
	{
	    g_string_append_printf (filelist, "file:%s\n", name);
	    g_free (name);
	}
    }
}

/*
 * utility function for appending pointers to a playlist (DND)
 */
static void
on_pm_dnd_get_playlist_foreach(GtkTreeModel *tm, GtkTreePath *tp,
			       GtkTreeIter *iter, gpointer data)
{
    Playlist *pl;
    GString *playlistlist = (GString *)data;

    g_return_if_fail (tm);
    g_return_if_fail (iter);
    g_return_if_fail (playlistlist);

    gtk_tree_model_get (tm, iter, PM_COLUMN_PLAYLIST, &pl, -1);
    g_return_if_fail (pl);
    g_string_append_printf (playlistlist, "%p\n", pl);
}


/*
 * utility function for appending ipod track pointers of a playlist
 */
static void 
on_pm_dnd_get_track_foreach(GtkTreeModel *tm, GtkTreePath *tp, 
			    GtkTreeIter *i, gpointer data)
{
    Playlist *pl;
    GList *gl;
    GString *tracklist = (GString *)data;

    g_return_if_fail (tm);
    g_return_if_fail (i);
    g_return_if_fail (tracklist);

    gtk_tree_model_get (tm, i, PM_COLUMN_PLAYLIST, &pl, -1);
    g_return_if_fail (pl);

    /* add all member-ids of entry to idlist */
    for (gl=pl->members; gl; gl=gl->next)
    {
	Track *tr = gl->data;
	g_return_if_fail (tr);
	g_string_append_printf (tracklist, "%p\n", tr);
    }
}

static void
tracks_moved_or_copied     (GdkDragContext  *context, gchar *tracks)
{
/*      printf ("ttracks_moved_or_copied ts/dc/action: %p/%d\n", context, context?context->action:0);  */
    if(tracks && context)
    {
	gint n=0;
	gchar *buf = NULL;
	gchar *ptr = tracks;
	iTunesDB *itdb;
	Playlist *pl = pm_get_selected_playlist ();

	g_return_if_fail (pl);
	itdb = pl->itdb;
	g_return_if_fail (itdb);

	/* count the number of tracks */
	while ((ptr=strchr (ptr, '\n')))
	{
	    ++n;
	    ++ptr;
	}

	if ((pl->type == ITDB_PL_TYPE_NORM) &&
	    (context->action == GDK_ACTION_MOVE))
	{
	    Track *track;
	    gchar *str = g_strdup (tracks);

	    while(parse_tracks_from_string(&str, &track))
	    {
		gp_playlist_remove_track (pl, track);
	    }
	    g_free (str);

	    buf = g_strdup_printf (
		ngettext ("Moved one track",
			  "Moved %d tracks", n), n);
	}
	else
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


static void
pm_drag_data_get (GtkWidget       *widget,
		  GdkDragContext  *context,
		  GtkSelectionData *data,
		  guint            info,
		  guint            time,
		  gpointer         user_data)
{
    GtkTreeSelection *ts = NULL;
    GString *reply = g_string_sized_new (2000);

    puts ("data_get");

    /* printf("sm drag get info: %d\n", info);*/
    if((data) && (ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget))))
    {
	switch (info)
	{
	case DND_GTKPOD_TRACKLIST:
	    gtk_tree_selection_selected_foreach(ts,
				    on_pm_dnd_get_track_foreach, reply);
	    break;
	case DND_GTKPOD_PLAYLISTLIST:
	    gtk_tree_selection_selected_foreach(ts,
				    on_pm_dnd_get_playlist_foreach, reply);
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

static void pm_drag_data_received (GtkWidget       *widget,
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

    printf ("drag_data_received: context->suggested_action: %d\n", context? context->suggested_action:-999999);

    g_return_if_fail (context);
    g_return_if_fail (data);
    g_return_if_fail (data->length > 0);

    if (g_object_get_data (G_OBJECT (widget), "drag_data_by_motion"))
    {   /* this callback was caused by pm_drag_motion -- we are
	 * supposed to gdk_drag_status () */
	puts ("by motion");
	g_object_set_data (G_OBJECT (widget), "drag_data_by_motion", NULL);
	gdk_drag_status (context, GDK_ACTION_COPY, time);
	return;
    }

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

	g_return_if_fail (path);

	if (context && (context->suggested_action & GDK_ACTION_MOVE))
	     del_src = TRUE;
	else del_src = FALSE;
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
/* puts(gtk_tree_path_to_string (path)); */
	if(gtk_tree_model_get_iter(model, &i, path))
	{
	    gtk_tree_model_get(model, &i, 0, &pl, -1);
	}
	if (!pl)
	{
	    gtk_drag_finish (context, FALSE, FALSE, time);
	    g_return_if_reached ();
	}
	/* get position of current path */
	if (gtk_tree_path_get_depth (path) == 1)
	{
	    position = 1;
	}
	else
	{
	    gint *indices = gtk_tree_path_get_indices (path);
	    /* need to add 1 because MPL is one level higher and not
	       counted */
	    position = indices[1] + 1;
	    /* adjust position */
	    if (pos == GTK_TREE_VIEW_DROP_AFTER)  ++position;
	}
/* printf("position: %d\n", position); */
	switch (info)
	{
	case DND_GTKPOD_TRACKLIST:
	    if ((pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE) ||
		(pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER))
	    { /* drop into existing playlist */
		add_tracklist_to_playlist (pl, data->data);
		/* this is a hack -- see comment at
		   tracks_moved_or_copied */
		tracks_moved_or_copied (context, data->data);
		gtk_drag_finish (context, TRUE, del_src, time);
	    }
	    else
	    { /* drop between playlists */
		Playlist *plitem;
		plitem = add_new_pl_user_name (pl->itdb, NULL, position);
		if (plitem)
		{
		    add_tracklist_to_playlist (plitem, data->data);
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
	    break;
	case DND_TEXT_PLAIN:
	    if ((pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE) ||
		(pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER))
	    { /* drop into existing playlist */
		add_text_plain_to_playlist (pl->itdb, pl, data->data,
					    0, NULL, NULL);
		gtk_drag_finish (context, TRUE, FALSE, time);
	    }
	    else
	    { /* drop between playlists */
		add_text_plain_to_playlist (pl->itdb, NULL,
					    data->data, position,
					    NULL, NULL);
		gtk_drag_finish (context, TRUE, del_src, time);
	    }
	    break;
	case DND_GTKPOD_PLAYLISTLIST:
	    /* dont allow moves before MPL */
	    printf ("not implemented!\n");
	    break;
	    position = atoi (gtk_tree_path_to_string (path));
	    if (position == 0)  pos = GTK_TREE_VIEW_DROP_AFTER;
	    pm_move_pathlist (data->data, path, pos);
	    gtk_drag_finish (context, TRUE, FALSE, time);
	    break;
	default:
	    gtkpod_warning (_("This DND type (%d) is not (yet) supported. If you feel implementing this would be useful, please contact the author.\n\n"), info);
	    gtk_drag_finish (context, FALSE, FALSE, time);
	    break;
	}
	gtk_tree_path_free(path);
    }
    else   gtk_drag_finish (context, FALSE, FALSE, time);
}





/*#if ((GTK_MAJOR_VERSION == 2) && (GTK_MINOR_VERSION < 2))*/
/* gtk_list_store_move_*() was introduced in 2.2, so we have to
 * emulate for 2.0 <= V < 2.2 (we require at least 2.0 anyway) */
/* !!! gtk_list_store_move() at least in 2.2.1 seems to be buggy: if
   moving before the first entry, the moved entry ends up
   last. Therefore I emulate the functions for all gtk versions */
static void pm_list_store_move (GtkListStore *store,
				 GtkTreeIter  *iter,
				 GtkTreeIter  *position,
				 gboolean     before)
{
    GtkTreeIter new_iter;
    Playlist *playlist = NULL;
    GtkTreeModel *model;

    /* insert new row before or after @position */
    if (before)  gtk_list_store_insert_before (store, &new_iter, position);
    else         gtk_list_store_insert_after (store, &new_iter, position);

    model = gtk_tree_view_get_model (playlist_treeview);

    /* get the content (track) of the row to move */
    gtk_tree_model_get (model, iter, PM_COLUMN_PLAYLIST, &playlist, -1);
    /* remove the old row */
    gtk_tree_store_remove (GTK_TREE_STORE (model), iter);

    /* set the content of the new row */
    gtk_tree_store_set (GTK_TREE_STORE (model), &new_iter,
			PM_COLUMN_PLAYLIST, playlist,
			-1);
}

void  pm_list_store_move_before (GtkListStore *store,
					 GtkTreeIter  *iter,
					 GtkTreeIter  *position)
{
    pm_list_store_move (store, iter, position, TRUE);
}

void  pm_list_store_move_after (GtkListStore *store,
					GtkTreeIter  *iter,
					GtkTreeIter  *position)
{
    pm_list_store_move (store, iter, position, FALSE);
}
/* #else*/
#if 0
/* starting V2.2 convenient gtk functions exist (but see comment above) */
void  pm_list_store_move_before (GtkListStore *store,
					 GtkTreeIter  *iter,
					 GtkTreeIter  *position)
{
    gtk_list_store_move_before (store, iter, position);
}

void  pm_list_store_move_after (GtkListStore *store,
					GtkTreeIter  *iter,
					GtkTreeIter  *position)
{
    gtk_list_store_move_after (store, iter, position);
}
#endif


/* move pathlist for track treeview */
gboolean pm_move_pathlist (gchar *data,
			   GtkTreePath *path,
			   GtkTreeViewDropPosition pos)
{
    return pmtm_move_pathlist (playlist_treeview, data, path, pos, PLAYLIST_TREEVIEW);
}

/* ---------------------------------------------------------------- */
/* Section for playlist display                                     */
/* other callbacks                                                  */
/* ---------------------------------------------------------------- */

static gboolean
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
		gp_do_selected_playlist (update_tracks);
		break;
	    case GDK_n:
		add_new_pl_or_spl_user_name (gp_get_active_itdb(),
					     NULL, -1);
		break;
	    default:
		break;
	}

    }
  return FALSE;
}




/* ---------------------------------------------------------------- */
/* Section for playlist display                                     */
/* ---------------------------------------------------------------- */


/* remove a track from a current playlist (model) */
void pm_remove_track (Playlist *playlist, Track *track)
{
    g_return_if_fail (playlist);
    g_return_if_fail (track);

    /* notify sort tab if currently selected playlist is affected */
    if (current_playlist)
    {   /* only remove if selected playlist is in same itdb as track */
	if (track->itdb == current_playlist->itdb)
	{
	    if ((playlist == current_playlist) ||
		(current_playlist->type == ITDB_PL_TYPE_MPL))
	    {
		st_remove_track (track, 0);
	    }
	}
    }
}


/* Add track to the display if it's in the currently displayed playlist.
 * @display: TRUE: add to track model (i.e. display it) */
void pm_add_track (Playlist *playlist, Track *track, gboolean display)
{
    if (playlist == current_playlist)
    {
	st_add_track (track, TRUE, display, 0); /* Add to first sort tab */
    }
}

/* Used by pm_name_changed() to find the playlist that
   changed name. If found, emit a "row changed" signal to display the change */
static gboolean sr_model_playlist_name_changed (GtkTreeModel *model,
					GtkTreePath *path,
					GtkTreeIter *iter,
					gpointer data)
{
  Playlist *playlist;

  gtk_tree_model_get (model, iter, PM_COLUMN_PLAYLIST, &playlist, -1);
  if(playlist == (Playlist *)data) {
    gtk_tree_model_row_changed (model, path, iter);
    return TRUE;
  }
  return FALSE;
}


/* One of the playlist names has changed (this happens when the
   iTunesDB is read */
void pm_name_changed (Playlist *pl)
{
  GtkTreeModel *model = gtk_tree_view_get_model (playlist_treeview);

  g_return_if_fail (pl);
  g_return_if_fail (model);

  gtk_tree_model_foreach (model, sr_model_playlist_name_changed, pl);
}


/* If a track got changed (i.e. it's ID3 entries have changed), we check
   if it's in the currently displayed playlist, and if yes, we notify the
   first sort tab of a change */
void pm_track_changed (Track *track)
{
  if (!current_playlist) return;
  /* Check if track is member of current playlist */
  if (g_list_find (current_playlist->members, track))
      st_track_changed (track, FALSE, 0);
}


/* Add playlist to the playlist model */
/* If @position = -1: append to end */
/* If @position >=0: insert at that position (count starts with MPL as
 * 0) */
void pm_add_playlist (Playlist *playlist, gint pos)
{
  GtkTreeIter mpl_iter;
  GtkTreeIter *mpl = NULL;
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkTreeSelection *selection;

  g_assert (playlist_treeview);
  g_return_if_fail (playlist);

  model = GTK_TREE_MODEL (gtk_tree_view_get_model (playlist_treeview));
  g_return_if_fail (model);

  if (playlist->type == ITDB_PL_TYPE_MPL)
  {   /* MPLs are always added top-level */
      mpl = NULL;
  }
  else
  {   /* We need to find the iter with the mpl in it */
      if (gtk_tree_model_get_iter_first (model, &mpl_iter))
      {
	  do
	  {
	      Playlist *pl;
	      gtk_tree_model_get (model, &mpl_iter,
				  PM_COLUMN_PLAYLIST, &pl, -1);
	      g_return_if_fail (pl);
	      if (pl->itdb == playlist->itdb)
	      {
		  mpl = &mpl_iter;
	      }
	  } while ((mpl == NULL) &&
		   gtk_tree_model_iter_next (model, &mpl_iter));
      }
      if (!mpl)
      {
	  g_warning ("Programming error: need to add mpl before adding normal playlists.\n");
      }
      /* reduce position by one because the MPL is not included in the
	 tree model's count */
      --pos;
  }

  gtk_tree_store_insert (GTK_TREE_STORE (model), &iter, mpl, pos);

  gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
		      PM_COLUMN_PLAYLIST, playlist,
		      -1);

  /* If the current_playlist is "playlist", we select it. This can
     happen during a display_reset */
  if (current_playlist == playlist)
  {
      selection = gtk_tree_view_get_selection (playlist_treeview);
      gtk_tree_selection_select_iter (selection, &iter);
  }
  else if (current_playlist == NULL)
  {
      /* If it's the first playlist (no playlist selected AND playlist is
	 the MPL, we select it, unless prefs_get_mpl_autoselect() says
	 otherwise */
      if ((playlist->type == ITDB_PL_TYPE_MPL) && prefs_get_mpl_autoselect())
      {
	  selection = gtk_tree_view_get_selection (playlist_treeview);
	  gtk_tree_selection_select_iter (selection, &iter);
      }
  }
}


/* Remove "playlist" from the display model. 
   "select": TRUE: a new playlist is selected
             FALSE: no selection is taking place
                    (useful when quitting program) */
void pm_remove_playlist (Playlist *playlist, gboolean select)
{
    static gboolean pm_delete_playlist_fe (GtkTreeModel *model,
					   GtkTreePath *path,
					   GtkTreeIter *iter,
					   gpointer data)
	{
	    Playlist *playlist;
	    
	    gtk_tree_model_get (model, iter, PM_COLUMN_PLAYLIST, &playlist, -1);
	    if(playlist == (Playlist *)data) {
		gtk_tree_store_remove (GTK_TREE_STORE (model), iter);
		return TRUE;
	    }
	    return FALSE;
	}
    GtkTreeModel *model;
    gboolean have_iter = FALSE;
    GtkTreeIter i;
    GtkTreeSelection *ts = NULL;

  g_return_if_fail (playlist);
  model = gtk_tree_view_get_model (playlist_treeview);
  g_return_if_fail (model);
  
  ts = gtk_tree_view_get_selection (playlist_treeview);
  if (select && (current_playlist == playlist))
  {
      /* We are about to delete the currently selected
	 playlist. Try to select the next. */
      if (gtk_tree_selection_get_selected (ts, NULL, &i))
      {
	  GtkTreePath *path = gtk_tree_model_get_path (model, &i);
	  if(gtk_tree_model_iter_next (model, &i))
	  {
	      have_iter = TRUE;
	  }
	  else
	  {   /* no next iter -- try previous iter */
	      if (gtk_tree_path_prev (path))
	      {   /* OK -- make iter from it */
		  gtk_tree_model_get_iter (model, &i, path);
		  have_iter = TRUE;
	      }
	  }
	  gtk_tree_path_free (path);
      }
  }
  /* find the pl and delete it */
  gtk_tree_model_foreach (model, pm_delete_playlist_fe, playlist);
  /* select our new iter !!! */
  if (have_iter && select)   gtk_tree_selection_select_iter(ts, &i);
}


/* Remove all playlists from the display model */
/* ATTENTION: the playlist_treeview and model might be changed by
   calling this function */
/* @clear_sort: TRUE: clear "sortable" setting of treeview */
void pm_remove_all_playlists (gboolean clear_sort)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  gint column;
  GtkSortType order;

  g_return_if_fail (playlist_treeview);
  model = gtk_tree_view_get_model (playlist_treeview);
  g_return_if_fail (model);

  while (gtk_tree_model_get_iter_first (model, &iter))
  {
      gtk_tree_store_remove (GTK_TREE_STORE (model), &iter);
  }
  if(clear_sort &&
     gtk_tree_sortable_get_sort_column_id (GTK_TREE_SORTABLE (model),
					   &column, &order))
  { /* recreate track treeview to unset sorted column */
      if (column >= 0)
      {
	  pm_create_treeview ();
      }
  }
}


/* Select specified playlist */
void pm_select_playlist (Playlist *playlist)
{
    gboolean pm_select_playlist_fe (GtkTreeModel *model,
				    GtkTreePath *path,
				    GtkTreeIter *iter,
				    gpointer data)
	{
	    Playlist *playlist;

	    gtk_tree_model_get (model, iter,
				PM_COLUMN_PLAYLIST, &playlist, -1);
	    if(playlist == data)
	    {
		GtkTreeSelection *ts = gtk_tree_view_get_selection (
		    playlist_treeview);
		gtk_tree_selection_select_iter (ts, iter);
		return TRUE;
	    }
	    return FALSE;
	}
    GtkTreeModel *model;

    g_return_if_fail (playlist_treeview);
    g_return_if_fail (playlist);
    model = gtk_tree_view_get_model (playlist_treeview);
    g_return_if_fail (model);

    /* find the pl and select it */
    gtk_tree_model_foreach (model, pm_select_playlist_fe,
			    playlist);
}


/* Unselect specified playlist */
void pm_unselect_playlist (Playlist *playlist)
{
    gboolean pm_unselect_playlist_fe (GtkTreeModel *model,
				      GtkTreePath *path,
				      GtkTreeIter *iter,
				      gpointer data)
	{
	    Playlist *playlist;

	    gtk_tree_model_get (model, iter,
				PM_COLUMN_PLAYLIST, &playlist, -1);
	    if(playlist == data)
	    {
		GtkTreeSelection *ts = gtk_tree_view_get_selection (
		    playlist_treeview);
		gtk_tree_selection_unselect_iter (ts, iter);
		return TRUE;
	    }
	    return FALSE;
	}
    GtkTreeModel *model;

    g_return_if_fail (playlist_treeview);
    g_return_if_fail (playlist);
    model = gtk_tree_view_get_model (playlist_treeview);
    g_return_if_fail (model);

    /* find the pl and unselect it */
    gtk_tree_model_foreach (model, pm_unselect_playlist_fe,
			    playlist);
}


static void pm_selection_changed_cb (gpointer user_data1, gpointer user_data2)
{
  GtkTreeSelection *selection = (GtkTreeSelection *)user_data1;
  GtkTreeModel *model;
  GtkTreeIter  iter;
  Playlist *new_playlist = NULL;

#if DEBUG_TIMING
  GTimeVal time;
  g_get_current_time (&time);
  printf ("pm_selection_changed_cb enter: %ld.%06ld sec\n",
	  time.tv_sec % 3600, time.tv_usec);
#endif

  if (gtk_tree_selection_get_selected (selection, &model, &iter) == FALSE)
  {  /* no selection -> reset sort tabs */
      st_init (-1, 0);
      current_playlist = NULL;
  }
  else
  {   /* handle new selection */
      gtk_tree_model_get (model, &iter, 
			  PM_COLUMN_PLAYLIST, &new_playlist,
			  -1);
      /* remove all entries from sort tab 0 */
      /* printf ("removing entries: %x\n", current_playlist);*/
      st_init (-1, 0);

      current_playlist = new_playlist;
      if (new_playlist->members)
      {
	  GTimeVal time;
	  float max_count = REFRESH_INIT_COUNT;
	  gint count = max_count - 1;
	  float ms;
	  GList *gl;

	  if (!prefs_get_block_display ())
	  {
	      block_selection (-1);
	      g_get_current_time (&time);
	  }
	  st_enable_disable_view_sort (0, FALSE);
	  for (gl=new_playlist->members; gl; gl=gl->next)
	  { /* add all tracks to sort tab 0 */
	      Track *track = gl->data;
	      if (stop_add == -1)  break;
	      st_add_track (track, FALSE, TRUE, 0);
	      --count;
	      if ((count < 0) && !prefs_get_block_display ())
	      {
		  gtkpod_tracks_statusbar_update();
		  while (gtk_events_pending ())       gtk_main_iteration ();
		  ms = get_ms_since (&time, TRUE);
		  /* first time takes significantly longer, so we adjust
		     the max_count */
		  if (max_count == REFRESH_INIT_COUNT) max_count *= 2.5;
		  /* average the new and the old max_count */
		  max_count *= (1 + 2 * REFRESH_MS / ms) / 3;
		  count = max_count - 1;
#if DEBUG_TIMING
		  printf("pm_s_c ms: %f mc: %f\n", ms, max_count);
#endif
	      }
	  }
	  st_enable_disable_view_sort (0, TRUE);
	  if (stop_add != -1) st_add_track (NULL, TRUE, TRUE, 0);
	  if (!prefs_get_block_display ())
	  {
	      while (gtk_events_pending ())	      gtk_main_iteration ();
	      release_selection (-1);
	  }
      }
      gtkpod_tracks_statusbar_update();
  }
#if DEBUG_TIMING
  g_get_current_time (&time);
  printf ("pm_selection_changed_cb exit:  %ld.%06ld sec\n",
	  time.tv_sec % 3600, time.tv_usec);
#endif 
  /* make only suitable delete menu items available */
  display_adjust_delete_menus ();
}

/* Callback function called when the selection
   of the playlist view has changed */
/* Instead of handling the selection directly, we add a
   "callback". Currently running display updates will be stopped
   before the pm_selection_changed_cb is actually called */
static void pm_selection_changed (GtkTreeSelection *selection,
				  gpointer user_data)
{
    space_data_update ();
    if (!pm_selection_blocked)
	add_selection_callback (-1, pm_selection_changed_cb,
				(gpointer)selection, user_data);
}


/* Stop editing. If @cancel is TRUE, the edited value will be
   discarded (I have the feeling that the "discarding" part does not
   work quite the way intended). */
void pm_stop_editing (gboolean cancel)
{
    GtkTreeViewColumn *col;

    g_return_if_fail (playlist_treeview);

    gtk_tree_view_get_cursor (playlist_treeview, NULL, &col);
    if (col)
    {
	if (!cancel && col->editable_widget)  
	    gtk_cell_editable_editing_done (col->editable_widget);
	if (col->editable_widget)
	    gtk_cell_editable_remove_widget (col->editable_widget);
    }
}


/* set/read the counter used to remember how often the sort column has
   been clicked.
   @inc: negative: reset counter to 0
   @inc: positive or zero : add to counter
   return value: new value of the counter */
static gint pm_sort_counter (gint inc)
{
    static gint cnt = 0;
    if (inc <0) cnt = 0;
    else        cnt += inc;
    return cnt;
}


/* Add all playlists of @itdb at position @pos */
void pm_add_itdb (iTunesDB *itdb, gint pos)
{
    GList *gl_pl;

    g_return_if_fail (itdb);

    for (gl_pl=itdb->playlists; gl_pl; gl_pl=gl_pl->next)
    {
	Playlist *pl = gl_pl->data;
	g_return_if_fail (pl);
	if (pl->type == ITDB_PL_TYPE_MPL)
	     pm_add_playlist (pl, pos);
	else pm_add_playlist (pl, -1);
    }
}


/* Helper function: add all playlists to playlist model */
void pm_add_all_playlists (void)
{
    GList *gl_itdb;    
    struct itdbs_head *itdbs_head;

    g_return_if_fail (gtkpod_window);
    itdbs_head = g_object_get_data (G_OBJECT (gtkpod_window),
				    "itdbs_head");
    g_return_if_fail (itdbs_head);
    for (gl_itdb=itdbs_head->itdbs; gl_itdb; gl_itdb=gl_itdb->next)
    {
	iTunesDB *itdb = gl_itdb->data;
	g_return_if_fail (itdb);
	pm_add_itdb (itdb, -1);
    }
}


/* Return path of playlist @pl. After use the return value must be
 * freed by calling gtk_tree_path_free() */
GtkTreePath *pm_get_path (Playlist *pl)
{
    struct userdata
    {
	Playlist *pl;
	GtkTreePath *path;
    };
    GtkTreeModel *model;
    struct userdata userdata;
    static gboolean pm_get_path_fe (GtkTreeModel *model,
				    GtkTreePath *path,
				    GtkTreeIter *iter,
				    gpointer data)
	{
	    struct userdata *ud = data;
	    Playlist *pl;

	    gtk_tree_model_get (model, iter,
				PM_COLUMN_PLAYLIST, &pl, -1);
	    if(pl == ud->pl)
	    {
		ud->path = gtk_tree_model_get_path (model, iter);
		return TRUE;
	    }
	    return FALSE;
	}
    g_return_val_if_fail (playlist_treeview, NULL);
    g_return_val_if_fail (pl, NULL);
    model = gtk_tree_view_get_model (playlist_treeview);
    g_return_val_if_fail (model, NULL);

    userdata.pl = pl;
    userdata.path = NULL;

    /* find the pl and fill in path */
    gtk_tree_model_foreach (model, pm_get_path_fe, &userdata);

    return userdata.path;
}



/* "unsort" the playlist view without causing the sort tabs to be
   touched. */
static void pm_unsort ()
{
    Playlist *cur_pl;

    pm_selection_blocked = TRUE;

    /* remember */
    cur_pl = pm_get_selected_playlist ();

    pm_remove_all_playlists (TRUE);

    pm_set_selected_playlist (cur_pl);

    /* add playlists back to model (without selecting) */
    pm_add_all_playlists ();

    pm_selection_blocked = FALSE;
    /* reset sort counter */
    pm_sort_counter (-1);
}


/* Set the sorting accordingly */
void pm_sort (GtkSortType order)
{
    GtkTreeModel *model= gtk_tree_view_get_model (playlist_treeview);
    g_return_if_fail (playlist_treeview);
    if (order != SORT_NONE)
    {
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model),
					      PM_COLUMN_PLAYLIST, order);
    }
    else
    { /* only unsort if treeview is sorted */
	gint column;
	GtkSortType order;
	if (gtk_tree_sortable_get_sort_column_id
	    (GTK_TREE_SORTABLE (model), &column, &order))
	    pm_unsort ();
    }
}

/**
 * pm_track_column_button_clicked
 * @tvc - the tree view colum that was clicked
 * @data - ignored user data
 * When the sort button is clicked we want to update our internal playlist
 * representation to what's displayed on screen.
 * If the button was clicked three times, the sort order is undone.
 */
static void
pm_track_column_button_clicked(GtkTreeViewColumn *tvc, gpointer data)
{
    gint cnt = pm_sort_counter (1);
    if (cnt >= 3)
    {
	prefs_set_pm_sort (SORT_NONE);
	pm_unsort (); /* also resets the sort_counter */
    }
    else
    {
	prefs_set_pm_sort (gtk_tree_view_column_get_sort_order (tvc));
	if (prefs_get_pm_autostore ())  pm_rows_reordered ();
    }
}



/**
 * Reorder playlists to match order of playlists displayed.
 * data_changed() is called when necessary.
 */
void
pm_rows_reordered (void)
{
    GtkTreeModel *tm = NULL;
    GtkTreeIter parent;
    gboolean p_valid;

    g_return_if_fail (playlist_treeview);
    tm = gtk_tree_view_get_model (GTK_TREE_VIEW(playlist_treeview));
    g_return_if_fail (tm);

    p_valid = gtk_tree_model_get_iter_first(tm, &parent);
    while(p_valid)
    {
	guint32 pos;
	Playlist *pl;
	iTunesDB *itdb;
	GtkTreeIter child;
	gboolean c_valid;

	/* get master playlist */
	gtk_tree_model_get (tm, &parent, PM_COLUMN_PLAYLIST, &pl, -1); 
	g_return_if_fail (pl);
	g_return_if_fail (pl->type == ITDB_PL_TYPE_MPL);
	itdb = pl->itdb;
	g_return_if_fail (itdb);

	pos = 1;
	/* get all children */
	c_valid = gtk_tree_model_iter_children (tm, &child, &parent);
	while (c_valid)
	{
	    gtk_tree_model_get (tm, &child,
				PM_COLUMN_PLAYLIST, &pl, -1);
	    g_return_if_fail (pl);
	    if (itdb_playlist_by_nr (itdb, pos) != pl)
	    {
		/* move the playlist to indicated position */
		g_return_if_fail (pl->type != ITDB_PL_TYPE_MPL);
		itdb_playlist_move (pl, pos);
		data_changed (itdb);
	    }
	    ++pos;
	    c_valid = gtk_tree_model_iter_next (tm, &child);
	}
	p_valid = gtk_tree_model_iter_next (tm, &parent);
    }
}


/* Function used to compare two cells during sorting (playlist view) */
gint pm_data_compare_func (GtkTreeModel *model,
			GtkTreeIter *a,
			GtkTreeIter *b,
			gpointer user_data)
{
  Playlist *playlist1;
  Playlist *playlist2;
  GtkSortType order;
  gint corr, colid;

/*  g_return_val_if_fail (model, 0);
  g_return_val_if_fail (a, 0);
  g_return_val_if_fail (b, 0);*/

  if (gtk_tree_sortable_get_sort_column_id (GTK_TREE_SORTABLE (model),
					    &colid, &order) == FALSE)
      return 0;
  gtk_tree_model_get (model, a, colid, &playlist1, -1);
  gtk_tree_model_get (model, b, colid, &playlist2, -1);
  g_return_val_if_fail (playlist1 && playlist2, 0);

  /* We make sure that the master playlist always stays on top */
  if (order == GTK_SORT_ASCENDING) corr = +1;
  else                             corr = -1;
  if (playlist1->type == ITDB_PL_TYPE_MPL) return (-corr);
  if (playlist2->type == ITDB_PL_TYPE_MPL) return (corr);

  /* compare the two entries */
  return compare_string (playlist1->name, playlist2->name);
}


/* Called when editable cell is being edited. Stores new data to
   the playlist list. */
static void
pm_cell_edited (GtkCellRendererText *renderer,
		const gchar         *path_string,
		const gchar         *new_text,
		gpointer             data)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  Playlist *playlist;
  gint column;

  model = (GtkTreeModel *)data;
  g_return_if_fail (model);
  if (!gtk_tree_model_get_iter_from_string (model, &iter, path_string))
  {
      g_return_if_reached ();
  }

  column = (gint)g_object_get_data (G_OBJECT (renderer), "column");
  gtk_tree_model_get (model, &iter, column, &playlist, -1);
  g_return_if_fail (playlist);

  /*printf("pm_cell_edited: column: %d  track:%lx\n", column, track);*/

  switch (column)
  {
  case PM_COLUMN_PLAYLIST:
      g_return_if_fail (new_text);
      /* We only do something, if the name actually got changed */
      if (!playlist->name ||
	  g_utf8_collate (playlist->name, new_text) != 0)
      {
	  g_free (playlist->name);
	  playlist->name = g_strdup (new_text);
	  data_changed (playlist->itdb);
      }
      break;
  }
}


/* The playlist data is stored in a separate list
   and only pointers to the corresponding playlist structure are placed
   into the model.
   This function reads the data for the given cell from the list and
   passes it to the renderer. */
static void pm_cell_data_func (GtkTreeViewColumn *tree_column,
			       GtkCellRenderer   *renderer,
			       GtkTreeModel      *model,
			       GtkTreeIter       *iter,
			       gpointer           data)
{
  Playlist *playlist;
  gint column;

  g_return_if_fail (renderer);
  g_return_if_fail (model);
  g_return_if_fail (iter);

  column = (gint)g_object_get_data (G_OBJECT (renderer), "column");
  gtk_tree_model_get (model, iter, column, &playlist, -1);
  g_return_if_fail (playlist);

  switch (column)
    {  /* We only have one column, so this code is overkill... */
    case PM_COLUMN_PLAYLIST: 
	if (playlist->type == ITDB_PL_TYPE_MPL)
	{   /* mark MPL */
	    g_object_set (G_OBJECT (renderer),
			  "text", playlist->name, 
			  "editable", TRUE,
			  "weight", PANGO_WEIGHT_BOLD,
			  NULL);
	}
	else
	{
	    g_object_set (G_OBJECT (renderer),
			  "text", playlist->name, 
			  "editable", TRUE,
			  "weight", PANGO_WEIGHT_NORMAL,
			  NULL);
	}
	break;
    }
}


/* set graphic indicator for smart playlists */
static void pm_cell_data_func_pix (GtkTreeViewColumn *tree_column,
				   GtkCellRenderer   *renderer,
				   GtkTreeModel      *model,
				   GtkTreeIter       *iter,
				   gpointer           data)
{
  Playlist *playlist;
  gint column;

  g_return_if_fail (renderer);
  g_return_if_fail (model);
  g_return_if_fail (iter);

  column = (gint)g_object_get_data (G_OBJECT (renderer), "column");
  gtk_tree_model_get (model, iter, column, &playlist, -1);
  g_return_if_fail (playlist);

  switch (column)
    {  /* We only have one column, so this code is overkill... */
    case PM_COLUMN_PLAYLIST: 
	if (playlist->is_spl)
	{
	    g_object_set (G_OBJECT (renderer),
			  "stock-id", "gtk-properties", NULL);
	}
	else if (playlist->type != ITDB_PL_TYPE_MPL)
	{
	    g_object_set (G_OBJECT (renderer),
			  "stock-id", "gtk-justify-left", NULL);
	}
	else
	{
	    g_object_set (G_OBJECT (renderer),
			  "stock-id", NULL, NULL);
	}
	break;
    }
}


/* Adds the columns to our playlist_treeview */
static void pm_add_columns (void)
{
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkTreeModel *model;

  model = gtk_tree_view_get_model (playlist_treeview);
  g_return_if_fail (model);

  /* playlist column */
  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (column, _("Playlists"));
  gtk_tree_view_column_set_sort_column_id (column, PM_COLUMN_PLAYLIST);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_column_set_sort_order (column, GTK_SORT_ASCENDING);
  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
				   PM_COLUMN_PLAYLIST,
				   pm_data_compare_func, column, NULL);
  gtk_tree_view_column_set_clickable(column, TRUE);
  g_signal_connect (G_OBJECT (column), "clicked",
		    G_CALLBACK (pm_track_column_button_clicked),
				(gpointer)PM_COLUMN_PLAYLIST);
  gtk_tree_view_append_column (playlist_treeview, column);

  /* cell for playlist name */
  renderer = gtk_cell_renderer_text_new ();
  g_signal_connect (G_OBJECT (renderer), "edited",
		    G_CALLBACK (pm_cell_edited), model);
  g_object_set_data (G_OBJECT (renderer), "column", (gint *)PM_COLUMN_PLAYLIST);
  gtk_tree_view_column_pack_end (column, renderer, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, pm_cell_data_func, NULL, NULL);


  /* cell for graphic indicator */
  renderer = gtk_cell_renderer_pixbuf_new ();
  g_object_set_data (G_OBJECT (renderer), "column", (gint *)PM_COLUMN_PLAYLIST);
  gtk_tree_view_column_pack_start (column, renderer, FALSE); 
  gtk_tree_view_column_set_cell_data_func (column, renderer, pm_cell_data_func_pix, NULL, NULL);
}


static void pm_select_current_position (gint x, gint y)
{
    GtkTreePath *path;

    g_return_if_fail (playlist_treeview);

    gtk_tree_view_get_path_at_pos (playlist_treeview,
				   x, y, &path, NULL, NULL, NULL);
    if (path)
    {
	GtkTreeSelection *ts = gtk_tree_view_get_selection
	    (playlist_treeview);
	gtk_tree_selection_select_path (ts, path);
	gtk_tree_path_free (path);
    }
}

static gboolean
pm_button_press (GtkWidget *w, GdkEventButton *e, gpointer data)
{
    g_return_val_if_fail (w && e, FALSE);
    switch(e->button)
    {
    case 3:
	pm_select_current_position (e->x, e->y);
	pm_context_menu_init ();
	return TRUE;
    default:
	return FALSE;
    }
}

/* Create playlist listview */
void pm_create_treeview (void)
{
  GtkTreeStore *model;
  GtkTreeSelection *selection;
  GtkWidget *playlist_window;
  GtkWidget *tree;

  playlist_window = lookup_widget (gtkpod_window, "playlist_window");
  g_return_if_fail (playlist_window);

  /* destroy old treeview */
  if (playlist_treeview)
  {
      model = GTK_TREE_STORE (gtk_tree_view_get_model(playlist_treeview));
      g_return_if_fail (model);
      g_object_unref (model);
      gtk_widget_destroy (GTK_WIDGET (playlist_treeview));
      playlist_treeview = NULL;
  }
  /* create new one */
  tree = gtk_tree_view_new ();
  gtk_widget_set_events (tree, GDK_KEY_RELEASE_MASK);
  gtk_widget_show (tree);
  playlist_treeview = GTK_TREE_VIEW (tree);
  gtk_container_add (GTK_CONTAINER (playlist_window), tree);

  /* create model */
  model =   gtk_tree_store_new (PM_NUM_COLUMNS, G_TYPE_POINTER);

  /* set tree model */
  gtk_tree_view_set_model (playlist_treeview, GTK_TREE_MODEL (model));
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (playlist_treeview), TRUE);
  gtk_tree_selection_set_mode (gtk_tree_view_get_selection (playlist_treeview),
			       GTK_SELECTION_SINGLE);
  selection = gtk_tree_view_get_selection (playlist_treeview);
  g_signal_connect (G_OBJECT (selection), "changed",
		    G_CALLBACK (pm_selection_changed), NULL);
  pm_add_columns ();

  gtk_drag_source_set (GTK_WIDGET (playlist_treeview), GDK_BUTTON1_MASK,
		       pm_drag_types, TGNR (pm_drag_types), GDK_ACTION_COPY);
  gtk_drag_dest_set (GTK_WIDGET (playlist_treeview),
		     GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT,
		     pm_drop_types, TGNR (pm_drop_types),
		     GDK_ACTION_COPY);


/*   gtk_tree_view_enable_model_drag_dest (playlist_treeview, */
/* 					pm_drop_types, TGNR (pm_drop_types), */
/* 					GDK_ACTION_COPY); */
  /* need the gtk_drag_dest_set() with no actions ("0") so that the
     data_received callback gets the correct info value. This is most
     likely a bug... */
/*   gtk_drag_dest_set_target_list (GTK_WIDGET (playlist_treeview), */
/* 				 gtk_target_list_new (pm_drop_types, */
/* 						      TGNR (pm_drop_types))); */

  g_signal_connect ((gpointer) playlist_treeview, "drag-begin",
		    G_CALLBACK (pm_drag_begin),
		    NULL);

  g_signal_connect ((gpointer) playlist_treeview, "drag-data-delete",
		    G_CALLBACK (pm_drag_data_delete),
		    NULL);

  g_signal_connect ((gpointer) playlist_treeview, "drag-data-get",
		    G_CALLBACK (pm_drag_data_get),
		    NULL);

  g_signal_connect ((gpointer) playlist_treeview, "drag-data-received",
		    G_CALLBACK (pm_drag_data_received),
		    NULL);

  g_signal_connect ((gpointer) playlist_treeview, "drag-drop",
		    G_CALLBACK (pm_drag_drop),
		    NULL);

  g_signal_connect ((gpointer) playlist_treeview, "drag-end",
		    G_CALLBACK (pm_drag_end),
		    NULL);

  g_signal_connect ((gpointer) playlist_treeview, "drag-leave",
		    G_CALLBACK (pm_drag_leave),
		    NULL);

  g_signal_connect ((gpointer) playlist_treeview, "drag-motion",
		    G_CALLBACK (pm_drag_motion),
		    NULL);

  g_signal_connect_after ((gpointer) playlist_treeview, "key_release_event",
			  G_CALLBACK (on_playlist_treeview_key_release_event),
			  NULL);
  g_signal_connect (G_OBJECT (playlist_treeview), "button-press-event",
		    G_CALLBACK (pm_button_press), model);
}



Playlist*
pm_get_selected_playlist (void)
{
/* return(current_playlist);*/
/* we can't just return the "current_playlist" because the context
   menus require the selection before "current_playlist" is updated */

    GtkTreeSelection *ts;
    GtkTreeIter iter;
    GtkTreeModel *model;
    Playlist *result = NULL;

    g_return_val_if_fail (playlist_treeview, NULL);
    ts = gtk_tree_view_get_selection (playlist_treeview);
    g_return_val_if_fail (ts, NULL);

    if (gtk_tree_selection_get_selected (ts, &model, &iter))
    {
	gtk_tree_model_get (model, &iter,
			    PM_COLUMN_PLAYLIST, &result, -1);
    }

    /* playlist was just changed -- wait until current_playlist is
       updated. */
    if (result != current_playlist)  result=NULL;
    return result;
}

/* use with care!! */
void
pm_set_selected_playlist (Playlist *pl)
{
    current_playlist = pl;
}


