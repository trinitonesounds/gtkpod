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

#ifndef CLARITY_WIDGET_H_
#define CLARITY_WIDGET_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

GType clarity_widget_get_type (void);

#define CLARITY_TYPE_WIDGET            (clarity_widget_get_type ())

#define CLARITY_WIDGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLARITY_TYPE_WIDGET, ClarityWidget))

#define CLARITY_IS_WIDGET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLARITY_TYPE_WIDGET))

#define CLARITY_WIDGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLARITY_TYPE_WIDGET, ClarityWidgetClass))

#define CLARITY_IS_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLARITY_TYPE_WIDGET))

#define CLARITY_WIDGET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLARITY_TYPE_WIDGET, ClarityWidgetClass))

typedef struct _ClarityWidgetPrivate ClarityWidgetPrivate;
typedef struct _ClarityWidget        ClarityWidget;
typedef struct _ClarityWidgetClass   ClarityWidgetClass;

struct _ClarityWidget {
    /*<private>*/
    GtkBox parent_instance;

    /* structure containing private members */
     /*<private>*/
     ClarityWidgetPrivate *priv;
};

struct _ClarityWidgetClass {
  GtkBoxClass parent_class;

};

void clarity_widget_set_background(ClarityWidget *self);

GtkWidget * clarity_widget_new();


// Signal callbacks
void clarity_widget_preference_changed_cb(GtkPodApp *app, gpointer pfname, gint32 value, gpointer data);
void clarity_widget_playlist_selected_cb(GtkPodApp *app, gpointer pl, gpointer data);
void clarity_widget_track_removed_cb(GtkPodApp *app, gpointer tk, gpointer data);
void clarity_widget_tracks_selected_cb(GtkPodApp *app, gpointer tks, gpointer data);
void clarity_widget_track_updated_cb(GtkPodApp *app, gpointer tk, gpointer data);
void clarity_widget_track_added_cb(GtkPodApp *app, gpointer tk, gpointer data);


G_END_DECLS



#endif /* CLARITY_WIDGET_H_ */
