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

#ifndef CLARITY_DND_SUPPORT_H_
#define CLARITY_DND_SUPPORT_H_

#include <gtk/gtk.h>
#include <libgtkpod/gp_itdb.h>
#include <libgtkpod/gp_private.h>
#include "plugin.h"

static GtkTargetEntry clarity_drop_types[] =
    {
        /* Konqueror supported flavours */
        { "image/jpeg", 0, DND_IMAGE_JPEG },

        /* Fallback flavours */
        { "text/plain", 0, DND_TEXT_PLAIN },
        { "STRING", 0, DND_TEXT_PLAIN }
    };

/**
 * dnd_clarity_drag_drop:
 *
 * Used by the drag and drop of a jpg. When a drop is
 * made, this determines whether the drop is valid
 * then requests the data from the source widget.
 *
 */
gboolean dnd_clarity_drag_drop(GtkWidget *widget, GdkDragContext *drag_context, gint x, gint y, guint time, gpointer user_data);

/**
 * dnd_clarity_drag_motion:
 *
 * Used by the drag and drop of a jpg. While the jpg is being
 * dragged, this reports to the source widget whether it is an
 * acceptable location to allow a drop.
 *
 */
gboolean dnd_clarity_drag_motion(GtkWidget *widget, GdkDragContext *dc, gint x, gint y, guint time, gpointer user_data);

/**
 * dnd_clarity_drag_data_received:
 *
 * Used by the drag and drop of a jpg. When the drop is performed, this
 * acts on the receipt of the data from the source widget and applies
 * the jpg to the track.
 *
 */
void dnd_clarity_drag_data_received(GtkWidget *widget, GdkDragContext *dc, gint x, gint y, GtkSelectionData *data, guint info, guint time, gpointer user_data);

#endif /* CLARITY_DND_SUPPORT_H_ */
