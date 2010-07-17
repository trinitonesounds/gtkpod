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
#include "photo_editor_context_menu.h"
#include "display_photo.h"
#include "libgtkpod/context_menus.h"
#include "libgtkpod/misc.h"

GtkWidget *gphoto_menuitem_remove_album_from_db_item(GtkWidget *menu)
{
    PhotoAlbum *selected_album = gphoto_get_selected_album();
    if (! selected_album || selected_album->album_type == 0x01)
        return menu; // no album or Photo Library selected

    return hookup_menu_item (
            menu,
            _("Remove Album"),
            GTK_STOCK_DELETE,
            G_CALLBACK (gphoto_remove_album_from_database),
            NULL);
}

GtkWidget *gphoto_menuitem_remove_photo_from_album_item(GtkWidget *menu)
{
    GtkWidget *mitem = hookup_menu_item (
            menu,
            _("Remove Photo"),
            GTK_STOCK_DELETE,
            G_CALLBACK (gphoto_remove_selected_photos_from_album),
            NULL);

    if (gphoto_get_selected_photo_count() == 0)
        gtk_widget_set_sensitive (mitem, FALSE);
    else
        gtk_widget_set_sensitive (mitem, TRUE);

    return mitem;
}

GtkWidget *gphoto_menuitem_rename_photoalbum_item(GtkWidget *menu)
{
    PhotoAlbum *selected_album = gphoto_get_selected_album();
        if (! selected_album || selected_album->album_type == 0x01)
            return menu; // no album or Photo Library selected

    return hookup_menu_item (
            menu,
            _("Rename Album"),
            GTK_STOCK_DELETE,
            G_CALLBACK (gphoto_rename_selected_album),
            NULL);
}

/**
 * photo_context_menu_init - initialize the right click menu for photo management display
 */
void gphoto_context_menu_init(gint component) {
    GtkWidget *menu = NULL;

    if (widgets_blocked)
        return;

    if (!gtkpod_get_current_itdb())
        return;

    menu = gtk_menu_new();

    switch (component) {
    case GPHOTO_ALBUM_VIEW:
        gphoto_menuitem_remove_album_from_db_item(menu);
        gphoto_menuitem_rename_photoalbum_item(menu);
        break;
    case GPHOTO_ICON_VIEW:
        gphoto_menuitem_remove_photo_from_album_item(menu);
        break;
    }

    /*
     * button should be button 0 as per the docs because we're calling
     * from a button release event
     */
    if (menu) {
        gtk_menu_popup(GTK_MENU (menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
    }
}
