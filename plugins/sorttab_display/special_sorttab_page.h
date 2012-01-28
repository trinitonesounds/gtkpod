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

#ifndef SPECIAL_SORTTAB_PAGE_PAGE_H_
#define SPECIAL_SORTTAB_PAGE_PAGE_H_

#include <gtk/gtk.h>
#include "sorttab_widget.h"
#include "special_sorttab_page.h"
#include "date_parser.h"

G_BEGIN_DECLS

GType special_sort_tab_page_get_type (void);

#define SPECIAL_SORT_TAB_TYPE_PAGE            (special_sort_tab_page_get_type ())

#define SPECIAL_SORT_TAB_PAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SPECIAL_SORT_TAB_TYPE_PAGE, SpecialSortTabPage))

#define SPECIAL_SORT_TAB_IS_PAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SPECIAL_SORT_TAB_TYPE_PAGE))

#define SPECIAL_SORT_TAB_PAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SPECIAL_SORT_TAB_TYPE_PAGE, SpecialSortTabPageClass))

#define SPECIAL_SORT_TAB_IS_PAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SPECIAL_SORT_TAB_TYPE_PAGE))

#define SPECIAL_SORT_TAB_PAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), SPECIAL_SORT_TAB_TYPE_PAGE, SpecialSortTabPageClass))


typedef struct _SpecialSortTabPagePrivate SpecialSortTabPagePrivate;
typedef struct _SpecialSortTabPage        SpecialSortTabPage;
typedef struct _SpecialSortTabPageClass   SpecialSortTabPageClass;

struct _SpecialSortTabPage {
    /*<private>*/
    GtkScrolledWindow parent_instance;

    /* structure containing private members */
     /*<private>*/
     SpecialSortTabPagePrivate *priv;
};

struct _SpecialSortTabPageClass {
    GtkScrolledWindowClass parent_class;

};

GtkWidget *special_sort_tab_page_new(SortTabWidget *st_widget_parent, gchar *glade_file_path);

void special_sort_tab_page_add_track(SpecialSortTabPage *self, Track *track, gboolean final, gboolean display);

void special_sort_tab_page_remove_track(SpecialSortTabPage *self, Track *track);

void special_sort_tab_page_track_changed(SpecialSortTabPage *self, Track *track, gboolean removed);

gchar *special_sort_tab_page_get_glade_file(SpecialSortTabPage *self);

SortTabWidget *special_sort_tab_page_get_parent(SpecialSortTabPage *self);

void special_sort_tab_page_store_state(SpecialSortTabPage *self);

TimeInfo *special_sort_tab_page_update_date_interval(SpecialSortTabPage *self, T_item item, gboolean force_update);

TimeInfo *special_sort_tab_page_get_timeinfo(SpecialSortTabPage *self, T_item item);

GList *special_sort_tab_page_get_selected_tracks(SpecialSortTabPage *self);

gboolean special_sort_tab_page_get_is_go(SpecialSortTabPage *self);

void special_sort_tab_page_set_is_go(SpecialSortTabPage *self, gboolean pass_on_new_members);

void special_sort_tab_page_clear(SpecialSortTabPage *self);

G_END_DECLS

#endif /* SPECIAL_SORTTAB_PAGE_H_ */
