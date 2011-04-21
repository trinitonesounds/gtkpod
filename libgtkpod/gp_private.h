/* Time-stamp: <2009-11-28 11:10:13 pgr>
 |  Copyright (C) 2002-2009 Jorg Schuler <jcsjcs at users sourceforge net>
 |  Copyright (C) 2009-2010 Paul Richardson <phantom_sf at users sourceforge net>
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

#ifndef GP_DEFINITIONS_H_
#define GP_DEFINITIONS_H_

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

/* tree sort cannot be unsorted by choosing the default sort
 * column. Set to 1 if it's broken, 0 if it's not broken */
#define BROKEN_GTK_TREE_SORT (!RUNTIME_GTK_CHECK_VERSION(2,5,4))

/* Drag and drop definitions */
#define TGNR(a) (guint)(sizeof(a)/sizeof(GtkTargetEntry))
#define DND_GTKPOD_TRACKLIST_TYPE "application/gtkpod-tracklist"
#define DND_GTKPOD_TM_PATHLIST_TYPE "application/gtkpod-tm_pathlist"
#define DND_GTKPOD_PLAYLISTLIST_TYPE "application/gtkpod-playlistlist"

/* Drag and drop types */
enum GtkPodDndTypes {
    DND_GTKPOD_TRACKLIST = 1000,
    DND_GTKPOD_TM_PATHLIST,
    DND_GTKPOD_PLAYLISTLIST,
    DND_TEXT_URI_LIST,
    DND_TEXT_PLAIN,
    DND_IMAGE_JPEG
};

/* types for sort */
enum GtkPodSortTypes {
    SORT_ASCENDING = GTK_SORT_ASCENDING,
    SORT_DESCENDING = GTK_SORT_DESCENDING,
    SORT_NONE = 10 * (GTK_SORT_ASCENDING + GTK_SORT_DESCENDING)
};

/* print some timing info for tuning purposes */
#define DEBUG_TIMING 0
/* print info when callbacks are initialized */
#define DEBUG_CB_INIT 0
/* print info when adding tracks */
#define DEBUG_ADD_TRACK 0

/* Prefs strings */
extern const gchar *TM_PREFS_SEARCH_COLUMN;

struct asf_data {
    GtkTreeIter *to_iter;
    GtkTreeViewDropPosition pos;
};

/* ------------------------------------------------------------

 Functions for treeview autoscroll (during DND)

 ------------------------------------------------------------ */

void gp_install_autoscroll_row_timeout(GtkWidget *widget);

void gp_remove_autoscroll_row_timeout(GtkWidget *widget);

/* -------------------------------------------------------------

 Error Handling

 ------------------------------------------------------------- */

#define GTKPOD_GENERAL_ERROR gtkpod_general_error_quark ()

typedef enum {
    GTKPOD_GENERAL_ERROR_FAILED
} GtkPodGeneralError;

GQuark gtkpod_general_error_quark (void);

void gtkpod_log_error(GError **error, gchar *msg);

#endif /* GP_DEFINITIONS_H_ */
