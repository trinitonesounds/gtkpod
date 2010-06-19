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

#ifndef COVERWEB_H_
#define COVERWEB_H_

typedef struct {
    GtkWidget *window;
    GtkWidget *menubar;
    GtkWidget *bookmark_menu_item;
    GtkWidget *bookmark_menu;
    GtkWidget *toolbar;
    GtkWidget *browser_window;
    GtkWidget *webview;

    GtkStatusbar *statusbar;
    guint status_context_id;
    gint load_progress;
    GtkWidget* uri_entry;

} WebBrowser;

/**
 *
 * init_web_browser
 *
 * Initialise the webkit browser
 *
 * @parent: Widget to house the browser
 */
WebBrowser *init_web_browser(GtkWidget *parent);

/**
 *
 * destroy_cover_web
 *
 * destroy the web browser and all associated items.
 */
void destroy_cover_web();

void update_bookmark_menu();

#endif /* COVERWEB_H_ */
