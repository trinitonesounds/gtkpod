/*
 |  Copyright (C) 2002-2010 Jorg Schuler <jcsjcs at users sourceforge net>
 |                                          Paul Richardson <phantom_sf at users.sourceforge.net>
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib/gi18n-lib.h>
#include "libgtkpod/context_menus.h"
#include "libgtkpod/misc.h"
#include "display_coverart.h"
#include "cover_display_context_menu.h"

static GtkWidget *add_get_cover_from_file(GtkWidget *menu) {
    return hookup_menu_item(menu, _("Select Cover From File"), GTK_STOCK_FLOPPY, G_CALLBACK (coverart_set_cover_from_file), NULL);
}

/*
 * display the dialog with the full size cd artwork cover
 * @mi - the menu item selected
 * @data - Ignored, should be NULL
 */
static void display_track_artwork(GtkMenuItem *mi, gpointer data) {
    if (gtkpod_get_selected_tracks())
        coverart_display_big_artwork(gtkpod_get_selected_tracks());
}

static GtkWidget *add_display_big_coverart(GtkWidget *menu) {
    const gchar *icon;
    /* gets defined in gtk+ V2.8, but we only require V2.6 */
#ifndef GTK_STOCK_FULLSCREEN
#define GTK_STOCK_FULLSCREEN "gtk-fullscreen"
#endif
    if (gtk_check_version(2, 8, 0) == NULL) {
        icon = GTK_STOCK_FULLSCREEN;
    }
    else {
        icon = NULL;
    }

    return hookup_menu_item(menu, _("View Full Size Artwork"), icon, G_CALLBACK (display_track_artwork), NULL);
}

/**
 * cad_context_menu_init - initialize the right click menu for coverart display
 */
void cad_context_menu_init(void) {
    if (widgets_blocked)
        return;

    GtkWidget *menu = NULL;

    if (gtkpod_get_selected_tracks()) {
        menu = gtk_menu_new();

        add_get_cover_from_file(menu);
        add_display_big_coverart(menu);
        add_edit_track_details(menu);

        /*
         * button should be button 0 as per the docs because we're calling
         * from a button release event
         */
        if (menu) {
            gtk_menu_popup(GTK_MENU (menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
        }
    }
}
