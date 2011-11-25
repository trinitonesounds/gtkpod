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

#ifndef SORT_TAB_WIDGET_H_
#define SORT_TAB_WIDGET_H_

#include <gtk/gtk.h>
#include "libgtkpod/gp_itdb.h"
#include "sorttab_conversion.h"

/* Number of search tabs to be supported. */
#define SORT_TAB_MAX (ST_CAT_NUM - 1)

G_BEGIN_DECLS

GType sort_tab_widget_get_type (void);

#define SORT_TAB_TYPE_WIDGET            (sort_tab_widget_get_type ())

#define SORT_TAB_WIDGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SORT_TAB_TYPE_WIDGET, SortTabWidget))

#define SORT_TAB_IS_WIDGET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SORT_TAB_TYPE_WIDGET))

#define SORT_TAB_WIDGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SORT_TAB_TYPE_WIDGET, SortTabWidgetClass))

#define SORT_TAB_IS_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SORT_TAB_TYPE_WIDGET))

#define SORT_TAB_WIDGET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), SORT_TAB_TYPE_WIDGET, SortTabWidgetClass))


typedef struct _SortTabWidgetPrivate SortTabWidgetPrivate;
typedef struct _SortTabWidget        SortTabWidget;
typedef struct _SortTabWidgetClass   SortTabWidgetClass;

struct _SortTabWidget {
    /*<private>*/
    GtkNotebook parent_instance;

    /* structure containing private members */
     /*<private>*/
     SortTabWidgetPrivate *priv;
};

struct _SortTabWidgetClass {
  GtkNotebookClass parent_class;

};

gint sort_tab_widget_get_max_index();

SortTabWidget *sort_tab_widget_new(gint inst, GtkWidget *parent, gchar *glade_xml_path);

SortTabWidget *sort_tab_widget_get_previous(SortTabWidget *self);

void sort_tab_widget_set_previous(SortTabWidget *self, SortTabWidget *prev);

SortTabWidget *sort_tab_widget_get_next(SortTabWidget *self);

void sort_tab_widget_set_next(SortTabWidget *self, SortTabWidget *next);

GtkWidget *sort_tab_widget_get_parent(SortTabWidget *self);

void sort_tab_widget_set_parent(SortTabWidget *self, GtkWidget *parent);

guint sort_tab_widget_get_instance(SortTabWidget *self);

guint sort_tab_widget_get_category(SortTabWidget *self);

gchar *sort_tab_widget_get_glade_path(SortTabWidget *self);

gboolean sort_tab_widget_is_all_tracks_added(SortTabWidget *self);

void sort_tab_widget_set_all_tracks_added(SortTabWidget *self, gboolean status);

GtkTreeModel *sort_tab_widget_get_normal_model(SortTabWidget *self);

void sort_tab_widget_add_track(SortTabWidget *self, Track *track, gboolean final, gboolean display);

void sort_tab_widget_remove_track(SortTabWidget *self, Track *track);

void sort_tab_widget_track_changed(SortTabWidget *self, Track *track, gboolean removed);

void sort_tab_widget_sort(SortTabWidget *self, enum GtkPodSortTypes order);

void sort_tab_widget_build(SortTabWidget *self, ST_CAT_item new_category);

void sort_tab_widget_refresh(SortTabWidget *self);

void sort_tab_widget_set_sort_enablement(SortTabWidget *self, gboolean enable);

void sort_tab_widget_delete_entry_head(SortTabWidget *self, DeleteAction deleteaction);

GList *sort_tab_widget_get_selected_tracks(SortTabWidget *self);

void sort_tab_widget_stop_editing(SortTabWidget *self, gboolean cancel);

G_END_DECLS

#endif /* SORT_TAB_WIDGET_H_ */
