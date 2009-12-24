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
|  $Id$
*/

#ifndef PLUGIN_H_
#define PLUGIN_H_

#include <libanjuta/anjuta-plugin.h>

/* Stock IDs */

#define UI_FILE GTKPOD_UI_DIR"/sorttab_display.ui"
#define GLADE_FILE GTKPOD_GLADE_DIR"/sorttab_display.glade"

typedef struct _SorttabDisplayPlugin SorttabDisplayPlugin;
typedef struct _SorttabDisplayPluginClass SorttabDisplayPluginClass;

struct _SorttabDisplayPlugin {
    AnjutaPlugin parent;
    GtkTreeView *sorttab_view;
    GtkWidget *pl_window;
    gint uiid;
    GtkActionGroup *action_group;
};

struct _SorttabDisplayPluginClass {
    AnjutaPluginClass parent_class;
};

#endif /* PLUGIN_H_ */
