/* Time-stamp: <2005-03-28 22:43:25 jcs>
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
#include "display_itdb.h"
#include "itdb.h"
#include "info.h"
#include "callbacks.h"
#include "misc.h"
#include "misc_track.h"
#include "file.h"
#include "context_menus.h"

/* pointer to the treeview for the track display */
static GtkTreeView *track_treeview = NULL;
/* array with pointers to the columns used in the track display */
static GtkTreeViewColumn *tm_columns[TM_NUM_COLUMNS];
/* column in which track pointer is stored */
static const gint READOUT_COL = 0;

static GtkTreeViewColumn *tm_add_column (TM_item tm_item, gint position);

/* Drag and drop definitions */
static GtkTargetEntry tm_drag_types [] = {
    { DND_GTKPOD_TM_PATHLIST_TYPE, 0, DND_GTKPOD_TM_PATHLIST },
    { DND_GTKPOD_TRACKLIST_TYPE, 0, DND_GTKPOD_TRACKLIST },
    { "text/plain", 0, DND_TEXT_PLAIN },
    { "STRING", 0, DND_TEXT_PLAIN }
};
static GtkTargetEntry tm_drop_types [] = {
    { DND_GTKPOD_TM_PATHLIST_TYPE, 0, DND_GTKPOD_TM_PATHLIST },
/*    { DND_GTKPOD_TRACKLIST_TYPE, 0, DND_GTKPOD_TRACKLIST },*/
    { "text/plain", 0, DND_TEXT_PLAIN },
    { "STRING", 0, DND_TEXT_PLAIN }
};


/* Note: the toggle buttons for tag_autoset and display_col in the
 * prefs_window are named after the the TM_COLUM_* numbers defined in
 * display.h (Title: tag_autoset0, Artist: tag_autoset1 etc.). Since
 * the labels to the buttons are set in prefs_window.c when creating
 * the window, you only need to name the buttons in the intended order
 * using glade-2. There is no need to label them. */
/* Strings associated to the column headers */
const gchar *tm_col_strings[] = {
    N_("Title"),             /*  0 */
    N_("Artist"),
    N_("Album"),
    N_("Genre"),
    N_("Composer"),
    N_("Track Nr (#)"),      /*  5 */
    N_("iPod ID"),
    N_("PC File"),
    N_("Transferred"),
    N_("File Size"),
    N_("Play Time"),         /* 10 */
    N_("Bitrate"),
    N_("Playcount"),
    N_("Rating"),
    N_("Time played"),
    N_("Time modified"),     /* 15 */
    N_("Volume"),
    N_("Year"),
    N_("CD Nr"),
    N_("Time created"),
    N_("iPod File"),         /* 20 */
    N_("Soundcheck"),
    N_("Samplerate"),
    N_("BPM"),
    N_("Kind"),
    N_("Grouping"),          /* 25 */
    N_("Compilation"),
    NULL };
/* Tooltips for prefs window */
const gchar *tm_col_tooltips[] = {
    NULL,                                              /*  0 */
    NULL,
    NULL,
    NULL,
    NULL,
    N_("Track Nr. and total number of tracks on CD"),  /*  5 */
    NULL,
    N_("Name of file on PC, if available"),
    N_("Whether the file has already been "
       "transferred to the iPod or not"),
    NULL,
    NULL,                                              /* 10 */
    NULL,
    N_("Number of times the track has been played"),
    N_("Star rating from 0 to 5"),
    N_("Time track has last been played"),
    N_("Time track has last been modified"),      /* 15 */
    N_("Manual volume adjust"),
    NULL,
    N_("CD Nr. and total number of CDS in set"),
    N_("Time track has been created (timestamp of file)"),
    N_("Name of file on the iPod"),                    /* 20 */
    N_("Volume adjust in dB (replay gain) -- "
       "you need to activate 'soundcheck' on the iPod"),
    NULL,
    N_("Supposedly something that tells the iPod to "
       "increase or decrease the playback speed"),
    NULL,
    NULL,                  /* 25 */
    NULL,
    NULL };


/* ---------------------------------------------------------------- */
/* Section for track display                                         */
/* ---------------------------------------------------------------- */


/* Append track to the track model (or write into @into_iter if != 0) */
void tm_add_track_to_track_model (Track *track, GtkTreeIter *into_iter)
{
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model (track_treeview);

    g_return_if_fail (model != NULL);

    if (into_iter)
	iter = *into_iter;
    else
	gtk_list_store_append (GTK_LIST_STORE (model), &iter);

    gtk_list_store_set (GTK_LIST_STORE (model), &iter,
			READOUT_COL, track, -1);
}



/* Used by remove_track() to remove track from model by calling
   gtk_tree_model_foreach ().
   Entry is deleted if data == track */
static gboolean tm_delete_track (GtkTreeModel *model,
				GtkTreePath *path,
				GtkTreeIter *iter,
				gpointer data)
{
  Track *track;

  gtk_tree_model_get (model, iter, READOUT_COL, &track, -1);
  if(track == (Track *)data)
  {
      GtkTreeSelection *selection = gtk_tree_view_get_selection
	  (track_treeview);
/*       printf("unselect...\n"); */
      gtk_tree_selection_unselect_iter (selection, iter);
/*       printf("...unselect done\n"); */
      gtk_list_store_remove (GTK_LIST_STORE (model), iter);
      return TRUE;
  }
  return FALSE;
}


/* Remove track from the display model */
void tm_remove_track (Track *track)
{
  GtkTreeModel *model = gtk_tree_view_get_model (track_treeview);
  if (model != NULL)
    gtk_tree_model_foreach (model, tm_delete_track, track);
}


/* Remove all tracks from the display model */
void tm_remove_all_tracks ()
{
  GtkTreeModel *model = gtk_tree_view_get_model (track_treeview);
  GtkTreeIter iter;

  while (gtk_tree_model_get_iter_first (model, &iter))
  {
      gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
  }
  tm_store_col_order ();
  tm_update_default_sizes ();
}

/* find out at which position column @tm_item is displayed */
/* static gint tm_get_col_position (TM_item tm_item) */
/* { */
/*     gint i; */
/*     GtkTreeViewColumn *col; */

/*     if (!track_treeview) return -1; */

/*     for (i=0; i<TM_NUM_COLUMNS_PREFS; ++i) */
/*     { */
/* 	col = gtk_tree_view_get_column (track_treeview, i); */
/* 	if (col->sort_column_id == tm_item) return i; */
/*     } */
/*     return -1; */
/* } */


/* store the order of the track view columns */
void tm_store_col_order (void)
{
    gint i;
    GtkTreeViewColumn *col;

    for (i=0; i<TM_NUM_COLUMNS; ++i)
    {
	col = gtk_tree_view_get_column (track_treeview, i);
	prefs_set_col_order (i, col->sort_column_id);
    }
}


/* Used by tm_track_changed() to find the track that
   changed name. If found, emit a "row changed" signal */
static gboolean tm_model_track_changed (GtkTreeModel *model,
				       GtkTreePath *path,
				       GtkTreeIter *iter,
				       gpointer data)
{
  Track *track;

  gtk_tree_model_get (model, iter, READOUT_COL, &track, -1);
  if(track == (Track *)data) {
    gtk_tree_model_row_changed (model, path, iter);
    return TRUE;
  }
  return FALSE;
}


/* One of the tracks has changed (this happens when the
   iTunesDB is read and some IDs are renumbered */
void tm_track_changed (Track *track)
{
  GtkTreeModel *model = gtk_tree_view_get_model (track_treeview);
  /*  printf("tm_track_changed enter\n");*/
  if (model != NULL)
    gtk_tree_model_foreach (model, tm_model_track_changed, track);
  /*  printf("tm_track_changed exit\n");*/
}



#if ((GTK_MAJOR_VERSION == 2) && (GTK_MINOR_VERSION < 2))
/* gtk_tree_selection_get_selected_rows() was introduced in 2.2 */
struct gtsgsr
{
    GtkTreeModel **model;
    GList        **list;
};

void  gtssf  (GtkTreeModel *model,
	      GtkTreePath *path,
	      GtkTreeIter *iter,
	      gpointer data)
{
    struct gtsgsr *gts = data;
    *gts->model = model;
    *gts->list = g_list_append (*gts->list, gtk_tree_path_copy (path));
}

GList *gtk_tree_selection_get_selected_rows (GtkTreeSelection *selection,
                                             GtkTreeModel     **model)
{
    struct gtsgsr gts;
    GList *list = NULL;

    gts.model = model;
    gts.list = &list;

    gtk_tree_selection_selected_foreach (selection, gtssf, &gts);
    return list;
}
#endif


/* Returns a string to be displayed in track_nr/tracks column */
/* You must not free or modify the returned string */
static const gchar *display_get_track_string (Track *track)
{
    static gchar str[20];

    /* Erase string */
    str[0] = 0;
    if (track)
    {
	if (track->tracks == 0)
	    snprintf (str, 20, "%d", track->track_nr);
	else
	    snprintf (str, 20, "%d/%d", track->track_nr, track->tracks);
    }
    return str;
}


/* Returns a string to be displayed in disk_nr/disks column */
/* You must not free or modify the returned string */
static const gchar *display_get_disk_string (Track *track)
{
    static gchar str[20];

    /* Erase string */
    str[0] = 0;
    if (track)
    {
	if (track->cds == 0)
	    snprintf (str, 20, "%d", track->cd_nr);
	else
	    snprintf (str, 20, "%d/%d", track->cd_nr, track->cds);
    }
    return str;
}


/* Called when editable cell is being edited. Stores new data to the
   track list. ID3 tags in the corresponding files are updated as
   well, if activated in the pref settings */
static void
tm_cell_edited (GtkCellRendererText *renderer,
		const gchar         *path_string,
		const gchar         *new_text,
		gpointer             data)
{
  GtkTreeModel *model;
  GtkTreeSelection *selection;
  TM_item column;
  gboolean multi_edit;
  gint sel_rows_num;
  GList *row_list, *row_node, *first;
  time_t t;


  column = (TM_item) g_object_get_data(G_OBJECT(renderer), "column");
  multi_edit = prefs_get_multi_edit ();
  if (column == TM_COLUMN_TITLE)
      multi_edit &= prefs_get_multi_edit_title ();
  selection = gtk_tree_view_get_selection(track_treeview);
  row_list = gtk_tree_selection_get_selected_rows(selection, &model);

/*   printf("tm_cell_edited: column: %d\n", column); */

  sel_rows_num = g_list_length (row_list);

  /* block widgets and update display if multi-edit is active */
  if (multi_edit && (sel_rows_num > 1)) block_widgets ();

  first = g_list_first (row_list);

  for (row_node = first;
       row_node && (multi_edit || (row_node == first));
       row_node = g_list_next(row_node))
  {
     Track *track;
     ExtraTrackData *etr;
     gboolean changed;
     GtkTreeIter iter;
     gint32 nr;
     gchar **itemp_utf8, *str;

     gtk_tree_model_get_iter(model, &iter, (GtkTreePath *) row_node->data);
     gtk_tree_model_get(model, &iter, READOUT_COL, &track, -1);
     g_return_if_fail (track);
     etr = track->userdata;
     g_return_if_fail (etr);


     changed = FALSE;

     switch(column)
     {
     case TM_COLUMN_TITLE:
     case TM_COLUMN_ALBUM:
     case TM_COLUMN_ARTIST:
     case TM_COLUMN_GENRE:
     case TM_COLUMN_COMPOSER:
     case TM_COLUMN_FDESC:
     case TM_COLUMN_GROUPING:
        itemp_utf8 = track_get_item_pointer (track, TM_to_T (column));
        if (g_utf8_collate (*itemp_utf8, new_text) != 0)
        {
           g_free (*itemp_utf8);
           *itemp_utf8 = g_strdup (new_text);
           changed = TRUE;
        }
        break;
     case TM_COLUMN_TRACK_NR:
        nr = atoi (new_text);
        if ((nr >= 0) && (nr != track->track_nr))
        {
           track->track_nr = nr;
           changed = TRUE;
        }
	str = strrchr (new_text, '/');
	if (str)
	{
	    nr = atoi (str+1);
	    if ((nr >= 0) && (nr != track->tracks))
	    {
		track->tracks = nr;
		changed = TRUE;
	    }
	}
	g_object_set (G_OBJECT (renderer),
		      "text", display_get_track_string (track), NULL);
        break;
     case TM_COLUMN_CD_NR:
        nr = atoi (new_text);
        if ((nr >= 0) && (nr != track->cd_nr))
        {
           track->cd_nr = nr;
           changed = TRUE;
        }
	str = strrchr (new_text, '/');
	if (str)
	{
	    nr = atoi (str+1);
	    if ((nr >= 0) && (nr != track->cds))
	    {
		track->cds = nr;
		changed = TRUE;
	    }
	}
	g_object_set (G_OBJECT (renderer),
		      "text", display_get_disk_string (track), NULL);
        break;
     case TM_COLUMN_YEAR:
        nr = atoi (new_text);
        if ((nr >= 0) && (nr != track->year))
        {
	   g_free (etr->year_str);
	   etr->year_str = g_strdup_printf ("%d", nr);
           track->year = nr;
           changed = TRUE;
        }
        break;
     case TM_COLUMN_PLAYCOUNT:
        nr = atoi (new_text);
        if ((nr >= 0) && (nr != track->playcount))
        {
           track->playcount = nr;
           changed = TRUE;
        }
        break;
     case TM_COLUMN_RATING:
        nr = atoi (new_text);
        if ((nr >= 0) && (nr <= 5) && (nr != track->rating))
        {
           track->rating = nr*RATING_STEP;
           changed = TRUE;
        }
        break;
     case TM_COLUMN_TIME_CREATED:
     case TM_COLUMN_TIME_PLAYED:
     case TM_COLUMN_TIME_MODIFIED:
	 t = time_string_to_time (new_text);
	 if ((t != -1) && (t != time_get_time (track, column)))
	 {
	     time_set_time (track, t, column);
	     changed = TRUE;
	 }
	 break;
     case TM_COLUMN_VOLUME:
        nr = atoi (new_text);
        if (nr != track->volume)
        {
	    track->volume = nr;
	    changed = TRUE;
        }
        break;
     case TM_COLUMN_SOUNDCHECK:
	nr = replaygain_to_soundcheck (atof (new_text));
/* 	printf("%d : %f\n", nr, atof (new_text)); */
        if (nr != track->soundcheck)
        {
	    track->soundcheck = nr;
	    changed = TRUE;
        }
        break;
     case TM_COLUMN_BITRATE:
        nr = atoi (new_text);
        if (nr != track->bitrate)
        {
	    track->bitrate = nr;
	    changed = TRUE;
        }
        break;
     case TM_COLUMN_SAMPLERATE:
        nr = atoi (new_text);
        if (nr != track->samplerate)
        {
	    track->samplerate = nr;
	    changed = TRUE;
        }
        break;
     case TM_COLUMN_BPM:
        nr = atoi (new_text);
        if (nr != track->BPM)
        {
	    track->BPM = nr;
	    changed = TRUE;
        }
        break;
     default:
        g_warning ("Programming error: tm_cell_edited: unknown track cell (%d) edited\n", column);
        break;
     }
/*      printf ("  changed: %d\n", changed); */
     if (changed)
     {
        track->time_modified = itdb_time_get_mac_time ();
        pm_track_changed (track);    /* notify playlist model... */
        data_changed (track->itdb); /* indicate that data has changed */

        if (prefs_get_id3_write())
        {
	    /* T_item tag_id;*/
           /* should we update all ID3 tags or just the one
              changed? -- obsoleted in 0.71*/
/*           if (prefs_get_id3_writeall ()) tag_id = T_ALL;
	     else                           tag_id = TM_to_T (column);*/
           write_tags_to_file (track);
           /* display possible duplicates that have been removed */
           gp_duplicate_remove (NULL, NULL);
        }
     }
     while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
  }

  if (multi_edit && (sel_rows_num > 1)) release_widgets ();

  g_list_foreach(row_list, (GFunc) gtk_tree_path_free, NULL);
  g_list_free(row_list);
}


/* The track data is stored in a separate list (static GList *tracks)
   and only pointers to the corresponding Track structure are placed
   into the model.
   This function reads the data for the given cell from the list and
   passes it to the renderer. */
static void tm_cell_data_func (GtkTreeViewColumn *tree_column,
			       GtkCellRenderer   *renderer,
			       GtkTreeModel      *model,
			       GtkTreeIter       *iter,
			       gpointer           data)
{
  Track *track;
  ExtraTrackData *etr;
  TM_item column;
  gchar text[21];
  gchar *buf = NULL;
  gchar *item_utf8 = NULL;

  column = (TM_item)g_object_get_data (G_OBJECT (renderer), "column");
  gtk_tree_model_get (model, iter, READOUT_COL, &track, -1);
  g_return_if_fail (track);
  etr = track->userdata;
  g_return_if_fail (etr);

  switch (column)
  {
  case TM_COLUMN_TITLE:
  case TM_COLUMN_ARTIST:
  case TM_COLUMN_ALBUM:
  case TM_COLUMN_GENRE:
  case TM_COLUMN_COMPOSER:
  case TM_COLUMN_FDESC:
  case TM_COLUMN_GROUPING:
      item_utf8 = track_get_item (track, TM_to_T (column));
      g_object_set (G_OBJECT (renderer),
		    "text", item_utf8,
		    "editable", TRUE, NULL);
      break;
  case TM_COLUMN_TRACK_NR:
      g_object_set (G_OBJECT (renderer),
		    "text", display_get_track_string (track),
		    "editable", TRUE,
		    "xalign", 1.0, NULL);
      break;
  case TM_COLUMN_CD_NR:
      g_object_set (G_OBJECT (renderer),
		    "text", display_get_disk_string (track),
		    "editable", TRUE,
		    "xalign", 1.0, NULL);
      break;
  case TM_COLUMN_IPOD_ID:
      if (track->id != -1)
      {
	  snprintf (text, 20, "%d", track->id);
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
  case TM_COLUMN_PC_PATH:
      g_object_set (G_OBJECT (renderer), "text", etr->pc_path_utf8, NULL);
      break;
  case TM_COLUMN_IPOD_PATH:
      g_object_set (G_OBJECT (renderer), "text", track->ipod_path, NULL);
      break;
  case TM_COLUMN_TRANSFERRED:
      g_object_set (G_OBJECT (renderer), "active", track->transferred, NULL);
      break;
  case TM_COLUMN_COMPILATION:
      g_object_set (G_OBJECT (renderer),
		    "active", !track->compilation,
		    "activatable", TRUE, NULL);
      break;
  case TM_COLUMN_SIZE:
      snprintf (text, 20, "%d", track->size);
      g_object_set (G_OBJECT (renderer),
		    "text", text,
		    "xalign", 1.0, NULL);
      break;
  case TM_COLUMN_TRACKLEN:
      snprintf (text, 20, "%d:%02d", track->tracklen/60000,
                                     (track->tracklen/1000)%60);
      g_object_set (G_OBJECT (renderer),
		    "text", text,
		    "xalign", 1.0, NULL);
      break;
  case TM_COLUMN_BITRATE:
      snprintf (text, 20, "%dk", track->bitrate);
      g_object_set (G_OBJECT (renderer),
		    "text", text,
		    "editable", TRUE,
		    "xalign", 1.0, NULL);
      break;
  case TM_COLUMN_SAMPLERATE:
      snprintf (text, 20, "%d", track->samplerate);
      g_object_set (G_OBJECT (renderer),
		    "text", text,
		    "editable", TRUE,
		    "xalign", 1.0, NULL);
      break;
  case TM_COLUMN_BPM:
      snprintf (text, 20, "%d", track->BPM);
      g_object_set (G_OBJECT (renderer),
		    "text", text,
		    "editable", TRUE,
		    "xalign", 1.0, NULL);
      break;
  case TM_COLUMN_PLAYCOUNT:
      snprintf (text, 20, "%d", track->playcount);
      g_object_set (G_OBJECT (renderer),
		    "text", text,
		    "editable", TRUE,
		    "xalign", 1.0, NULL);
      break;
  case TM_COLUMN_YEAR:
      snprintf (text, 20, "%d", track->year);
      g_object_set (G_OBJECT (renderer),
		    "text", text,
		    "editable", TRUE,
		    "xalign", 1.0, NULL);
      break;
  case TM_COLUMN_RATING:
      snprintf (text, 20, "%d", track->rating/RATING_STEP);
      g_object_set (G_OBJECT (renderer),
		    "text", text,
		    "editable", TRUE,
		    "xalign", 1.0, NULL);
      break;
  case TM_COLUMN_TIME_PLAYED:
  case TM_COLUMN_TIME_MODIFIED:
  case TM_COLUMN_TIME_CREATED:
      buf = time_field_to_string (track, column);
      g_object_set (G_OBJECT (renderer),
		    "text", buf,
 		    "editable", TRUE,
		    "xalign", 0.0, NULL);
      C_FREE (buf);
      break;
  case TM_COLUMN_VOLUME:
      snprintf (text, 20, "%d", track->volume);
      g_object_set (G_OBJECT (renderer),
		    "text", text,
		    "editable", TRUE,
		    "xalign", 1.0, NULL);
      break;
  case TM_COLUMN_SOUNDCHECK:
/*       printf ("%p:%f : %d\n", track, */
/* 	      soundcheck_to_replaygain (track->soundcheck), */
/* 	      track->soundcheck); */
      snprintf (text, 20, "%0.2f",
		soundcheck_to_replaygain (track->soundcheck));
      g_object_set (G_OBJECT (renderer),
		    "text", text,
		    "editable", TRUE,
		    "xalign", 1.0, NULL);
      break;
  default:
      g_warning ("Programming error: unknown column in tm_cell_data_func: %d\n", column);
      break;
  }
}



/* Called when a toggle cell is being changed. Stores new data to the
   track list. */
static void
tm_cell_toggled (GtkCellRendererToggle *renderer,
		 gchar *arg1,
		 gpointer user_data)
{
  GtkTreeModel *model;
  GtkTreeSelection *selection;
  TM_item column;
  gboolean multi_edit;
  gint sel_rows_num;
  GList *row_list, *row_node, *first;
  gboolean active;

  column = (TM_item) g_object_get_data(G_OBJECT(renderer), "column");
  multi_edit = prefs_get_multi_edit ();
  selection = gtk_tree_view_get_selection(track_treeview);
  row_list = gtk_tree_selection_get_selected_rows(selection, &model);

/*  printf("tm_cell_toggled: column: %d, arg1: %p\n", column, arg1); */

  sel_rows_num = g_list_length (row_list);

  /* block widgets and update display if multi-edit is active */
  if (multi_edit && (sel_rows_num > 1)) block_widgets ();

  first = g_list_first (row_list);


  g_object_get (G_OBJECT (renderer), "active", &active, NULL);

  for (row_node = first;
       row_node && (multi_edit || (row_node == first));
       row_node = g_list_next(row_node))
  {
     Track *track;
     gboolean changed;
     GtkTreeIter iter;

     gtk_tree_model_get_iter(model, &iter, (GtkTreePath *) row_node->data);
     gtk_tree_model_get(model, &iter, READOUT_COL, &track, -1);
     changed = FALSE;

     switch(column)
     {
     case TM_COLUMN_TITLE:
	 if ((active && (track->checked == 0)) ||
	     (!active && (track->checked == 1)))
	     changed = TRUE;
	 if (active) track->checked = 1;
	 else        track->checked = 0;
	 break;
     case TM_COLUMN_COMPILATION:
	 if ((active && (track->compilation == 0)) ||
	     (!active && (track->compilation == 1)))
	     changed = TRUE;
	 if (active) track->compilation = 1;
	 else        track->compilation = 0;
        break;
     default:
        g_warning ("Programming error: tm_cell_toggled: unknown track cell (%d) edited\n", column);
        break;
     }
/*      printf ("  changed: %d\n", changed); */
     if (changed)
     {
        track->time_modified = itdb_time_get_mac_time ();
/*        pm_track_changed (track);  notify playlist model... -- not
 *        necessary here because only the track model is affected */
        data_changed (track->itdb);  /* indicate that data has changed */
     }
     while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
  }

  if (multi_edit && (sel_rows_num > 1)) release_widgets ();

  g_list_foreach(row_list, (GFunc) gtk_tree_path_free, NULL);
  g_list_free(row_list);
}



/* The track data is stored in a separate list (static GList *tracks)
   and only pointers to the corresponding Track structure are placed
   into the model.
   This function reads the data for the given cell from the list and
   passes it to the renderer. */
static void tm_cell_data_func_toggle (GtkTreeViewColumn *tree_column,
				      GtkCellRenderer   *renderer,
				      GtkTreeModel      *model,
				      GtkTreeIter       *iter,
				      gpointer           data)
{
  Track *track;
  TM_item column;

  column = (TM_item)g_object_get_data (G_OBJECT (renderer), "column");
  gtk_tree_model_get (model, iter, READOUT_COL, &track, -1);

  switch (column)
  {
  case TM_COLUMN_TITLE:
      g_object_set (G_OBJECT (renderer),
		    "active", !track->checked,
		    "activatable", TRUE, NULL);
      break;
  default:
      g_warning ("Programming error: unknown column in tm_cell_data_func_toggle: %d\n", column);
      break;
  }
}

/**
 * tm_get_nr_of_tracks - get the number of tracks displayed
 * currently in the track model Returns - the number of tracks displayed
 * currently
 */
guint
tm_get_nr_of_tracks(void)
{
    GtkTreeIter i;
    guint result = 0;
    gboolean valid = FALSE;
    GtkTreeModel *tm = NULL;

    if((tm = gtk_tree_view_get_model(GTK_TREE_VIEW(track_treeview))))
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
 * Reorder tracks in playlist to match order of tracks displayed in track
 * view. Only the subset of tracks currently displayed is reordered.
 * data_changed() is called when necessary.
 */
void
tm_rows_reordered (void)
{
    Playlist *current_pl;

    g_return_if_fail (track_treeview);
    current_pl = pm_get_selected_playlist ();

    if(current_pl)
    {
	GtkTreeModel *tm = NULL;
	GtkTreeIter i;
	GList *new_list = NULL, *old_pos_l = NULL;
	gboolean valid = FALSE;
	GList *nlp, *olp;
	gboolean changed = FALSE;
	iTunesDB *itdb = NULL;

	tm = gtk_tree_view_get_model (track_treeview);
	g_return_if_fail (tm);

	valid = gtk_tree_model_get_iter_first (tm,&i);
	while (valid)
	{
	    Track *new_track;
	    gint old_position;

	    gtk_tree_model_get (tm, &i, READOUT_COL, &new_track, -1);
	    g_return_if_fail (new_track);

	    if (!itdb) itdb = new_track->itdb;
	    new_list = g_list_append (new_list, new_track);
	    /* what position was this track in before? */
	    old_position = g_list_index (current_pl->members, new_track);
	    /* check if we already used this position before (can
	       happen if track has been added to playlist more than
	       once */
	    while ((old_position != -1) &&
		   g_list_find (old_pos_l, (gpointer)old_position))
	    {  /* find next occurence */
		GList *link;
		gint next;
		link = g_list_nth (current_pl->members, old_position + 1);
		next = g_list_index (link, new_track);
		if (next == -1)   old_position = -1;
		else              old_position += (1+next);
	    }
	    /* we make a sorted list of the old positions */
	    old_pos_l = g_list_insert_sorted (old_pos_l,
					      (gpointer)old_position,
					      comp_int);
	    valid = gtk_tree_model_iter_next (tm, &i);
	}
	nlp = new_list;
	olp = old_pos_l;
	while (nlp && olp)
	{
	    GList *old_link;
	    gint position = (gint)olp->data;

	    /* if position == -1 one of the tracks in the track view
	       could not be found in the selected playlist -> stop! */
	    if (position == -1)
	    {
		g_warning ("Programming error: tm_rows_reordered_callback: track in track view was not in selected playlist\n");
		g_return_if_reached ();
	    }
	    old_link = g_list_nth (current_pl->members, position);
	    /* replace old track with new track */
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
	/* if we changed data, mark data as changed and adopt order in
	   sort tabs */
	if (changed)
	{
	    data_changed (itdb);
	    st_adopt_order_in_playlist ();
	}
    }
}


static void
on_trackids_list_foreach ( GtkTreeModel *tm, GtkTreePath *tp,
			   GtkTreeIter *i, gpointer data)
{
    Track *tr = NULL;
    GList *l = *((GList**)data);
    gtk_tree_model_get(tm, i, READOUT_COL, &tr, -1);
    g_return_if_fail (tr);
    l = g_list_append(l, (gpointer)tr->id);
    *((GList**)data) = l;
}


/* return a list containing the track IDs of all tracks currently being
   selected */
GList *
tm_get_selected_trackids(void)
{
    GList *result = NULL;
    GtkTreeSelection *ts = NULL;

    if((ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(track_treeview))))
    {
	gtk_tree_selection_selected_foreach(ts, on_trackids_list_foreach,
					    &result);
    }
    return(result);
}

/* return a list containing the track IDs of all tracks currently being
   displayed */
GList *
tm_get_all_trackids(void)
{
    static gboolean
	on_all_trackids_list_foreach (GtkTreeModel *tm, GtkTreePath *tp,
				      GtkTreeIter *i, gpointer data)
	{
	    on_trackids_list_foreach (tm, tp, i, data);
	    return FALSE;
	}
    GList *result = NULL;
    GtkTreeModel *model;

    if((model = gtk_tree_view_get_model (track_treeview)))
    {
	gtk_tree_model_foreach(model, on_all_trackids_list_foreach,
			       &result);
    }
    return(result);
}

static void
on_tracks_list_foreach ( GtkTreeModel *tm, GtkTreePath *tp,
			 GtkTreeIter *i, gpointer data)
{
    Track *tr = NULL;
    GList *l = *((GList**)data);
    gtk_tree_model_get(tm, i, READOUT_COL, &tr, -1);
    g_return_if_fail (tr);
    l = g_list_append(l, tr);
    *((GList**)data) = l;
}


/* return a list containing pointers to all tracks currently being
   selected */
GList *
tm_get_selected_tracks(void)
{
    GList *result = NULL;
    GtkTreeSelection *ts = NULL;

    if((ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(track_treeview))))
    {
	gtk_tree_selection_selected_foreach(ts, on_tracks_list_foreach,
					    &result);
    }
    return(result);
}


static gboolean
on_all_tracks_list_foreach (GtkTreeModel *tm, GtkTreePath *tp,
			   GtkTreeIter *i, gpointer data)
{
    on_tracks_list_foreach (tm, tp, i, data);
    return FALSE;
}

/* return a list containing pointers to all tracks currently being
   displayed */
GList *
tm_get_all_tracks(void)
{
    GList *result = NULL;
    GtkTreeModel *model;

    if((model = gtk_tree_view_get_model (track_treeview)))
    {
	gtk_tree_model_foreach(model, on_all_tracks_list_foreach,
			       &result);
    }
    return(result);
}


/* Stop editing. If @cancel is TRUE, the edited value will be
   discarded (I have the feeling that the "discarding" part does not
   work quite the way intended). */
void tm_stop_editing (gboolean cancel)
{
    GtkTreeViewColumn *col;

    if (!track_treeview)  return;

    gtk_tree_view_get_cursor (track_treeview, NULL, &col);
    if (col)
    {
	/* Before removing the widget we set multi_edit to FALSE. That
	   way at most one entry will be changed (this also doesn't
	   seem to work the way intended) */
	gboolean me = prefs_get_multi_edit ();
	prefs_set_multi_edit (FALSE);
	if (!cancel && col->editable_widget)
	    gtk_cell_editable_editing_done (col->editable_widget);
	if (col->editable_widget)
	    gtk_cell_editable_remove_widget (col->editable_widget);
	prefs_set_multi_edit (me);
    }
}



/* Function to compare @tm_item of @track1 and @track2. Used by
   tm_data_compare_func() */
static gint tm_data_compare (Track *track1, Track *track2,
			     TM_item tm_item)
{
  gint cmp;
  ExtraTrackData *etr1, *etr2;

  switch (tm_item)
  {
  case TM_COLUMN_TITLE:
  case TM_COLUMN_ALBUM:
  case TM_COLUMN_GENRE:
  case TM_COLUMN_COMPOSER:
  case TM_COLUMN_FDESC:
  case TM_COLUMN_GROUPING:
      return compare_string (track_get_item (track1, TM_to_T (tm_item)),
			     track_get_item (track2, TM_to_T (tm_item)));
  case TM_COLUMN_ARTIST:
      return compare_string_fuzzy (track_get_item (track1, TM_to_T (tm_item)),
			     track_get_item (track2, TM_to_T (tm_item)));
  case TM_COLUMN_TRACK_NR:
      cmp = track1->tracks - track2->tracks;
      if (cmp == 0) cmp = track1->track_nr - track2->track_nr;
      return cmp;
  case TM_COLUMN_CD_NR:
      cmp = track1->cds - track2->cds;
      if (cmp == 0) cmp = track1->cd_nr - track2->cd_nr;
      return cmp;
  case TM_COLUMN_IPOD_ID:
      return track1->id - track2->id;
  case TM_COLUMN_PC_PATH:
      g_return_val_if_fail (track1 && track2, 0);
      etr1 = track1->userdata;
      etr2 = track2->userdata;
      g_return_val_if_fail (etr1 && etr2, 0);
      return g_utf8_collate (etr1->pc_path_utf8, etr2->pc_path_utf8);
  case TM_COLUMN_IPOD_PATH:
      return g_utf8_collate (track1->ipod_path, track2->ipod_path);
  case TM_COLUMN_TRANSFERRED:
      if(track1->transferred == track2->transferred) return 0;
      if(track1->transferred == TRUE) return 1;
      else return -1;
  case TM_COLUMN_COMPILATION:
      if(track1->compilation == track2->compilation) return 0;
      if(track1->compilation == TRUE) return 1;
      else return -1;
  case TM_COLUMN_SIZE:
      return track1->size - track2->size;
  case TM_COLUMN_TRACKLEN:
      return track1->tracklen - track2->tracklen;
  case TM_COLUMN_BITRATE:
      return track1->bitrate - track2->bitrate;
  case TM_COLUMN_SAMPLERATE:
      return track1->samplerate - track2->samplerate;
  case TM_COLUMN_BPM:
      return track1->BPM - track2->BPM;
  case TM_COLUMN_PLAYCOUNT:
      return track1->playcount - track2->playcount;
  case  TM_COLUMN_RATING:
      return track1->rating - track2->rating;
  case TM_COLUMN_TIME_CREATED:
  case TM_COLUMN_TIME_PLAYED:
  case TM_COLUMN_TIME_MODIFIED:
      return COMP (time_get_time (track1, tm_item),
		   time_get_time (track2, tm_item));
  case  TM_COLUMN_VOLUME:
      return track1->volume - track2->volume;
  case  TM_COLUMN_SOUNDCHECK:
      /* If soundcheck is unset (0) use 0 dB (1000) */
      return (track1->soundcheck? track1->soundcheck:1000) - 
	  (track2->soundcheck? track2->soundcheck:1000);
  case TM_COLUMN_YEAR:
      return track1->year - track2->year;
  default:
      g_warning ("Programming error: tm_data_compare_func: no sort method for tm_item %d\n", tm_item);
      break;
  }
  return 0;
}


/* Function used to compare two cells during sorting (track view) */
gint tm_data_compare_func (GtkTreeModel *model,
			GtkTreeIter *a,
			GtkTreeIter *b,
			gpointer user_data)
{
  Track *track1;
  Track *track2;
  gint column;
  GtkSortType order;
  gint result;

  gtk_tree_model_get (model, a, READOUT_COL, &track1, -1);
  gtk_tree_model_get (model, b, READOUT_COL, &track2, -1);
  if(gtk_tree_sortable_get_sort_column_id (GTK_TREE_SORTABLE (model),
					   &column, &order) == FALSE)
      return 0;

  result = tm_data_compare (track1, track2, column);
  return result;
}


/* set/read the counter used to remember how often the sort column has
   been clicked.
   @inc: negative: reset counter to 0
   @inc: positive or zero : add to counter
   return value: new value of the counter */
static gint tm_sort_counter (gint inc)
{
    static gint cnt = 0;

    if (inc <0)
    {
	cnt = 0;
    }
    else
    {
	cnt += inc;
    }   
    return cnt;
}


/* Redisplays the tracks in the track view according to the order
 * stored in the sort tab view. This only works if the track view is
 * not sorted --> skip if sorted */

void tm_adopt_order_in_sorttab (void)
{
    if (prefs_get_tm_sort () == SORT_NONE)
    {
	GList *gl, *tracks = NULL;

	/* retrieve the currently displayed tracks (non ordered) from
	   the last sort tab or from the selected playlist if no sort
	   tabs are being used */
	tm_remove_all_tracks ();
	tracks = display_get_selected_members (prefs_get_sort_tab_num()-1);
	for (gl=tracks; gl; gl=gl->next)
	    tm_add_track_to_track_model ((Track *)gl->data, NULL);
    }
}


/* redisplay the contents of the track view in it's unsorted order */
static void tm_unsort (void)
{
    if (track_treeview)
    {
	GtkTreeModel *model= gtk_tree_view_get_model (track_treeview);

	prefs_set_tm_sort (SORT_NONE);
	if (!BROKEN_GTK_TREE_SORT)
	{
	    gtk_tree_sortable_set_sort_column_id
		(GTK_TREE_SORTABLE (model),
		 GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID,
		 GTK_SORT_ASCENDING);
	    tm_adopt_order_in_sorttab ();
	}
	else
	{
	    gtkpod_warning (_("Cannot unsort track view because of a bug in the GTK lib you are using (%d.%d.%d < 2.5.4). Once you sort the track view, you cannot go back to the unsorted state.\n\n"), gtk_major_version, gtk_minor_version, gtk_micro_version);
	}
	tm_sort_counter (-1);
    }
}


/* when clicking the same table header three times, the sorting is
   aborted */
static void
tm_track_column_button_clicked(GtkTreeViewColumn *tvc, gpointer data)
{
    static gint lastcol = -1; /* which column was sorted last time? */
    gint newcol = gtk_tree_view_column_get_sort_column_id (tvc);

    if (newcol != lastcol)
    {
	tm_sort_counter (-1);
	lastcol = newcol;
    }

    if (tm_sort_counter (1) >= 3)
    { /* after clicking three times, reset sort order! */
	tm_unsort ();  /* also resets sort counter */
    }
    else
    {
	prefs_set_tm_sort (gtk_tree_view_column_get_sort_order (tvc));
    }
    prefs_set_tm_sortcol (newcol);
    if(prefs_get_tm_autostore ())  tm_rows_reordered ();
/*     sort_window_update (); */
}


void tm_sort (TM_item col, GtkSortType order)
{
    if (track_treeview)
    {
	GtkTreeModel *model= gtk_tree_view_get_model (track_treeview);
	if (order != SORT_NONE)
	{
	    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model),
						  col, order);
	}
	else
	{ /* only unsort if treeview is sorted */
	    gint column;
	    GtkSortType order;
	    if (gtk_tree_sortable_get_sort_column_id
		(GTK_TREE_SORTABLE (model), &column, &order))
	    {
		/* column == -2 actually is not defined, but it means
		   that the model is unsorted. The sortable interface
		   is badly implemented in gtk 2.4 */
		if (column != -2)
		    tm_unsort ();
	    }
	}
    }
}


/* Add one column at position @pos. This code is used over and over
   by tm_add_column() -- therefore I put it into a separate function */
static GtkTreeViewColumn *tm_add_text_column (TM_item col_id,
					      gchar *name,
					      GtkCellRenderer *renderer,
					      gboolean editable,
					      gint pos)
{
    GtkTreeViewColumn *column;
    GtkTreeModel *model = gtk_tree_view_get_model (track_treeview);

    if (!renderer)    renderer = gtk_cell_renderer_text_new ();
    if (editable)
    {
	g_signal_connect (G_OBJECT (renderer), "edited",
			  G_CALLBACK (tm_cell_edited), model);
    }
    g_object_set_data (G_OBJECT (renderer), "editable", (gint *)editable);
    g_object_set_data (G_OBJECT (renderer), "column", (gint *)col_id);
    column = gtk_tree_view_column_new ();
    gtk_tree_view_column_set_title (column, name);
    gtk_tree_view_column_pack_end (column, renderer, FALSE);
    gtk_tree_view_column_set_cell_data_func (column, renderer,
					     tm_cell_data_func, NULL, NULL);
    gtk_tree_view_column_set_sort_column_id (column, col_id);
    gtk_tree_view_column_set_resizable (column, TRUE);
/*     gtk_tree_view_column_set_clickable(column, TRUE); */
    gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width (column,
					  prefs_get_tm_col_width (col_id));
    g_signal_connect (G_OBJECT (column), "clicked",
		      G_CALLBACK (tm_track_column_button_clicked),
		      (gpointer)col_id);
    gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model), col_id,
				     tm_data_compare_func, NULL, NULL);
    gtk_tree_view_column_set_reorderable (column, TRUE);
    gtk_tree_view_insert_column (track_treeview, column, pos);
    tm_columns[col_id] = column;
    return column;
}



/* Adds the columns to our track_treeview */
static GtkTreeViewColumn *tm_add_column (TM_item tm_item, gint pos)
{
  GtkTreeModel *model = gtk_tree_view_get_model (track_treeview);
  GtkTreeViewColumn *col = NULL;
  gchar *text = NULL;
  gboolean editable = TRUE;          /* default */
  GtkCellRenderer *renderer = NULL;  /* default */
  GtkTooltips *tt = GTK_TOOLTIPS (lookup_widget (gtkpod_window,
						 "tooltips"));

  if ((tm_item) < 0 || (tm_item >= TM_NUM_COLUMNS))  return NULL;

  text = gettext (tm_col_strings[tm_item]);

  switch (tm_item)
  {
  case TM_COLUMN_TITLE:
  case TM_COLUMN_ARTIST:
  case TM_COLUMN_ALBUM:
  case TM_COLUMN_GENRE:
  case TM_COLUMN_COMPOSER:
  case TM_COLUMN_FDESC:
  case TM_COLUMN_GROUPING:
  case TM_COLUMN_RATING:
  case TM_COLUMN_BITRATE:
  case TM_COLUMN_SAMPLERATE:
  case TM_COLUMN_BPM:
      break;
  case TM_COLUMN_TRACK_NR:
      text = _("#");
      break;
  case TM_COLUMN_CD_NR:
      text = _("CD");
      break;
  case TM_COLUMN_IPOD_ID:
      text = _("ID");
      editable = FALSE;
      break;
  case TM_COLUMN_PC_PATH:
  case TM_COLUMN_IPOD_PATH:
      editable = FALSE;
      break;
  case TM_COLUMN_TRANSFERRED:
      text = _("Trnsfrd");
      editable = FALSE;
      renderer = gtk_cell_renderer_toggle_new ();
      break;
  case TM_COLUMN_COMPILATION:
      text = _("Cmpl");
      editable = FALSE;
      renderer = gtk_cell_renderer_toggle_new ();
      break;
  case TM_COLUMN_SIZE:
      editable = FALSE;
      break;
  case TM_COLUMN_TRACKLEN:
      text = _("Time");
      editable = FALSE;
      break;
  case TM_COLUMN_PLAYCOUNT:
      text = _("Plycnt");
      break;
  case TM_COLUMN_TIME_PLAYED:
      text = _("Played");
      break;
  case TM_COLUMN_TIME_MODIFIED:
      text = _("Modified");
      break;
  case TM_COLUMN_TIME_CREATED:
      text = _("Created");
      break;
  case TM_COLUMN_YEAR:
      text = _("Year");
      break;
  case TM_COLUMN_VOLUME:
      text = _("Vol.");
      break;
  case TM_COLUMN_SOUNDCHECK:
      text = _("Sndchk.");
      break;
  case TM_NUM_COLUMNS:
      break;
  }
  col = tm_add_text_column (tm_item, text, renderer, editable, pos);
  if (col && (tm_item == TM_COLUMN_TITLE))
  {
      renderer = gtk_cell_renderer_toggle_new ();
      g_object_set_data (G_OBJECT (renderer), "column",
			 (gint *)tm_item);
      g_signal_connect (G_OBJECT (renderer), "toggled",
			G_CALLBACK (tm_cell_toggled), model);
      gtk_tree_view_column_pack_start (col, renderer, FALSE);
      gtk_tree_view_column_set_cell_data_func (col, renderer, tm_cell_data_func_toggle, NULL, NULL);
  }
  if (col && (tm_item == TM_COLUMN_COMPILATION))
  {
      g_signal_connect (G_OBJECT (renderer), "toggled",
			G_CALLBACK (tm_cell_toggled), model);
  }      
  if (col && (pos != -1))
  {
      gtk_tree_view_column_set_visible (col,
					prefs_get_col_visible (tm_item));
  }
  if (tt && tm_col_tooltips[tm_item])
      gtk_tooltips_set_tip (tt, col->button, 
			    gettext (tm_col_tooltips[tm_item]),
			    NULL);
  return col;
}


/* Adds the columns to our track_treeview */
static void tm_add_columns (void)
{
    gint i;

    for (i=0; i<TM_NUM_COLUMNS; ++i)
    {
	tm_add_column (prefs_get_col_order (i), -1);
    }
    tm_show_preferred_columns();
}

static void tm_select_current_position (gint x, gint y)
{
    if (track_treeview)
    {
	GtkTreePath *path;

	gtk_tree_view_get_path_at_pos (track_treeview,
				       x, y, &path, NULL, NULL, NULL);
	if (path)
	{
	    GtkTreeSelection *ts = gtk_tree_view_get_selection (track_treeview);
	    gtk_tree_selection_select_path (ts, path);
	    gtk_tree_path_free (path);
	}
    }
}

static gboolean
tm_button_press_event(GtkWidget *w, GdkEventButton *e, gpointer data)
{
    if(w && e)
    {
	switch(e->button)
	{
	    case 3:
		tm_select_current_position (e->x, e->y);
		tm_context_menu_init ();
		return TRUE;
	    default:
		break;
	}

    }
    return(FALSE);
}

/* called when the track selection changes */
static void
tm_selection_changed_event(GtkTreeSelection *selection, gpointer data)
{
    info_update_track_view_selected ();
}

/* Create tracks treeview */
void tm_create_treeview (void)
{
  GtkTreeModel *model = NULL;
  GtkWidget *track_window = lookup_widget (gtkpod_window, "track_window");
  GtkTreeSelection *select;
  GtkWidget *stv = gtk_tree_view_new ();

  /* create tree view */
  if (track_treeview)
  {   /* delete old tree view */
      model = gtk_tree_view_get_model (track_treeview);
      /* FIXME: how to delete model? */
      gtk_widget_destroy (GTK_WIDGET (track_treeview));
  }
  track_treeview = GTK_TREE_VIEW (stv);
  gtk_widget_show (stv);
  gtk_container_add (GTK_CONTAINER (track_window), stv);
  /* create model (we only need one column for the model -- only a
   * pointer to the track has to be stored) */
  model = GTK_TREE_MODEL (
      gtk_list_store_new (1, G_TYPE_POINTER));
  gtk_tree_view_set_model (track_treeview, GTK_TREE_MODEL (model));
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (track_treeview), TRUE);
  select = gtk_tree_view_get_selection (track_treeview);
  gtk_tree_selection_set_mode (select,
			       GTK_SELECTION_MULTIPLE);
  g_signal_connect (G_OBJECT (select) , "changed",
		    G_CALLBACK (tm_selection_changed_event),
		    NULL);
  tm_add_columns ();
  gtk_drag_source_set (GTK_WIDGET (track_treeview), GDK_BUTTON1_MASK,
		       tm_drag_types, TGNR (tm_drag_types),
		       GDK_ACTION_COPY|GDK_ACTION_MOVE);
  gtk_tree_view_enable_model_drag_dest(track_treeview, tm_drop_types,
				       TGNR (tm_drop_types),
				       GDK_ACTION_COPY|GDK_ACTION_MOVE);
  /* need the gtk_drag_dest_set() with no actions ("0") so that the
     data_received callback gets the correct info value. This is most
     likely a bug... */
  gtk_drag_dest_set_target_list (GTK_WIDGET (track_treeview),
				 gtk_target_list_new (tm_drop_types,
						      TGNR (tm_drop_types)));

  g_signal_connect ((gpointer) stv, "drag_data_get",
		    G_CALLBACK (on_track_treeview_drag_data_get),
		    NULL);
  g_signal_connect ((gpointer) stv, "drag_data_received",
		    G_CALLBACK (on_track_treeview_drag_data_received),
		    NULL);
  g_signal_connect_after ((gpointer) stv, "key_release_event",
			  G_CALLBACK (on_track_treeview_key_release_event),
			  NULL);
  g_signal_connect ((gpointer) track_treeview, "button-press-event",
		    G_CALLBACK (tm_button_press_event),
		    NULL);
}


void
tm_show_preferred_columns(void)
{
    GtkTreeViewColumn *tvc = NULL;
    gboolean visible;
    gint i;

    for (i=0; i<TM_NUM_COLUMNS; ++i)
    {
	tvc = gtk_tree_view_get_column (track_treeview, i);
	visible = prefs_get_col_visible (prefs_get_col_order (i));
	gtk_tree_view_column_set_visible (tvc, visible);
    }
}


/* update the cfg structure (preferences) with the current sizes /
   positions (called by display_update_default_sizes():
   column widths of track model */
void tm_update_default_sizes (void)
{
    gint i;
    GtkTreeViewColumn *col;

    /* column widths */
    for (i=0; i<TM_NUM_COLUMNS; ++i)
    {
	col = tm_columns [i];
	if (col)
	{
	    prefs_set_tm_col_width (i,
				    gtk_tree_view_column_get_width (col));
	}
    }
}


/* Compare function to avoid sorting */
static gint tm_nosort_comp (GtkTreeModel *model,
			    GtkTreeIter *a,
			    GtkTreeIter *b,
			    gpointer user_data)
{
    return 0;
}


/* Disable sorting of the view during lengthy updates. */
/* @enable: TRUE: enable, FALSE: disable */
void tm_enable_disable_view_sort (gboolean enable)
{
    static gint disable_count = 0;

    if (enable)
    {
	disable_count--;
	if (disable_count < 0)
	    fprintf (stderr, "Programming error: disable_count < 0\n");
	if (disable_count == 0 && track_treeview)
	{
	    if ((prefs_get_tm_sort() != SORT_NONE) &&
		prefs_get_disable_sorting ())
	    {
		/* Re-enable sorting */
		GtkTreeModel *model = gtk_tree_view_get_model (track_treeview);
		if (BROKEN_GTK_TREE_SORT)
		{
		    gtk_tree_sortable_set_sort_func (
			GTK_TREE_SORTABLE (model),
			prefs_get_tm_sortcol (),
			tm_data_compare_func, NULL, NULL);
		}
		else
		{
		    gtk_tree_sortable_set_sort_column_id (
			GTK_TREE_SORTABLE (model),
			prefs_get_tm_sortcol (),
			prefs_get_tm_sort ());
		}
	    }
	}
    }
    else
    {
	if (disable_count == 0 && track_treeview)
	{
	    if ((prefs_get_tm_sort() != SORT_NONE) &&
		prefs_get_disable_sorting ())
	    {
		/* Disable sorting */
		GtkTreeModel *model = gtk_tree_view_get_model (track_treeview);
		if (BROKEN_GTK_TREE_SORT)
		{
		    gtk_tree_sortable_set_sort_func (
			GTK_TREE_SORTABLE (model),
			prefs_get_tm_sortcol (),
			tm_nosort_comp, NULL, NULL);
		}
		else
		{
		    gtk_tree_sortable_set_sort_column_id (
			GTK_TREE_SORTABLE (model),
			GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID,
			prefs_get_tm_sort ());
		}
	    }
	}
	disable_count++;
    }
}




/* ---------------------------------------------------------------- */
/* Section for drag and drop                                        */
/* ---------------------------------------------------------------- */

/*#if ((GTK_MAJOR_VERSION == 2) && (GTK_MINOR_VERSION < 2))*/
/* gtk_list_store_move_*() was introduced in 2.2, so we have to
 * emulate for 2.0 <= V < 2.2 (we require at least 2.0 anyway) */
/* !!! gtk_list_store_move() at least in 2.2.1 seems to be buggy: if
   moving before the first entry, the moved entry ends up
   last. Therefore I emulate the functions for all gtk versions */
static void tm_list_store_move (GtkListStore *store,
				 GtkTreeIter  *iter,
				 GtkTreeIter  *position,
				 gboolean     before)
{
    GtkTreeIter new_iter;
    Track *track = NULL;
    GtkTreeModel *model;

    /* insert new row before or after @position */
    if (before)  gtk_list_store_insert_before (store, &new_iter, position);
    else         gtk_list_store_insert_after (store, &new_iter, position);

    model = gtk_tree_view_get_model (track_treeview);

    /* get the content (track) of the row to move */
    gtk_tree_model_get (model, iter, READOUT_COL, &track, -1);
    /* remove the old row */
    gtk_list_store_remove (GTK_LIST_STORE (model), iter);

    /* set the content of the new row */
    gtk_list_store_set (GTK_LIST_STORE (model), &new_iter,
			READOUT_COL, track, -1);
}

void  tm_list_store_move_before (GtkListStore *store,
					 GtkTreeIter  *iter,
					 GtkTreeIter  *position)
{
    tm_list_store_move (store, iter, position, TRUE);
}

void  tm_list_store_move_after (GtkListStore *store,
					GtkTreeIter  *iter,
					GtkTreeIter  *position)
{
    tm_list_store_move (store, iter, position, FALSE);
}
/* #else*/
#if 0
/* starting V2.2 convenient gtk functions exist (but see comment above) */
void  tm_list_store_move_before (GtkListStore *store,
					 GtkTreeIter  *iter,
					 GtkTreeIter  *position)
{
    gtk_list_store_move_before (store, iter, position);
}

void  tm_list_store_move_after (GtkListStore *store,
					GtkTreeIter  *iter,
					GtkTreeIter  *position)
{
    gtk_list_store_move_after (store, iter, position);
}
#endif


/* move pathlist for track treeview */
gboolean tm_move_pathlist (gchar *data,
			   GtkTreePath *path,
			   GtkTreeViewDropPosition pos)
{
    return pmtm_move_pathlist (track_treeview, data, path, pos, TRACK_TREEVIEW);
}


/* Callback for adding tracks within tm_add_filelist */
void tm_addtrackfunc (Playlist *plitem, Track *track, gpointer data)
{
    struct asf_data *asf = (struct asf_data *)data;
    GtkTreeModel *model;
    GtkTreeIter new_iter;

    model = gtk_tree_view_get_model (track_treeview);

/*    printf("plitem: %p\n", plitem);
      if (plitem) printf("plitem->type: %d\n", plitem->type);*/
    /* add to playlist but not to the display */
    gp_playlist_add_track (plitem, track, FALSE);

    /* create new iter in track view */
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
    tm_add_track_to_track_model (track, &new_iter);
}


/* DND: insert a list of files before/after @path
   @data: list of files
   @path: where to drop
   @pos:  before/after...
*/
gboolean tm_add_filelist (gchar *data,
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

    model = gtk_tree_view_get_model (track_treeview);
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
    /* initialize add-track-struct */
    asf = g_malloc (sizeof (struct asf_data));
    asf->to_iter = &to_iter;
    asf->pos = pos;
    /* add the files to playlist -- but have tm_addtrackfunc() called
       for every added track */
    add_text_plain_to_playlist (current_playlist->itdb,
				current_playlist, use_data, 0,
				tm_addtrackfunc, asf);
    tm_rows_reordered ();
    g_free (asf);
    C_FREE (buf);
    return TRUE;
}


/*
 * utility function for appending ipod track ids for track view (DND)
 */
void
on_tm_dnd_get_track_foreach(GtkTreeModel *tm, GtkTreePath *tp,
			    GtkTreeIter *i, gpointer data)
{
    Track *tr;
    GString *tracklist = (GString *)data;

    g_return_if_fail (tracklist);

    gtk_tree_model_get(tm, i, READOUT_COL, &tr, -1);
    g_return_if_fail (tr);

    g_string_append_printf (tracklist, "%p\n", tr);
}


/*
 * utility function for appending path for track view or playlist view (DND)
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
 * utility function for appending file for track view (DND)
 */
void
on_tm_dnd_get_file_foreach(GtkTreeModel *tm, GtkTreePath *tp,
			   GtkTreeIter *iter, gpointer data)
{
    Track *track;
    GString *filelist = (GString *)data;
    gchar *name;

    gtk_tree_model_get(tm, iter, READOUT_COL, &track, -1);
    name = get_track_name_on_disk_verified (track);
    if (name)
    {
	g_string_append_printf (filelist, "file:%s\n", name);
	g_free (name);
    }
}
