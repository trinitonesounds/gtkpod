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
#include "clarity_context_menu.h"
#include "clarity_canvas.h"

static GtkWidget *add_get_cover_from_file(GtkWidget *menu, ClarityCanvas *ccanvas) {
    return hookup_menu_item(menu, _("Select Cover From File"), GTK_STOCK_FLOPPY, G_CALLBACK (on_clarity_set_cover_menuitem_activate), ccanvas);
}

/**
 * clarity_context_menu_init - initialize the right click menu for clarity
 */
void clarity_context_menu_init(ClarityCanvas *ccanvas) {
    if (widgets_blocked)
        return;

    GtkWidget *menu = NULL;

    AlbumItem *item = clarity_canvas_get_current_album_item(ccanvas);
    gtkpod_set_selected_tracks(item->tracks);

    if (gtkpod_get_selected_tracks()) {
        menu = gtk_menu_new();

        add_get_cover_from_file(menu, ccanvas);
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
