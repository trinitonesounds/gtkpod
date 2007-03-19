/* Time-stamp: <2007-03-19 23:11:13 jcs>
|
|  Copyright (C) 2002-2005 Jorg Schuler <jcsjcs at users sourceforge net>
|  Part of the gtkpod project.
| 
|  URL: http://www.gtkpod.org/
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

#ifndef __DISPLAY_PRIVATE_H__
#define __DISPLAY_PRIVATE_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "display.h"
#include "misc.h"
#include "display_coverart.h"

/* tree sort cannot be unsorted by choosing the default sort
 * column. Set to 1 if it's broken, 0 if it's not broken */
#define BROKEN_GTK_TREE_SORT (!RUNTIME_GTK_CHECK_VERSION(2,5,4))

/* This was defined in 2.5.4 -- as I want to detect whether
   GTK_TREE_SORT is BROKEN at run-time (see above), I need to define
   it here in case it's not defined */
#ifndef GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID 
#define GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID (-2)
#endif

/* print some timing info for tuning purposes */
#define DEBUG_TIMING 0
/* print info when callbacks are initialized */
#define DEBUG_CB_INIT 0
/* print info when adding tracks */
#define DEBUG_ADD_TRACK 0

/* used for display organization */
void pm_create_treeview (void);
void pm_set_selected_playlist(Playlist *pl);
void pm_remove_all_playlists (gboolean clear_sort);
void pm_add_all_itdbs (void);
void tm_create_treeview (void);
void tm_track_changed (Track *track);
void tm_remove_track (Track *track);
void tm_remove_all_tracks (void);
void st_remove_all_entries_from_model (guint32 inst);
void st_track_changed (Track *track, gboolean removed, guint32 inst);
void st_add_track (Track *track, gboolean final, gboolean display, guint32 inst);
void st_create_tabs (void);
void st_remove_track (Track *track, guint32 inst);
void st_init (ST_CAT_item new_category, guint32 inst);
void st_cleanup (void);
void st_set_default_sizes (void);
void st_update_default_sizes (void);
void tm_update_default_sizes (void);
void st_show_hide_tooltips (void);
GList *st_get_selected_members (guint32 inst);

void st_enable_disable_view_sort (gint inst, gboolean enable);
void tm_enable_disable_view_sort (gboolean enable);


/* Drag and drop definitions */
#define TGNR(a) (guint)(sizeof(a)/sizeof(GtkTargetEntry))
#define DND_GTKPOD_TRACKLIST_TYPE "application/gtkpod-tracklist"
#define DND_GTKPOD_TM_PATHLIST_TYPE "application/gtkpod-tm_pathlist"
#define DND_GTKPOD_PLAYLISTLIST_TYPE "application/gtkpod-playlistlist"

/* Prefs strings */
extern const gchar *TM_PREFS_SEARCH_COLUMN;

struct asf_data
{
    GtkTreeIter *to_iter;
    GtkTreeViewDropPosition pos;
};

#endif /* __DISPLAY_PRIVATE_H__ */
