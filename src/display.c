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
#include "misc.h"

/* pointer to the treeview for the song display */
static GtkTreeView *song_treeview = NULL;
/* pointer to the treeview for the playlist display */
static GtkTreeView *playlist_treeview = NULL;
/* array with pointers to the columns used in the song display */
static GtkTreeViewColumn *sm_columns[SM_NUM_COLUMNS];
/* array with pointers to the sorttabs */
static SortTab *sorttab[SORT_TAB_NUM];
/* pointer to the currently selected playlist */
static Playlist *current_playlist = NULL;

/* used for stopping of display refresh */
typedef void (*br_callback)(gpointer user_data1, gpointer user_data2);
static void block_selection (gint inst);
static void release_selection (gint inst);
static void add_selection_callback (gint inst, br_callback brc, gpointer user_data1, gpointer user_data2);
static gint stop_add = SORT_TAB_NUM;


static void sm_song_changed (Song *song);
static void sm_remove_song (Song *song);
static void sm_remove_all_songs (void);
static void sm_add_song_to_song_model (Song *song);
static void st_song_changed (Song *song, gboolean removed, guint32 inst);
static void st_add_song (Song *song, gboolean final, guint32 inst);
static void st_remove_song (Song *song, guint32 inst);
static void st_init (gint32 new_category, guint32 inst);

static void pm_dnd_advertise(GtkTreeView *v);

/* ---------------------------------------------------------------- */
/* Section for playlist display                                     */
/* ---------------------------------------------------------------- */


/* remove a song from a current playlist (model) */
void pm_remove_song (Playlist *playlist, Song *song)
{
  /* notify sort tab if currently selected playlist is affected */
  if (playlist == current_playlist) st_remove_song (song, 0);
}


/* Add song to the display if it's in the currently displayed playlist */
void pm_add_song (Playlist *playlist, Song *song)
{
  if (playlist == current_playlist) 
    st_add_song (song, TRUE, 0); /* Add to first sort tab */
}

/* advertise dnd for the playlist model GtkTreeView
 */
static void
pm_dnd_advertise(GtkTreeView *v)
{
    GtkTargetEntry target[] = {
	{ "gtkpod/file", 0, 1000 },
	{ "text/plain", 0, 1001 }
    };
    guint target_size = (guint)(sizeof(target)/sizeof(GtkTargetEntry));

    if(!v) return;
    gtk_tree_view_enable_model_drag_dest(v, target, target_size, 
					    GDK_ACTION_COPY);
}

static void st_dnd_advertise (GtkTreeView *v)
{
    GtkTargetEntry target[] = {
	{ "gtkpod/file", 0, 1000 },
	{ "text/plain", 0, 1001 }
    };
    guint target_size = (guint)(sizeof(target)/sizeof(GtkTargetEntry));

    if(!v) return;
    gtk_tree_view_enable_model_drag_source(v, GDK_BUTTON1_MASK, target, 
					    target_size, GDK_ACTION_COPY);
}

static void
sm_dnd_advertise(GtkTreeView *v)
{
    GtkTargetEntry target[] = {
	{ "gtkpod/file", 0, 1000 },
	{ "text/plain", 0, 1001 }
    };
    guint target_size = (guint)(sizeof(target)/sizeof(GtkTargetEntry));

    if(!v) return;
    gtk_tree_view_enable_model_drag_source(v, GDK_BUTTON1_MASK, target, 
					    target_size, GDK_ACTION_COPY);
    gtk_tree_view_enable_model_drag_dest(v, target, target_size, 
					    GDK_ACTION_COPY);
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
void pm_add_playlist (Playlist *playlist)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkTreeSelection *selection;

  model = gtk_tree_view_get_model (playlist_treeview);
  g_return_if_fail (model != NULL);

  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		      PM_COLUMN_PLAYLIST, playlist, -1);
  /* If it's the first playlist (no playlist selected AND playlist ist
     the MPL, we select it, unless prefs_get_mpl_autoselect() says
     otherwise */
  if (current_playlist == NULL)
  {
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


static void pm_selection_changed_cb (gpointer user_data1, gpointer user_data2)
{
  GtkTreeSelection *selection = (GtkTreeSelection *)user_data1;
  GtkTreeModel *model;
  GtkTreeIter  iter;
  Playlist *new_playlist;
  Song *song;
  guint32 n,i;

  if (gtk_tree_selection_get_selected (selection, &model, &iter) == FALSE)
  {  /* no selection -> reset sort tabs */
      st_init (-1, 0);
      current_playlist = NULL;
      return;
  }
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
      if (!prefs_get_block_display ())  block_selection (-1);
      for (i=0; i<n; ++i)
      { /* add all songs to sort tab 0 */
	  if (stop_add == -1)  break;
	  song = get_song_in_playlist_by_nr (new_playlist, i);
	  st_add_song (song, FALSE, 0);
	  while (((i%10) == 0) &&
		 !prefs_get_block_display() && gtk_events_pending ())
	      gtk_main_iteration ();
      }
      if (stop_add != -1) st_add_song (NULL, TRUE, 0);
      if (!prefs_get_block_display ())
      {
	  while (gtk_events_pending ())	      gtk_main_iteration ();
	  release_selection (-1);
      }
  }
  gtkpod_songs_statusbar_update();
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


void
pm_select_playlist_reinit(Playlist *pl)
{
    GtkTreeIter i;
    GtkTreeModel *tm = NULL, *sm = NULL;
    Playlist *playlist = NULL;
    GtkTreeSelection *ts = NULL;

    if((ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(playlist_treeview))))
    {
	if(gtk_tree_selection_get_selected(ts, &tm, &i))
	{
	    gtk_tree_model_get (tm, &i, PM_COLUMN_PLAYLIST, &playlist, -1);
	    if((sm = gtk_tree_view_get_model(song_treeview)))
	    {
		gtk_tree_sortable_set_sort_column_id(
			GTK_TREE_SORTABLE(sm), SM_COLUMN_NONE,
			GTK_SORT_ASCENDING);
	    }
	    if((playlist) && (playlist == pl))
		pm_selection_changed(ts, NULL); 
	}
    }
    else
    {
	gtkpod_warning (_("Nothing is selected currently in Playlist Tree\n"));
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
  GtkTreeViewColumn *column;
  GtkSortType sort;
  gint corr;

  gtk_tree_model_get (model, a, PM_COLUMN_PLAYLIST, &playlist1, -1);
  gtk_tree_model_get (model, b, PM_COLUMN_PLAYLIST, &playlist2, -1);
  column = (GtkTreeViewColumn *)user_data;
  /* we make sure that the master playlist always stays on top */
  /* This is a hack... for some reason GTK2 doesn't set the sort order
     until after the sort has been done... */
  if (!gtk_tree_view_column_get_sort_indicator (column))
    corr = -1;
  else
    {
      sort = gtk_tree_view_column_get_sort_order (column);
      if (sort == GTK_SORT_ASCENDING)  corr = 1;
      else corr = -1;
    }
  if (playlist1->type == PL_TYPE_MPL) return 1*corr;
  if (playlist2->type == PL_TYPE_MPL) return -1*corr;
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
      if(playlist->type == PL_TYPE_NORM) 
      {
	g_object_set (G_OBJECT (renderer), "text", playlist->name, 
		    "editable", TRUE, NULL);
      }
      else	/* no renaming of master playlist */
      {
	g_object_set (G_OBJECT (renderer), "text", playlist->name, NULL);
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
    GtkTreeIter i;
    GList *new_list = NULL;
    gboolean valid = FALSE;
    GtkTreeModel *tm = NULL;
    Playlist *new_pl = NULL;
			    
    if((tm = gtk_tree_view_get_model(GTK_TREE_VIEW(playlist_treeview))))
    {
	valid =gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tm),&i);
	while(valid)
	{
	    gtk_tree_model_get(tm, &i, 0, &new_pl, -1); 
	    new_list = g_list_append(new_list, new_pl);
	    valid = gtk_tree_model_iter_next(tm, &i);
	} 
	reset_playlists_to_new_list(new_list);
    }
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


/* Create playlist listview */
static void create_playlist_listview (GtkWidget *gtkpod)
{
  GtkTreeModel *model;
  GtkTreeSelection *selection;

  /* create model */
  model =   GTK_TREE_MODEL (gtk_list_store_new (PM_NUM_COLUMNS,
						G_TYPE_POINTER));
  /* set tree view */
  playlist_treeview = GTK_TREE_VIEW (lookup_widget (gtkpod, "playlist_treeview"));
  gtk_tree_view_set_model (playlist_treeview, GTK_TREE_MODEL (model));
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (playlist_treeview), TRUE);
  gtk_tree_selection_set_mode (gtk_tree_view_get_selection (playlist_treeview),
			       GTK_SELECTION_SINGLE);
  selection = gtk_tree_view_get_selection (playlist_treeview);
  g_signal_connect (G_OBJECT (selection), "changed",
		    G_CALLBACK (pm_selection_changed), NULL);
  pm_add_columns ();
  pm_dnd_advertise(playlist_treeview);
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

    for(i=0; i<SORT_TAB_NUM; ++i)
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

    for(i=0; i<SORT_TAB_NUM; ++i)
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
    if ((inst >= 0) && (inst < SORT_TAB_NUM) && sorttab[inst])
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
  if(entry == (TabEntry *)data) {
    gtk_list_store_remove (GTK_LIST_STORE (model), iter);
    return TRUE;
  }
  return FALSE;
}


/* Remove entry from the display model and the sorttab */
static void st_remove_entry_from_model (TabEntry *entry, guint32 inst)
{
  GtkTreeModel *model = sorttab[inst]->model;
  if (model && entry)
    {
      if (entry == sorttab[inst]->current_entry)
	{
	  sorttab[inst]->current_entry = NULL;
	}
      gtk_tree_model_foreach (model, st_delete_entry_from_model, entry);
      sorttab[inst]->entries = g_list_remove (sorttab[inst]->entries, entry);
      g_list_free (entry->members);
      C_FREE (entry->name);
      g_free (entry);
    }
}

/* Remove all entries from the display model and the sorttab */
static void st_remove_all_entries_from_model (guint32 inst)
{
  TabEntry *entry;
  SortTab *st = sorttab[inst];

  if (st)
  {
      while (st->entries != NULL)
      {
	  entry = (TabEntry *)g_list_nth_data (st->entries, 0);
	  st_remove_entry_from_model (entry, inst);
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
    gboolean current_entry=FALSE, valid;
    TabEntry *next=NULL, *entry2=NULL;
    GtkTreeIter iter, iter2;
    GtkTreeSelection *selection;
    SortTab *st = sorttab[inst];

    if (!entry) return;
    /* is the entry empty (contains no songs)? */
    if (g_list_length (entry->members) != 0) return;
    /* if the entry is the master entry 'All' -> the tab is empty,
       re-init tab */
    if (entry->master)
    {
	st_redisplay (inst);
	return;
    }
    
    /* is the entry currently selected? Remember! */
    selection = gtk_tree_view_get_selection (st->treeview[st->current_category]);
    if (sorttab[inst]->current_entry == entry)
    {
	current_entry = TRUE;
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
    st_remove_entry_from_model (entry, inst);
    /* if we have a next entry, select it. */
    if (next)
    {
	gtk_tree_selection_select_iter (selection, &iter);
    }
    else
    { /* redisplay next instance if removed entry was selected (should be empty) */
	if (current_entry)  st_redisplay (inst+1);
    }
}

/* Get the correct name for the entry according to currently
   selected category (page). Do _not_ g_free() the return value! */
static gchar *st_get_entryname (Song *song, guint32 inst)
{
  switch (sorttab[inst]->current_category)
    {
    case ST_CAT_ARTIST:
      return song->artist;
    case ST_CAT_ALBUM:
      return song->album;
    case ST_CAT_GENRE:
      return song->genre;
    case ST_CAT_TITLE:
      return song->title;
    }
  g_warning ("Programming error: st_get_entryname: undefined category\n");
  return NULL;
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
  GList *entries;
  TabEntry *entry;
  guint i;

  if (name == NULL) return NULL;
  entries = sorttab[inst]->entries;
  /* check if we need to return the master entry */
  if ((strlen (name) == 1) && (*name == -1))
  {
      entry = (TabEntry *)g_list_nth_data (entries, 0);
  }
  else
  {
      i=1; /* skip master entry, which is supposed to be at first position */
      while ((entry = (TabEntry *)g_list_nth_data (entries, i)) != NULL)
      {
	  if (g_utf8_collate (entry->name, name) == 0)   break; /* found! */
	  ++i;
      }
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

  if (inst == SORT_TAB_NUM)
    {
      sm_song_changed (song);
      return;
    }
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
		  st_add_song (song, TRUE, inst+1);
		}
	    }
	}
    }
}


/* Add song to sort tab. If the song matches the currently
   selected sort criteria, it will be passed on to the next
   sort tab. The last sort tab will pass the song on to the
   song model (currently two sort tabs).
   When the first song is added, the "All" entry is created.
   If prefs_get_st_autoselect(inst) is true, the "All" entry is
   automatically selected, if there was no former selection */
static void st_add_song (Song *song, gboolean final, guint32 inst)
{
  static gint count = 0;
  TabEntry *entry, *iter_entry;
  SortTab *st;
  gchar *entryname;
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  TabEntry *select_entry = NULL;

  if (inst == SORT_TAB_NUM)
  {  /* just add to song model */
      if (song != NULL)    sm_add_song_to_song_model (song);
      if (final || (++count % 20 == 0))
	  gtkpod_songs_statusbar_update();
  }
  else
  {
      st = sorttab[inst];
      if (song != NULL)
      {
	  /* add song to "All" (master) entry */
	  entry = g_list_nth_data (st->entries, 0);
	  if (entry == NULL)
	  { /* doesn't exist yet -- let's create it */
	      entry = g_malloc0 (sizeof (TabEntry));
	      entry->name = g_strdup (_("All"));
	      entry->master = TRUE;
	      st_add_entry (entry, inst);
	  }
	  entry->members = g_list_append (entry->members, song);
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
	      st_add_song (song, final, inst+1);
	  }
	  /* select current entry if it corresponds to the last
	     selection, or last_entry if that's the master entry */
	  if (!st->current_entry)
	  {	      
	      TabEntry *last_entry = st_get_entry_by_name (
		  st->lastselection[st->current_category], inst);
	      if (entry == last_entry)
	      {
		  select_entry = entry;
		  st_add_song (song, final, inst+1);
	      }
	      else if (last_entry && last_entry->master)
	      {
		  select_entry = last_entry;
		  st_add_song (song, final, inst+1);
	      }
	  }
      }
      /* select "All" if it's the last song added, no entry currently
      selected (including "select_entry", which is to be selected" and
      prefs_get_st_autoselect() allows us to select "All" */
      if (final && !st->current_entry && !select_entry &&
	  prefs_get_st_autoselect (inst))
      { /* auto-select entry "All" */
	  select_entry = g_list_nth_data (st->entries, 0);
	  /* add all songs to next sort tab */
	  if (select_entry)
	  {
	      GList *gl_song;
	      for (gl_song = select_entry->members; gl_song; gl_song=gl_song->next)
	      {
		  st_add_song ((Song *)gl_song->data, final, inst+1);
	      }
	  }
      }

      if (select_entry)
      {  /* select current select_entry */
	  /* printf("%d: selecting: %x: %s\n", inst, select_entry, select_entry->name);*/
	  st->current_entry = select_entry;
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
		  gtk_tree_selection_unselect_all (selection);
		  gtk_tree_selection_select_iter (selection, &iter);
		  break;
	      }
	  } while (gtk_tree_model_iter_next (st->model, &iter));
      }
      else if (!song && final)
      {
	  st_add_song (NULL, final, inst+1);
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

  if (inst == SORT_TAB_NUM)
    {
      sm_remove_song (song);
    }
  else
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
/* if reinit == TRUE, it will also remember, if "All" was
   selected. Normally it does not specifically remember and will
   select "All" in accordance to the prefs settings. */
static void st_init (gint32 new_category, guint32 inst)
{
  SortTab *st;
  gint cat;

  if (inst == SORT_TAB_NUM)
    {
      sm_remove_all_songs ();
    }
  else
    {
      st = sorttab[inst];
      if (st == NULL) return; /* could happen during initialisation */
      cat = st->current_category;
      if (st->current_entry != NULL)
      {
	  C_FREE (st->lastselection[cat]);
	  if (!st->current_entry->master)
	  {
	      st->lastselection[cat] = g_strdup (st->current_entry->name);
	  }
	  else
	  {
	      gchar buf[] = {-1, 0}; /* this is how I mark the "All"
				      * entry as string: should be
				      * illegal UTF8 */
	      st->lastselection[cat] = g_strdup (buf);
	  }
	  st->current_entry = NULL;
      }
      if (new_category != -1)
	{
	  st->current_category = new_category;
	  prefs_set_st_category (inst, new_category);
	}
      st_remove_all_entries_from_model (inst);
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

  inst = st_get_instance_from_notebook (notebook);
  if (inst == -1) return; /* invalid notebook */
  if (stop_add <= inst)  return;
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
      /* block playlist view and all sort tab notebooks <= inst */
      if (!prefs_get_block_display ())  block_selection (inst);
      /* add all songs previously present to sort tab */
      for (i=0; i<n; ++i)
      {
	  if (stop_add <= inst)  break;
	  song = (Song *)g_list_nth_data (copy, i);
	  st_add_song (song, FALSE, inst);
	  while (((i%10) == 0) &&
		 !prefs_get_block_display() && gtk_events_pending ())
	      gtk_main_iteration ();
      }
      if (n && (stop_add > inst)) st_add_song (NULL, TRUE, inst);
      if (!prefs_get_block_display ())
      {
	  while (gtk_events_pending ())      gtk_main_iteration ();
	  release_selection (inst);
      }
  }
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
  add_selection_callback (inst, st_page_selected_cb,
			  (gpointer)notebook, (gpointer)page);
}


/* Redisplay the sort tab "inst". Called from the menu item "Re-Init" */
void st_redisplay (guint32 inst)
{
    if (!(inst < SORT_TAB_NUM)) return; /* error! */
    block_widgets (); /* might take a while, so we'll do refreshs */
    if (sorttab[inst])
	st_page_selected (sorttab[inst]->notebook,
			  sorttab[inst]->current_category);
    release_widgets ();
}

/* Start sorting */
void st_sort (guint32 inst, GtkSortType order)
{
    if (!(inst < SORT_TAB_NUM)) return; /* error! */
    if (sorttab[inst])
	gtk_tree_sortable_set_sort_column_id (
	    GTK_TREE_SORTABLE (sorttab[inst]->model),
	    ST_COLUMN_ENTRY, order);
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

  /* printf("%d: entered\n", inst);*/
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
	  st_init (-1, inst+1);
      }
      return; 
  }
  gtk_tree_model_get (model, &iter, 
		      ST_COLUMN_ENTRY, &new_entry,
		      -1);
  /* printf("selected instance %d, entry %x (was: %x)\n", inst,
   * new_entry, st->current_entry);*/
  if (new_entry == st->current_entry) return; /* important: otherwise
						 st_add_song will not work correctly */
  /* initialize next instance */
  st_init (-1, inst+1);
  st->current_entry = new_entry;
  n = g_list_length (new_entry->members); /* number of members */
  if (n > 0)
  {
      if (!prefs_get_block_display ())  block_selection (inst);
      for (i=0; i<n; ++i)
      { /* add all member songs to next instance */
	  if (stop_add <= inst) break;
	  song = (Song *)g_list_nth_data (new_entry->members, i);
	  st_add_song (song, FALSE, inst+1);
	  while (((i%10) == 0) &&
		 !prefs_get_block_display() && gtk_events_pending ())
	      gtk_main_iteration ();
      }
      if (stop_add > inst)  st_add_song (NULL, TRUE, inst+1);
      if (!prefs_get_block_display ())
      {
	  while (gtk_events_pending ())	  gtk_main_iteration ();
	  release_selection (inst);
      }
  }
  gtkpod_songs_statusbar_update();
}


/* Callback function called when the selection
   of the sort tab view has changed */
/* Instead of handling the selection directly, we add a
   "callback". Currently running display updates will be stopped
   before the st_selection_changed_cb is actually called */
static void st_selection_changed (GtkTreeSelection *selection,
				  gpointer user_data)
{
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
  gint column;
  gint i, n, inst;
  Song *song;
  GList *members;

  inst = (guint32)data;
  model = sorttab[inst]->model;
  path = gtk_tree_path_new_from_string (path_string);
  column = (gint)g_object_get_data (G_OBJECT (renderer), "column");
  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get (model, &iter, column, &entry, -1);

  /*printf("Inst %d: st_cell_edited: column: %d  :%lx\n", inst, column, entry);*/

  switch (column)
    {
    case ST_COLUMN_ENTRY:
      /* We only do something, if the name actually got changed */
      if (g_utf8_collate (entry->name, new_text) != 0)
	{
	  g_free (entry->name);
	  entry->name = g_strdup (new_text);
	  /* Now we look up all the songs and change the ID3 Tag as well */
	  /* We make a copy of the current members list, as it may change
             during the process */
	  members = g_list_copy (entry->members);
	  n = g_list_length (members);
	  /* block user input if we write tags (might take a while) */
	  if (prefs_get_id3_write ())   block_widgets ();
	  for (i=0; i<n; ++i) {
	    song = (Song *)g_list_nth_data (members, i);
	    /*printf("%d/%d: %x\n", i+1, n, song);*/
	    switch (sorttab[inst]->current_category)
	      {
	      case ST_CAT_ARTIST:
		g_free (song->artist);
		g_free (song->artist_utf16);
		song->artist = g_strdup (new_text);
		song->artist_utf16 = g_utf8_to_utf16 (new_text, -1,
						      NULL, NULL, NULL);
		break;
	      case ST_CAT_ALBUM:
		g_free (song->album);
		g_free (song->album_utf16);
		song->album = g_strdup (new_text);
		song->album_utf16 = g_utf8_to_utf16 (new_text, -1,
						      NULL, NULL, NULL);
		break;
	      case ST_CAT_GENRE:
		g_free (song->genre);
		g_free (song->genre_utf16);
		song->genre = g_strdup (new_text);
		song->genre_utf16 = g_utf8_to_utf16 (new_text, -1,
						      NULL, NULL, NULL);
		break;
	      case ST_CAT_TITLE:
		g_free (song->title);
		g_free (song->title_utf16);
		song->title = g_strdup (new_text);
		song->title_utf16 = g_utf8_to_utf16 (new_text, -1,
						      NULL, NULL, NULL);
		break;
	      }
	    pm_song_changed (song);
	    /* If prefs say to write changes to file, do so */
	    if (prefs_get_id3_write ())
	    {
		gint tag_id;
		/* should we update all ID3 tags or just the one
		   changed? */
		if (prefs_get_id3_writeall ()) tag_id = S_ALL;
		else		               tag_id = ST_to_S (column);
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
  GtkSortType sort;
  GtkTreeViewColumn *column;
  gint corr;

  gtk_tree_model_get (model, a, ST_COLUMN_ENTRY, &entry1, -1);
  gtk_tree_model_get (model, b, ST_COLUMN_ENTRY, &entry2, -1);
  column = (GtkTreeViewColumn *)user_data;
  /* We make sure that the "all" entry always stay on top */
  /* This is a hack... for some reason GTK2 doesn't set the sort order
     until after the sort has been done... */
  if (!gtk_tree_view_column_get_sort_indicator (column))
    corr = -1;
  else
    {
      sort = gtk_tree_view_column_get_sort_order (column);
      if (sort == GTK_SORT_ASCENDING)  corr = 1;
      else corr = -1;
    }
  if (entry1->master) return 1*corr;
  if (entry2->master) return -1*corr;
  /* Otherwise return the comparison */
  return g_utf8_collate (g_utf8_casefold (entry1->name, -1), 
			 g_utf8_casefold (entry2->name, -1));
}


/* Create songs listview */
static void st_create_listview (GtkWidget *gtkpod, gint inst)
{
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkTreeModel *model;
  GtkListStore *liststore;
  GtkTreeView *treeview;
  GtkTreeSelection *selection;
  gint i;
  gchar *name;

  /* create model */
  liststore = gtk_list_store_new (ST_NUM_COLUMNS, G_TYPE_POINTER);
  model = GTK_TREE_MODEL (liststore);
  sorttab[inst]->model = model;
  /* set tree view */
  for (i=0; i<ST_CAT_NUM; ++i)
    {
      name = g_strdup_printf ("st%d_cat%d_treeview", inst, i);
      treeview = GTK_TREE_VIEW (lookup_widget (gtkpod, name));
      sorttab[inst]->treeview[i] = treeview;
      g_free (name);
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
      g_object_set_data (G_OBJECT (renderer), "column", (gint *)ST_COLUMN_ENTRY);
      column = gtk_tree_view_column_new ();
      gtk_tree_view_column_pack_start (column, renderer, TRUE);
      column = gtk_tree_view_column_new_with_attributes ("", renderer, NULL);
      gtk_tree_view_column_set_cell_data_func (column, renderer, st_cell_data_func, NULL, NULL);
      gtk_tree_view_column_set_sort_column_id (column, ST_COLUMN_ENTRY);
      gtk_tree_view_column_set_resizable (column, TRUE);
      gtk_tree_view_column_set_sort_order (column, GTK_SORT_ASCENDING);
      gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (liststore),
				       ST_COLUMN_ENTRY,
				       st_data_compare_func, column, NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_set_headers_visible (treeview, FALSE);
      st_dnd_advertise(treeview);
    }
}



/* Create sort tabs */
static void create_sort_tabs (GtkWidget *gtkpod)
{
  gint inst;
  gchar *name;

  /* we count downward here because the smaller sort tabs might try to
     initialize the higher one's -> create the higher ones first */
  for (inst=SORT_TAB_NUM-1; inst>=0; --inst)
    {
      sorttab[inst] = g_malloc0 (sizeof (SortTab));
      name = g_strdup_printf ("sorttab%d", inst);
      sorttab[inst]->notebook = GTK_NOTEBOOK (lookup_widget (gtkpod, name));
      g_free (name);
      st_create_listview (gtkpod, inst);
      gtk_notebook_set_current_page (sorttab[inst]->notebook,
				     prefs_get_st_category(inst));
    }
}

/* Clean up the memory used by sort tabs (program quit). */
static void cleanup_sort_tabs (void)
{
  gint i,j;
  for (i=0; i<SORT_TAB_NUM; ++i)
    {
      if (sorttab[i] != NULL)
	{
	  st_remove_all_entries_from_model (i);
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


/* Append song to the song model */
static void sm_add_song_to_song_model (Song *song)
{
  GtkTreeIter iter;
  GtkTreeModel *model = gtk_tree_view_get_model (song_treeview);

  g_return_if_fail (model != NULL);

  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		      SM_COLUMN_TITLE, song,
		      SM_COLUMN_ARTIST, song,
		      SM_COLUMN_ALBUM, song,
		      SM_COLUMN_GENRE, song,
		      SM_COLUMN_TRACK_NR, song,
		      SM_COLUMN_IPOD_ID, song,
		      SM_COLUMN_PC_PATH, song,
		      SM_COLUMN_TRANSFERRED, song,
		      SM_COLUMN_NONE, song,
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
static void sm_remove_all_songs (void)
{
  GtkTreeModel *model = gtk_tree_view_get_model (song_treeview);
  GtkTreeIter iter;

  while (gtk_tree_model_get_iter_first (model, &iter))
    {
      gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
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
  gint column;
  gboolean changed = FALSE; /* really changed anything? */
  gchar *track_text = NULL;

  model = (GtkTreeModel *)data;
  path = gtk_tree_path_new_from_string (path_string);
  column = (gint)g_object_get_data (G_OBJECT (renderer), "column");
  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get (model, &iter, column, &song, -1);

  /*printf("sm_cell_edited: column: %d  song:%lx\n", column, song);*/

  switch (column)
    {
    case SM_COLUMN_ALBUM:
      if (g_utf8_collate (song->album, new_text) != 0)
	{
	  g_free (song->album);
	  g_free (song->album_utf16);
	  song->album = g_strdup (new_text);
	  song->album_utf16 = g_utf8_to_utf16 (new_text, -1, NULL, NULL, NULL);
	  changed = TRUE;
	}
      break;
    case SM_COLUMN_ARTIST:
      if (g_utf8_collate (song->artist, new_text) != 0)
	{
	  g_free (song->artist);
	  g_free (song->artist_utf16);
	  song->artist = g_strdup (new_text);
	  song->artist_utf16 = g_utf8_to_utf16 (new_text, -1, NULL, NULL, NULL);
	  changed = TRUE;
	}
      break;
    case SM_COLUMN_TITLE:
      if (g_utf8_collate (song->title, new_text) != 0)
	{
	  g_free (song->title);
	  g_free (song->title_utf16);
	  song->title = g_strdup (new_text);
	  song->title_utf16 = g_utf8_to_utf16 (new_text, -1, NULL, NULL, NULL);
	  changed = TRUE;
	}
      break;
    case SM_COLUMN_GENRE:
      if (g_utf8_collate (song->genre, new_text) != 0)
	{
	  g_free (song->genre);
	  g_free (song->genre_utf16);
	  song->genre = g_strdup (new_text);
	  song->genre_utf16 = g_utf8_to_utf16 (new_text, -1, NULL, NULL, NULL);
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
      gint tag_id;
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
  gint column;
  gchar text[11];

  column = (gint)g_object_get_data (G_OBJECT (renderer), "column");
  gtk_tree_model_get (model, iter, SM_COLUMN_ALBUM, &song, -1);

  switch (column)
    {
    case SM_COLUMN_TITLE: 
      g_object_set (G_OBJECT (renderer), "text", song->title, 
		    "editable", TRUE, NULL);
      break;
    case SM_COLUMN_ARTIST: 
      g_object_set (G_OBJECT (renderer), "text", song->artist, 
		    "editable", TRUE, NULL);
      break;
    case SM_COLUMN_ALBUM: 
      g_object_set (G_OBJECT (renderer), "text", song->album, 
		    "editable", TRUE, NULL);
      break;
    case SM_COLUMN_GENRE: 
      g_object_set (G_OBJECT (renderer), "text", song->genre, 
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
      g_object_set (G_OBJECT (renderer), "active", song->pc_path_utf8, NULL);
      break;
    case SM_COLUMN_TRANSFERRED:
      g_object_set (G_OBJECT (renderer), "active", song->transferred, NULL);
      break;
    case SM_COLUMN_NONE:
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
/**
 * If/When we can ever trap songview changes this code *should* reorder all
 * user defined playlists to match the listing on screen
 */
void
sm_rows_reordered_callback(void)
{
    Song *new_song = NULL;
    Playlist *current_pl = NULL;
		    
    if((current_pl = get_currently_selected_playlist()))
    {
	GtkTreeIter i;
	GList *new_list = NULL;
	gboolean valid = FALSE;
	GtkTreeModel *tm = NULL;
			    
	if(current_pl->type == PL_TYPE_MPL) 
	    return;
	
	if((tm = gtk_tree_view_get_model(GTK_TREE_VIEW(song_treeview))))
	{
	    valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tm),&i);
	    while(valid)
	    {
		gtk_tree_model_get(tm, &i, 0, &new_song, -1); 
		new_list = g_list_append(new_list, new_song);
		valid = gtk_tree_model_iter_next(tm, &i);
	    }
	    g_list_free(current_pl->members);
	    current_pl->members = new_list;
	}
    }
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
  GtkSortType order;

  gtk_tree_model_get (model, a, SM_COLUMN_ALBUM, &song1, -1);
  gtk_tree_model_get (model, b, SM_COLUMN_ALBUM, &song2, -1);
  if(gtk_tree_sortable_get_sort_column_id (GTK_TREE_SORTABLE (model),
					   &column, &order) == FALSE) return 0;
  switch (column)
    {
	case SM_COLUMN_TITLE:
	    return g_utf8_collate (g_utf8_casefold (song1->title, -1),
		    g_utf8_casefold (song2->title, -1));
	case SM_COLUMN_ARTIST:
	    return g_utf8_collate (g_utf8_casefold (song1->artist, -1),
		    g_utf8_casefold (song2->artist, -1));
	case SM_COLUMN_ALBUM:
	    return g_utf8_collate (g_utf8_casefold (song1->album, -1),
		    g_utf8_casefold (song2->album, -1));
	case SM_COLUMN_GENRE:
	    return g_utf8_collate (g_utf8_casefold (song1->genre, -1),
		    g_utf8_casefold (song2->genre, -1));
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
	    break;
	case SM_COLUMN_NONE: 
	    if (current_playlist)
		return((g_list_index(current_playlist->members, song1)) -
		       (g_list_index(current_playlist->members, song2)));
	    break;
	default:
	    gtkpod_warning("No sort for column %d\n", column);
	    break;
    }
  return 0;
}

static void
sm_song_column_button_clicked(GtkTreeViewColumn *tvc, gpointer data)
{
    Playlist *pl = NULL;

    if((pl = get_currently_selected_playlist()) &&
	    (pl->type != PL_TYPE_MPL))
    {
	switch((gint)data)
	{
	    case SM_COLUMN_TITLE:
	    case SM_COLUMN_ARTIST:
	    case SM_COLUMN_ALBUM:
	    case SM_COLUMN_GENRE:
	    case SM_COLUMN_TRACK_NR:
		sm_rows_reordered_callback();
		break;
	    default:
		fprintf(stderr, "Unknown clicked:%d\n",(gint)data);
		break;
	}
    }
}

/* Adds the columns to our song_treeview */
static void add_song_columns ()
{
  gint col_id;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkTreeModel *model = gtk_tree_view_get_model (song_treeview);
  
  /* title column */
  col_id = SM_COLUMN_TITLE;
  renderer = gtk_cell_renderer_text_new ();
  g_signal_connect (G_OBJECT (renderer), "edited",
		    G_CALLBACK (sm_cell_edited), model);
  g_object_set_data (G_OBJECT (renderer), "column", (gint *)col_id);
  column = gtk_tree_view_column_new_with_attributes (_("Title"), renderer,
							NULL);
  gtk_tree_view_column_set_cell_data_func (column, renderer,
					    sm_cell_data_func, NULL, NULL);
  gtk_tree_view_column_set_sort_column_id (column, col_id);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_column_set_clickable(column, TRUE);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  sm_columns[col_id] = column;
  gtk_tree_view_column_set_fixed_width (column,
					prefs_get_sm_col_width (col_id));
  g_signal_connect (G_OBJECT (column), "clicked",
		    G_CALLBACK (sm_song_column_button_clicked),
				(gpointer)col_id);
  gtk_tree_view_append_column (song_treeview, column);

  /* artist column */
  col_id = SM_COLUMN_ARTIST;
  renderer = gtk_cell_renderer_text_new ();
  g_signal_connect (G_OBJECT (renderer), "edited",
		    G_CALLBACK (sm_cell_edited), model);
  g_object_set_data (G_OBJECT (renderer), "column", (gint *)col_id);
  column = gtk_tree_view_column_new_with_attributes (_("Artist"), renderer,
						    NULL);
  gtk_tree_view_column_set_cell_data_func (column, renderer,
					    sm_cell_data_func, NULL, NULL);
  gtk_tree_view_column_set_sort_column_id (column, col_id);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_column_set_clickable(column, TRUE);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  sm_columns[col_id] = column;
  gtk_tree_view_column_set_fixed_width (column,
					prefs_get_sm_col_width (col_id));
  g_signal_connect (G_OBJECT (column), "clicked",
		    G_CALLBACK (sm_song_column_button_clicked),
				(gpointer)col_id);
  gtk_tree_view_append_column (song_treeview, column);
  
  /* album column */
  col_id = SM_COLUMN_ALBUM;
  renderer = gtk_cell_renderer_text_new ();
  g_signal_connect (G_OBJECT (renderer), "edited",
		    G_CALLBACK (sm_cell_edited), model);
  g_object_set_data (G_OBJECT (renderer), "column", (gint *)col_id);
  column = gtk_tree_view_column_new_with_attributes (_("Album"), renderer, NULL);
  gtk_tree_view_column_set_cell_data_func (column, renderer,
					    sm_cell_data_func, NULL, NULL);
  gtk_tree_view_column_set_sort_column_id (column, col_id);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_column_set_clickable(column, TRUE);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  sm_columns[col_id] = column;
  gtk_tree_view_column_set_fixed_width (column,
					prefs_get_sm_col_width (col_id));
  g_signal_connect (G_OBJECT (column), "clicked",
		    G_CALLBACK (sm_song_column_button_clicked),
				(gpointer)col_id);
  gtk_tree_view_append_column (song_treeview, column);

  /* genre column */
  col_id = SM_COLUMN_GENRE;
  renderer = gtk_cell_renderer_text_new ();
  g_signal_connect (G_OBJECT (renderer), "edited",
		    G_CALLBACK (sm_cell_edited), model);
  g_object_set_data (G_OBJECT (renderer), "column", (gint *)col_id);
  column = gtk_tree_view_column_new_with_attributes (_("Genre"), renderer, NULL);
  gtk_tree_view_column_set_cell_data_func (column, renderer,
					    sm_cell_data_func, NULL, NULL);
  gtk_tree_view_column_set_sort_column_id (column, col_id);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_column_set_clickable(column, TRUE);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  sm_columns[col_id] = column;
  gtk_tree_view_column_set_fixed_width (column,
					prefs_get_sm_col_width (col_id));
  g_signal_connect (G_OBJECT (column), "clicked",
		    G_CALLBACK (sm_song_column_button_clicked),
				(gpointer)col_id);
  gtk_tree_view_append_column (song_treeview, column);
  
  /* track column */
  col_id = SM_COLUMN_TRACK_NR;
  renderer = gtk_cell_renderer_text_new ();
  g_signal_connect (G_OBJECT (renderer), "edited",
		    G_CALLBACK (sm_cell_edited), model);
  g_object_set_data (G_OBJECT (renderer), "column", (gint *)col_id);
  column = gtk_tree_view_column_new_with_attributes (_("Track Number"),
							renderer, NULL);
  gtk_tree_view_column_set_cell_data_func (column, renderer,
					    sm_cell_data_func, NULL, NULL);
  gtk_tree_view_column_set_sort_column_id (column, col_id);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_column_set_clickable(column, TRUE);
  sm_columns[col_id] = column;
  gtk_tree_view_column_set_fixed_width (column,
					prefs_get_sm_col_width (col_id));
  g_signal_connect (G_OBJECT (column), "clicked",
		    G_CALLBACK (sm_song_column_button_clicked),
				(gpointer)col_id);
  gtk_tree_view_append_column (song_treeview, column);
  
  /* ipod_id column */
  col_id = SM_COLUMN_IPOD_ID;
  renderer = gtk_cell_renderer_text_new ();
  g_object_set_data (G_OBJECT (renderer), "column", (gint *)col_id);
  column = gtk_tree_view_column_new_with_attributes (_("iPod ID"), renderer,
						    NULL);
  gtk_tree_view_column_set_cell_data_func (column, renderer,
					    sm_cell_data_func, NULL, NULL);
  gtk_tree_view_column_set_sort_column_id (column, col_id);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  sm_columns[col_id] = column;
  gtk_tree_view_column_set_fixed_width (column,
					prefs_get_sm_col_width (col_id));
  gtk_tree_view_append_column (song_treeview, column);

  /* pc_path column */
  col_id = SM_COLUMN_PC_PATH;
  renderer = gtk_cell_renderer_text_new ();
  g_signal_connect (G_OBJECT (renderer), "edited",
		    G_CALLBACK (sm_cell_edited), model);
  g_object_set_data (G_OBJECT (renderer), "column", (gint *)col_id);
  column = gtk_tree_view_column_new_with_attributes (_("PC File"), renderer,
						    NULL);
  gtk_tree_view_column_set_cell_data_func (column, renderer,
					    sm_cell_data_func, NULL, NULL);
  gtk_tree_view_column_set_sort_column_id (column, col_id);
  gtk_tree_view_column_set_resizable (column, TRUE);
  sm_columns[col_id] = column;
  gtk_tree_view_column_set_fixed_width (column,
					prefs_get_sm_col_width (col_id));
  gtk_tree_view_append_column (song_treeview, column);
  
  /* transferred column */
  col_id = SM_COLUMN_TRANSFERRED;
  renderer = gtk_cell_renderer_toggle_new ();
  g_object_set_data (G_OBJECT (renderer), "column", 
	      (gint*)col_id);
  column = gtk_tree_view_column_new_with_attributes (_("Trnsfrd"), renderer,
						    NULL);
  gtk_tree_view_column_set_cell_data_func (column, renderer,
					    sm_cell_data_func, NULL, NULL);
  gtk_tree_view_column_set_sort_column_id (column, col_id);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  sm_columns[col_id] = column;
  gtk_tree_view_column_set_fixed_width (column,
					prefs_get_sm_col_width (col_id));
  gtk_tree_view_append_column (song_treeview, column);
  
  /* Gtkpod playlist sorting */
  col_id = SM_COLUMN_NONE;
  renderer = gtk_cell_renderer_toggle_new ();
  g_object_set_data (G_OBJECT (renderer), "column", 
	      (gint*)col_id);
  column = gtk_tree_view_column_new_with_attributes ("gtkpod", renderer,
						    NULL);
  gtk_tree_view_column_set_sort_column_id (column, col_id);
  gtk_tree_view_column_set_resizable (column, TRUE);
  sm_columns[col_id] = column;
  gtk_tree_view_column_set_fixed_width (column,
					prefs_get_sm_col_width (col_id));
  gtk_tree_view_append_column (song_treeview, column);

  sm_show_preferred_columns();
}



/* creates the model and sets the sort functions */
static GtkTreeModel *create_song_model (void)
{
  GtkListStore *model;

  /* create list store */
  model = gtk_list_store_new (SM_NUM_COLUMNS, G_TYPE_POINTER, G_TYPE_POINTER,
			      G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER,
			      G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER,
			      G_TYPE_POINTER);

  /* define sort functions for all of our lists */
  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
	  SM_COLUMN_TITLE, sm_data_compare_func, NULL, NULL);
  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
	  SM_COLUMN_ARTIST, sm_data_compare_func, NULL, NULL);
  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
	  SM_COLUMN_ALBUM, sm_data_compare_func, NULL, NULL);
  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
	  SM_COLUMN_GENRE, sm_data_compare_func, NULL, NULL);
  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
	  SM_COLUMN_TRACK_NR, sm_data_compare_func, NULL, NULL);
  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
	  SM_COLUMN_IPOD_ID, sm_data_compare_func, NULL, NULL);
  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
	  SM_COLUMN_PC_PATH, sm_data_compare_func, NULL, NULL);
  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
	  SM_COLUMN_TRANSFERRED, sm_data_compare_func, NULL, NULL);
  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
	  SM_COLUMN_NONE, sm_data_compare_func, NULL, NULL);

  return GTK_TREE_MODEL (model);
}



/* Create songs listview */
static void create_song_listview (GtkWidget *gtkpod)
{
  GtkTreeModel *model;

  /* create model */
  model = create_song_model ();
  /* set tree view */
  song_treeview = GTK_TREE_VIEW (lookup_widget (gtkpod, "song_treeview"));
  gtk_tree_view_set_model (song_treeview, GTK_TREE_MODEL (model));
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (song_treeview), TRUE);
  gtk_tree_selection_set_mode (gtk_tree_view_get_selection (song_treeview),
			       GTK_SELECTION_MULTIPLE);
  add_song_columns ();
  sm_dnd_advertise(song_treeview);
}


/* Create the different listviews to display the various information */
void create_display (GtkWidget *gtkpod)
{
    GtkWidget *stop_button;

    create_song_listview (gtkpod);
    create_sort_tabs (gtkpod);
    create_playlist_listview (gtkpod);
    /* set certain sizes, positions, widths... to default values */
    display_set_default_sizes ();
    /* Hide the "stop_button" */
    stop_button = lookup_widget (gtkpod_window, "stop_button");
    if (stop_button) gtk_widget_hide (stop_button);
}


/* Clean up used memory (when quitting the program) */
void cleanup_display (void)
{
  cleanup_sort_tabs ();
}

/*
 * utility function for appending ipod song ids for song treeview callback
 */
void 
on_song_listing_drag_foreach(GtkTreeModel *tm, GtkTreePath *tp, 
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

void
sm_show_preferred_columns(void)
{
    GtkTreeViewColumn *tvc = NULL;
	
    if(prefs_get_song_list_show_all())
    {
	if((tvc = gtk_tree_view_get_column(song_treeview, SM_COLUMN_ALBUM)))
	    gtk_tree_view_column_set_visible(tvc, TRUE);
	if((tvc = gtk_tree_view_get_column(song_treeview, SM_COLUMN_TRACK_NR)))
	    gtk_tree_view_column_set_visible(tvc, TRUE);
	if((tvc = gtk_tree_view_get_column(song_treeview, SM_COLUMN_GENRE)))
	    gtk_tree_view_column_set_visible(tvc, TRUE);
	if((tvc = gtk_tree_view_get_column(song_treeview, SM_COLUMN_ARTIST)))
	    gtk_tree_view_column_set_visible(tvc, TRUE);
    }
    else
    {
	tvc = gtk_tree_view_get_column(song_treeview, SM_COLUMN_ALBUM);
	if(prefs_get_song_list_show_album())
	    gtk_tree_view_column_set_visible(tvc, TRUE);
	else
	    gtk_tree_view_column_set_visible(tvc, FALSE);
	
	tvc = gtk_tree_view_get_column(song_treeview, SM_COLUMN_TRACK_NR);
	if(prefs_get_song_list_show_track())
	    gtk_tree_view_column_set_visible(tvc, TRUE);
	else
	    gtk_tree_view_column_set_visible(tvc, FALSE);
	
	tvc = gtk_tree_view_get_column(song_treeview, SM_COLUMN_GENRE);
	if(prefs_get_song_list_show_genre())
	    gtk_tree_view_column_set_visible(tvc, TRUE);
	else
	    gtk_tree_view_column_set_visible(tvc, FALSE);
	
	tvc = gtk_tree_view_get_column(song_treeview, SM_COLUMN_ARTIST);
	if(prefs_get_song_list_show_artist())
	    gtk_tree_view_column_set_visible(tvc, TRUE);
	else
	    gtk_tree_view_column_set_visible(tvc, FALSE);
    }
#if TRUE
    if((tvc = gtk_tree_view_get_column(song_treeview, SM_COLUMN_PC_PATH)))
	gtk_tree_view_column_set_visible(tvc, FALSE);
    if((tvc = gtk_tree_view_get_column(song_treeview, SM_COLUMN_IPOD_ID)))
	gtk_tree_view_column_set_visible(tvc, FALSE);
    if((tvc = gtk_tree_view_get_column(song_treeview, SM_COLUMN_TRANSFERRED)))
	gtk_tree_view_column_set_visible(tvc, FALSE);
    if((tvc = gtk_tree_view_get_column(song_treeview, SM_COLUMN_NONE)))
	gtk_tree_view_column_set_visible(tvc, FALSE);
#endif
}

Playlist*
get_currently_selected_playlist(void)
{
    return(current_playlist);
}

void
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
get_currently_selected_songs(void)
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
	for (i=0; i<PANED_NUM; ++i)
	{
	    if (prefs_get_paned_pos (i) != -1)
	    {
		buf = g_strdup_printf ("paned%d", i);
		if((w = lookup_widget(gtkpod_window,  buf)))
		    gtk_paned_set_position (GTK_PANED (w),
					    prefs_get_paned_pos (i));
		g_free (buf);
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
	for (i=0; i<PANED_NUM; ++i)
	{
	    buf = g_strdup_printf ("paned%d", i);
	    if((w = lookup_widget(gtkpod_window,  buf)))
	    {
		prefs_set_paned_pos (i,
				     gtk_paned_get_position (GTK_PANED (w)));
	    }
	    g_free (buf);
	}
    }
}


/* ------------------------------------------------------------

           Functions for stopping display update 

   ------------------------------------------------------------ */

enum {
    BR_BLOCK,
    BR_RELEASE,
    BR_ADD
};

/* called by block_selection() and release_selection */
static void block_release_selection (gint inst, gint action,
				     br_callback brc,
				     gpointer user_data1,
				     gpointer user_data2)
{
    static gint count_st[SORT_TAB_NUM];
    static gint count_pl = 0;
    static GtkWidget *stop_button = NULL;
    gint i;
    /* instance that has a pending callback */
    static gint level = SORT_TAB_NUM; /* no level -> no registered callback */
    static br_callback r_brc;
    static gpointer r_user_data1;
    static gpointer r_user_data2;

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
	for (i=0; (i<=inst) && (i<SORT_TAB_NUM); ++i)
	{
	    ++count_st[i];
	}
	break;
    case BR_RELEASE:
	for (i=0; (i<=inst) && (i<SORT_TAB_NUM); ++i)
	{
	    --count_st[i];
	    if ((count_st[i] == 0) && (stop_add == i))
		stop_add = SORT_TAB_NUM;
	}
	--count_pl;
	if (count_pl == 0)
	{
	    if (stop_button) gtk_widget_hide (stop_button);
	    stop_add = SORT_TAB_NUM;
	    release_widgets ();
	}
	/* check if we have to call a callback */
	if (level < SORT_TAB_NUM)
	{
	    if (((level == -1) && (count_pl == 0)) ||
		((level >= 0) && (count_st[level] == 0)))
	    {
		level = SORT_TAB_NUM;
		r_brc (r_user_data1, r_user_data2);
	    }
	}
	break;
    case BR_ADD:
	if (((inst == -1) && (count_pl == 0)) ||
	    ((inst >= 0) && (count_st[inst] == 0)))
	{ /* OK, we can just call the desired function */
	    if (level > inst)
	    {
		level = SORT_TAB_NUM;
		brc (user_data1, user_data2);
	    }
	}
	else
	{
	    if (inst < level)
	    {   /* We need to emit a stop_add signal */
		stop_add = inst;
		/* and safe the callback data */
		level = inst;
		r_brc = brc;
		r_user_data1 = user_data1;
		r_user_data2 = user_data2;
	    }
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

static void add_selection_callback (gint inst, br_callback brc,
				    gpointer user_data1, gpointer user_data2)
{
    block_release_selection (inst, BR_ADD, brc, user_data1, user_data2);
}
