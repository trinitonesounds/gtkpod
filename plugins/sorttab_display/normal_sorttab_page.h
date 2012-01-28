/*
 |  Copyright (C) 2002-2011 Jorg Schuler <jcsjcs at users sourceforge net>
 |                                             Paul Richardson <phantom_sf at users.sourceforge.net>
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
 */

#ifndef NORMAL_SORT_TAB_PAGE_H_
#define NORMAL_SORT_TAB_PAGE_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

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

    /* set if this is the "All" entry */
    gboolean master;

    /* set if this is the "Compilation" entry */
    gboolean compilation;

    /* GList with member tracks (pointer to "Track") */
    GList *members;
} TabEntry;

/* "Column numbers" in sort tab model */
typedef enum  {
  ST_COLUMN_ENTRY = 0,
  ST_NUM_COLUMNS
} ST_item;

GType normal_sort_tab_page_get_type (void);

#define NORMAL_SORT_TAB_TYPE_PAGE            (normal_sort_tab_page_get_type ())

#define NORMAL_SORT_TAB_PAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NORMAL_SORT_TAB_TYPE_PAGE, NormalSortTabPage))

#define NORMAL_SORT_TAB_IS_PAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NORMAL_SORT_TAB_TYPE_PAGE))

#define NORMAL_SORT_TAB_PAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NORMAL_SORT_TAB_TYPE_PAGE, NormalSortTabPageClass))

#define NORMAL_SORT_TAB_IS_PAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), NORMAL_SORT_TAB_TYPE_PAGE))

#define NORMAL_SORT_TAB_PAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NORMAL_SORT_TAB_TYPE_PAGE, NormalSortTabPageClass))


typedef struct _NormalSortTabPagePrivate NormalSortTabPagePrivate;
typedef struct _NormalSortTabPage        NormalSortTabPage;
typedef struct _NormalSortTabPageClass   NormalSortTabPageClass;

struct _NormalSortTabPage {
    /*<private>*/
    GtkTreeView parent_instance;

    /* structure containing private members */
     /*<private>*/
     NormalSortTabPagePrivate *priv;
};

struct _NormalSortTabPageClass {
  GtkTreeViewClass parent_class;

};

GtkWidget *normal_sort_tab_page_new();

void normal_sort_tab_page_set_unselected(NormalSortTabPage *self, gboolean state);

void normal_sort_tab_page_add_track(NormalSortTabPage *self, Track *track, gboolean final, gboolean display);

void normal_sort_tab_page_remove_track(NormalSortTabPage *self, Track *track);

void normal_sort_tab_page_track_changed(NormalSortTabPage *self, Track *track, gboolean removed);

void normal_sort_tab_page_clear(NormalSortTabPage *self);

GList *normal_sort_tab_page_get_selected_tracks(NormalSortTabPage *self);

void normal_sort_tab_page_sort(NormalSortTabPage *self, enum GtkPodSortTypes order);

void normal_sort_tab_page_stop_editing(NormalSortTabPage *self, gboolean cancel);

G_END_DECLS

#endif /* NORMAL_SORT_TAB_PAGE_H_ */
