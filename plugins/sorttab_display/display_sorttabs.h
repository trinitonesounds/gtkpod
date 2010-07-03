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

#ifndef __DISPLAY_SORTTABS_H__
#define __DISPLAY_SORTTABS_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include "date_parser.h"
#include "sorttab_conversion.h"
#include "libgtkpod/gtkpod_app_iface.h"
#include "libgtkpod/gp_itdb.h"

/* Number of search tabs to be supported. */
#define SORT_TAB_MAX (ST_CAT_NUM-1)

/* Number of GtkPaned elements in the main window. The positions of
 * these elements will be stored in the prefs file and be set to the
 * last value when starting gtkpod again */
/* Number defined with glade ("paned%d") */
enum {
    PANED_PLAYLIST = 0,
    PANED_TRACKLIST,
    PANED_STATUS1,
    PANED_STATUS2,
    PANED_NUM_GLADE
};
/* Number created in display.c (for sort tabs, stored in st_paned[]) */
#define PANED_NUM_ST (SORT_TAB_MAX-1)
/* Total number */
#define PANED_NUM (PANED_NUM_GLADE+PANED_NUM_ST)

/* max. number of stars */
#define RATING_MAX 5

/* struct for each entry in sort tab */
typedef struct {
  gchar *name;

  /* The sort key can be compared with other sort keys using strcmp
   * and it will give the expected result, according to the user
   * settings. Must be regenerated if the user settings change.
   */
  gchar *name_sortkey;

  /* The fuzzy sortkey can be used to compare discarding some
   * prefixes, such as "the", "el", "la", etc. If NULL, you should use
   * name_sortkey instead.
   */
  gchar *name_fuzzy_sortkey;

  gboolean master; /* set if this is the "All" entry */
  gboolean compilation; /* set if this is the "Compilation" entry */
  GList *members;  /* GList with member tracks (pointer to "Track") */
} TabEntry;

/* struct with data corresponding to each sort tab */
typedef struct {
  guint current_category;            /* current page (category) selected) */
  gboolean final;                    /* have all tracks been added? */
  GtkWidget *window[ST_CAT_NUM];     /* pointer to scrolled window */
  /* The following are used for "normal" categories (not ST_CAT_SPECIAL) */
  GtkTreeModel *model;               /* pointer to model used */
  GtkNotebook *notebook;             /* pointer to notebook used */
  GtkTreeView *treeview[ST_CAT_NUM]; /* pointer to treeviews used */
  GList *entries;                    /* list with entries */
  TabEntry *current_entry;           /* pointer to currently selected entry */
  gchar *lastselection[ST_CAT_NUM];  /* name of entry last selected */
  GHashTable *entry_hash;            /* table for quick find of tab entries */
  gboolean unselected;               /* unselected item since last st_init? */
  /* The following are used for "special" categories (ST_CAT_SPECIAL) */
  GList *sp_members;                 /* list of tracks in sorttab */
  GList *sp_selected;                /* list of tracks selected */
  gboolean is_go;                    /* pass new members on automatically */
  TimeInfo ti_added;                 /* TimeInfo "added" (sp)         */
  TimeInfo ti_modified;              /* TimeInfo "modified" (sp)      */
  TimeInfo ti_played;                /* TimeInfo "played" (sp)        */
  GtkTooltipsData *sp_tooltips_data; /* ptr to tooltips in special st */
  /* function used for string comparisons, set in on_st_switch_page   */
  gint (*entry_compare_func) (const TabEntry *a, const TabEntry *b);
} SortTab;

/* "Column numbers" in sort tab model */
typedef enum  {
  ST_COLUMN_ENTRY = 0,
  ST_NUM_COLUMNS
} ST_item;

/* used for the ST_CAT_SPECIAL user_data (see st_create_special and
 * the corresponding signal functions) */
#define SP_SHIFT 9
#define SP_MASK ((1<<SP_SHIFT)-1)

GtkPaned *sorttab_parent;

void st_create_tabs(GtkPaned *parent, gchar *glade_path);
void st_init(ST_CAT_item new_category, guint32 inst);
void st_add_track(Track *track, gboolean final, gboolean display, guint32 inst);
void st_remove_track(Track *track, guint32 inst);
void st_enable_disable_view_sort(gint inst, gboolean enable);
void st_track_changed(Track *track, gboolean removed, guint32 inst);
void st_redisplay(guint32 inst);
GList *st_get_selected_members(guint32 inst);
void st_update_paned_position();
void st_show_visible(void);
TabEntry *st_get_selected_entry(gint inst);
gint st_get_sort_tab_number(gchar *text);
void st_delete_entry_head(gint inst, DeleteAction deleteaction);
void st_stop_editing(gint inst, gboolean cancel);
void st_sort(GtkSortType order);
void st_remove_entry(TabEntry *entry, guint32 inst);
void st_cleanup(void);

void cal_open_calendar(gint inst, T_item item);

void sorttab_display_select_playlist_cb(GtkPodApp *app, gpointer pl, gpointer data);
void sorttab_display_track_removed_cb(GtkPodApp *app, gpointer tk, gint32 pos, gpointer data);
void sorttab_display_track_updated_cb(GtkPodApp *app, gpointer tk, gpointer data);
void sorttab_display_preference_changed_cb(GtkPodApp *app, gpointer pfname, gint32 value, gpointer data);
void sorttab_display_tracks_reordered_cb(GtkPodApp *app, gpointer data);

#endif /* __DISPLAY_SORTTAB_H__ */
