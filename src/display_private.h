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

#ifndef __DISPLAY_PRIVATE_H__
#define __DISPLAY_PRIVATE_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "display.h"

/* used for stopping of display refresh */
typedef void (*br_callback)(gpointer user_data1, gpointer user_data2);
void block_selection (gint inst);
void release_selection (gint inst);
void add_selection_callback (gint inst, br_callback brc, gpointer user_data1, gpointer user_data2);
gboolean selection_callback_timeout (gpointer data);
extern gint stop_add;

/* used for display organization */
void pm_create_treeview (void);
void pm_set_selected_playlist(Playlist *pl);
void pm_remove_all_playlists (gboolean clear_sort);
void sm_create_treeview (void);
void sm_song_changed (Song *song);
void sm_remove_song (Song *song);
void sm_remove_all_songs (gboolean clear_sort);
void st_remove_all_entries_from_model (gboolean clear_sort, guint32 inst);
void st_song_changed (Song *song, gboolean removed, guint32 inst);
void st_add_song (Song *song, gboolean final, gboolean display, guint32 inst);
void st_create_tabs (void);
void st_remove_song (Song *song, guint32 inst);
void st_init (ST_CAT_item new_category, guint32 inst);
void st_create_notebook (gint inst);
void st_redisplay (guint32 inst);
void st_cleanup (void);
void st_set_default_sizes (void);
void st_update_default_sizes (void);
void st_adopt_order_in_playlist (void);
void sm_update_default_sizes (void);


/* Drag and drop definitions */
#define TGNR(a) (guint)(sizeof(a)/sizeof(GtkTargetEntry))
#define DND_GTKPOD_IDLIST_TYPE "application/gtkpod-idlist"
#define DND_GTKPOD_SM_PATHLIST_TYPE "application/gtkpod-sm_pathlist"
#define DND_GTKPOD_PM_PATHLIST_TYPE "application/gtkpod-pm_pathlist"
struct asf_data
{
    GtkTreeIter *to_iter;
    GtkTreeViewDropPosition pos;
};

typedef enum
{
    SONG_TREEVIEW,
    PLAYLIST_TREEVIEW,
    SORTTAB_TREEVIEW
} TreeViewType;

void  sm_list_store_move_before (GtkListStore *store,
				 GtkTreeIter  *iter,
				 GtkTreeIter  *position);
void  sm_list_store_move_after (GtkListStore *store,
				GtkTreeIter  *iter,
				GtkTreeIter  *position);
void  pm_list_store_move_before (GtkListStore *store,
				 GtkTreeIter  *iter,
				 GtkTreeIter  *position);
void  pm_list_store_move_after (GtkListStore *store,
				GtkTreeIter  *iter,
				GtkTreeIter  *position);
gboolean pmsm_move_pathlist (GtkTreeView *treeview,
			     gchar *data,
			     GtkTreePath *path,
			     GtkTreeViewDropPosition pos,
			     TreeViewType tvt);

#endif /* __DISPLAY_PRIVATE_H__ */
