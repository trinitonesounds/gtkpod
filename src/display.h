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

#ifndef __TREE_H__
#define __TREE_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include "song.h"
#include "playlist.h"

/* Number of search tabs to be supported.
   Note: the number of search tabs displayed on the screen
   is not changed automatically! */
#define SORT_TAB_NUM 2

/* Categories in each sort tab (page numbers) */
enum {
  ST_CAT_ARTIST = 0,
  ST_CAT_ALBUM,
  ST_CAT_GENRE,
  ST_CAT_TITLE,
  ST_CAT_NUM
};

/* struct for each entry in sort tab */
typedef struct {
  gchar *name;
  gboolean master; /* set if this is the "All" entry */
  GList *members;
} TabEntry;

/* struct with data corresponding to each sort tab */
typedef struct {
  guint current_category;            /* current page (category) selected) */
  GtkTreeModel *model;               /* pointer to model used */
  GtkNotebook *notebook;             /* pointer to notebook used */
  GtkTreeView *treeview[ST_CAT_NUM]; /* pointer to treeviews used */
  GList *entries;                    /* list with entries */
  TabEntry *current_entry;           /* pointer to currently selected entry */
  gchar *lastselection[ST_CAT_NUM];  /* name of entry last selected */
} SortTab;

/* "Column numbers" in sort tab model */
enum  {
  ST_COLUMN_ENTRY = 0,
  ST_NUM_COLUMNS
};

/* Column numbers in song model */
enum  {
  SM_COLUMN_ALBUM = 0,
  SM_COLUMN_ARTIST,
  SM_COLUMN_TITLE,
  SM_COLUMN_GENRE,
  SM_COLUMN_PC_PATH,
  SM_COLUMN_IPOD_ID,
  /*  SM_COLUMN_TRANSFERRED,*/
  SM_NUM_COLUMNS
};

/* "Column numbers" in playlist model */
enum  {
  PM_COLUMN_PLAYLIST = 0,
  PM_NUM_COLUMNS
};


void create_listviews (GtkWidget *gtkpod);
void destroy_listview (void);

void pm_remove_playlist (Playlist *playlist);
void pm_add_playlist (Playlist *playlist);
void pm_remove_song (Playlist *playlist, Song *song);
void pm_add_song (Playlist *playlist, Song *song);
void pm_name_changed (Playlist *playlist);
void pm_song_changed (Song *song);

void st_page_selected (GtkNotebook *notebook, guint page);
#endif __TREE_H__
