/*
|  Copyright (C) 2002-2007 Jorg Schuler <jcsjcs at users sourceforge net>
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

#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <glade/glade.h>
#include "itdb.h"
#include "display_itdb.h"
#include "display_coverart.h"

/* Main XML glade file */
GladeXML *main_window_xml;

/* pointer to main window */
extern GtkWidget *gtkpod_window;

/* types for sort */
enum
{
    SORT_ASCENDING = GTK_SORT_ASCENDING,
    SORT_DESCENDING = GTK_SORT_DESCENDING,
    SORT_NONE = 10*(GTK_SORT_ASCENDING+GTK_SORT_DESCENDING),
};

/* Categories in each sort tab (page numbers) */
typedef enum {
  ST_CAT_ARTIST = 0,
  ST_CAT_ALBUM,
  ST_CAT_GENRE,
  ST_CAT_COMPOSER,
  ST_CAT_TITLE,
  ST_CAT_YEAR,
  ST_CAT_SPECIAL,
  ST_CAT_NUM
} ST_CAT_item;

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

/* struct for time info (added, played, modified) */
typedef struct {
    gchar *int_str;   /* copy of string specified in the sort tab */
    gboolean valid;   /* is current string valid? */
    time_t lower;     /* timestamp for lower limit */
    time_t upper;     /* timestamp for upper limit ("-1": no limit) */
    GtkWidget *entry; /* pointer to GtkEntry in sort tab */
    GtkWidget *active;/* pointer to toggle button */
} TimeInfo;

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

/* Column numbers in track model */
/* Note: add corresponding entries to T_item and TM_to_T() as well 
 * (below and in misc_conversion.c). 
 * IMPORTANT: Do not change the order -- always add new entries at the
 * end */
typedef enum  {
  TM_COLUMN_TITLE = 0,
  TM_COLUMN_ARTIST,
  TM_COLUMN_ALBUM,
  TM_COLUMN_GENRE,
  TM_COLUMN_COMPOSER,
  TM_COLUMN_TRACK_NR,         /*  5 */
  TM_COLUMN_IPOD_ID,
  TM_COLUMN_PC_PATH,
  TM_COLUMN_TRANSFERRED,
  TM_COLUMN_SIZE,
  TM_COLUMN_TRACKLEN,         /* 10 */
  TM_COLUMN_BITRATE,
  TM_COLUMN_PLAYCOUNT,
  TM_COLUMN_RATING,
  TM_COLUMN_TIME_PLAYED,
  TM_COLUMN_TIME_MODIFIED,    /* 15 */
  TM_COLUMN_VOLUME,
  TM_COLUMN_YEAR,
  TM_COLUMN_CD_NR,
  TM_COLUMN_TIME_ADDED,
  TM_COLUMN_IPOD_PATH,        /* 20 */
  TM_COLUMN_SOUNDCHECK,
  TM_COLUMN_SAMPLERATE,
  TM_COLUMN_BPM,
  TM_COLUMN_FILETYPE,
  TM_COLUMN_GROUPING,         /* 25 */
  TM_COLUMN_COMPILATION,
  TM_COLUMN_COMMENT,
  TM_COLUMN_CATEGORY,
  TM_COLUMN_DESCRIPTION,
  TM_COLUMN_PODCASTURL,       /* 30 */
  TM_COLUMN_PODCASTRSS,
  TM_COLUMN_SUBTITLE,
  TM_COLUMN_TIME_RELEASED,
  TM_COLUMN_THUMB_PATH,
  TM_COLUMN_MEDIA_TYPE,       /* 35 */
  TM_COLUMN_TV_SHOW,
  TM_COLUMN_TV_EPISODE,
  TM_COLUMN_TV_NETWORK,
  TM_COLUMN_SEASON_NR,
  TM_COLUMN_EPISODE_NR,       /* 40 */
  TM_COLUMN_ALBUMARTIST,
  TM_COLUMN_SORT_ARTIST,
  TM_COLUMN_SORT_TITLE,
  TM_COLUMN_SORT_ALBUM,
  TM_COLUMN_SORT_ALBUMARTIST, /* 45 */
  TM_COLUMN_SORT_COMPOSER,
  TM_COLUMN_SORT_TVSHOW,
  TM_COLUMN_LYRICS,
  TM_NUM_COLUMNS
} TM_item;

/* A means to address the fields by uniform IDs. May be extended as
 * needed. You should extend "track_get_item_pointer()" defined in
 * track.c as well for string fields. */
/* Add corresponding entries to t_strings[] and t_tooltips[] in
   misc_conversion.c! */
/* Used in prefs_window.c to label the sort_ign_field<num> buttons */
/* Used in details.c to label the detail_label_<num> labels */
typedef enum {
    T_ALL = 0,      /* all fields */
    T_ALBUM,
    T_ARTIST,
    T_TITLE,
    T_GENRE,
    T_COMMENT,      /*  5 */
    T_COMPOSER,
    T_FILETYPE,
    T_PC_PATH,
    T_IPOD_PATH,
    T_IPOD_ID,      /* 10 */
    T_TRACK_NR,
    T_TRANSFERRED,
    T_SIZE,
    T_TRACKLEN,
    T_BITRATE,      /* 15 */
    T_SAMPLERATE,
    T_BPM,
    T_PLAYCOUNT,
    T_RATING,
    T_TIME_ADDED,   /* 20 */
    T_TIME_PLAYED,
    T_TIME_MODIFIED,
    T_VOLUME,
    T_SOUNDCHECK,
    T_YEAR,         /* 25 */
    T_CD_NR,
    T_GROUPING,
    T_COMPILATION,
    T_CATEGORY,
    T_DESCRIPTION,  /* 30 */
    T_PODCASTURL,
    T_PODCASTRSS,
    T_SUBTITLE,
    T_TIME_RELEASED,
    T_CHECKED,      /* 35 */
    T_STARTTIME,
    T_STOPTIME,
    T_REMEMBER_PLAYBACK_POSITION,
    T_SKIP_WHEN_SHUFFLING,
    T_THUMB_PATH,   /* 40 */
    T_MEDIA_TYPE,
    T_TV_SHOW,
    T_TV_EPISODE,
    T_TV_NETWORK,
    T_SEASON_NR,    /* 45 */
    T_EPISODE_NR,
    T_ALBUMARTIST,
    T_SORT_ARTIST,
    T_SORT_TITLE,
    T_SORT_ALBUM,   /* 50 */
    T_SORT_ALBUMARTIST,
    T_SORT_COMPOSER,
    T_SORT_TVSHOW,
    T_GAPLESS_TRACK_FLAG,
    T_LYRICS,
    T_ITEM_NUM,
} T_item;


/* number of entries with "autoset empty tag to filename " feature */
#define TM_NUM_TAGS_PREFS (5)

/* "Column numbers" in playlist model */
typedef enum  {
  PM_COLUMN_ITDB = 0,
  PM_COLUMN_TYPE,
  PM_COLUMN_PLAYLIST,
  PM_COLUMN_PHOTOS,
  PM_NUM_COLUMNS
} PM_column_type;

/* Drag and drop types */
enum {
    DND_GTKPOD_TRACKLIST = 1000,
    DND_GTKPOD_TM_PATHLIST,
    DND_GTKPOD_PLAYLISTLIST,
    DND_TEXT_URI_LIST,
    DND_TEXT_PLAIN,
    DND_IMAGE_JPEG
};

/* used for the ST_CAT_SPECIAL user_data (see st_create_special and
 * the corresponding signal functions) */
#define SP_SHIFT 9
#define SP_MASK ((1<<SP_SHIFT)-1)

void display_create (void);
void display_cleanup (void);
void display_adjust_menus (void);
void display_show_hide_toolbar (void);
void display_show_hide_searchbar (void);
void display_reset (gint inst);
GList *display_get_selection (guint32 inst);
GList *display_get_selected_members (gint inst);
void display_remove_autoscroll_row_timeout (GtkWidget *widget);
void display_install_autoscroll_row_timeout (GtkWidget *widget);
void display_image_dialog (GdkPixbuf *image);

Playlist* pm_get_selected_playlist (void);
iTunesDB* pm_get_selected_itdb (void);
gint pm_get_position_for_itdb (iTunesDB *itdb);
void pm_remove_playlist (Playlist *playlist, gboolean select);
void pm_add_child (iTunesDB *itdb, PM_column_type type, gpointer item, gint position);
void pm_add_itdb (iTunesDB *itdb, gint pos);
void pm_select_playlist (Playlist *playlist);
void pm_unselect_playlist (Playlist *playlist);
void pm_remove_track (Playlist *playlist, Track *track);
void pm_add_track (Playlist *playlist, Track *track, gboolean display);
void pm_itdb_name_changed (iTunesDB *itdb);
void pm_track_changed (Track *track);
void pm_sort (GtkSortType order);
void pm_stop_editing (gboolean cancel);
void pm_set_playlist_renderer_pix (GtkCellRenderer *renderer,
			  Playlist *playlist);
void pm_set_playlist_renderer_text (GtkCellRenderer *renderer,
			   Playlist *playlist);
void pm_show_all_playlists ();

void st_stop_editing (gint inst, gboolean cancel);
gboolean st_set_selection (Itdb_Track *track);
void st_redisplay (guint32 inst);
void st_sort (GtkSortType order);
void st_remove_entry (TabEntry *entry, guint32 inst);
gint st_get_instance_from_treeview (GtkTreeView *tv);
void st_show_visible (void);
void st_arrange_visible_sort_tabs (void);
void st_adopt_order_in_playlist (void);
TabEntry *st_get_selected_entry (gint inst);
void st_update_paned_position ();
void st_rebuild_sortkeys ();


void cal_open_calendar (gint inst, T_item item);
void sp_go (guint32 inst);
void sp_conditions_changed (guint32 inst);

void tm_add_track_to_track_model (Track *track, GtkTreeIter *into_iter);
gint tm_get_nr_of_tracks(void);
void tm_rows_reordered(void);
gboolean tm_add_filelist (gchar *data, GtkTreePath *path,
			  GtkTreeViewDropPosition pos);
void tm_stop_editing (gboolean cancel);
void tm_show_preferred_columns(void);
void tm_store_col_order (void);
void tm_adopt_order_in_sorttab (void);

void tm_sort (TM_item col, GtkSortType order);
gint tm_sort_counter (gint inc);
GList* tm_get_selected_trackids(void);
GList* tm_get_selected_tracks(void);
GList* tm_get_all_trackids(void);
GList* tm_get_all_tracks(void);

void display_update_default_sizes (void);
void display_set_default_sizes (void);
void display_show_hide_tooltips (void);
void display_set_info_window_menu (void);

void spl_edit (Playlist *spl);
void spl_edit_new (iTunesDB *itdb, gchar *name, gint32 pos);
#endif 
