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
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "display_private.h"
#include "prefs.h"
#include "misc.h"
#include "support.h"
#include "context_menus.h"
#include "callbacks.h"

/* pointer to the treeview for the playlist display */
static GtkTreeView *playlist_treeview = NULL;
/* pointer to the currently selected playlist */
static Playlist *current_playlist = NULL;

/* Drag and drop definitions */
static GtkTargetEntry pm_drag_types [] = {
    { DND_GTKPOD_PM_PATHLIST_TYPE, 0, DND_GTKPOD_PM_PATHLIST },
    { DND_GTKPOD_IDLIST_TYPE, 0, DND_GTKPOD_IDLIST },
    { "text/plain", 0, DND_TEXT_PLAIN },
    { "STRING", 0, DND_TEXT_PLAIN }
};
static GtkTargetEntry pm_drop_types [] = {
    { DND_GTKPOD_PM_PATHLIST_TYPE, 0, DND_GTKPOD_PM_PATHLIST },
    { DND_GTKPOD_IDLIST_TYPE, 0, DND_GTKPOD_IDLIST },
    { "text/plain", 0, DND_TEXT_PLAIN },
    { "STRING", 0, DND_TEXT_PLAIN }
};


/* ---------------------------------------------------------------- */
/* Section for playlist display                                     */
/* ---------------------------------------------------------------- */


/* remove a song from a current playlist (model) */
void pm_remove_song (Playlist *playlist, Song *song)
{
  /* notify sort tab if currently selected playlist is affected */
  if (current_playlist && 
      ((playlist == current_playlist) ||
       (current_playlist->type == PL_TYPE_MPL)))
  {
      st_remove_song (song, 0);
  }
}


/* Add song to the display if it's in the currently displayed playlist.
 * @display: TRUE: add to song model (i.e. display it) */
void pm_add_song (Playlist *playlist, Song *song, gboolean display)
{
    if (playlist == current_playlist)
    {
	st_add_song (song, TRUE, display, 0); /* Add to first sort tab */
    }
}

/* Used by model_playlist_name_changed() to find the playlist that
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
void pm_name_changed (Playlist *playlist)
{
  GtkTreeModel *model = gtk_tree_view_get_model (playlist_treeview);
  if (model != NULL)
    gtk_tree_model_foreach (model, sr_model_playlist_name_changed, playlist);
}


/* If a song got changed (i.e. it's ID3 entries have changed), we check
   if it's in the currently displayed playlist, and if yes, we notify the
   first sort tab of a change */
void pm_song_changed (Song *song)
{
  gint i,n;

  if (!current_playlist) return;
  /* Check if song is member of current playlist */
  n = get_nr_of_songs_in_playlist (current_playlist);
  for (i=0; i<n; ++i)
    {
      if (song == get_song_in_playlist_by_nr (current_playlist, i))
	{  /* It's a member! Let's notify the first sort tab */
	  st_song_changed (song, FALSE, 0);
	  break;
	}
    }
}


/* Append playlist to the playlist model */
/* If @position = -1: append to end */
/* If @position >=0: insert at that position */
void pm_add_playlist (Playlist *playlist, gint position)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkTreeSelection *selection;

  model = gtk_tree_view_get_model (playlist_treeview);
  g_return_if_fail (model != NULL);

  if (position == -1)  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  else  gtk_list_store_insert (GTK_LIST_STORE (model), &iter, position);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		      PM_COLUMN_PLAYLIST, playlist, -1);
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
      if ((playlist->type == PL_TYPE_MPL) && prefs_get_mpl_autoselect())
      {
	  selection = gtk_tree_view_get_selection (playlist_treeview);
	  gtk_tree_selection_select_iter (selection, &iter);
      }
  }
}

/* Used by pm_remove_playlist() to remove playlist from model by calling
   gtk_tree_model_foreach () */ 
static gboolean pm_delete_playlist_fe (GtkTreeModel *model,
				       GtkTreePath *path,
				       GtkTreeIter *iter,
				       gpointer data)
{
  Playlist *playlist;

  gtk_tree_model_get (model, iter, PM_COLUMN_PLAYLIST, &playlist, -1);
  if(playlist == (Playlist *)data) {
    gtk_list_store_remove (GTK_LIST_STORE (model), iter);
    return TRUE;
  }
  return FALSE;
}


/* Remove "playlist" from the display model. 
   "select": TRUE: a new playlist is selected
             FALSE: no selection is taking place
                    (useful when quitting program) */
void pm_remove_playlist (Playlist *playlist, gboolean select)
{
  GtkTreeModel *model = gtk_tree_view_get_model (playlist_treeview);
  gboolean have_iter = FALSE;
  GtkTreeIter i,in;
  GtkTreeSelection *ts = NULL;

  if (model != NULL)
    {
      ts = gtk_tree_view_get_selection(playlist_treeview);
      if (select && (current_playlist == playlist))
	{
	  /* We are about to delete the currently selected
	     playlist. Try to select the next. */
	  if(gtk_tree_selection_get_selected(ts, NULL, &i))
	    {
	      if(gtk_tree_model_iter_next(model, &i))
		{
		  have_iter = TRUE;
		}
	    }
	}
      /* find the pl and delete it */
      gtk_tree_model_foreach (model, pm_delete_playlist_fe, playlist);
      if (select && (current_playlist == playlist) && !have_iter)
	{
	  /* We deleted the current playlist which was the last.
	     Now we try to select the currently last playlist */
	  if(gtk_tree_model_get_iter_first(model, &in))
	    {
	      i = in;
	      while (gtk_tree_model_iter_next (model, &in))
		{
		  i = in;
		}
	      have_iter = TRUE;
	    }
	}
      /* select our iter !!! */
      if (have_iter && select)   gtk_tree_selection_select_iter(ts, &i);
    }
}


/* Remove all playlists from the display model */
/* ATTENTION: the playlist_treeview and model might be changed by
   calling this function */
/* @clear_sort: TRUE: clear "sortable" setting of treeview */
void pm_remove_all_playlists (gboolean clear_sort)
{
  GtkTreeModel *model = gtk_tree_view_get_model (playlist_treeview);
  GtkTreeIter iter;
  gint column;
  GtkSortType order;

  while (gtk_tree_model_get_iter_first (model, &iter))
  {
      gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
  }
  if(clear_sort &&
     gtk_tree_sortable_get_sort_column_id (GTK_TREE_SORTABLE (model),
					   &column, &order))
  { /* recreate song treeview to unset sorted column */
      if (column >= 0)
      {
	  pm_create_treeview ();
      }
  }
}

/* Used by pm_select_playlist() to select the specified playlist by
   calling gtk_tree_model_foreach () */ 
static gboolean pm_select_playlist_fe (GtkTreeModel *model,
				       GtkTreePath *path,
				       GtkTreeIter *iter,
				       gpointer data)
{
    Playlist *playlist;

    gtk_tree_model_get (model, iter, PM_COLUMN_PLAYLIST, &playlist, -1);
    if(playlist == data)
    {
	GtkTreeSelection *ts = gtk_tree_view_get_selection (playlist_treeview);
	gtk_tree_selection_select_iter (ts, iter);
	return TRUE;
    }
    return FALSE;
}

/* Select specified playlist */
void pm_select_playlist (Playlist *playlist)
{
  GtkTreeModel *model = gtk_tree_view_get_model (playlist_treeview);

  if (model != NULL)
  {
      /* find the pl and select it */
      gtk_tree_model_foreach (model, pm_select_playlist_fe, playlist);
  }
}


static void pm_selection_changed_cb (gpointer user_data1, gpointer user_data2)
{
  GtkTreeSelection *selection = (GtkTreeSelection *)user_data1;
  GtkTreeModel *model;
  GtkTreeIter  iter;
  Playlist *new_playlist;

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
	  for (gl=new_playlist->members; gl; gl=gl->next)
	  { /* add all songs to sort tab 0 */
	      Song *song = gl->data;
	      if (stop_add == -1)  break;
	      st_add_song (song, FALSE, TRUE, 0);
	      --count;
	      if ((count < 0) && !prefs_get_block_display ())
	      {
		  gtkpod_songs_statusbar_update();
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
	  if (stop_add != -1) st_add_song (NULL, TRUE, TRUE, 0);
	  if (!prefs_get_block_display ())
	  {
	      while (gtk_events_pending ())	      gtk_main_iteration ();
	      release_selection (-1);
	  }
      }
      gtkpod_songs_statusbar_update();
  }
#if DEBUG_TIMING
  g_get_current_time (&time);
  printf ("pm_selection_changed_cb exit:  %ld.%06ld sec\n",
	  time.tv_sec % 3600, time.tv_usec);
#endif 
}

/* Callback function called when the selection
   of the playlist view has changed */
/* Instead of handling the selection directly, we add a
   "callback". Currently running display updates will be stopped
   before the pm_selection_changed_cb is actually called */
static void pm_selection_changed (GtkTreeSelection *selection,
				  gpointer user_data)
{
    add_selection_callback (-1, pm_selection_changed_cb,
			    (gpointer)selection, user_data);
}


/* Stop editing. If @cancel is TRUE, the edited value will be
   discarded (I have the feeling that the "discarding" part does not
   work quite the way intended). */
void pm_stop_editing (gboolean cancel)
{
    if (playlist_treeview)
    {
	GtkTreeViewColumn *col;
	gtk_tree_view_get_cursor (playlist_treeview, NULL, &col);
	if (col)
	{
	    if (!cancel && col->editable_widget)  
		gtk_cell_editable_editing_done (col->editable_widget);
	    if (col->editable_widget)
		gtk_cell_editable_remove_widget (col->editable_widget);
	}
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

    if((tm = gtk_tree_view_get_model(GTK_TREE_VIEW(playlist_treeview))))
    {
	GtkTreeIter i;
	gboolean valid = FALSE;
	gint pos = 0;

	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tm),&i);
	while(valid)
	{
	    Playlist *pl;

	    gtk_tree_model_get(tm, &i, 0, &pl, -1); 
	    if (get_playlist_by_nr (pos) != pl)
	    {
		/* move the playlist to indicated position */
		move_playlist (pl, pos);
		data_changed ();
	    }
	    valid = gtk_tree_model_iter_next(tm, &i);
	    ++pos;
	}
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

  if(gtk_tree_sortable_get_sort_column_id (GTK_TREE_SORTABLE (model),
					   &colid, &order) == FALSE)
      return 0;
  gtk_tree_model_get (model, a, colid, &playlist1, -1);
  gtk_tree_model_get (model, b, colid, &playlist2, -1);

  /* We make sure that the master playlist always stays on top */
  if (order == GTK_SORT_ASCENDING) corr = +1;
  else                             corr = -1;
  if (playlist1->type == PL_TYPE_MPL) return (-corr);
  if (playlist2->type == PL_TYPE_MPL) return (corr);

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
  GtkTreePath *path;
  GtkTreeIter iter;
  Playlist *playlist;
  gint column;

  model = (GtkTreeModel *)data;
  path = gtk_tree_path_new_from_string (path_string);
  column = (gint)g_object_get_data (G_OBJECT (renderer), "column");
  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get (model, &iter, column, &playlist, -1);

  /*printf("pm_cell_edited: column: %d  song:%lx\n", column, song);*/

  switch (column)
    {
    case PM_COLUMN_PLAYLIST:
      /* We only do something, if the name actually got changed */
      if (g_utf8_collate (playlist->name, new_text) != 0)
	{
	  g_free (playlist->name);
	  g_free (playlist->name_utf16);
	  playlist->name = g_strdup (new_text);
	  playlist->name_utf16 = g_utf8_to_utf16 (new_text, -1,
						  NULL, NULL, NULL);
	  data_changed ();
	}
      break;
    }
  gtk_tree_path_free (path);
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

  column = (gint)g_object_get_data (G_OBJECT (renderer), "column");
  gtk_tree_model_get (model, iter, PM_COLUMN_PLAYLIST, &playlist, -1);

  switch (column)
    {  /* We only have one column, so this code is overkill... */
    case PM_COLUMN_PLAYLIST: 
	g_object_set (G_OBJECT (renderer),
		      "text", playlist->name, 
		      "editable", TRUE,
		      "foreground", NULL,
		      NULL);
	if (playlist->type == PL_TYPE_MPL)
	{ /* mark MPL in red */
	    g_object_set (G_OBJECT (renderer), "foreground", "red", NULL);
	}
	break;
    }
}

/**
 * pm_song_column_button_clicked
 * @tvc - the tree view colum that was clicked
 * @data - ignored user data
 * When the sort button is clicked we want to update our internal playlist
 * representation to what's displayed on screen.
 */
static void
pm_song_column_button_clicked(GtkTreeViewColumn *tvc, gpointer data)
{
    if(prefs_get_save_sorted_order ())  pm_rows_reordered ();

}

/* Adds the columns to our playlist_treeview */
static void pm_add_columns ()
{
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkTreeModel *model = gtk_tree_view_get_model (playlist_treeview);


  /* playlist column */
  renderer = gtk_cell_renderer_text_new ();
  g_signal_connect (G_OBJECT (renderer), "edited",
		    G_CALLBACK (pm_cell_edited), model);
  g_object_set_data (G_OBJECT (renderer), "column", (gint *)PM_COLUMN_PLAYLIST);
  column = gtk_tree_view_column_new_with_attributes (_("Playlists"), renderer, NULL);
  gtk_tree_view_column_set_cell_data_func (column, renderer, pm_cell_data_func, NULL, NULL);
  gtk_tree_view_column_set_sort_column_id (column, PM_COLUMN_PLAYLIST);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_column_set_sort_order (column, GTK_SORT_ASCENDING);
  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
				   PM_COLUMN_PLAYLIST,
				   pm_data_compare_func, column, NULL);
  gtk_tree_view_column_set_clickable(column, TRUE);
  g_signal_connect (G_OBJECT (column), "clicked",
		    G_CALLBACK (pm_song_column_button_clicked),
				(gpointer)PM_COLUMN_PLAYLIST);
  gtk_tree_view_append_column (playlist_treeview, column);
}


/* Start sorting */
void pm_sort (GtkSortType order)
{
    GtkTreeModel *model;

    if (playlist_treeview)
    {
	model = gtk_tree_view_get_model (playlist_treeview);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model),
					      PM_COLUMN_PLAYLIST, order);
    }
}

static void pm_select_current_position (gint x, gint y)
{
    if (playlist_treeview)
    {
	GtkTreePath *path;

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
}

static gboolean
pm_button_press (GtkWidget *w, GdkEventButton *e, gpointer data)
{
    if(w && e)
    {
	switch(e->button)
	{
	    case 3:
		pm_select_current_position (e->x, e->y);
		pm_context_menu_init ();
		return TRUE;
	    default:
		break;
	}
	
    }
    return(FALSE);
}

/* Create playlist listview */
void pm_create_treeview (void)
{
  GtkTreeModel *model;
  GtkTreeSelection *selection;
  GtkWidget *playlist_window;
  GtkWidget *tree;

  playlist_window = lookup_widget (gtkpod_window, "playlist_window");
  /* destroy old treeview */
  if (playlist_treeview)
  {
      /* FIXME: how do we delete the model? */
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
  model =   GTK_TREE_MODEL (gtk_list_store_new (PM_NUM_COLUMNS,
						G_TYPE_POINTER));
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
  gtk_tree_view_enable_model_drag_dest (playlist_treeview,
					pm_drop_types, TGNR (pm_drop_types),
					GDK_ACTION_COPY);
  /* need the gtk_drag_dest_set() with no actions ("0") so that the
     data_received callback gets the correct info value. This is most
     likely a bug... */
  gtk_drag_dest_set_target_list (GTK_WIDGET (playlist_treeview),
				 gtk_target_list_new (pm_drop_types,
						      TGNR (pm_drop_types)));

  g_signal_connect ((gpointer) playlist_treeview, "drag_data_get",
		    G_CALLBACK (on_playlist_treeview_drag_data_get),
		    NULL);
  g_signal_connect ((gpointer) playlist_treeview, "drag_data_received",
		    G_CALLBACK (on_playlist_treeview_drag_data_received),
		    NULL);
  g_signal_connect_after ((gpointer) playlist_treeview, "key_release_event",
			  G_CALLBACK (on_playlist_treeview_key_release_event),
			  NULL);
  g_signal_connect (G_OBJECT (playlist_treeview), "button-press-event",
		    G_CALLBACK (pm_button_press), model);
}



Playlist*
pm_get_selected_playlist(void)
{
/* return(current_playlist);*/
/* we can't just return the "current_playlist" because the context
   menus require the selection before "current_playlist" is updated */

    Playlist *result = NULL;

    if (playlist_treeview)
    {
	GtkTreeSelection *ts = gtk_tree_view_get_selection (playlist_treeview);
	GtkTreeIter iter;
	GtkTreeModel *model;

	if (gtk_tree_selection_get_selected (ts, &model, &iter))
	{
	    gtk_tree_model_get (model, &iter,
				PM_COLUMN_PLAYLIST, &result, -1);
	}
    }
    return result;
}

/* use with care!! */
void
pm_set_selected_playlist(Playlist *pl)
{
    current_playlist = pl;
}


/* ---------------------------------------------------------------- */
/* Section for drag and drop                                        */
/* ---------------------------------------------------------------- */

#if ((GTK_MAJOR_VERSION == 2) && (GTK_MINOR_VERSION < 2))
/* gtk_list_store_move_*() was introduced in 2.2, so we have to
 * emulate for 2.0 <= V < 2.2 (we require at least 2.0 anyway) */
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

    /* get the content (playlist) of the row to move */
    gtk_tree_model_get (model, iter, PM_COLUMN_PLAYLIST, &playlist, -1);
    /* remove the old row */
    gtk_list_store_remove (GTK_LIST_STORE (model), iter);
    /* set the content of the new row */
    gtk_list_store_set (GTK_LIST_STORE (model), &new_iter,
			PM_COLUMN_PLAYLIST, playlist, -1);
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
#else
/* starting V2.2 convenient gtk functions exist */
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


/* move pathlist for song treeview */
gboolean pm_move_pathlist (gchar *data,
			   GtkTreePath *path,
			   GtkTreeViewDropPosition pos)
{
    return pmsm_move_pathlist (playlist_treeview, data, path, pos, PLAYLIST_TREEVIEW);
}

/*
 * utility function for appending ipod song ids for playlist
 */
void 
on_pm_dnd_get_id_foreach(GtkTreeModel *tm, GtkTreePath *tp, 
			 GtkTreeIter *i, gpointer data)
{
    Playlist *pl;
    GString *idlist = (GString *)data;

    gtk_tree_model_get (tm, i, PM_COLUMN_PLAYLIST, &pl, -1);
    if(pl && idlist)
    {
	GList *l;
	Song *s;

	/* add all member-ids of entry to idlist */
	for (l=pl->members; l; l=l->next)
	{
	    s = (Song *)l->data;
	    g_string_append_printf (idlist, "%d\n", s->ipod_id);
	}
    }
}

/*
 * utility function for appending file for song view (DND)
 */
void 
on_pm_dnd_get_file_foreach(GtkTreeModel *tm, GtkTreePath *tp, 
			   GtkTreeIter *iter, gpointer data)
{
    Playlist *pl;
    GString *filelist = (GString *)data;

    /* get current playlist */
    gtk_tree_model_get(tm, iter, PM_COLUMN_PLAYLIST, &pl, -1); 
    if (pl && filelist)
    {
	Song *s;
	gchar *name;
	GList *l;

	for (l=pl->members; l; l=l->next)
	{
	    s = (Song *)l->data;
	    name = get_song_name_on_disk_verified (s);
	    if (name)
	    {
		g_string_append_printf (filelist, "file:%s\n", name);
		g_free (name);
	    }
	}
    }
}

