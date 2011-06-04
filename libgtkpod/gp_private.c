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

#include "gp_private.h"

/* ------------------------------------------------------------

 Functions for treeview autoscroll (during DND)

 ------------------------------------------------------------ */

static void _remove_scroll_row_timeout(GtkWidget *widget) {
    g_return_if_fail(widget);

    g_object_set_data(G_OBJECT(widget), "scroll_row_timeout", NULL);
    g_object_set_data(G_OBJECT(widget), "scroll_row_times", NULL);
}

static gint gp_autoscroll_row_timeout(gpointer data) {
    GtkTreeView *treeview = data;
    gint px, py;
    GdkModifierType mask;
    GdkRectangle vis_rect;
    guint times;
    gboolean resp = TRUE;
    const gint SCROLL_EDGE_SIZE = 12;

    g_return_val_if_fail(data, FALSE);

    gdk_threads_enter();

    times = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(data), "scroll_row_times"));

    gdk_window_get_pointer(gtk_tree_view_get_bin_window(treeview), &px, &py, &mask);
    gtk_tree_view_get_visible_rect(treeview, &vis_rect);
    /*     printf ("px/py, w/h, mask: %d/%d, %d/%d, %d\n", px, py, */
    /*      vis_rect.width, vis_rect.height, mask); */
    if ((vis_rect.height > 2.2 * SCROLL_EDGE_SIZE) && ((py < SCROLL_EDGE_SIZE) || (py > vis_rect.height
            - SCROLL_EDGE_SIZE))) {
        GtkTreePath *path = NULL;

        if (py < SCROLL_EDGE_SIZE / 3)
            ++times;
        if (py > vis_rect.height - SCROLL_EDGE_SIZE / 3)
            ++times;

        if (times > 0) {
            if (gtk_tree_view_get_path_at_pos(treeview, px, py, &path, NULL, NULL, NULL)) {
                if (py < SCROLL_EDGE_SIZE)
                    gtk_tree_path_prev(path);
                if (py > vis_rect.height - SCROLL_EDGE_SIZE)
                    gtk_tree_path_next(path);
                gtk_tree_view_scroll_to_cell(treeview, path, NULL, FALSE, 0, 0);
            }
            times = 0;
        }
        else {
            ++times;
        }
    }
    else {
        times = 0;
    }
    g_object_set_data(G_OBJECT(data), "scroll_row_times", GUINT_TO_POINTER(times));
    if (mask == 0) {
        _remove_scroll_row_timeout(data);
        resp = FALSE;
    }
    gdk_threads_leave();
    return resp;
}

void gp_install_autoscroll_row_timeout(GtkWidget *widget) {
    if (!g_object_get_data(G_OBJECT(widget), "scroll_row_timeout")) { /* install timeout function for autoscroll */
        guint timeout = g_timeout_add(75, gp_autoscroll_row_timeout, widget);
        g_object_set_data(G_OBJECT(widget), "scroll_row_timeout", GUINT_TO_POINTER(timeout));
    }
}

void gp_remove_autoscroll_row_timeout(GtkWidget *widget) {
    guint timeout;

    g_return_if_fail(widget);

    timeout = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(widget), "scroll_row_timeout"));

    if (timeout != 0) {
        g_source_remove(timeout);
        _remove_scroll_row_timeout(widget);
    }
}

GQuark gtkpod_general_error_quark (void)
{
  return g_quark_from_static_string ("gtkpod-general-error-quark");
}

void gtkpod_log_error_printf(GError **error, gchar *msg, ...) {
    gchar* buf;
    va_list args;
    va_start (args, msg);
    buf = g_strdup_vprintf(msg, args);
    va_end (args);

    gtkpod_log_error(error, buf);
    g_free(buf);
}

void gtkpod_log_error(GError **error, gchar *msg) {
    g_set_error (error,
                GTKPOD_GENERAL_ERROR,                       /* error domain */
                GTKPOD_GENERAL_ERROR_FAILED,               /* error code */
		 "%s", msg);
}
