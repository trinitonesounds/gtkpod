/*
 |  Copyright (C) 2003 Corey Donohoe <atmos at atmos dot org>
 |  Copyright (C) 2003-2007 Jorg Schuler <jcsjcs at users sourceforge net>
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib/gi18n-lib.h>
#include "itdb.h"
#include "file.h"
#include "misc.h"
#include "misc_track.h"
#include "misc_playlist.h"
#include "prefs.h"
#include "tools.h"
#include "syncdir.h"

#define LOCALDEBUG 0

/**
 * Attach a menu item to your context menu
 *
 * @m - the GtkMenu we're attaching to
 * @str - a gchar* with the menu label
 * @stock - name of the stock icon (or NULL if none is to be used)
 * @func - the callback for when the item is selected (or NULL)
 * @user_data - parameter to pass into the function callback
 */
GtkWidget *hookup_menu_item(GtkWidget *m, const gchar *str, const gchar *stock, GCallback func, gpointer userdata) {
    GtkWidget *mi;

    if (stock) {
        GtkWidget *image;
        mi = gtk_image_menu_item_new_with_mnemonic(str);
        image = gtk_image_new_from_stock(stock, GTK_ICON_SIZE_MENU);
        gtk_widget_show(image);
        gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM (mi), image);
    }
    else {
        mi = gtk_menu_item_new_with_label(str);
    }
    gtk_widget_show(mi);
    gtk_widget_set_sensitive(mi, TRUE);
    gtk_widget_add_events(mi, GDK_BUTTON_RELEASE_MASK);
    if (func)
        g_signal_connect(G_OBJECT(mi), "activate", func, userdata);
    gtk_container_add(GTK_CONTAINER (m), mi);
    return mi;
}

/**
 *  Add separator to Menu @m and return pointer to separator widget
 */
GtkWidget *add_separator(GtkWidget *menu) {
    GtkWidget *sep = NULL;
    if (menu) {
        sep = gtk_separator_menu_item_new();
        gtk_widget_show(sep);
        gtk_widget_set_sensitive(sep, TRUE);
        gtk_container_add(GTK_CONTAINER (menu), sep);
    }
    return sep;
}

void context_menu_delete_track_head(GtkMenuItem *mi, gpointer data) {
    DeleteAction deleteaction = GPOINTER_TO_INT (data);
    delete_track_head(deleteaction);
}

void context_menu_delete_playlist_head(GtkMenuItem *mi, gpointer data) {
    DeleteAction deleteaction = GPOINTER_TO_INT (data);
    delete_playlist_head(deleteaction);
}

GtkWidget *add_delete_playlist_but_keep_tracks(GtkWidget *menu) {
    return hookup_menu_item(menu, _("Delete But Keep Tracks"), GTK_STOCK_DELETE, G_CALLBACK (context_menu_delete_playlist_head), GINT_TO_POINTER (DELETE_ACTION_PLAYLIST));
}

/*
 * play_entries_now - play the entries currently selected in xmms
 * @mi - the menu item selected
 * @data - Ignored, should be NULL
 */
static void play_entries_now(GtkMenuItem *mi, gpointer data) {
    tools_play_tracks(gtkpod_get_selected_tracks());
}

GtkWidget *add_play_now(GtkWidget *menu) {
    return hookup_menu_item(menu, _("Play Now"), GTK_STOCK_CDROM, G_CALLBACK (play_entries_now), NULL);
}

/*
 * play_entries_now - play the entries currently selected in xmms
 * @mi - the menu item selected
 * @data - Ignored, should be NULL
 */
static void play_entries_enqueue(GtkMenuItem *mi, gpointer data) {
    tools_enqueue_tracks(gtkpod_get_selected_tracks());
}

GtkWidget *add_enqueue(GtkWidget *menu) {
    return hookup_menu_item(menu, _("Enqueue"), GTK_STOCK_CDROM, G_CALLBACK (play_entries_enqueue), NULL);
}

/*
 * update_entries - update the entries currently selected
 * @mi - the menu item selected
 * @data - Ignored, should be NULL
 */
static void update_tracks_from_file(GtkMenuItem *mi, gpointer data) {
    update_tracks(gtkpod_get_selected_tracks());
}

GtkWidget *add_update_tracks_from_file(GtkWidget *menu) {
    return hookup_menu_item(menu, _("Update Tracks from File"), GTK_STOCK_REFRESH, G_CALLBACK (update_tracks_from_file), NULL);
}


