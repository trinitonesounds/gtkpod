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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <support.h>
#include <limits.h>

#include "prefs.h"
#include "display.h"
#include "song.h"
#include "playlist.h"
#include "interface.h"

static GtkTreeView *song_treeview = NULL;
static GtkTreeView *playlist_treeview = NULL;

static SortTab *sorttab[SORT_TAB_NUM];

static Playlist *current_playlist = NULL;

static void sm_song_changed (Song *song);
static void sm_remove_song (Song *song);
static void sm_remove_all_song (void);
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

static void
st_dnd_advertise(GtkTreeView *v)
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
  /* If it's the first playlist, we select the item, causing a
     callback which will initialize the select1 treeview and so on... */
  if (current_playlist == NULL)
    {
      selection = gtk_tree_view_get_selection (playlist_treeview);
      gtk_tree_selection_select_iter (selection, &iter);
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
    /* this is deleting two rows at a time. !!! */
    /*
    gtk_tree_model_row_deleted (model, path);
    */
    return TRUE;
  }
  return FALSE;
}


/* Remove playlist from the display models */
void pm_remove_playlist (Playlist *playlist)
{
  GtkTreeModel *model = gtk_tree_view_get_model (playlist_treeview);
  if (model != NULL)
  {
    GtkTreeIter i;
    GtkTreeSelection *ts = NULL;
    
    /* 
     * get the currently selected playlist from the playlist view  and its
     * iterator 
     */
    ts = gtk_tree_view_get_selection(playlist_treeview);
    if((gtk_tree_selection_get_selected(ts, NULL, &i)))
    {
	/* if we can't select the next iterator, grab the first one*/
	if(!(gtk_tree_model_iter_next(model, &i)))
	{
		gtk_tree_model_get_iter_first(model, &i);
	}
    
	/* find the pl and delete it */
	gtk_tree_model_foreach (model, pm_delete_playlist, playlist);
	
	/* select our new found iter !!! */
	gtk_tree_selection_select_iter(ts, &i);
    }
  }
}



/* Callback function called when the selection
   of the playlist view has changed */
static void pm_selection_changed (GtkTreeSelection *selection,
				  gpointer user_data)
{
  GtkTreeModel *model;
  GtkTreeIter  iter;
  Playlist *new_playlist;
  Song *song;
  guint32 n,i;

  if (gtk_tree_selection_get_selected (selection, &model, &iter) == FALSE)
    return; /* no selection -- strange! */
  gtk_tree_model_get (model, &iter, 
		      PM_COLUMN_PLAYLIST, &new_playlist,
		      -1);
  /* no change in selection -- strange! */
  /* if (new_playlist == current_playlist) return; */
  /* remove all entries from sort tab 0 */
  /* printf ("removing entries: %x\n", current_playlist);*/
  st_init (-1, 0);
  current_playlist = new_playlist;
  n = get_nr_of_songs_in_playlist (new_playlist);
  for (i=0; i<n; ++i)
    { /* add all songs to sort tab 0 */
      song = get_song_in_playlist_by_nr (new_playlist, i);
      st_add_song (song, FALSE, 0);
    }
  st_add_song (NULL, TRUE, 0);
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
      g_free (playlist->name);
      g_free (playlist->name_utf16);
      playlist->name = g_strdup (new_text);
      playlist->name_utf16 = g_utf8_to_utf16 (new_text, -1,
					      NULL, NULL, NULL);
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
      if(playlist->type == 0) 
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
  pm_dnd_advertise(GTK_TREE_VIEW(lookup_widget(gtkpod, "playlist_treeview")));
}



/* ---------------------------------------------------------------- */
/* Section for sort tab display                                     */
/* ---------------------------------------------------------------- */


/* Get the instance of the sort tab that corresponds to
   "notebook". Returns -1 if sort tab could not be found
   and prints error message */
static gint st_get_instance (GtkNotebook *notebook)
{
  gint i=0;
  while (i<SORT_TAB_NUM)
    {
      if (sorttab[i]->notebook == notebook) return i;
      ++i;
    }
  g_warning (_("Programming error (st_get_instance): notebook could not be found.\n"));
  return -1;
}


/* Append playlist to the playlist model. If it's the first entry,
   we select it, causing a callback that will reset the next
   instance */
static void st_add_entry (TabEntry *entry, guint32 inst)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkTreeSelection *selection;
  SortTab *st;

  st = sorttab[inst];
  model = st->model;
  g_return_if_fail (model != NULL);
  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		      ST_COLUMN_ENTRY, entry, -1);
  st->entries = g_list_append (st->entries, entry);
  if (st->current_entry == NULL)
    {
      selection = gtk_tree_view_get_selection
	(st->treeview[st->current_category]);
      gtk_tree_selection_select_iter (selection, &iter);
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
  if(entry == (TabEntry *)data) {
    gtk_list_store_remove (GTK_LIST_STORE (model), iter);
    gtk_tree_model_row_deleted (model, path);
    return TRUE;
  }
  return FALSE;
}


/* Remove entry from the display model and the sorttab */
static void st_remove_entry (TabEntry *entry, guint32 inst)
{
  GtkTreeModel *model = sorttab[inst]->model;
  if (model != NULL)
    {
      if (entry == sorttab[inst]->current_entry)
	{
	  sorttab[inst]->current_entry = NULL;
	}
      gtk_tree_model_foreach (model, st_delete_entry_from_model, entry);
      sorttab[inst]->entries = g_list_remove (sorttab[inst]->entries, entry);
      g_list_free (entry->members);
      if (entry->name) g_free (entry->name);
      g_free (entry);
    }
}

/* Remove all entries from the display model and the sorttab */
static void st_remove_all_entries (guint32 inst)
{
  TabEntry *entry;

  while (sorttab[inst]->entries != NULL)
    {
      entry = (TabEntry *)g_list_nth_data (sorttab[inst]->entries, 0);
      st_remove_entry (entry, inst);
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

  /*  printf("%d: looking for song: %x\n", inst, song);*/
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
   Skips the master entry! */
static TabEntry *st_get_entry_by_name (gchar *name, guint32 inst)
{
  GList *entries;
  TabEntry *entry;
  guint i;

  /*  printf("%d: looking for entry: %s\n", inst, name);*/
  if (name == NULL) return NULL;
  entries = sorttab[inst]->entries;
  i=1; /* skip master entry, which is supposed to be at first position */
  while ((entry = (TabEntry *)g_list_nth_data (entries, i)) != NULL)
    {
      /*      printf("    comparing with entry: %s\n", entry->name);*/
      if (g_utf8_collate (entry->name, name) == 0)   break; /* found! */
      ++i;
    }
  /*  printf("    returning entry: %x\n", entry);*/
  return entry;
}


/* removes a song from the entry it is currently in to the one it
   should be in according (if a Tag had been changed).
   Returns TRUE, if song has been moved, FALSE otherwise */
static gboolean st_recategorize_song (Song *song, guint32 inst)
{
  TabEntry *oldentry, *newentry;
  /* SortTab *st = sorttab[inst]; */
  gchar *entryname;

  oldentry = st_get_entry_by_song (song, inst);
  /*  printf("%d: recat_oldentry: %x\n", inst, oldentry);*/
/* should not happen: song is not in sort tab */
  if (oldentry == NULL) return FALSE;
  entryname = st_get_entryname (song, inst);
  /*  printf("%d: recat_entryname: %s\n", inst, entryname);*/
  newentry = st_get_entry_by_name (entryname, inst);
  /*  printf("%d: recat_newentry: %x\n", inst, newentry);*/
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
      if (g_list_find (st->current_entry->members, song) != NULL)
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
   song model (currently two sort tabs). */
static void st_add_song (Song *song, gboolean final, guint32 inst)
{
  TabEntry *entry, *iter_entry;
  SortTab *st;
  gchar *entryname;
  GtkTreeSelection *selection;
  GtkTreeIter iter;

  if (inst == SORT_TAB_NUM)
    {  /* just add to song model */
      if (song != NULL)    sm_add_song_to_song_model (song);
    }
  else
    {
      st = sorttab[inst];
      if (song != NULL)
	{
	  /* add song to "All" (master) entry */
	  entry = g_list_nth_data (st->entries, 0);
	  g_return_if_fail (entry != NULL);
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
	  if (final)   /* only one song was added -> add to next instance */
	    {
	      /* add if "All" or "entry" is selected */
	      if (st->current_entry->master || (st->current_entry == entry))
		{
		  st_add_song (song, final, inst+1);
		}
	    }
	}
      else
	{ /* We land here after a set of songs has been added. We try to
	     see if the old selection matches one of the new entries then
	     select either the previously selected entry or "All" */
	  if (final)
	    { /* should always be final! */
	      entry = st_get_entry_by_name (st->lastselection[st->current_category],
				    inst);
	      if (entry == NULL)
		{ /* Entry was not previously selected -> set to "All" */
		  entry = g_list_nth_data (st->entries, 0);
		  g_return_if_fail (entry != NULL); /* Programming error! */
		}
	      if (!gtk_tree_model_get_iter_first (st->model, &iter))
		{
		  g_warning ("Programming error: st_add_song: iter invalid\n");
		  return;
		}
	      /* printf("Instance %d looking for entry %x\n", inst, entry);*/
	      do {
	        gtk_tree_model_get (st->model, &iter, 
				    ST_COLUMN_ENTRY, &iter_entry,
				    -1);
		if (iter_entry == entry)
		  {
		    selection = gtk_tree_view_get_selection
		      (st->treeview[st->current_category]);
		    /* We may need to unselect the previous selection */
		    /* printf("Instance %d: Un-Selecting entry %x\n", inst, iter_entry);*/
		    gtk_tree_selection_unselect_all (selection);
		    /* printf("Instance %d: Selecting entry %x\n", inst, iter_entry);*/
		    gtk_tree_selection_select_iter (selection, &iter);
		    break;
		  }
	      } while (gtk_tree_model_iter_next (st->model, &iter));
	    }
	}
    }
}


/* Remove song from sort tab. If the song matches the currently
   selected sort criteria, it will be passed on to the next
   sort tab (i.e. removed). The last sort tab will remove the
   song from the song model (currently two sort tabs). */
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
      if ((st->current_entry == entry) || (st->current_entry == master))
	st_remove_song (song, inst+1);
    }
}


/* Init a sort tab: all current entries are removed. The next sort
   tab is initialized automatically because the new entry "All" gets
   selected. Set new_category to -1 if the current category is to be
   left unchanged (used by st_page_changed). */
static void st_init (gint32 new_category, guint32 inst)
{
  TabEntry *entry;
  SortTab *st;
  gint cat;

  if (inst == SORT_TAB_NUM)
    {
      sm_remove_all_song ();
    }
  else
    {
      st = sorttab[inst];
      cat = st->current_category;
      /* Copy selected entry name if not master */
      if (st->lastselection[cat])
	{
	  g_free (st->lastselection[cat]);
	  st->lastselection[cat] = NULL;
	}
      if (st->current_entry != NULL)
	{
	  if (!st->current_entry->master)
	    {
	      st->lastselection[cat] =
		g_strdup (st->current_entry->name);
	    }
	}
      if (new_category != -1)
	{
	  st->current_category = new_category;
	}
      st_remove_all_entries (inst);
      entry = g_malloc0 (sizeof (TabEntry));
      entry->name = g_strdup (_("All"));
      entry->master = TRUE;
      st_add_entry (entry, inst);
    }
}




/* Called when page in sort tab is selected */
void st_page_selected (GtkNotebook *notebook, guint page)
{
  guint32 inst;
  GList *copy = NULL;
  TabEntry *master;
  SortTab *st;
  gint i,n;
  Song *song;

  inst = st_get_instance (notebook);
  st = sorttab[inst];
  master = g_list_nth_data (st->entries, 0);
  /* Copy members before they get deleted by st_init */
  if (master != NULL)
    {
      copy = g_list_copy (master->members);
    }
  /* re-initialize current instance */
  st_init (page, inst);
  /* add all songs previously present to sort tab */
  n = g_list_length (copy);
  for (i=0; i<n; ++i)
    {
      song = (Song *)g_list_nth_data (copy, i);
      st_add_song (song, FALSE, inst);
    }
  st_add_song (NULL, TRUE, inst);
  g_list_free (copy);
}


/* Callback function called when the selection
   of the sort tab view has changed */
static void st_selection_changed (GtkTreeSelection *selection,
				  gpointer user_data)
{
  GtkTreeModel *model;
  GtkTreeIter  iter;
  TabEntry *new_entry;
  Song *song;
  SortTab *st;
  guint32 n,i;
  guint32 inst;


  inst = (guint32) user_data;
  st = sorttab[inst];
  if (st == NULL) return;
  if (gtk_tree_selection_get_selected (selection, &model, &iter) == FALSE)
    return; /* no selection -- strange! */
  gtk_tree_model_get (model, &iter, 
		      ST_COLUMN_ENTRY, &new_entry,
		      -1);
  /*  printf("selected instance %d, entry %x (was: %x)\n", inst, new_entry, st->current_entry);*/
  /* no change in selection -- strange! */
  /* if (new_entry == st->current_entry) return; */
  /* initialize next instance */
  st_init (-1, inst+1);
  st->current_entry = new_entry;
  n = g_list_length (new_entry->members); /* number of members */
  for (i=0; i<n; ++i)
    { /* add all member songs to next instance */
      song = (Song *)g_list_nth_data (new_entry->members, i);
      st_add_song (song, FALSE, inst+1);
    }
  st_add_song (NULL, TRUE, inst+1);
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
	    if (cfg->writeid3) write_tags_to_file (song);
	  }
	  g_list_free (members);
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
    }
}



/* Create sort tabs */
static void create_sort_tabs (GtkWidget *gtkpod)
{
  gint i;
  gchar *name;

  for (i=0; i<SORT_TAB_NUM; ++i)
    {
      sorttab[i] = g_malloc0 (sizeof (SortTab));
      name = g_strdup_printf ("sorttab%d", i);
      sorttab[i]->notebook = GTK_NOTEBOOK (lookup_widget (gtkpod, name));
      g_free (name);
      st_create_listview (gtkpod, i);
      gtk_notebook_set_current_page (sorttab[i]->notebook, i<ST_CAT_NUM ? i:0);
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
	  for (j=0; j<ST_CAT_NUM; ++j)
	    {
	      if (sorttab[i]->lastselection[j])
		g_free (sorttab[i]->lastselection[j]);
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
		      SM_COLUMN_ALBUM, song,
		      SM_COLUMN_ARTIST, song,
		      SM_COLUMN_TITLE, song,
		      SM_COLUMN_GENRE, song,
		      SM_COLUMN_TRACK, song,
		      SM_COLUMN_IPOD_ID, song,
		      SM_COLUMN_PC_PATH, song,
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
    /* this is deleting two rows !!! */
    /*
    gtk_tree_model_row_deleted (model, path);
    */
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
static void sm_remove_all_song (void)
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
    case SM_COLUMN_TRACK:
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
  if (changed) pm_song_changed (song); /* notify playlist model... */
  /* If anything changed and prefs say to write changes to file, do so */
  if (changed && cfg->writeid3) write_tags_to_file (song);
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
    case SM_COLUMN_ALBUM: 
      g_object_set (G_OBJECT (renderer), "text", song->album, 
		    "editable", TRUE, NULL);
      break;
    case SM_COLUMN_ARTIST: 
      g_object_set (G_OBJECT (renderer), "text", song->artist, 
		    "editable", TRUE, NULL);
      break;
    case SM_COLUMN_TITLE: 
      g_object_set (G_OBJECT (renderer), "text", song->title, 
		    "editable", TRUE, NULL);
      break;
    case SM_COLUMN_GENRE: 
      g_object_set (G_OBJECT (renderer), "text", song->genre, 
		    "editable", TRUE, NULL);
      break;
    case SM_COLUMN_PC_PATH: 
      g_object_set (G_OBJECT (renderer), "text", song->pc_path_utf8, NULL);
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
    case SM_COLUMN_TRACK:
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
    default:
      fprintf(stderr, "Unknown column in sm_cell_data_func: %d\n", column);
      break;

      /*    case SM_COLUMN_TRANSFERRED:
      g_object_set (G_OBJECT (renderer), "active", song->transferred, NULL);
      break;*/
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
    case SM_COLUMN_ALBUM:
      return g_utf8_collate (g_utf8_casefold (song1->album, -1), 
			     g_utf8_casefold (song2->album, -1));
    case SM_COLUMN_ARTIST:
      return g_utf8_collate (g_utf8_casefold (song1->artist, -1),
			     g_utf8_casefold (song2->artist, -1));
    case SM_COLUMN_TITLE:
      return g_utf8_collate (g_utf8_casefold (song1->title, -1),
			     g_utf8_casefold (song2->title, -1));
    case SM_COLUMN_GENRE:
      return g_utf8_collate (g_utf8_casefold (song1->genre, -1),
			     g_utf8_casefold (song2->genre, -1));
    case SM_COLUMN_PC_PATH:
      return g_utf8_collate (song1->pc_path_utf8, song2->pc_path_utf8);
    case SM_COLUMN_IPOD_ID:
      return song1->ipod_id - song2->ipod_id;
    case SM_COLUMN_TRACK:
      return song1->track_nr - song2->track_nr;
      /*    case SM_COLUMN_TRANSFERRED:
      if(song1->transferred == song2->transferred) return 0;
      if(song1->transferred == TRUE) return 1;
      return -1;*/
    }
  return 0;
}


/* Adds the columns to our song_treeview */
static void add_song_columns ()
{
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkTreeModel *model = gtk_tree_view_get_model (song_treeview);
  
  /* title column */
  renderer = gtk_cell_renderer_text_new ();
  g_signal_connect (G_OBJECT (renderer), "edited",
		    G_CALLBACK (sm_cell_edited), model);
  g_object_set_data (G_OBJECT (renderer), "column", (gint *)SM_COLUMN_TITLE);
  column = gtk_tree_view_column_new_with_attributes (_("Title"), renderer, NULL);
  gtk_tree_view_column_set_cell_data_func (column, renderer, sm_cell_data_func, NULL, NULL);
  gtk_tree_view_column_set_sort_column_id (column, SM_COLUMN_TITLE);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_column_set_fixed_width (column, 100);  /* FIXME */
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_append_column (song_treeview, column);

  /* artist column */
  renderer = gtk_cell_renderer_text_new ();
  g_signal_connect (G_OBJECT (renderer), "edited",
		    G_CALLBACK (sm_cell_edited), model);
  g_object_set_data (G_OBJECT (renderer), "column", (gint *)SM_COLUMN_ARTIST);
  column = gtk_tree_view_column_new_with_attributes (_("Artist"), renderer, NULL);
  gtk_tree_view_column_set_cell_data_func (column, renderer, sm_cell_data_func, NULL, NULL);
  gtk_tree_view_column_set_sort_column_id (column, SM_COLUMN_ARTIST);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_column_set_fixed_width (column, 100);  /* FIXME */
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_append_column (song_treeview, column);
  
  /* album column */
  renderer = gtk_cell_renderer_text_new ();
  g_signal_connect (G_OBJECT (renderer), "edited",
		    G_CALLBACK (sm_cell_edited), model);
  g_object_set_data (G_OBJECT (renderer), "column", (gint *)SM_COLUMN_ALBUM);
  column = gtk_tree_view_column_new_with_attributes (_("Album"), renderer, NULL);
  gtk_tree_view_column_set_cell_data_func (column, renderer, sm_cell_data_func, NULL, NULL);
  gtk_tree_view_column_set_sort_column_id (column, SM_COLUMN_ALBUM);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_column_set_fixed_width (column, 100);  /* FIXME */
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_append_column (song_treeview, column);

  /* genre column */
  renderer = gtk_cell_renderer_text_new ();
  g_signal_connect (G_OBJECT (renderer), "edited",
		    G_CALLBACK (sm_cell_edited), model);
  g_object_set_data (G_OBJECT (renderer), "column", (gint *)SM_COLUMN_GENRE);
  column = gtk_tree_view_column_new_with_attributes (_("Genre"), renderer, NULL);
  gtk_tree_view_column_set_cell_data_func (column, renderer, sm_cell_data_func, NULL, NULL);
  gtk_tree_view_column_set_sort_column_id (column, SM_COLUMN_GENRE);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (song_treeview, column);
  
  /* track column */
  renderer = gtk_cell_renderer_text_new ();
  g_signal_connect (G_OBJECT (renderer), "edited",
		    G_CALLBACK (sm_cell_edited), model);
  g_object_set_data (G_OBJECT (renderer), "column", (gint *)SM_COLUMN_TRACK);
  column = gtk_tree_view_column_new_with_attributes (_("Track Number"),
							renderer, NULL);
  gtk_tree_view_column_set_cell_data_func (column, renderer,
					    sm_cell_data_func, NULL, NULL);
  gtk_tree_view_column_set_sort_column_id (column, SM_COLUMN_TRACK);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (song_treeview, column);

  /* ipod_id column */
  /*
  renderer = gtk_cell_renderer_text_new ();
  g_object_set_data (G_OBJECT (renderer), "column", (gint *)SM_COLUMN_IPOD_ID);
  column = gtk_tree_view_column_new_with_attributes (_("Ipod ID"), renderer, NULL);
  gtk_tree_view_column_set_cell_data_func (column, renderer, sm_cell_data_func, NULL, NULL);
  gtk_tree_view_column_set_sort_column_id (column, SM_COLUMN_IPOD_ID);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (song_treeview, column);
  */
  /* transferred column */
  /*
  renderer = gtk_cell_renderer_toggle_new ();
  g_object_set_data (G_OBJECT (renderer), "column", (gint *)SM_COLUMN_TRANSFERRED);
  column = gtk_tree_view_column_new_with_attributes (_("Trnsfrd"), renderer, NULL);
  gtk_tree_view_column_set_cell_data_func (column, renderer, sm_cell_data_func, NULL, NULL);
  gtk_tree_view_column_set_sort_column_id (column, SM_COLUMN_TRANSFERRED);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (treeview, column);
  */
  /* pc_path column */
  /*
  renderer = gtk_cell_renderer_text_new ();
  g_signal_connect (G_OBJECT (renderer), "edited",
		    G_CALLBACK (sm_cell_edited), model);
  g_object_set_data (G_OBJECT (renderer), "column", (gint *)SM_COLUMN_PC_PATH);
  column = gtk_tree_view_column_new_with_attributes (_("PC File"), renderer, NULL);
  gtk_tree_view_column_set_cell_data_func (column, renderer, sm_cell_data_func, NULL, NULL);
  gtk_tree_view_column_set_sort_column_id (column, SM_COLUMN_PC_PATH);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_column_set_fixed_width (column, 100);  
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_append_column (song_treeview, column);
  */
  sm_show_preferred_columns();
}



/* creates the model and sets the sort functions */
static GtkTreeModel *create_song_model (void)
{
  GtkListStore *model;

  /* create list store */
  model = gtk_list_store_new (SM_NUM_COLUMNS, G_TYPE_POINTER, G_TYPE_POINTER,
			      G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER,
			      G_TYPE_POINTER, G_TYPE_POINTER);
  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
				 SM_COLUMN_ALBUM,
				 sm_data_compare_func, NULL, NULL);
  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
				 SM_COLUMN_ARTIST,
				 sm_data_compare_func, NULL, NULL);
  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
				 SM_COLUMN_TITLE,
				 sm_data_compare_func, NULL, NULL);
  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
				 SM_COLUMN_GENRE,
				 sm_data_compare_func, NULL, NULL);
  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
				 SM_COLUMN_PC_PATH,
				 sm_data_compare_func, NULL, NULL);
  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
				 SM_COLUMN_IPOD_ID,
				 sm_data_compare_func, NULL, NULL);
  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
				 SM_COLUMN_TRACK, sm_data_compare_func,
				 NULL, NULL);
  /*  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
				 SM_COLUMN_TRANSFERRED,
				 sm_data_compare_func, NULL, NULL);*/

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
  st_dnd_advertise(GTK_TREE_VIEW(lookup_widget(gtkpod, "song_treeview")));
}





/* Create the different listviews to display the various information */
void create_listviews (GtkWidget *gtkpod)
{
  create_song_listview (gtkpod);
  create_playlist_listview (gtkpod);
  create_sort_tabs (gtkpod);
}


/* Clean up used memory (when quitting the program) */
void cleanup_listviews (void)
{
  cleanup_sort_tabs ();
}

/*
 * utility function for appending ipod song ids for playlist callback
 */
void 
on_song_listing_drag_foreach(GtkTreeModel *tm, GtkTreePath *tp, 
				 GtkTreeIter *i, gpointer data)
{
    Song *s;
    gchar *new = NULL;
    gchar *filelist = *((gchar**)data);
    gtk_tree_model_get(tm, i, 0, &s, -1); 
    /* can call on 0 cause s is consistent across all of the columns */
    if(s)
    {
	gchar buf[PATH_MAX];
	if(filelist)
	{
	    snprintf(buf, PATH_MAX, "%s%d\n", filelist, s->ipod_id);
	    g_free(filelist);
	}
	else
	{
	    snprintf(buf, PATH_MAX, "%d\n", s->ipod_id);
	}
	new = g_strdup(buf);
	*((gchar**)data) = new;
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
	if((tvc = gtk_tree_view_get_column(song_treeview, SM_COLUMN_TRACK)))
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
	
	tvc = gtk_tree_view_get_column(song_treeview, SM_COLUMN_TRACK);
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
