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

#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#include "support.h"
#include "prefs.h"
#include "display.h"
#include "song.h"
#include "playlist.h"
#include "interface.h"
#include "callbacks.h"
#include "misc.h"
#include "file.h"
#include "context_menus.h"


/* pointer to the treeview for the song display */
static GtkTreeView *song_treeview = NULL;
/* pointer to the treeview for the playlist display */
static GtkTreeView *playlist_treeview = NULL;
/* array with pointers to the columns used in the song display */
static GtkTreeViewColumn *sm_columns[SM_NUM_COLUMNS];
/* array with pointers to the sort tabs */
static SortTab *sorttab[SORT_TAB_MAX];
/* pointer to paned elements holding the sort tabs */
static GtkPaned *st_paned[PANED_NUM_ST];
/* pointer to the currently selected playlist */
static Playlist *current_playlist = NULL;

/* used for stopping of display refresh */
typedef void (*br_callback)(gpointer user_data1, gpointer user_data2);
static void block_selection (gint inst);
static void release_selection (gint inst);
static void add_selection_callback (gint inst, br_callback brc, gpointer user_data1, gpointer user_data2);
static gboolean selection_callback_timeout (gpointer data);
static gint stop_add = SORT_TAB_MAX;

/* used for display organization */
static void pm_create_treeview (void);
static void sm_song_changed (Song *song);
static void sm_remove_song (Song *song);
static void sm_remove_all_songs (void);
static GtkTreeViewColumn *sm_add_column (SM_item sm_item, gint position);
static void sm_create_treeview (void);
static void st_song_changed (Song *song, gboolean removed, guint32 inst);
static void st_add_song (Song *song, gboolean final, gboolean display, guint32 inst);
static void st_remove_song (Song *song, guint32 inst);
static void st_init (ST_CAT_item new_category, guint32 inst);
static void st_create_notebook (gint inst);

/* Drag and drop definitions */
#define TGNR(a) (guint)(sizeof(a)/sizeof(GtkTargetEntry))
#define DND_GTKPOD_IDLIST_TYPE "application/gtkpod-idlist"
#define DND_GTKPOD_SM_PATHLIST_TYPE "application/gtkpod-sm_pathlist"
#define DND_GTKPOD_PM_PATHLIST_TYPE "application/gtkpod-pm_pathlist"
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
static GtkTargetEntry st_drag_types [] = {
    { DND_GTKPOD_IDLIST_TYPE, 0, DND_GTKPOD_IDLIST },
    { "text/plain", 0, DND_TEXT_PLAIN },
    { "STRING", 0, DND_TEXT_PLAIN }
};
static GtkTargetEntry sm_drag_types [] = {
    { DND_GTKPOD_SM_PATHLIST_TYPE, 0, DND_GTKPOD_SM_PATHLIST },
    { DND_GTKPOD_IDLIST_TYPE, 0, DND_GTKPOD_IDLIST },
    { "text/plain", 0, DND_TEXT_PLAIN },
    { "STRING", 0, DND_TEXT_PLAIN }
};
static GtkTargetEntry sm_drop_types [] = {
    { DND_GTKPOD_SM_PATHLIST_TYPE, 0, DND_GTKPOD_SM_PATHLIST },
    { DND_GTKPOD_IDLIST_TYPE, 0, DND_GTKPOD_IDLIST },
    { "text/plain", 0, DND_TEXT_PLAIN },
    { "STRING", 0, DND_TEXT_PLAIN }
};
struct asf_data
{
    GtkTreeIter *to_iter;
    GtkTreeViewDropPosition pos;
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
static gboolean pm_delete_playlist (GtkTreeModel *model,
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
      gtk_tree_model_foreach (model, pm_delete_playlist, playlist);
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
static void pm_remove_all_playlists (gboolean clear_sort)
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


static void pm_selection_changed_cb (gpointer user_data1, gpointer user_data2)
{
  GtkTreeSelection *selection = (GtkTreeSelection *)user_data1;
  GtkTreeModel *model;
  GtkTreeIter  iter;
  Playlist *new_playlist;
  Song *song;
  guint32 n,i;

#if DEBUG_TIMING
  GTimeVal time;
  g_get_current_time (&time);
  printf ("pm_selection_changed_cb enter: %ld.%06ld sec\n",
	  time.tv_sec % 3600, time.tv_usec);
#endif DEBUG_TIMING

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
      n = get_nr_of_songs_in_playlist (new_playlist);
      if (n > 0)
      {
	  GTimeVal time;
	  float max_count = REFRESH_INIT_COUNT;
	  gint count = max_count - 1;
	  float ms;
	  if (!prefs_get_block_display ())
	  {
	      block_selection (-1);
	      g_get_current_time (&time);
	  }
	  for (i=0; i<n; ++i)
	  { /* add all songs to sort tab 0 */
	      if (stop_add == -1)  break;
	      song = get_song_in_playlist_by_nr (new_playlist, i);
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
#endif DEBUG_TIMING
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

  /* otherwise just compare the entries */
  return g_utf8_collate (g_utf8_casefold (playlist1->name, -1), 
			     g_utf8_casefold (playlist2->name, -1));
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
	g_object_set (G_OBJECT (renderer), "text", playlist->name, 
		      "editable", TRUE, NULL);
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
				(gpointer)SM_COLUMN_TITLE);
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


static gboolean
pm_button_release(GtkWidget *w, GdkEventButton *e, gpointer data)
{
    if(w && e)
    {
	switch(e->button)
	{
	    case 3:
		pm_context_menu_init();
		break;
	    default:
		break;
	}
	
    }
    return(FALSE);
}

/* Create playlist listview */
static void pm_create_treeview (void)
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
  g_signal_connect (G_OBJECT (playlist_treeview), "button-release-event",
		    G_CALLBACK (pm_button_release), model);
}



/* ---------------------------------------------------------------- */
/* Section for sort tab display                                     */
/* ---------------------------------------------------------------- */


/* Get the instance of the sort tab that corresponds to
   "notebook". Returns -1 if sort tab could not be found
   and prints error message */
static gint st_get_instance_from_notebook (GtkNotebook *notebook)
{
    gint i;

    for(i=0; i<SORT_TAB_MAX; ++i)
    {
	if (sorttab[i] && (sorttab[i]->notebook == notebook)) return i;
    }
/*  g_warning ("Programming error (st_get_instance_from_notebook): notebook could
    not be found.\n"); function somehow can get called after notebooks got
    destroyed */
    return -1;
}

/* Get the instance of the sort tab that corresponds to
   "treeview". Returns -1 if sort tab could not be found
   and prints error message */
gint st_get_instance_from_treeview (GtkTreeView *tv)
{
    gint i,cat;

    for(i=0; i<SORT_TAB_MAX; ++i)
    {
	for(cat=0; cat<ST_CAT_NUM; ++cat)
	{
	    if (sorttab[i] && (sorttab[i]->treeview[cat] == tv)) return i;
	}
    }
    return -1;
}


/* returns the selected entry (used by delete_entry_head() */
TabEntry *st_get_selected_entry (gint inst)
{
    if ((inst >= 0) && (inst < SORT_TAB_MAX) && sorttab[inst])
	return sorttab[inst]->current_entry;
    return NULL;
}


/* Append playlist to the playlist model. */
static void st_add_entry (TabEntry *entry, guint32 inst)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    SortTab *st;

    st = sorttab[inst];
    model = st->model;
    g_return_if_fail (model != NULL);
    gtk_list_store_append (GTK_LIST_STORE (model), &iter);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter,
			ST_COLUMN_ENTRY, entry, -1);
    st->entries = g_list_append (st->entries, entry);
    if (!entry->master)
    {
	if (!st->entry_hash)
	{
	    st->entry_hash = g_hash_table_new (g_str_hash, g_str_equal);
	}
	g_hash_table_insert (st->entry_hash, entry->name, entry);
    }
}

/* Used by st_remove_entry_from_model() to remove entry from model by calling
   gtk_tree_model_foreach () */ 
static gboolean st_delete_entry_from_model (GtkTreeModel *model,
					    GtkTreePath *path,
					    GtkTreeIter *iter,
					    gpointer data)
{
  TabEntry *entry;

  gtk_tree_model_get (model, iter, ST_COLUMN_ENTRY, &entry, -1);
  if(entry == (TabEntry *)data)
  {
      gtk_list_store_remove (GTK_LIST_STORE (model), iter);
      return TRUE;
  }
  return FALSE;
}


/* Remove entry from the display model and the sorttab */
static void st_remove_entry_from_model (TabEntry *entry, guint32 inst)
{
    SortTab *st = sorttab[inst];
    GtkTreeModel *model = st->model;
    if (model && entry)
    {
/* 	printf ("entry: %p, cur_entry: %p\n", entry, st->current_entry); */
	if (entry == st->current_entry)
	{
	    GtkTreeSelection *selection = gtk_tree_view_get_selection
		(st->treeview[st->current_category]);
	    st->current_entry = NULL;
	    /* We have to unselect the previous selection */
	    gtk_tree_selection_unselect_all (selection);
	}
	gtk_tree_model_foreach (model, st_delete_entry_from_model, entry);
	st->entries = g_list_remove (st->entries, entry);
	g_list_free (entry->members);
	/* remove entry from hash */
	if (st->entry_hash)
	{
	    TabEntry *hashed_entry = 
		(TabEntry *)g_hash_table_lookup (st->entry_hash, entry->name);
	    if (hashed_entry == entry)
		g_hash_table_remove (st->entry_hash, entry->name);
	}
	C_FREE (entry->name);
	g_free (entry);
    }
}

/* Remove all entries from the display model and the sorttab */
/* @clear_sort: reset sorted columns to the non-sorted state */
static void st_remove_all_entries_from_model (gboolean clear_sort,
					      guint32 inst)
{
  TabEntry *entry;
  SortTab *st = sorttab[inst];
  gint column;
  GtkSortType order;

  if (st)
  {
      if (st->current_entry)
      {
	  GtkTreeSelection *selection = gtk_tree_view_get_selection
	      (st->treeview[st->current_category]);
	  st->current_entry = NULL;
	  /* We may have to unselect the previous selection */
	  gtk_tree_selection_unselect_all (selection);
      }
      while (st->entries != NULL)
      {
	  entry = (TabEntry *)g_list_nth_data (st->entries, 0);
	  st_remove_entry_from_model (entry, inst);
      }
      if (st->entry_hash)  g_hash_table_destroy (st->entry_hash);
      st->entry_hash = NULL;

      if(clear_sort &&
	 gtk_tree_sortable_get_sort_column_id (GTK_TREE_SORTABLE (st->model),
					       &column, &order))
      { /* recreate song treeview to unset sorted column */
	  if (column >= 0)
	  {
	      st_create_notebook (inst);
	  }
      }
  }
}

/* Remove "entry" from the model (used by delete_entry_ok()). The
 * entry should be empty (otherwise it's not removed).
 * If "entry" is the master entry 'All', the sort tab is redisplayed
 * (it's empty).
 * If the entry is currently selected (usually will be), the next
 * or previous entry will be selected automatically (unless it's the
 * master entry and prefs_get_st_autoselect() says don't select the 'All'
 * entry). If no new entry is selected, the next sort tab will be
 * redisplayed (should be empty) */
void st_remove_entry (TabEntry *entry, guint32 inst)
{
    TabEntry *next=NULL;
    GtkTreeIter iter;
    GtkTreeSelection *selection;
    SortTab *st = sorttab[inst];

    if (!entry) return;
    /* is the entry empty (contains no songs)? */
    if (g_list_length (entry->members) != 0) return;
    /* if the entry is the master entry 'All' -> the tab is empty,
       re-init tab */
    if (entry->master)
    {
	st_init (-1, inst);
	return;
    }

    /* is the entry currently selected? Remember! */
    selection = gtk_tree_view_get_selection (st->treeview[st->current_category]);
#if 0  /* it doesn't make much sense to select the next entry, or? */
    if (sorttab[inst]->current_entry == entry)
    {
	gboolean valid;
	TabEntry *entry2=NULL;
	GtkTreeIter iter2;
	/* what's the next entry (displayed)? */
	if (gtk_tree_selection_get_selected (selection, NULL, &iter))
	{ /* found selected entry -> now chose next one */
	    if (gtk_tree_model_iter_next (st->model, &iter))
	    {
		gtk_tree_model_get(st->model, &iter, ST_COLUMN_ENTRY, &next, -1);
	    }
	    else
	    { /* no next entry, try to find previous one */
		/* There doesn't seem to be a ..._iter_previous()
		 * call... */
		next = NULL;
		valid = gtk_tree_model_get_iter_first(st->model, &iter2);
		while(valid)
		{
		    gtk_tree_model_get(st->model, &iter2, ST_COLUMN_ENTRY, &entry2, -1);
		    if (entry == entry2)   break;  /* found it */
		    iter = iter2;
		    next = entry2;
		    valid = gtk_tree_model_iter_next(st->model, &iter2);
		}
		if (!valid) next = NULL;
	    }
	    /* don't select master entry 'All' until requested to do so */
	    if (next && next->master && !prefs_get_st_autoselect (inst))
		next = NULL;
	}
    }
#endif
    /* remove entry from display model */
    st_remove_entry_from_model (entry, inst);
    /* if we have a next entry, select it. */
    if (next)
    {
	gtk_tree_selection_select_iter (selection, &iter);
    }
}

/* Get the correct name for the entry according to currently
   selected category (page). Do _not_ g_free() the return value! */
static gchar *st_get_entryname (Song *song, guint32 inst)
{
    S_item s_item = ST_to_S (sorttab[inst]->current_category);

    return song_get_item_utf8 (song, s_item);
}


/* Returns the entry "song" is stored in or NULL. The master entry
   "All" is skipped */
static TabEntry *st_get_entry_by_song (Song *song, guint32 inst)
{
  GList *entries;
  TabEntry *entry;
  guint i;

  if (song == NULL) return NULL;
  entries = sorttab[inst]->entries;
  i=1; /* skip master entry, which is supposed to be at first position */
  while ((entry = (TabEntry *)g_list_nth_data (entries, i)) != NULL)
    {
      if (g_list_find (entry->members, song) != NULL)   break; /* found! */
      ++i;
    }
  return entry;
}


/* Find TabEntry with name "name". Return NULL if no entry was found.
   If "name" is {-1, 0x00}, it returns the master entry. Otherwise
   it skips the master entry. */
static TabEntry *st_get_entry_by_name (gchar *name, guint32 inst)
{
  TabEntry *entry = NULL;
  SortTab *st = sorttab[inst];
  GList *entries = st->entries;

  if (name == NULL) return NULL;
  /* check if we need to return the master entry */
  if ((strlen (name) == 1) && (*name == -1))
  {
      entry = (TabEntry *)g_list_nth_data (entries, 0);
  }
  else
  {
      if (st->entry_hash)
	  entry = g_hash_table_lookup (st->entry_hash, name);
  }
  return entry;
}


/* moves a song from the entry it is currently in to the one it
   should be in according to its tags (if a Tag had been changed).
   Returns TRUE, if song has been moved, FALSE otherwise */
/* 07 Feb 2003: I decided that recategorizing is a bad thing: the
   current code only moves the songs "up" in the entry list, so it's
   incomplete. More important: it leaves the display in an
   inconsistent state: the songs are not removed from the
   songview (this would confuse the user). But if he changes the entry
   again, nothing happens to the songs displayed, because they are no
   longer members. Merging the two identical entries is no option
   either, because that takes away the possibility to easily "undo"
   what you have just done. It's also not intuitive if you have
   additional songs appear on the screen. JCS */
static gboolean st_recategorize_song (Song *song, guint32 inst)
{
#if 0
  TabEntry *oldentry, *newentry;
  gchar *entryname;

  oldentry = st_get_entry_by_song (song, inst);
  /*  printf("%d: recat_oldentry: %x\n", inst, oldentry);*/
  /* should not happen: song is not in sort tab */
  if (oldentry == NULL) return FALSE;
  entryname = st_get_entryname (song, inst);
  newentry = st_get_entry_by_name (entryname, inst);
  if (newentry == NULL)
    { /* not found, create new one */
      newentry = g_malloc0 (sizeof (TabEntry));
      newentry->name = g_strdup (entryname);
      newentry->master = FALSE;
      st_add_entry (newentry, inst);
    }
  if (newentry != oldentry)
    { /* song category changed */
      /* add song to entry members list */
      newentry->members = g_list_append (newentry->members, song); 
      /* remove song from old entry members list */
      oldentry->members = g_list_remove (oldentry->members, song);
      /*  printf("%d: recat_return_TRUE\n", inst);*/
      return TRUE;
    }
  /*  printf("%d: recat_return_FALSE\n", inst);*/
#endif
  return FALSE;
}


/* Some tags of a song currently stored in a sort tab have been changed.
   - if not "removed"
     - if the song is in the entry currently selected:
       - remove entry and put into correct category
       - if current entry != "All":
         - if sort category changed:
           - notify next sort tab ("removed")
	 - if sort category did not change:
	   - notify next sort tab ("not removed")
       - if current entry == "All":
         - notify next sort tab ("not removed")
     - if the song is not in the entry currently selected (I don't know
       how that could happen, though):
       - if sort category changed: remove entry and put into correct category
       - if this "correct" category is selected, call st_add_song for next
         instance.
   - if "removed"
     - remove the song from the sort tab
     - if song was in the entry currently selected, notify next instance
       ("removed")
  "removed": song has been removed from sort tab. This is different
  from st_remove_song, because we will not notify the song model if a
  song has been removed: it might confuse the user if the song, whose
  tabs he/she just edited, disappeared from the display */
static void st_song_changed (Song *song, gboolean removed, guint32 inst)
{
  SortTab *st;
  TabEntry *master, *entry;

  if (inst == prefs_get_sort_tab_num ())
    {
      sm_song_changed (song);
      return;
    }
  else if (inst < prefs_get_sort_tab_num ())
  {
      st = sorttab[inst];
      master = g_list_nth_data (st->entries, 0);
      if (master == NULL) return; /* should not happen */
      /* if song is not in tab, don't proceed (should not happen) */
      if (g_list_find (master->members, song) == NULL) return;
      if (removed)
      {
	  /* remove "song" from master entry "All" */
	  master->members = g_list_remove (master->members, song);
	  /* find entry which other entry contains the song... */
	  entry = st_get_entry_by_song (song, inst);
	  /* ...and remove it */
	  if (entry) entry->members = g_list_remove (entry->members, song);
	  if ((st->current_entry == entry) || (st->current_entry == master))
	      st_song_changed (song, TRUE, inst+1);
      }
      else
      {
	  if (st->current_entry &&
	      g_list_find (st->current_entry->members, song) != NULL)
	  { /* "song" is in currently selected entry */
	      if (!st->current_entry->master)
	      { /* it's not the master list */
		  if (st_recategorize_song (song, inst))
		      st_song_changed (song, TRUE, inst+1);
		  else st_song_changed (song, FALSE, inst+1);
	      }
	      else
	      { /* master entry ("All") is currently selected */
		  st_recategorize_song (song, inst);
		  st_song_changed (song, FALSE, inst+1);
	      }
	  }
	  else
	  { /* "song" is not in an entry currently selected */
	      if (st_recategorize_song (song, inst))
	      { /* song was moved to a different entry */
		  if (st_get_entry_by_song (song, inst) == st->current_entry)
		  { /* this entry is selected! */
		      st_add_song (song, TRUE, TRUE, inst+1);
		  }
	      }
	  }
      }
  }
}


/* Reorders the songs stored in the sort tabs according to the order
 * in the selected playlist. This has to be done e.g. if we change the
 * order in the song view.
 * 
 * Right now I simply delete all members of all tab entries, then add
 * the songs again without having them added to the song view. For my
 * 2459 songs that takes approx. 1.3 seconds (850 MHz AMD Duron) */
static void st_adopt_order_in_playlist ()
{
    gint inst;

#if DEBUG_TIMING
    GTimeVal time;
    g_get_current_time (&time);
    printf ("st_adopt_order_in_playlist enter: %ld.%06ld sec\n",
	    time.tv_sec % 3600, time.tv_usec);
#endif DEBUG_TIMING

    /* first delete all songs in all visible sort tabs */
    for (inst = 0; inst< prefs_get_sort_tab_num (); ++inst)
    {
	SortTab *st = sorttab[inst];
	GList *link;
	for (link=st->entries; link; link=link->next)
	{   /* in each entry delete all songs */
	    TabEntry *entry = (TabEntry *)link->data;
	    g_list_free (entry->members);
	    entry->members = NULL;
	}
    }

    /* now add the songs again, without adding them to the song view */
    if (current_playlist)
    {
	GList *link;

	for (link=current_playlist->members; link; link=link->next)
	{
	    st_add_song ((Song *)link->data, FALSE, FALSE, 0);
	}
    }
#if DEBUG_TIMING
    g_get_current_time (&time);
    printf ("st_adopt_order_in_playlist enter: %ld.%06ld sec\n",
	    time.tv_sec % 3600, time.tv_usec);
#endif DEBUG_TIMING
}


/* Add song to sort tab. If the song matches the currently
   selected sort criteria, it will be passed on to the next
   sort tab. The last sort tab will pass the song on to the
   song model (currently two sort tabs).
   When the first song is added, the "All" entry is created.
   If prefs_get_st_autoselect(inst) is true, the "All" entry is
   automatically selected, if there was no former selection
   @display: TRUE: add to song model (i.e. display it) */
static void st_add_song (Song *song, gboolean final, gboolean display, guint32 inst)
{
  static gint count = 0;
  TabEntry *entry, *master_entry, *iter_entry;
  SortTab *st;
  gchar *entryname;
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  TabEntry *select_entry = NULL;
  gboolean first = FALSE;

  if (inst == prefs_get_sort_tab_num ())
  {  /* just add to song model */
      if ((song != NULL) && display)    sm_add_song_to_song_model (song, NULL);
      if (final || (++count % 20 == 0))
	  gtkpod_songs_statusbar_update();
  }
  else if (inst < prefs_get_sort_tab_num ())
  {
      st = sorttab[inst];
      st->final = final;

/*       if (song)   printf ("%d: add song: %s\n", inst, song->title); */
/*       else        printf ("%d: add song: %p\n", inst, song); */

      if (song != NULL)
      {
	  /* add song to "All" (master) entry */
	  master_entry = g_list_nth_data (st->entries, 0);
	  if (master_entry == NULL)
	  { /* doesn't exist yet -- let's create it */
	      master_entry = g_malloc0 (sizeof (TabEntry));
	      master_entry->name = g_strdup (_("All"));
	      master_entry->master = TRUE;
	      st_add_entry (master_entry, inst);
	      first = TRUE; /* this is the first song */
	  }
	  master_entry->members = g_list_append (master_entry->members, song);
	  /* Check whether entry of same name already exists */
	  entryname = st_get_entryname (song, inst);
	  entry = st_get_entry_by_name (entryname, inst);
	  if (entry == NULL)
	  { /* not found, create new one */
	      entry = g_malloc0 (sizeof (TabEntry));
	      entry->name = g_strdup (entryname);
	      entry->master = FALSE;
	      st_add_entry (entry, inst);
	  }
	  /* add song to entry members list */
	  entry->members = g_list_append (entry->members, song);
	  /* add song to next tab if "entry" is selected */
	  if (st->current_entry &&
	      ((st->current_entry->master) || (entry == st->current_entry)))
	  {
	      st_add_song (song, final, display, inst+1);
	  }
	  /* check if we should select some entry */
	  if (!st->current_entry)
	  {
	      if (st->lastselection[st->current_category] == NULL)
	      {
		  /* no last selection -- check if we should select "All" */
		  /* only select "All" when currently adding the first song */
		  if (first && prefs_get_st_autoselect (inst))
		  {
		      select_entry = master_entry;
		  }
	      }
	      else
	      {
		  /* select current entry if it corresponds to the last
		     selection, or last_entry if that's the master entry */
		  TabEntry *last_entry = st_get_entry_by_name (
		      st->lastselection[st->current_category], inst);
		  if (last_entry &&
		      ((entry == last_entry) || last_entry->master))
		  {
		      select_entry = last_entry;
		  }
	      }
	  }
      }
      /* select "All" if it's the last song added, no entry currently
	 selected (including "select_entry", which is to be selected" and
	 prefs_get_st_autoselect() allows us to select "All" */
      if (final && !st->current_entry && !select_entry &&
	  !st->unselected && prefs_get_st_autoselect (inst))
      { /* auto-select entry "All" */
	  select_entry = g_list_nth_data (st->entries, 0);
      }

      if (select_entry)
      {  /* select current select_entry */
/* 	  printf("%d: selecting: %p: %s\n", inst, select_entry, select_entry->name); */
	  if (!gtk_tree_model_get_iter_first (st->model, &iter))
	  {
	      g_warning ("Programming error: st_add_song: iter invalid\n");
	      return;
	  }
	  do {
	      gtk_tree_model_get (st->model, &iter, 
				  ST_COLUMN_ENTRY, &iter_entry,
				  -1);
	      if (iter_entry == select_entry)
	      {
		  selection = gtk_tree_view_get_selection
		      (st->treeview[st->current_category]);
		  /* We may need to unselect the previous selection */
		  /* gtk_tree_selection_unselect_all (selection); */
		  st->current_entry = select_entry;
		  gtk_tree_selection_select_iter (selection, &iter);
		  break;
	      }
	  } while (gtk_tree_model_iter_next (st->model, &iter));
      }
      else if (!song && final)
      {
	  st_add_song (NULL, final, display, inst+1);
      }
  }
}


/* Remove song from sort tab. If the song matches the currently
   selected sort criteria, it will be passed on to the next
   sort tab (i.e. removed).
   The last sort tab will remove the
   song from the song model (currently two sort tabs). */
/* 02. Feb 2003: bugfix: song is always passed on to the next sort
 * tab: it might have been recategorized, but still be displayed. JCS */
static void st_remove_song (Song *song, guint32 inst)
{
  TabEntry *master, *entry;
  SortTab *st;

  if (inst == prefs_get_sort_tab_num ())
    {
      sm_remove_song (song);
    }
  else if (inst < prefs_get_sort_tab_num ())
    {
      st = sorttab[inst];
      master = g_list_nth_data (st->entries, 0);
      if (master == NULL) return; /* should not happen! */
      /* remove "song" from master entry "All" */
      master->members = g_list_remove (master->members, song);
      /* find entry which other entry contains the song... */
      entry = st_get_entry_by_song (song, inst);
      /* ...and remove it */
      if (entry) entry->members = g_list_remove (entry->members, song);
      st_remove_song (song, inst+1);
    }
}


/* Init a sort tab: all current entries are removed. The next sort tab
   is initialized as well (st_init (-1, inst+1)).  Set new_category to
   -1 if the current category is to be left unchanged */
/* Normally we do not specifically remember the "All" entry and will
   select "All" in accordance to the prefs settings. */
static void st_init (ST_CAT_item new_category, guint32 inst)
{
  SortTab *st;
  ST_CAT_item cat;

  if (inst == prefs_get_sort_tab_num ())
  {
      sm_remove_all_songs ();
      gtkpod_songs_statusbar_update ();
      return;
  }
  st = sorttab[inst];
  if (inst < prefs_get_sort_tab_num ())
  {
      if (st == NULL) return; /* could happen during initialisation */
      st->unselected = FALSE; /* nothing was unselected so far */
      st->final = TRUE;       /* all songs are added */
      cat = st->current_category;
#if 0
      if (st->current_entry != NULL)
      {
	  if (!st->current_entry->master)
	  {
	      C_FREE (st->lastselection[cat]);
	      st->lastselection[cat] = g_strdup (st->current_entry->name);
	  }
/* don't remember entry 'All' */
#if 0
	  else
	  {
	      gchar buf[] = {-1, 0}; /* this is how I mark the "All"
				      * entry as string: should be
				      * illegal UTF8 */
	      C_FREE (st->lastselection[cat]);
	      st->lastselection[cat] = g_strdup (buf);*/
	  }
#endif
      }
#endif
  }
  if (new_category != -1)
  {
      st->current_category = new_category;
      prefs_set_st_category (inst, new_category);
  }
  if (inst < prefs_get_sort_tab_num ())
  {
      st_remove_all_entries_from_model (FALSE, inst);
      st_init (-1, inst+1);
  }
}


static void st_page_selected_cb (gpointer user_data1, gpointer user_data2)
{
  GtkNotebook *notebook = (GtkNotebook *)user_data1;
  guint page = (guint)user_data2;
  guint32 inst;
  GList *copy = NULL;
  SortTab *st;
  gint i,n;
  Song *song;

#if DEBUG_TIMING
  GTimeVal time;
  g_get_current_time (&time);
  printf ("st_page_selected_cb enter: %ld.%06ld sec\n",
	  time.tv_sec % 3600, time.tv_usec);
#endif DEBUG_TIMING

  inst = st_get_instance_from_notebook (notebook);
/*  printf("ps%d: cat: %d\n", inst, page);*/

  if (inst == -1) return; /* invalid notebook */
  if (stop_add < (gint)inst)  return;
  st = sorttab[inst];
  /* re-initialize current instance */
  st_init (page, inst);
  /* Get list of songs to re-insert */
  if (inst == 0)
  {
      if (current_playlist)  copy = current_playlist->members;
  }
  else
  {
      if (sorttab[inst-1] && sorttab[inst-1]->current_entry)
	  copy = sorttab[inst-1]->current_entry->members;
  }
  n = g_list_length (copy);
  if (n > 0)
  {
      GTimeVal time;
      float max_count = REFRESH_INIT_COUNT;
      gint count = max_count - 1;
      float ms;
      /* block playlist view and all sort tab notebooks <= inst */
      if (!prefs_get_block_display ())
      {
	  block_selection (inst-1);
	  g_get_current_time (&time);
      }
      /* add all songs previously present to sort tab */
      for (i=0; i<n; ++i)
      {
	  if (stop_add < (gint)inst)  break;
	  song = (Song *)g_list_nth_data (copy, i);
	  st_add_song (song, FALSE, TRUE, inst);
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
	      printf("st_p_s ms: %f mc: %f\n", ms, max_count);
#endif
	  }
      }
      if (n && (stop_add >= (gint)inst))
      {
	  gboolean final = TRUE;  /* playlist is always complete */
	  /* if playlist is not source, get final flag from
	   * corresponding sorttab */
	  if ((inst > 0) && (sorttab[inst-1])) final = sorttab[inst-1]->final;
	  st_add_song (NULL, final, TRUE, inst);
      }
      if (!prefs_get_block_display ())
      {
	  while (gtk_events_pending ())      gtk_main_iteration ();
	  release_selection (inst-1);
      }
  }
#if DEBUG_TIMING
  g_get_current_time (&time);
  printf ("st_page_selected_cb exit:  %ld.%06ld sec\n",
	  time.tv_sec % 3600, time.tv_usec);
#endif DEBUG_TIMING
}


/* Called when page in sort tab is selected */
/* Instead of handling the selection directly, we add a
   "callback". Currently running display updates will be stopped
   before the st_page_selected_cb is actually called */
void st_page_selected (GtkNotebook *notebook, guint page)
{
  guint32 inst;

  inst = st_get_instance_from_notebook (notebook);
  if (inst == -1) return; /* invalid notebook */
  /* inst-1: changing a page in the first sort tab is like selecting a
     new playlist and so on. Therefore we subtract 1 from the
     instance. */
  add_selection_callback (inst-1, st_page_selected_cb,
			  (gpointer)notebook, (gpointer)page);
}


/* Redisplay the sort tab "inst" */
void st_redisplay (guint32 inst)
{
    if (inst < prefs_get_sort_tab_num ())
    {
	if (sorttab[inst])
	    st_page_selected (sorttab[inst]->notebook,
			      sorttab[inst]->current_category);
    }
}

/* Start sorting */
void st_sort (guint32 inst, GtkSortType order)
{
    if (inst < prefs_get_sort_tab_num ())
    {
	if (sorttab[inst])
	    gtk_tree_sortable_set_sort_column_id (
		GTK_TREE_SORTABLE (sorttab[inst]->model),
		ST_COLUMN_ENTRY, order);
    }
}


static void st_selection_changed_cb (gpointer user_data1, gpointer user_data2)
{
  GtkTreeSelection *selection = (GtkTreeSelection *)user_data1;
  guint32 inst = (guint32)user_data2;
  GtkTreeModel *model;
  GtkTreeIter  iter;
  TabEntry *new_entry;
  Song *song;
  SortTab *st;
  guint32 n,i;

#if DEBUG_TIMING
  GTimeVal time;
  g_get_current_time (&time);
  printf ("st_selection_changed_cb enter: %ld.%06ld sec\n",
	  time.tv_sec % 3600, time.tv_usec);
#endif DEBUG_TIMING

/*   printf("st_s_c_cb %d: entered\n", inst); */
  st = sorttab[inst];
  if (st == NULL) return;
  if (gtk_tree_selection_get_selected (selection, &model, &iter) == FALSE)
  {
      /* no selection -- unselect current selection (unless
         st->current_entry == NULL -- that means we're in the middle
         of a  st_init() (removing all entries). In that case we don't
	 want to forget our last selection! */
      if (st->current_entry)
      {
	  st->current_entry = NULL;
	  C_FREE (st->lastselection[st->current_category]);
	  st->unselected = TRUE;
      }
	  st_init (-1, inst+1);
  }
  else
  {   /* handle new selection */
      gtk_tree_model_get (model, &iter, 
			  ST_COLUMN_ENTRY, &new_entry,
			  -1);
      /* printf("selected instance %d, entry %x (was: %x)\n", inst,
       * new_entry, st->current_entry);*/

      /* initialize next instance */
      st_init (-1, inst+1);
      /* remember new selection */
      st->current_entry = new_entry;
      if (!new_entry->master)
      {
	  C_FREE (st->lastselection[st->current_category]);
	  st->lastselection[st->current_category] = g_strdup (new_entry->name);
      }
      st->unselected = FALSE;

      n = g_list_length (new_entry->members); /* number of members */
      if (n > 0)
      {
	  GTimeVal time;
	  float max_count = REFRESH_INIT_COUNT;
	  gint count = max_count - 1;
	  float ms;
	  if (!prefs_get_block_display ())
	  {
	      block_selection (inst);
	      g_get_current_time (&time);
	  }
	  for (i=0; i<n; ++i)
	  { /* add all member songs to next instance */
	      if (stop_add <= (gint)inst) break;
	      song = (Song *)g_list_nth_data (new_entry->members, i);
	      st_add_song (song, FALSE, TRUE, inst+1);
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
		  printf("st_s_c ms: %f mc: %f\n", ms, max_count);
#endif
	      }
	  }
	  if (stop_add > (gint)inst)  st_add_song (NULL, TRUE, st->final, inst+1);
	  if (!prefs_get_block_display ())
	  {
	      while (gtk_events_pending ())	  gtk_main_iteration ();
	      release_selection (inst);
	  }
      }
      gtkpod_songs_statusbar_update();
  }
#if DEBUG_TIMING
  g_get_current_time (&time);
  printf ("st_selection_changed_cb exit:  %ld.%06ld sec\n",
	  time.tv_sec % 3600, time.tv_usec);
#endif DEBUG_TIMING
}


/* Callback function called when the selection
   of the sort tab view has changed */
/* Instead of handling the selection directly, we add a
   "callback". Currently running display updates will be stopped
   before the st_selection_changed_cb is actually called */
static void st_selection_changed (GtkTreeSelection *selection,
				  gpointer user_data)
{
/*     printf("st_s_c\n"); */
    add_selection_callback ((gint)user_data, st_selection_changed_cb,
			    (gpointer)selection, user_data);
}


/* Called when editable cell is being edited. Stores new data to
   the entry list and changes all members. */
static void
st_cell_edited (GtkCellRendererText *renderer,
		const gchar         *path_string,
		const gchar         *new_text,
		gpointer             data)
{
  GtkTreeModel *model;
  GtkTreePath *path;
  GtkTreeIter iter;
  TabEntry *entry;
  ST_item column;
  gint i, n, inst;
  GList *members;
  SortTab *st;

  inst = (guint32)data;
  st = sorttab[inst];
  model = st->model;
  path = gtk_tree_path_new_from_string (path_string);
  column = (ST_item)g_object_get_data (G_OBJECT (renderer), "column");
  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get (model, &iter, column, &entry, -1);

  /*printf("Inst %d: st_cell_edited: column: %d  :%lx\n", inst, column, entry);*/

  switch (column)
    {
    case ST_COLUMN_ENTRY:
      /* We only do something, if the name actually got changed */
      if (g_utf8_collate (entry->name, new_text) != 0)
      {
	  /* remove old hash entry if available */
	  TabEntry *hash_entry =
	      g_hash_table_lookup (st->entry_hash, entry->name);
	  if (hash_entry == entry)
	      g_hash_table_remove (st->entry_hash, entry->name);
	  /* replace entry name */
	  g_free (entry->name);
	  entry->name = g_strdup (new_text);
	  /* re-insert into hash table if the same name doesn't
	     already exist */
	  if (g_hash_table_lookup (st->entry_hash, entry->name) == NULL)
	      g_hash_table_insert (st->entry_hash, entry->name, entry);
	  /* Now we look up all the songs and change the ID3 Tag as well */
	  /* We make a copy of the current members list, as it may change
             during the process */
	  members = g_list_copy (entry->members);
	  n = g_list_length (members);
	  /* block user input if we write tags (might take a while) */
	  if (prefs_get_id3_write ())   block_widgets ();
	  for (i=0; i<n; ++i)
	  {
	      Song *song = (Song *)g_list_nth_data (members, i);
	      S_item s_item = ST_to_S (sorttab[inst]->current_category);
	      gchar **itemp_utf8 = song_get_item_pointer_utf8 (song, s_item);
	      gunichar2 **itemp_utf16 = song_get_item_pointer_utf16(song, s_item);
	      g_free (*itemp_utf8);
	      g_free (*itemp_utf16);
	      *itemp_utf8 = g_strdup (new_text);
	      *itemp_utf16 = g_utf8_to_utf16 (new_text, -1, NULL, NULL, NULL);
	      pm_song_changed (song);
	      /* If prefs say to write changes to file, do so */
	      if (prefs_get_id3_write ())
	      {
		  S_item tag_id;
		  /* should we update all ID3 tags or just the one
		     changed? */
		  if (prefs_get_id3_writeall ()) tag_id = S_ALL;
		  else		                 tag_id = s_item;
		  write_tags_to_file (song, tag_id);
		  while (widgets_blocked && gtk_events_pending ())
		      gtk_main_iteration ();
	      }
	  }
	  g_list_free (members);
	  /* allow user input again */
	  if (prefs_get_id3_write ())   release_widgets ();
	  /* display possible duplicates that have been removed */
	  remove_duplicate (NULL, NULL);
	  data_changed (); /* indicate that data has changed */
	}
      break;
    default:
	break;
    }
  gtk_tree_path_free (path);
}


/* The sort tab entries are stored in a separate list (sorttab->entries)
   and only pointers to the corresponding TabEntry structure are placed
   into the model.
   This function reads the data for the given cell from the list and
   passes it to the renderer. */
static void st_cell_data_func (GtkTreeViewColumn *tree_column,
			       GtkCellRenderer   *renderer,
			       GtkTreeModel      *model,
			       GtkTreeIter       *iter,
			       gpointer           data)
{
  TabEntry *entry;
  gint column;
  gboolean editable;

  column = (gint)g_object_get_data (G_OBJECT (renderer), "column");
  gtk_tree_model_get (model, iter, ST_COLUMN_ENTRY, &entry, -1);

  switch (column)
    {  /* We only have one column, so this code is overkill... */
    case ST_COLUMN_ENTRY: 
      if (entry->master) editable = FALSE;
      else               editable = TRUE;
      g_object_set (G_OBJECT (renderer), "text", entry->name, 
		    "editable", editable, NULL);
      break;
    }
}

/* Function used to compare two cells during sorting (sorttab view) */
gint st_data_compare_func (GtkTreeModel *model,
			   GtkTreeIter *a,
			   GtkTreeIter *b,
			   gpointer user_data)
{
  TabEntry *entry1;
  TabEntry *entry2;
  GtkSortType order;
  gint corr, colid;

  gtk_tree_model_get (model, a, ST_COLUMN_ENTRY, &entry1, -1);
  gtk_tree_model_get (model, b, ST_COLUMN_ENTRY, &entry2, -1);
  if(gtk_tree_sortable_get_sort_column_id (GTK_TREE_SORTABLE (model),
					   &colid, &order) == FALSE)
      return 0;

  /* We make sure that the "all" entry always stays on top */
  if (order == GTK_SORT_ASCENDING) corr = +1;
  else                             corr = -1;
  if (entry1->master) return (-corr);
  if (entry2->master) return (corr);

  /* Otherwise return the comparison */
  return g_utf8_collate (g_utf8_casefold (entry1->name, -1), 
			 g_utf8_casefold (entry2->name, -1));
}


/* Make the appropriate number of sort tab instances visible */
/* Also: make the menu items "more/less sort tabs" active/inactive as
 * needed */
void st_show_visible (void)
{
    gint i,n;
    GtkWidget *w;

    if (!st_paned[0])  return;

    /* first initialize (clear) all sorttabs */
    n = prefs_get_sort_tab_num ();
    prefs_set_sort_tab_num (SORT_TAB_MAX, FALSE);
    st_init (-1, 0);
    prefs_set_sort_tab_num (n, FALSE);

    /* set the visible elements */
    for (i=0; i<n; ++i)
    {
	gtk_widget_show (GTK_WIDGET (sorttab[i]->notebook));
	if (i < PANED_NUM_ST)	gtk_widget_show (GTK_WIDGET (st_paned[i]));
    }
    /* set the invisible elements */
    for (i=n; i<SORT_TAB_MAX; ++i)
    {
	gtk_widget_hide (GTK_WIDGET (sorttab[i]->notebook));
	if (i < PANED_NUM_ST)	gtk_widget_hide (GTK_WIDGET (st_paned[i]));
    }

    /* activate / deactiveate "less sort tabs" menu item */
    w = lookup_widget (gtkpod_window, "less_sort_tabs");
    if (n == 0) gtk_widget_set_sensitive (w, FALSE);
    else        gtk_widget_set_sensitive (w, TRUE);

    /* activate / deactiveate "more sort tabs" menu item */
    w = lookup_widget (gtkpod_window, "more_sort_tabs");
    if (n == SORT_TAB_MAX) gtk_widget_set_sensitive (w, FALSE);
    else                   gtk_widget_set_sensitive (w, TRUE);

    /* redisplay */
    st_redisplay (0);
}


/* Created paned elements for sorttabs */
static void st_create_paned (void)
{
    gint i;

    /* sanity check */
    if (st_paned[0])  return;

    for (i=0; i<PANED_NUM_ST; ++i)
    {
	GtkWidget *paned;

	paned = gtk_hpaned_new ();
	gtk_widget_show (paned);
 	if (i==0)
	{
	    GtkWidget *parent;
	    parent = lookup_widget (gtkpod_window, "paned1");
	    gtk_paned_pack1 (GTK_PANED (parent), paned, TRUE, TRUE);
	}
	else
	{
	    gtk_paned_pack2 (st_paned[i-1], paned, TRUE, TRUE);
	}
	st_paned[i] = GTK_PANED (paned);
    }
}

static gboolean
st_button_release_event(GtkWidget *w, GdkEventButton *e, gpointer data)
{
    if(w && e)
    {
	switch(e->button)
	{
	    case 3:
		st_context_menu_init((gint)data);
		break;
	    default:
		break;
	}
	
    }
    return(FALSE);
}

/* Create songs listview */
static void st_create_listview (gint inst)
{
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkTreeModel *model;
  GtkListStore *liststore;
  GtkTreeView *treeview;
  GtkTreeSelection *selection;
  gint i;
  SortTab *st = sorttab[inst];

  /* create model */
  if (st->model)
  {
      /* FIXME: how do we delete the model? */
  }
  liststore = gtk_list_store_new (ST_NUM_COLUMNS, G_TYPE_POINTER);
  model = GTK_TREE_MODEL (liststore);
  st->model = model;
  /* set tree views */
  for (i=0; i<ST_CAT_NUM; ++i)
    {
      treeview = st->treeview[i];
      gtk_tree_view_set_model (treeview, model);
      gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (treeview), TRUE);
      gtk_tree_selection_set_mode (gtk_tree_view_get_selection (treeview),
				   GTK_SELECTION_SINGLE);
      selection = gtk_tree_view_get_selection (treeview);
      g_signal_connect (G_OBJECT (selection), "changed",
			G_CALLBACK (st_selection_changed), (gpointer)inst);
      /* Add column */
      renderer = gtk_cell_renderer_text_new ();
      g_signal_connect (G_OBJECT (renderer), "edited",
			G_CALLBACK (st_cell_edited), GINT_TO_POINTER(inst));
      g_object_set_data (G_OBJECT (renderer), "column",
			 (gint *)ST_COLUMN_ENTRY);
      column = gtk_tree_view_column_new ();
      gtk_tree_view_column_pack_start (column, renderer, TRUE);
      column = gtk_tree_view_column_new_with_attributes ("", renderer, NULL);
      gtk_tree_view_column_set_cell_data_func (column, renderer,
					       st_cell_data_func, NULL, NULL);
      gtk_tree_view_column_set_sort_column_id (column, ST_COLUMN_ENTRY);
      gtk_tree_view_column_set_resizable (column, TRUE);
      gtk_tree_view_column_set_sort_order (column, GTK_SORT_ASCENDING);
      gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (liststore),
				       ST_COLUMN_ENTRY,
				       st_data_compare_func, column, NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_set_headers_visible (treeview, FALSE);
      gtk_drag_source_set (GTK_WIDGET (treeview), GDK_BUTTON1_MASK,
		      st_drag_types, TGNR (st_drag_types), GDK_ACTION_COPY);
      g_signal_connect (G_OBJECT (treeview), "button-release-event",
			G_CALLBACK (st_button_release_event), GINT_TO_POINTER(inst));
    }
}


/* create the treeview for category @st_cat of instance @inst */
static void st_create_treeview (gint inst, ST_CAT_item st_cat)
{
  GtkWidget *st0_notebook;
  GtkWidget *st0_window0;
  GtkWidget *st0_cat0_treeview;
  GtkWidget *st0_label0 = NULL;
  SortTab *st = sorttab[inst];

  /* destroy treeview if already present */
  if (st->treeview[st_cat])
  {
      gtk_widget_destroy (GTK_WIDGET (st->treeview[st_cat]));
      st->treeview[st_cat] = NULL;
  }

  st0_notebook = GTK_WIDGET (st->notebook);

  if (st->window[st_cat])
  {
      st0_window0 = GTK_WIDGET (st->window[st_cat]);
  }
  else
  {   /* create window if not already present */
      st0_window0 = gtk_scrolled_window_new (NULL, NULL);
      gtk_widget_show (st0_window0);
      gtk_container_add (GTK_CONTAINER (st0_notebook), st0_window0);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (st0_window0), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
      st->window[st_cat] = st0_window0;
  }
  /* create treeview */
  st0_cat0_treeview = gtk_tree_view_new ();
  gtk_widget_show (st0_cat0_treeview);
  gtk_container_add (GTK_CONTAINER (st0_window0), st0_cat0_treeview);

  switch (st_cat)
  {
  case ST_CAT_ARTIST:
      st0_label0 = gtk_label_new (_("Artist"));
      break;
  case ST_CAT_ALBUM:
      st0_label0 = gtk_label_new (_("Album"));
      break;
  case ST_CAT_GENRE:
      st0_label0 = gtk_label_new (_("Genre"));
      break;
  case ST_CAT_COMPOSER:
      st0_label0 = gtk_label_new (_("Comp."));
      break;
  case ST_CAT_TITLE:
      st0_label0 = gtk_label_new (_("Title"));
      break;
  case ST_CAT_NUM:
      st0_label0 = gtk_label_new (("Programming Error"));
      break;
  }
  gtk_widget_show (st0_label0);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (st0_notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (st0_notebook), st_cat), st0_label0);
  gtk_label_set_justify (GTK_LABEL (st0_label0), GTK_JUSTIFY_LEFT);
  g_signal_connect ((gpointer) st0_cat0_treeview, "drag_data_get",
                    G_CALLBACK (on_st_treeview_drag_data_get),
                    NULL);
  g_signal_connect_after ((gpointer) st0_cat0_treeview, "key_release_event",
                          G_CALLBACK (on_st_treeview_key_release_event),
                          NULL);

  st->treeview[st_cat] = GTK_TREE_VIEW (st0_cat0_treeview);
}


/* create all ST_CAT_NUM treeviews in sort tab of instance @inst, then
 * set the model */
static void st_create_treeviews (gint inst)
{
  st_create_treeview (inst, ST_CAT_ARTIST);
  st_create_treeview (inst, ST_CAT_ALBUM);
  st_create_treeview (inst, ST_CAT_GENRE);
  st_create_treeview (inst, ST_CAT_COMPOSER);
  st_create_treeview (inst, ST_CAT_TITLE);
  st_create_listview (inst);
}  


/* Create notebook and fill in sorttab[@inst] */
static void st_create_notebook (gint inst)
{
  GtkWidget *st0_notebook;
  GtkPaned *paned;
  gint i, page;
  SortTab *st = sorttab[inst];

  if (st->notebook)
  {
      gtk_widget_destroy (GTK_WIDGET (st->notebook));
      st->notebook = NULL;
      for (i=0; i<ST_CAT_NUM; ++i)
      {
	  st->treeview[i] = NULL;
	  st->window[i] = NULL;
      }
  }

  /* paned elements exist? */
  if (!st_paned[0])   st_create_paned ();

  st0_notebook = gtk_notebook_new ();
  if (inst < prefs_get_sort_tab_num ()) gtk_widget_show (st0_notebook);
  else                                  gtk_widget_hide (st0_notebook);
  /* which pane? */
  if (inst == SORT_TAB_MAX-1)  i = inst-1;
  else                         i = inst;
  paned = st_paned[i];
  /* how to pack? */
  if (inst == SORT_TAB_MAX-1)
      gtk_paned_pack2 (paned, st0_notebook, TRUE, TRUE);
  else
      gtk_paned_pack1 (paned, st0_notebook, FALSE, TRUE);
  gtk_notebook_set_scrollable (GTK_NOTEBOOK (st0_notebook), TRUE);
  g_signal_connect ((gpointer) st0_notebook, "switch_page",
                    G_CALLBACK (on_sorttab_switch_page),
                    NULL);

  st->notebook = GTK_NOTEBOOK (st0_notebook);
  st_create_treeviews (inst);
  page = prefs_get_st_category (inst);
  st->current_category = page;
  gtk_notebook_set_current_page (st->notebook, page);
/*  st_init (page, inst);*/
}


/* Create sort tabs */
static void st_create_tabs ()
{
  gint inst;
/*   gchar *name; */

  /* we count downward here because the smaller sort tabs might try to
     initialize the higher one's -> create the higher ones first */
  for (inst=SORT_TAB_MAX-1; inst>=0; --inst)
    {
	sorttab[inst] = g_malloc0 (sizeof (SortTab));
/*       name = g_strdup_printf ("st%d_notebook", inst); */
/*       sorttab[inst]->notebook = GTK_NOTEBOOK (lookup_widget (gtkpod, name)); */
/*       g_free (name); */
      st_create_notebook (inst);
    }
  st_show_visible ();
}

/* Clean up the memory used by sort tabs (program quit). */
static void cleanup_sort_tabs (void)
{
  gint i,j;
  for (i=0; i<SORT_TAB_MAX; ++i)
    {
      if (sorttab[i] != NULL)
	{
	  st_remove_all_entries_from_model (FALSE, i);
	  for (j=0; j<ST_CAT_NUM; ++j)
	    {
		C_FREE (sorttab[i]->lastselection[j]);
	    }
	  g_free (sorttab[i]);
	  sorttab[i] = NULL;
	}
    }
}

/* ---------------------------------------------------------------- */
/* Section for song display                                         */
/* ---------------------------------------------------------------- */


/* Append song to the song model (or write into @into_iter if != 0) */
void sm_add_song_to_song_model (Song *song, GtkTreeIter *into_iter)
{
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model (song_treeview);

    g_return_if_fail (model != NULL);

    if (into_iter)
	iter = *into_iter;
    else
	gtk_list_store_append (GTK_LIST_STORE (model), &iter);

    gtk_list_store_set (GTK_LIST_STORE (model), &iter,
			SM_COLUMN_TITLE, song,
			SM_COLUMN_ARTIST, song,
			SM_COLUMN_ALBUM, song,
			SM_COLUMN_GENRE, song,
			SM_COLUMN_COMPOSER, song,
			SM_COLUMN_TRACK_NR, song,
			SM_COLUMN_IPOD_ID, song,
			SM_COLUMN_PC_PATH, song,
			SM_COLUMN_TRANSFERRED, song,
			-1);
}



/* Used by remove_song() to remove song from model by calling
   gtk_tree_model_foreach ().
   Entry is deleted if data == song */
static gboolean sm_delete_song (GtkTreeModel *model,
				GtkTreePath *path,
				GtkTreeIter *iter,
				gpointer data)
{
  Song *song;

  gtk_tree_model_get (model, iter, SM_COLUMN_ALBUM, &song, -1);
  if(song == (Song *)data) {
    gtk_list_store_remove (GTK_LIST_STORE (model), iter);
    return TRUE;
  }
  return FALSE;
}


/* Remove song from the display model */
static void sm_remove_song (Song *song)
{
  GtkTreeModel *model = gtk_tree_view_get_model (song_treeview);
  if (model != NULL)
    gtk_tree_model_foreach (model, sm_delete_song, song);
}


/* Remove all songs from the display model */
/* ATTENTION: the treeview and model might be changed by calling this
   function */
static void sm_remove_all_songs (void)
{
  GtkTreeModel *model = gtk_tree_view_get_model (song_treeview);
  GtkTreeIter iter;
  gint column;
  GtkSortType order;

  while (gtk_tree_model_get_iter_first (model, &iter))
  {
      gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
  }
  if(gtk_tree_sortable_get_sort_column_id (GTK_TREE_SORTABLE (model),
					   &column, &order))
  { /* recreate song treeview to unset sorted column */
      if (column >= 0)
      {
	  sm_store_col_order ();
	  display_update_default_sizes ();
	  sm_create_treeview ();
      }
  }
}


/* find out at which position column @sm_item is displayed */
/* static gint sm_get_col_position (SM_item sm_item) */
/* { */
/*     gint i; */
/*     GtkTreeViewColumn *col; */

/*     if (!song_treeview) return -1; */

/*     for (i=0; i<SM_NUM_COLUMNS_PREFS; ++i) */
/*     { */
/* 	col = gtk_tree_view_get_column (song_treeview, i); */
/* 	if (col->sort_column_id == sm_item) return i; */
/*     } */
/*     return -1; */
/* } */


/* store the order of the song view columns */
void sm_store_col_order (void)
{
    gint i;
    GtkTreeViewColumn *col;

    for (i=0; i<SM_NUM_COLUMNS_PREFS; ++i)
    {
	col = gtk_tree_view_get_column (song_treeview, i);
	prefs_set_col_order (i, col->sort_column_id);
    }
}


/* Used by sm_song_changed() to find the song that
   changed name. If found, emit a "row changed" signal */
static gboolean sm_model_song_changed (GtkTreeModel *model,
				       GtkTreePath *path,
				       GtkTreeIter *iter,
				       gpointer data)
{
  Song *song;

  gtk_tree_model_get (model, iter, SM_COLUMN_ALBUM, &song, -1);
  if(song == (Song *)data) {
    gtk_tree_model_row_changed (model, path, iter);
    return TRUE;
  }
  return FALSE;
}


/* One of the songs has changed (this happens when the
   iTunesDB is read and some IDs are renumbered */
static void sm_song_changed (Song *song)
{
  GtkTreeModel *model = gtk_tree_view_get_model (song_treeview);
  /*  printf("sm_song_changed enter\n");*/
  if (model != NULL)
    gtk_tree_model_foreach (model, sm_model_song_changed, song);
  /*  printf("sm_song_changed exit\n");*/
}





/* Called when editable cell is being edited. Stores new data to
   the song list. Eventually the ID3 tags in the corresponding
   files should be changed as well, if activated in the pref settings */
static void
sm_cell_edited (GtkCellRendererText *renderer,
		const gchar         *path_string,
		const gchar         *new_text,
		gpointer             data)
{
  GtkTreeModel *model;
  GtkTreePath *path;
  GtkTreeIter iter;
  Song *song;
  SM_item column;
  gboolean changed = FALSE; /* really changed anything? */
  gchar *track_text = NULL;
  gchar **itemp_utf8 = NULL;
  gunichar2 **itemp_utf16 = NULL;

  model = (GtkTreeModel *)data;
  path = gtk_tree_path_new_from_string (path_string);
  column = (SM_item)g_object_get_data (G_OBJECT (renderer), "column");
  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get (model, &iter, column, &song, -1);

  /*printf("sm_cell_edited: column: %d  song:%lx\n", column, song);*/

  switch (column)
  {
  case SM_COLUMN_ALBUM:
  case SM_COLUMN_ARTIST:
  case SM_COLUMN_TITLE:
  case SM_COLUMN_GENRE:
  case SM_COLUMN_COMPOSER:
      itemp_utf8 = song_get_item_pointer_utf8 (song, SM_to_S (column));
      itemp_utf16 = song_get_item_pointer_utf16 (song, SM_to_S (column));
      if (g_utf8_collate (*itemp_utf8, new_text) != 0)
      {
	  g_free (*itemp_utf8);
	  g_free (*itemp_utf16);
	  *itemp_utf8 = g_strdup (new_text);
	  *itemp_utf16 = g_utf8_to_utf16 (new_text, -1, NULL, NULL, NULL);
	  changed = TRUE;
      }
      break;
  case SM_COLUMN_TRACK_NR:
      track_text = g_strdup_printf("%d", song->track_nr);
      if (g_utf8_collate(track_text, new_text) != 0)
      {
	  song->track_nr = atoi(new_text);
	  changed = TRUE;
      }
      break;
  default:
      fprintf(stderr, "Unknown song cell edited with value %d\n", column);
      break;
  }
  if (changed)
  {
      pm_song_changed (song); /* notify playlist model... */
      data_changed ();        /* indicate that data has changed */
  }
  /* If anything changed and prefs say to write changes to file, do so */
  if (changed && prefs_get_id3_write ())
  {
      S_item tag_id;
      /* should we update all ID3 tags or just the one
	 changed? */
      if (prefs_get_id3_writeall ()) tag_id = S_ALL;
      else		             tag_id = SM_to_S (column);
      write_tags_to_file (song, tag_id);
      /* display possible duplicates that have been removed */
      remove_duplicate (NULL, NULL);
  }
  gtk_tree_path_free (path);
}


/* The song data is stored in a separate list (static GList *songs)
   and only pointers to the corresponding Song structure are placed
   into the model.
   This function reads the data for the given cell from the list and
   passes it to the renderer. */
static void sm_cell_data_func (GtkTreeViewColumn *tree_column,
			       GtkCellRenderer   *renderer,
			       GtkTreeModel      *model,
			       GtkTreeIter       *iter,
			       gpointer           data)
{
  Song *song;
  SM_item column;
  gchar text[11];
  gchar *item_utf8 = NULL;

  column = (SM_item)g_object_get_data (G_OBJECT (renderer), "column");
  gtk_tree_model_get (model, iter, SM_COLUMN_ALBUM, &song, -1);

  switch (column)
  {
  case SM_COLUMN_TITLE:
  case SM_COLUMN_ARTIST:
  case SM_COLUMN_ALBUM:
  case SM_COLUMN_GENRE:
  case SM_COLUMN_COMPOSER:
      item_utf8 = song_get_item_utf8 (song, SM_to_S (column));
      g_object_set (G_OBJECT (renderer), "text", item_utf8, 
		    "editable", TRUE, NULL);
      break;
  case SM_COLUMN_TRACK_NR:
      if (song->track_nr >= 0)
      {
	  snprintf (text, 10, "%d", song->track_nr);
	  g_object_set (G_OBJECT (renderer), "text", text, "editable", TRUE,
			NULL);
      } 
      else
      {
	  g_object_set (G_OBJECT (renderer), "text", "0", NULL);
      }
      break;
  case SM_COLUMN_IPOD_ID:
      if (song->ipod_id != -1)
      {
	  snprintf (text, 10, "%d", song->ipod_id);
	  g_object_set (G_OBJECT (renderer), "text", text, NULL);
      } 
      else
      {
	  g_object_set (G_OBJECT (renderer), "text", "--", NULL);
      }
      break;
  case SM_COLUMN_PC_PATH:
      g_object_set (G_OBJECT (renderer), "text", song->pc_path_utf8, NULL);
      break;
  case SM_COLUMN_TRANSFERRED:
      g_object_set (G_OBJECT (renderer), "active", song->transferred, NULL);
      break;
  default:
      gtkpod_warning("Unknown column in sm_cell_data_func: %d\n", column);
      break;
  }
}

/**
 * sm_get_nr_of_songs - get the number of songs displayed
 * currently in the song model Returns - the number of songs displayed
 * currently
 */
guint
sm_get_nr_of_songs(void)
{
    GtkTreeIter i;
    guint result = 0;
    gboolean valid = FALSE;
    GtkTreeModel *tm = NULL;
			    
    if((tm = gtk_tree_view_get_model(GTK_TREE_VIEW(song_treeview))))
    {
	if((valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tm),&i)))
	{
	    result++;
	    while((valid = gtk_tree_model_iter_next(tm,&i)))
		result++;
	}
    }
    return(result);

}


static gint comp_int (gconstpointer a, gconstpointer b)
{
    return ((gint)a)-((gint)b);
}


/**
 * Reorder songs in playlist to match order of songs displayed in song
 * view. Only the subset of songs currently displayed is reordered.
 * data_changed() is called when necessary.
 */
void
sm_rows_reordered (void)
{
    Playlist *current_pl = pm_get_selected_playlist();
    gboolean changed = FALSE;
		    
    if(current_pl)
    {
	GtkTreeModel *tm = NULL;

	if((tm = gtk_tree_view_get_model(GTK_TREE_VIEW(song_treeview))))
	{
	    GtkTreeIter i;
	    GList *new_list = NULL, *old_pos_l = NULL;
	    gboolean valid = FALSE;
	    GList *nlp, *olp;

	    valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tm),&i);
	    while(valid)
	    {
		Song *new_song;
		gint old_position;

		gtk_tree_model_get(tm, &i, 0, &new_song, -1); 
		new_list = g_list_append(new_list, new_song);
		/* what position was this song in before? */
		old_position = g_list_index (current_pl->members, new_song);
		/* check if we already used this position before (can
		   happen if song has been added to playlist more than
		   once */
		while ((old_position != -1) &&
		       g_list_find (old_pos_l, (gpointer)old_position))
		{  /* find next occurence */
		    GList *link;
		    gint next;
		    link = g_list_nth (current_pl->members, old_position + 1);
		    next = g_list_index (link, new_song);
		    if (next == -1)   old_position = -1;
		    else              old_position += (1+next);
		}
		/* we make a sorted list of the old positions */
		old_pos_l = g_list_insert_sorted (old_pos_l,
						  (gpointer)old_position,
						  comp_int);
		valid = gtk_tree_model_iter_next(tm, &i);
	    }
	    nlp = new_list;
	    olp = old_pos_l;
	    while (nlp && olp)
	    {
		GList *old_link;
		gint position = (gint)olp->data;

		/* if position == -1 one of the songs in the song view
		   could not be found in the selected playlist -> stop! */
		if (position == -1)
		{
		    printf("Prgramming error (sm_rows_reordered_callback): song in song view was not in selected playlist\n");
		    break;
		}
		old_link = g_list_nth (current_pl->members, position);
		/* replace old song with new song */
		if (old_link->data != nlp->data)
		{
		    old_link->data = nlp->data;
		    changed = TRUE;
		}
		/* next */
		nlp = nlp->next;
		olp = olp->next;
	    }
	    g_list_free (new_list);
	    g_list_free (old_pos_l);
	}
    }
    /* if we changed data, mark data as changed and adopt order in
       sort tabs */
    if (changed)
    {
	data_changed ();
	st_adopt_order_in_playlist ();
    }
}


static void
on_selected_songids_list_foreach ( GtkTreeModel *tm, GtkTreePath *tp, 
				 GtkTreeIter *i, gpointer data)
{
    Song *s = NULL;
    GList *l = *((GList**)data);
    gtk_tree_model_get(tm, i, 0, &s, -1);
    /* can call on 0 cause s is consistent across all of the columns */
    if(s)
    {
	l = g_list_append(l, (gpointer)s->ipod_id);
	*((GList**)data) = l;
    }
}

GList *
sm_get_selected_songids(void)
{
    GList *result = NULL;
    GtkTreeSelection *ts = NULL;

    if((ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(song_treeview))))
    {
	gtk_tree_selection_selected_foreach(ts,on_selected_songids_list_foreach,
					    &result);
    }
    return(result);
}


static void
on_selected_songs_list_foreach ( GtkTreeModel *tm, GtkTreePath *tp, 
				 GtkTreeIter *i, gpointer data)
{
    Song *s = NULL;
    GList *l = *((GList**)data);
    gtk_tree_model_get(tm, i, 0, &s, -1);
    /* can call on 0 cause s is consistent across all of the columns */
    if(s)
    {
	l = g_list_append(l, s);
	*((GList**)data) = l;
    }
}

GList *
sm_get_selected_songs(void)
{
    GList *result = NULL;
    GtkTreeSelection *ts = NULL;

    if((ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(song_treeview))))
    {
	gtk_tree_selection_selected_foreach(ts,on_selected_songs_list_foreach,
					    &result);
    }
    return(result);
}


#if ((GTK_MAJOR_VERSION == 2) && (GTK_MINOR_VERSION < 2))
/* gtk_list_store_move_*() was introduced in 2.2, so we have to
 * emulate for 2.0 <= V < 2.2 (we require at least 2.0 anyway) */
static void sm_list_store_move (GtkListStore *store,
				 GtkTreeIter  *iter,
				 GtkTreeIter  *position,
				 gboolean     before)
{
    GtkTreeIter new_iter;
    Song *song = NULL;
    GtkTreeModel *model;

    /* insert new row before or after @position */
    if (before)  gtk_list_store_insert_before (store, &new_iter, position);
    else         gtk_list_store_insert_after (store, &new_iter, position);

    model = gtk_tree_view_get_model (song_treeview);

    /* get the content (song) of the row to move */
    gtk_tree_model_get (model, iter, SM_COLUMN_ALBUM, &song, -1);
    /* remove the old row */
    gtk_list_store_remove (GTK_LIST_STORE (model), iter);

    /* set the content of the new row */
    sm_add_song_to_song_model (song, &new_iter);
}


static void  sm_list_store_move_before (GtkListStore *store,
					 GtkTreeIter  *iter,
					 GtkTreeIter  *position)
{
    sm_list_store_move (store, iter, position, TRUE);
}


static void  sm_list_store_move_after (GtkListStore *store,
					GtkTreeIter  *iter,
					GtkTreeIter  *position)
{
    sm_list_store_move (store, iter, position, FALSE);
}

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


static void  pm_list_store_move_before (GtkListStore *store,
					 GtkTreeIter  *iter,
					 GtkTreeIter  *position)
{
    pm_list_store_move (store, iter, position, TRUE);
}


static void  pm_list_store_move_after (GtkListStore *store,
					GtkTreeIter  *iter,
					GtkTreeIter  *position)
{
    pm_list_store_move (store, iter, position, FALSE);
}
#else
/* starting V2.2 convenient gtk functions exist */
#define sm_list_store_move_before gtk_list_store_move_before
#define sm_list_store_move_after gtk_list_store_move_after
#define pm_list_store_move_before gtk_list_store_move_before
#define pm_list_store_move_after gtk_list_store_move_after
#endif



/* Move the paths listed in @data before or after (according to @pos)
   @path. Used for DND */
static gboolean pmsm_move_pathlist (GtkTreeView *treeview,
				    gchar *data,
				    GtkTreePath *path,
				    GtkTreeViewDropPosition pos)
{
    GtkTreeIter to_iter;
    GtkTreeIter *from_iter;
    GtkTreeModel *model;
    GList *iterlist = NULL;
    GList *link;
    gchar **paths, **pathp;

    if (!data)          return FALSE;
    if (!strlen (data)) return FALSE;
    model = gtk_tree_view_get_model (treeview);
    if (!gtk_tree_model_get_iter (model, &to_iter, path))  return FALSE;

    /* split the path list into individual strings */
    paths = g_strsplit (data, "\n", -1);
    pathp = paths;
    /* Convert the list of paths into a list of iters */
    while (*pathp)
    {
	/* check that we won't move the master playlist (path = "0") */
	if (!( (treeview == playlist_treeview) && (atoi (*pathp) == 0) ))
	{
	    from_iter = g_malloc (sizeof (GtkTreeIter));
	    if ((strlen (*pathp) > 0) &&
		gtk_tree_model_get_iter_from_string (model, from_iter, *pathp))
	    {
		iterlist = g_list_append (iterlist, from_iter);
	    }
	}
	++pathp;
    }
    g_strfreev (paths);
    /* Move the iters in iterlist before or after @to_iter */
    switch (pos)
    {
    case GTK_TREE_VIEW_DROP_INTO_OR_BEFORE:
    case GTK_TREE_VIEW_DROP_INTO_OR_AFTER:
    case GTK_TREE_VIEW_DROP_AFTER:
	while (iterlist)
	{
	    link = g_list_last (iterlist);
	    from_iter = (GtkTreeIter *)link->data;
	    if (treeview == song_treeview)
		sm_list_store_move_after (GTK_LIST_STORE (model),
					  from_iter, &to_iter);
	    if (treeview == playlist_treeview)
		pm_list_store_move_after (GTK_LIST_STORE (model),
					  from_iter, &to_iter);
	    iterlist = g_list_delete_link (iterlist, link);
	    g_free (from_iter);
	}
	break;
    case GTK_TREE_VIEW_DROP_BEFORE:
	while (iterlist)
	{
	    link = g_list_first (iterlist);
	    from_iter = (GtkTreeIter *)link->data;
	    if (treeview == song_treeview)
		sm_list_store_move_before (GTK_LIST_STORE (model),
					   from_iter, &to_iter);
	    if (treeview == playlist_treeview)
		pm_list_store_move_before (GTK_LIST_STORE (model),
					   from_iter, &to_iter);
	    iterlist = g_list_delete_link (iterlist, link);
	    g_free (from_iter);
	}
	break;
    }
    if (treeview == song_treeview)      sm_rows_reordered ();
    if (treeview == playlist_treeview)  pm_rows_reordered ();
    return TRUE;
}


/* move pathlist for song treeview */
gboolean sm_move_pathlist (gchar *data,
			   GtkTreePath *path,
			   GtkTreeViewDropPosition pos)
{
    return pmsm_move_pathlist (song_treeview, data, path, pos);
}

/* move pathlist for song treeview */
gboolean pm_move_pathlist (gchar *data,
			   GtkTreePath *path,
			   GtkTreeViewDropPosition pos)
{
    return pmsm_move_pathlist (playlist_treeview, data, path, pos);
}


/* Callback for adding songs within sm_add_filelist */
void sm_addsongfunc (Playlist *plitem, Song *song, gpointer data)
{
    struct asf_data *asf = (struct asf_data *)data;
    GtkTreeModel *model;
    GtkTreeIter new_iter;

    model = gtk_tree_view_get_model (song_treeview);

/*    printf("plitem: %p\n", plitem);
      if (plitem) printf("plitem->type: %d\n", plitem->type);*/
    /* add to playlist but not to the display */
    add_song_to_playlist (plitem, song, FALSE);

    /* create new iter in song view */
    switch (asf->pos)
    {
    case GTK_TREE_VIEW_DROP_INTO_OR_BEFORE:
    case GTK_TREE_VIEW_DROP_INTO_OR_AFTER:
    case GTK_TREE_VIEW_DROP_AFTER:
	gtk_list_store_insert_after (GTK_LIST_STORE (model),
				     &new_iter, asf->to_iter);
	break;
    case GTK_TREE_VIEW_DROP_BEFORE:
	gtk_list_store_insert_before (GTK_LIST_STORE (model),
				      &new_iter, asf->to_iter);
	break;	
    }
    /* set the iter */
    sm_add_song_to_song_model (song, &new_iter);
}


/* DND: insert a list of files before/after @path
   @data: list of files
   @path: where to drop
   @pos:  before/after...
*/
gboolean sm_add_filelist (gchar *data,
			  GtkTreePath *path,
			  GtkTreeViewDropPosition pos)
{
    GtkTreeModel *model;
    GtkTreeIter to_iter;
    struct asf_data *asf;
    gchar *buf = NULL, *use_data;
    gchar **files, **filep;

    if (!data)             return FALSE;
    if (!strlen (data))    return FALSE;
    if (!current_playlist) return FALSE;

    model = gtk_tree_view_get_model (song_treeview);
    if (!gtk_tree_model_get_iter (model, &to_iter, path))  return FALSE;

    if (pos != GTK_TREE_VIEW_DROP_BEFORE)
    {   /* need to reverse the list of files -- otherwise wie add them
	 * in reverse order */
	/* split the path list into individual strings */
	gint len = strlen (data) + 1;
	files = g_strsplit (data, "\n", -1);
	filep = files;
	/* find the end of the list */
	while (*filep) ++filep;
	/* reserve memory */
	buf = g_malloc0 (len);
	/* reverse the list */
	while (filep != files)
	{
	    --filep;
	    g_strlcat (buf, *filep, len);
	    g_strlcat (buf, "\n", len);
	}
	g_strfreev (files);
	use_data = buf;
    }
    else
    {
	use_data = data;
    }

/*     printf("filelist: (%s) -> (%s)\n", data, use_data); */
    /* initialize add-song-struct */
    asf = g_malloc (sizeof (struct asf_data));
    asf->to_iter = &to_iter;
    asf->pos = pos;
    /* add the files to playlist -- but have sm_addsongfunc() called
       for every added song */
    add_text_plain_to_playlist (current_playlist, use_data, 0,
				sm_addsongfunc, asf);
    sm_rows_reordered ();
    g_free (asf);
    C_FREE (buf);
    return TRUE;
}

/* Function used to compare two cells during sorting (song view) */
gint sm_data_compare_func (GtkTreeModel *model,
			GtkTreeIter *a,
			GtkTreeIter *b,
			gpointer user_data)
{
  Song *song1;
  Song *song2;
  gint column;
  SM_item sm_item;
  GtkSortType order;
  gchar *item1_utf8, *item2_utf8;

  gtk_tree_model_get (model, a, SM_COLUMN_ALBUM, &song1, -1);
  gtk_tree_model_get (model, b, SM_COLUMN_ALBUM, &song2, -1);
  if(gtk_tree_sortable_get_sort_column_id (GTK_TREE_SORTABLE (model),
					   &column, &order) == FALSE) return 0;
  sm_item = (SM_item) column;
  /*printf ("sm_comp: %d\n", sm_item);*/
  switch (sm_item)
  {
  case SM_COLUMN_TITLE:
  case SM_COLUMN_ARTIST:
  case SM_COLUMN_ALBUM:
  case SM_COLUMN_GENRE:
  case SM_COLUMN_COMPOSER:
      item1_utf8 = song_get_item_utf8 (song1, SM_to_S (sm_item));
      item2_utf8 = song_get_item_utf8 (song2, SM_to_S (sm_item));
      return g_utf8_collate (g_utf8_casefold (item1_utf8, -1),
			     g_utf8_casefold (item2_utf8, -1));
  case SM_COLUMN_TRACK_NR:
      return song1->track_nr - song2->track_nr;
  case SM_COLUMN_IPOD_ID:
      return song1->ipod_id - song2->ipod_id;
  case SM_COLUMN_PC_PATH:
      return g_utf8_collate (song1->pc_path_utf8, song2->pc_path_utf8);
  case SM_COLUMN_TRANSFERRED:
      if(song1->transferred == song2->transferred) return 0;
      if(song1->transferred == TRUE) return 1;
      else return -1;
  default:
      gtkpod_warning("No sort for column %d\n", column);
      break;
  }
  return 0;
}


gint default_comp  (GtkTreeModel *model,
		    GtkTreeIter *a,
		    GtkTreeIter *b,
		    gpointer user_data)
{
    return 0;
}

static void
sm_song_column_button_clicked(GtkTreeViewColumn *tvc, gpointer data)
{
    if(prefs_get_save_sorted_order ())  sm_rows_reordered ();
}


/* Add one column at position @pos. This code is used over and over
   by sm_add_column() -- therefore I put it into a separate function */
static GtkTreeViewColumn *sm_add_text_column (gint col_id,
					      gchar *name,
					      GtkCellRenderer *renderer,
					      gboolean editable,
					      gint pos)
{
    GtkTreeViewColumn *column;
    GtkTreeModel *model = gtk_tree_view_get_model (song_treeview);

    if (!renderer)    renderer = gtk_cell_renderer_text_new ();
    if (editable)
    {
	g_signal_connect (G_OBJECT (renderer), "edited",
			  G_CALLBACK (sm_cell_edited), model);
    }
    g_object_set_data (G_OBJECT (renderer), "editable", (gint *)editable);
    g_object_set_data (G_OBJECT (renderer), "column", (gint *)col_id);
    column = gtk_tree_view_column_new_with_attributes (name, renderer, NULL);
    gtk_tree_view_column_set_cell_data_func (column, renderer,
					     sm_cell_data_func, NULL, NULL);
    gtk_tree_view_column_set_sort_column_id (column, col_id);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_clickable(column, TRUE);
    gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width (column,
					  prefs_get_sm_col_width (col_id));
    g_signal_connect (G_OBJECT (column), "clicked",
		      G_CALLBACK (sm_song_column_button_clicked),
		      (gpointer)col_id);
    gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model), col_id,
				     sm_data_compare_func, NULL, NULL);
    gtk_tree_view_column_set_reorderable (column, TRUE);
    gtk_tree_view_insert_column (song_treeview, column, pos);
    sm_columns[col_id] = column;
    return column;
}



/* Adds the columns to our song_treeview */
static GtkTreeViewColumn *sm_add_column (SM_item sm_item, gint pos)
{
  GtkTreeViewColumn *col = NULL;
  GtkCellRenderer *renderer;

  switch (sm_item)
  {
  case SM_COLUMN_TITLE:
      col = sm_add_text_column (SM_COLUMN_TITLE, _("Title"), NULL, TRUE, pos);
      break;
  case SM_COLUMN_ARTIST:
      col = sm_add_text_column (SM_COLUMN_ARTIST, _("Artist"), NULL, TRUE, pos);
      break;
  case SM_COLUMN_ALBUM:
      col = sm_add_text_column (SM_COLUMN_ALBUM, _("Album"), NULL, TRUE, pos);
      break;
  case SM_COLUMN_GENRE:
      col = sm_add_text_column (SM_COLUMN_GENRE, _("Genre"), NULL, TRUE, pos);
      break;
  case SM_COLUMN_COMPOSER:
      col = sm_add_text_column (SM_COLUMN_COMPOSER, _("Composer"), NULL, TRUE, pos);
      break;
  case SM_COLUMN_TRACK_NR:
      col = sm_add_text_column (SM_COLUMN_TRACK_NR, _("#"), NULL, TRUE, pos);
      break;
  case SM_COLUMN_IPOD_ID:
      col = sm_add_text_column (SM_COLUMN_IPOD_ID, _("ID"), NULL, FALSE, pos);
      break;
  case SM_COLUMN_PC_PATH:
      col = sm_add_text_column (SM_COLUMN_PC_PATH, _("PC File"), NULL, FALSE, pos);
      break;
  case SM_COLUMN_TRANSFERRED:
      renderer = gtk_cell_renderer_toggle_new ();
      col = sm_add_text_column (SM_COLUMN_TRANSFERRED, _("Trnsfrd"),
			  renderer, FALSE, pos);
      break;
    case SM_NUM_COLUMNS:
      break;
  }
  if (col && (pos != -1))
      gtk_tree_view_column_set_visible (col,
					prefs_get_col_visible (sm_item));
  return col;
}


/* Adds the columns to our song_treeview */
static void sm_add_columns (void)
{
    gint i;

    for (i=0; i<SM_NUM_COLUMNS; ++i)
    {
	sm_add_column (prefs_get_col_order (i), -1);
    }
    sm_show_preferred_columns();
}

static gboolean
sm_button_release_event(GtkWidget *w, GdkEventButton *e, gpointer data)
{
    if(w && e)
    {
	switch(e->button)
	{
	    case 3:
		sm_context_menu_init();
		break;
	    default:
		break;
	}
	
    }
    return(FALSE);
}

/* Create songs treeview */
static void sm_create_treeview (void)
{
  GtkTreeModel *model = NULL;
  GtkWidget *song_window = lookup_widget (gtkpod_window, "song_window");
  GtkWidget *stv = gtk_tree_view_new ();

  /* create tree view */
  if (song_treeview)
  {   /* delete old tree view */
      model = gtk_tree_view_get_model (song_treeview);
      /* FIXME: how to delete model? */
      gtk_widget_destroy (GTK_WIDGET (song_treeview));
  }
  song_treeview = GTK_TREE_VIEW (stv);
  gtk_widget_show (stv);
  gtk_container_add (GTK_CONTAINER (song_window), stv);
  /* create model */
  model = GTK_TREE_MODEL (
      gtk_list_store_new (SM_NUM_COLUMNS, G_TYPE_POINTER,
			  G_TYPE_POINTER, G_TYPE_POINTER,
			  G_TYPE_POINTER, G_TYPE_POINTER,
			  G_TYPE_POINTER, G_TYPE_POINTER,
			  G_TYPE_POINTER, G_TYPE_POINTER,
			  G_TYPE_POINTER));
  gtk_tree_view_set_model (song_treeview, GTK_TREE_MODEL (model));
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (song_treeview), TRUE);
  gtk_tree_selection_set_mode (gtk_tree_view_get_selection (song_treeview),
			       GTK_SELECTION_MULTIPLE);
  sm_add_columns ();
  gtk_drag_source_set (GTK_WIDGET (song_treeview), GDK_BUTTON1_MASK,
		       sm_drag_types, TGNR (sm_drag_types), GDK_ACTION_COPY);
/*  gtk_tree_view_enable_model_drag_source (song_treeview, GDK_BUTTON1_MASK,
					  sm_drag_types, TGNR (sm_drag_types),
					  GDK_ACTION_COPY);*/
  gtk_tree_view_enable_model_drag_dest(song_treeview, sm_drop_types,
				       TGNR (sm_drop_types), GDK_ACTION_COPY);
  /* need the gtk_drag_dest_set() with no actions ("0") so that the
     data_received callback gets the correct info value. This is most
     likely a bug... */
  gtk_drag_dest_set_target_list (GTK_WIDGET (song_treeview),
				 gtk_target_list_new (sm_drop_types,
						      TGNR (sm_drop_types)));

  g_signal_connect ((gpointer) stv, "drag_data_get",
		    G_CALLBACK (on_song_treeview_drag_data_get),
		    NULL);
  g_signal_connect_after ((gpointer) stv, "key_release_event",
			  G_CALLBACK (on_song_treeview_key_release_event),
			  NULL);
  g_signal_connect ((gpointer) stv, "drag_data_received",
		    G_CALLBACK (on_song_treeview_drag_data_received),
		    NULL);
  g_signal_connect ((gpointer) song_treeview, "button-release-event",
		    G_CALLBACK (sm_button_release_event),
		    NULL);
}

/* Create the different listviews to display the various information */
void display_create (GtkWidget *gtkpod)
{
    GtkWidget *stop_button;

    sm_create_treeview ();
    st_create_tabs ();
    pm_create_treeview ();
    /* set certain sizes, positions, widths... to default values */
    display_set_default_sizes ();
    /* Hide the "stop_button" */
    stop_button = lookup_widget (gtkpod_window, "stop_button");
    if (stop_button) gtk_widget_hide (stop_button);
    /* Hide/Show the toolbar */
    display_show_hide_toolbar ();
}

/* redisplay the entire display (playlists, sort tabs, song view) and
 * reset the sorted treeviews to normal (according to @inst) */
/* @inst: which treeviews should be reset to normal?
   -2: all treeviews
   -1: only playlist
    0...SORT_TAB_MAX-1: sort tab of instance @inst
    SORT_TAB_MAX: song treeview */
void display_reset (gint inst)
{
    gint i,n;
    Playlist *cur_pl;

    /* remember */
    cur_pl = current_playlist;

    /* remove all playlists from model (and reset "sortable") */
    if ((inst == -2) || (inst == -1))	pm_remove_all_playlists (TRUE);
    else                                pm_remove_all_playlists (FALSE);

    /* reset the sort tabs and song view */
    st_init (-1, 0);

    /* reset "sortable" */
    for (i=0; i<SORT_TAB_MAX; ++i)
    {
	if ((inst == -2) || (inst == i))
	    st_remove_all_entries_from_model (TRUE, i);
    }

    /* add playlists back to model (without selecting) */
    current_playlist = cur_pl;
    n = get_nr_of_playlists ();
    for (i=0; i<n; ++i)
    {
	pm_add_playlist (get_playlist_by_nr (i), -1);
    }
}


/* Clean up used memory (when quitting the program) */
void display_cleanup (void)
{
  cleanup_sort_tabs ();
}

/*
 * utility function for appending ipod song ids for song view (DND)
 */
void 
on_sm_dnd_get_id_foreach(GtkTreeModel *tm, GtkTreePath *tp, 
			 GtkTreeIter *i, gpointer data)
{
    Song *s;
    GString *filelist = (GString *)data;

    gtk_tree_model_get(tm, i, SM_COLUMN_TITLE, &s, -1); 
    /* can call on 0 cause s is consistent across all of the columns */
    if(s)
    {
	g_string_append_printf (filelist, "%d\n", s->ipod_id);
    }
}


/*
 * utility function for appending path for song view or playlist view (DND)
 */
void 
on_dnd_get_path_foreach(GtkTreeModel *tm, GtkTreePath *tp, 
			GtkTreeIter *iter, gpointer data)
{
    GString *filelist = (GString *)data;
    gchar *ps = gtk_tree_path_to_string (tp);
    g_string_append_printf (filelist, "%s\n", ps);
    g_free (ps);
}

/*
 * utility function for appending file for song view (DND)
 */
void 
on_sm_dnd_get_file_foreach(GtkTreeModel *tm, GtkTreePath *tp, 
			   GtkTreeIter *iter, gpointer data)
{
    Song *s;
    GString *filelist = (GString *)data;
    gchar *name;

    gtk_tree_model_get(tm, iter, SM_COLUMN_TITLE, &s, -1); 
    /* can call on 0 cause s is consistent across all of the columns */
    name = get_song_name_on_disk_verified (s);
    if (name)
    {
	g_string_append_printf (filelist, "file:%s\n", name);
	g_free (name);
    }
}

/*
 * utility function for appending ipod song ids for st treeview callback
 */
void 
on_st_listing_drag_foreach(GtkTreeModel *tm, GtkTreePath *tp, 
			   GtkTreeIter *i, gpointer data)
{
    TabEntry *entry;
    GString *idlist = (GString *)data;

    gtk_tree_model_get (tm, i, ST_COLUMN_ENTRY, &entry, -1);
    /* can call on 0 cause s is consistent across all of the columns */
    if(entry && idlist)
    {
	GList *l;
	Song *s;

	/* add all member-ids of entry to idlist */
	for (l=entry->members; l; l=l->next)
	{
	    s = (Song *)l->data;
	    g_string_append_printf (idlist, "%d\n", s->ipod_id);
	}
    }
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

void
sm_show_preferred_columns(void)
{
    GtkTreeViewColumn *tvc = NULL;
    gboolean visible;
    gint i;
    
    for (i=0; i<SM_NUM_COLUMNS_PREFS; ++i)
    {
	tvc = gtk_tree_view_get_column (song_treeview, i);
	visible = prefs_get_col_visible (prefs_get_col_order (i));
	gtk_tree_view_column_set_visible (tvc, visible);
    }
}

Playlist*
pm_get_selected_playlist(void)
{
    return(current_playlist);
}

/* make the toolbar visible or hide it depending on the value set in
   the prefs */
void display_show_hide_toolbar (void)
{
    GtkWidget *tb = lookup_widget (gtkpod_window, "toolbar");
    GtkWidget *mi = lookup_widget (gtkpod_window, "toolbar_menu");

    if (prefs_get_display_toolbar ())
    {
	gtk_toolbar_set_style (GTK_TOOLBAR (tb), prefs_get_toolbar_style ());
	gtk_widget_show (tb);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (mi), TRUE);
    }
    else
    {
	gtk_widget_hide (tb);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (mi), FALSE);
    }
}


/* set the default sizes for the gtkpod main window according to prefs:
   x, y, position of the PANED_NUM GtkPaned elements (the widht of the
   colums is set when setting up the colums in the listview */
void display_set_default_sizes (void)
{
    gint defx, defy, i;
    GtkWidget *w;
    gchar *buf;

    /* x,y-size */
    prefs_get_size_gtkpod (&defx, &defy);
    gtk_window_set_default_size (GTK_WINDOW (gtkpod_window), defx, defy);

    /* GtkPaned elements */
    if (gtkpod_window)
    {
	/* Elements defined with glade */
	for (i=0; i<PANED_NUM_GLADE; ++i)
	{
	    if (prefs_get_paned_pos (i) != -1)
	    {
		buf = g_strdup_printf ("paned%d", i);
		if((w = lookup_widget(gtkpod_window,  buf)))
		    gtk_paned_set_position (
			GTK_PANED (w), prefs_get_paned_pos (i));
		g_free (buf);
	    }
	}
	/* Elements defined with display.c (sort tab hpaned) */
	for (i=0; i<PANED_NUM_ST; ++i)
	{
	    if (prefs_get_paned_pos (PANED_NUM_GLADE + i) != -1)
	    {
		if (st_paned[i])
		    gtk_paned_set_position (
			st_paned[i], prefs_get_paned_pos (PANED_NUM_GLADE+i));
	    }
	}
    }
}


/* update the cfg structure (preferences) with the current sizes /
   positions:
   x,y size of main window
   column widths of song model
   position of GtkPaned elements */
void display_update_default_sizes (void)
{
    gint x,y,i;
    gchar *buf;
    GtkWidget *w;
    GtkTreeViewColumn *col;

    /* column widths */
    for (i=0; i<SM_NUM_COLUMNS_PREFS; ++i)
    {
	col = sm_columns [i];
	if (col)
	{
	    prefs_set_sm_col_width (i,
				    gtk_tree_view_column_get_width (col));
	}
    }

    /* x,y size of main window */
    if (gtkpod_window)
    {
	gtk_window_get_size (GTK_WINDOW (gtkpod_window), &x, &y);
	prefs_set_size_gtkpod (x, y);
    }

    /* GtkPaned elements */
    if (gtkpod_window)
    {
	/* Elements defined with glade */
	for (i=0; i<PANED_NUM_GLADE; ++i)
	{
	    buf = g_strdup_printf ("paned%d", i);
	    if((w = lookup_widget(gtkpod_window,  buf)))
	    {
		prefs_set_paned_pos (i,
				     gtk_paned_get_position (GTK_PANED (w)));
	    }
	    g_free (buf);
	}
	/* Elements defined with display.c (sort tab hpaned) */
	for (i=0; i<PANED_NUM_ST; ++i)
	{
	    if (st_paned[i])
		prefs_set_paned_pos (i + PANED_NUM_GLADE,
				     gtk_paned_get_position (st_paned[i]));
	}
    }
}


/* ------------------------------------------------------------

           Functions for stopping display update 

   ------------------------------------------------------------ */

enum {
    BR_BLOCK,
    BR_RELEASE,
    BR_ADD,
    BR_CALL,
};

/* called by block_selection() and release_selection */
static void block_release_selection (gint inst, gint action,
				     br_callback brc,
				     gpointer user_data1,
				     gpointer user_data2)
{
    static gint count_st[SORT_TAB_MAX];
    static gint count_pl = 0;
    static GtkWidget *stop_button = NULL;
    /* instance that has a pending callback */
    static gint level = SORT_TAB_MAX; /* no level -> no registered callback */
    static br_callback r_brc;
    static gpointer r_user_data1;
    static gpointer r_user_data2;
    static guint timeout_id = 0;
    gint i;

    /* lookup stop_button */
    if (stop_button == NULL)
    {
	stop_button = lookup_widget (gtkpod_window, "stop_button");
	if (stop_button == NULL)
	    fprintf (stderr, "Programming error: stop_button not found\n");
    }

    switch (action)
    {
    case BR_BLOCK:
	if (count_pl == 0)
	{
	    block_widgets ();
	    if (stop_button) gtk_widget_show (stop_button);
	}
	++count_pl;
	for (i=0; (i<=inst) && (i<SORT_TAB_MAX); ++i)
	{
	    ++count_st[i];
	}
	break;
    case BR_RELEASE:
	for (i=0; (i<=inst) && (i<SORT_TAB_MAX); ++i)
	{
	    --count_st[i];
	    if ((count_st[i] == 0) && (stop_add == i))
		stop_add = SORT_TAB_MAX;
	}
	--count_pl;
	if (count_pl == 0)
	{
	    if (stop_button) gtk_widget_hide (stop_button);
	    stop_add = SORT_TAB_MAX;
	    release_widgets ();
	}
	/* check if we have to call a callback */
	if (level < SORT_TAB_MAX)
	{
	    if (((level == -1) && (count_pl == 0)) ||
		((level >= 0) && (count_st[level] == 0)))
	    {
		level = SORT_TAB_MAX;
		r_brc (r_user_data1, r_user_data2);
	    }
	}
	break;
    case BR_ADD:
/*	printf("adding: inst: %d, level: %d, brc: %p, data1: %p, data2: %p\n",
	inst, level, brc, user_data1, user_data2);*/
	if (((inst == -1) && (count_pl == 0)) ||
	    ((inst >= 0) && (count_st[inst] == 0)))
	{ /* OK, we could just call the desired function because there
	    is nothing to be stopped. However, due to a bug or a
            inevitability of gtk+, if we interrupt the selection
	    process with a gtk_main_iteration() call the button
	    release event will be "discarded". Therefore we register
	    the call here and have it activated with a timeout
	    function. */
	    /* We overwrite an older callback of the same level or
	     * higher  */
	    if (level >= inst)
	    {
		level = inst;
		r_brc = brc;
		r_user_data1 = user_data1;
		r_user_data2 = user_data2;
		if (timeout_id == 0)
		{
		    timeout_id = 
			gtk_idle_add_priority (G_PRIORITY_HIGH,
					       selection_callback_timeout,
					       NULL);
		}	       
	    }
	}
	else
	{
	    if (inst < level)
	    {   /* Once the functions have stopped, down to the
		specified level/instance, the desired function is
		called (about 15 lines up: r_brc (...)) */
		/* We need to emit a stop_add signal */
		stop_add = inst;
		/* and safe the callback data */
		level = inst;
		r_brc = brc;
		r_user_data1 = user_data1;
		r_user_data2 = user_data2;
	    }
	}
	break;
    case BR_CALL:
	if (timeout_id)
	{
	    gtk_timeout_remove (timeout_id);
	    timeout_id = 0;
	}
	if (level == SORT_TAB_MAX) break;  /* hmm... what happened to
					      our callback? */
	if (((level == -1) && (count_pl == 0)) ||
	    ((level >= 0) && (count_st[level] == 0)))
	{ /* Let's call the callback function */
		level = SORT_TAB_MAX;
		r_brc (r_user_data1, r_user_data2);
	}
	else
	{ /* This is strange and should not happen -- let's forget
	   * about the callback */
	    level = SORT_TAB_MAX;
	}
	break;
    default:
	fprintf (stderr, "Programming error: unknown BR_...: %d\n", action);
	break;
    }
}

/* Will block the possibility to select another playlist / sort tab
   page / tab entry. Will also block the widgets and activate the
   "stop button" that can be pressed to stop the update
   process. "inst" is the sort tab instance up to which the selections
   should be blocked. "-1" corresponds to the playlist view */
static void block_selection (gint inst)
{
    block_release_selection (inst, BR_BLOCK, NULL, NULL, NULL);
}

/* Makes selection possible again */
static void release_selection (gint inst)
{
    block_release_selection (inst, BR_RELEASE, NULL, NULL, NULL);
}


/* Stops the display updates down to instance "inst". "-1" is the
 * playlist view */
void stop_display_update (gint inst)
{
    stop_add = inst;
}

/* registers @brc to be called as soon as all functions down to
   instance @inst have been stopped */
static void add_selection_callback (gint inst, br_callback brc,
				    gpointer user_data1, gpointer user_data2)
{
    block_release_selection (inst, BR_ADD, brc, user_data1, user_data2);
}

/* Called as a high priority timeout to initiate the callback of @brc
   in the last function */
static gboolean selection_callback_timeout (gpointer data)
{
    block_release_selection (0, BR_CALL, NULL, NULL, NULL);
    return FALSE;
}
