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
#include "display_private.h"
#include "song.h"
#include "playlist.h"
#include "interface.h"
#include "callbacks.h"
#include "misc.h"
#include "file.h"
#include "context_menus.h"


/* pointer to the treeview for the song display */
static GtkTreeView *song_treeview = NULL;
/* array with pointers to the columns used in the song display */
static GtkTreeViewColumn *sm_columns[SM_NUM_COLUMNS];

static GtkTreeViewColumn *sm_add_column (SM_item sm_item, gint position);

/* Drag and drop definitions */
static GtkTargetEntry sm_drag_types [] = {
    { DND_GTKPOD_SM_PATHLIST_TYPE, 0, DND_GTKPOD_SM_PATHLIST },
    { DND_GTKPOD_IDLIST_TYPE, 0, DND_GTKPOD_IDLIST },
    { "text/plain", 0, DND_TEXT_PLAIN },
    { "STRING", 0, DND_TEXT_PLAIN }
};
static GtkTargetEntry sm_drop_types [] = {
    { DND_GTKPOD_SM_PATHLIST_TYPE, 0, DND_GTKPOD_SM_PATHLIST },
/*    { DND_GTKPOD_IDLIST_TYPE, 0, DND_GTKPOD_IDLIST },*/
    { "text/plain", 0, DND_TEXT_PLAIN },
    { "STRING", 0, DND_TEXT_PLAIN }
};


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
			SM_COLUMN_SIZE, song,
			SM_COLUMN_SONGLEN, song,
			SM_COLUMN_BITRATE, song,
			SM_COLUMN_PLAYCOUNT, song,
			SM_COLUMN_RATING, song,
			SM_COLUMN_TIME_CREATED, song,
			SM_COLUMN_TIME_PLAYED, song,
			SM_COLUMN_TIME_MODIFIED, song,
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
void sm_remove_song (Song *song)
{
  GtkTreeModel *model = gtk_tree_view_get_model (song_treeview);
  if (model != NULL)
    gtk_tree_model_foreach (model, sm_delete_song, song);
}


/* Remove all songs from the display model */
/* ATTENTION: the treeview and model might be changed by calling this
   function */
void sm_remove_all_songs (gboolean clear_sort)
{
  GtkTreeModel *model = gtk_tree_view_get_model (song_treeview);
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
void sm_song_changed (Song *song)
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
  guint32 nr;
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
      nr = atoi (new_text);
      if ((nr >= 0) && (nr != song->track_nr))
      {
	  song->track_nr = nr;
	  changed = TRUE;
      }
      break;
  case SM_COLUMN_PLAYCOUNT:
      nr = atoi (new_text);
      if ((nr >= 0) && (nr != song->playcount))
      {
	  song->playcount = nr;
	  changed = TRUE;
      }
      break;
  case SM_COLUMN_RATING:
      nr = atoi (new_text);
      if ((nr >= 0) && (nr <= 5) && (nr != song->rating))
      {
	  song->rating = nr*RATING_STEP;
	  changed = TRUE;
      }
      break;
  case SM_COLUMN_TIME_CREATED:
  case SM_COLUMN_TIME_PLAYED:
  case SM_COLUMN_TIME_MODIFIED:
      break;
  default:
      g_warning ("Programming error: sm_cell_edited: unknown song cell (%d) edited\n", column);
      break;
  }
  if (changed)
  {
      song->time_modified = time_get_mac_time ();
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
  gchar text[21];
  gchar *buf = NULL;
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
      g_object_set (G_OBJECT (renderer),
		    "text", item_utf8, 
		    "editable", TRUE, NULL);
      break;
  case SM_COLUMN_TRACK_NR:
      if (song->track_nr >= 0)
      {
	  snprintf (text, 20, "%d", song->track_nr);
	  g_object_set (G_OBJECT (renderer),
			"text", text,
			"editable", TRUE,
			"xalign", 1.0, NULL);
      } 
      else
      {
	  g_object_set (G_OBJECT (renderer),
			"text", "0",
			"editable", TRUE,
			"xalign", 1.0, NULL);
      }
      break;
  case SM_COLUMN_IPOD_ID:
      if (song->ipod_id != -1)
      {
	  snprintf (text, 20, "%d", song->ipod_id);
	  g_object_set (G_OBJECT (renderer),
			"text", text,
			"xalign", 1.0, NULL);
      } 
      else
      {
	  g_object_set (G_OBJECT (renderer),
			"text", "--",
			"xalign", 1.0, NULL);
      }
      break;
  case SM_COLUMN_PC_PATH:
      g_object_set (G_OBJECT (renderer), "text", song->pc_path_utf8, NULL);
      break;
  case SM_COLUMN_TRANSFERRED:
      g_object_set (G_OBJECT (renderer), "active", song->transferred, NULL);
      break;
  case SM_COLUMN_SIZE:
      snprintf (text, 20, "%d", song->size);
      g_object_set (G_OBJECT (renderer),
		    "text", text,
		    "xalign", 1.0, NULL);
      break;
  case SM_COLUMN_SONGLEN:
      snprintf (text, 20, "%d:%02d", song->songlen/60000,
                                     (song->songlen/1000)%60);
      g_object_set (G_OBJECT (renderer),
		    "text", text,
		    "xalign", 1.0, NULL);
      break;
  case SM_COLUMN_BITRATE:
      snprintf (text, 20, "%dk", song->bitrate/1000);
      g_object_set (G_OBJECT (renderer),
		    "text", text,
		    "xalign", 1.0, NULL);
      break;
  case SM_COLUMN_PLAYCOUNT:
      snprintf (text, 20, "%d", song->playcount);
      g_object_set (G_OBJECT (renderer),
		    "text", text,
		    "editable", TRUE,
		    "xalign", 1.0, NULL);
      break;
  case SM_COLUMN_RATING:
      snprintf (text, 20, "%d", song->rating/RATING_STEP);
      g_object_set (G_OBJECT (renderer),
		    "text", text,
		    "editable", TRUE,
		    "xalign", 1.0, NULL);
      break;
  case SM_COLUMN_TIME_CREATED:
  case SM_COLUMN_TIME_PLAYED:
  case SM_COLUMN_TIME_MODIFIED:
      buf = time_field_to_string (song, column);
      g_object_set (G_OBJECT (renderer),
		    "text", buf,
/* don't know how to make it editable yet */
/* 		    "editable", TRUE, */
		    "xalign", 0.0, NULL);
      C_FREE (buf);
      break;
  default:
      g_warning ("Programming error: unknown column in sm_cell_data_func: %d\n", column);
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
		    g_warning ("Programming error: sm_rows_reordered_callback: song in song view was not in selected playlist\n");
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
      return compare_string (song_get_item_utf8 (song1, SM_to_S (sm_item)),
			     song_get_item_utf8 (song2, SM_to_S (sm_item)));
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
  case SM_COLUMN_SIZE:
      return song1->size - song2->size;
  case SM_COLUMN_SONGLEN:
      return song1->songlen - song2->songlen;
  case SM_COLUMN_BITRATE:
      return song1->bitrate - song2->bitrate;
  case SM_COLUMN_PLAYCOUNT:
      return song1->playcount - song2->playcount;
  case  SM_COLUMN_RATING:
      return song1->rating - song2->rating;
  case SM_COLUMN_TIME_CREATED:
  case SM_COLUMN_TIME_PLAYED:
  case SM_COLUMN_TIME_MODIFIED:
      return time_get_time (song1, sm_item) - time_get_time (song2, sm_item);
  default:
      g_warning ("Programming error: sm_data_compare_func: no sort method for column %d\n", column);
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
static GtkTreeViewColumn *sm_add_text_column (SM_item col_id,
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
  gchar *text = NULL;
  gboolean editable = TRUE;          /* default */
  GtkCellRenderer *renderer = NULL;  /* default */

  if ((sm_item) < 0 || (sm_item >= SM_NUM_COLUMNS))  return NULL;

  switch (sm_item)
  {
  case SM_COLUMN_TITLE:
      text = _("Title");
      break;
  case SM_COLUMN_ARTIST:
      text = _("Artist");
      break;
  case SM_COLUMN_ALBUM:
      text = _("Album");
      break;
  case SM_COLUMN_GENRE:
      text = _("Genre");
      break;
  case SM_COLUMN_COMPOSER:
      text = _("Composer");
      break;
  case SM_COLUMN_TRACK_NR:
      text = _("#");
      break;
  case SM_COLUMN_IPOD_ID:
      text = _("ID");
      editable = FALSE;
      break;
  case SM_COLUMN_PC_PATH:
      text = _("PC File");
      editable = FALSE;
      break;
  case SM_COLUMN_TRANSFERRED:
      text = _("Trnsfrd");
      editable = FALSE;
      renderer = gtk_cell_renderer_toggle_new ();
      break;
  case SM_COLUMN_SIZE:
      text = _("File Size");
      editable = FALSE;
      break;
  case SM_COLUMN_SONGLEN:
      text = _("Time");
      editable = FALSE;
      break;
  case SM_COLUMN_BITRATE:
      text = _("Bitrate");
      editable = FALSE;
      break;
  case SM_COLUMN_PLAYCOUNT:
      text = _("Playcount");
      break;
  case SM_COLUMN_RATING:
      text = _("Rating");
      break;
  case SM_COLUMN_TIME_CREATED:
      text = _("Imported");
      editable = FALSE;
      break;
  case SM_COLUMN_TIME_PLAYED:
      text = _("Played");
      editable = FALSE;
      break;
  case SM_COLUMN_TIME_MODIFIED:
      text = _("Modified");
      editable = FALSE;
      break;
  case SM_NUM_COLUMNS:
      break;
  }
  col = sm_add_text_column (sm_item, text, renderer, editable, pos);
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
void sm_create_treeview (void)
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
			  G_TYPE_POINTER, G_TYPE_POINTER,
			  G_TYPE_POINTER, G_TYPE_POINTER,
			  G_TYPE_POINTER, G_TYPE_POINTER,
			  G_TYPE_POINTER, G_TYPE_POINTER));
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


/* update the cfg structure (preferences) with the current sizes /
   positions (called by display_update_default_sizes():
   column widths of song model */
void sm_update_default_sizes (void)
{
    gint i;
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
}




/* ---------------------------------------------------------------- */
/* Section for drag and drop                                        */
/* ---------------------------------------------------------------- */

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

void  sm_list_store_move_before (GtkListStore *store,
					 GtkTreeIter  *iter,
					 GtkTreeIter  *position)
{
    sm_list_store_move (store, iter, position, TRUE);
}

void  sm_list_store_move_after (GtkListStore *store,
					GtkTreeIter  *iter,
					GtkTreeIter  *position)
{
    sm_list_store_move (store, iter, position, FALSE);
}
#else
/* starting V2.2 convenient gtk functions exist */
void  sm_list_store_move_before (GtkListStore *store,
					 GtkTreeIter  *iter,
					 GtkTreeIter  *position)
{
    gtk_list_store_move_before (store, iter, position);
}

void  sm_list_store_move_after (GtkListStore *store,
					GtkTreeIter  *iter,
					GtkTreeIter  *position)
{
    gtk_list_store_move_after (store, iter, position);
}
#endif


/* move pathlist for song treeview */
gboolean sm_move_pathlist (gchar *data,
			   GtkTreePath *path,
			   GtkTreeViewDropPosition pos)
{
    return pmsm_move_pathlist (song_treeview, data, path, pos, SONG_TREEVIEW);
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
    Playlist *current_playlist = pm_get_selected_playlist ();

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
    /* can call on SM_COLUMN_TITLE cause s is consistent across all of
       the columns */
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
    /* can call on SM_COLUMN_TITLE cause s is consistent across all of
     * the columns */
    name = get_song_name_on_disk_verified (s);
    if (name)
    {
	g_string_append_printf (filelist, "file:%s\n", name);
	g_free (name);
    }
}

