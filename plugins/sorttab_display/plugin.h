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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <libanjuta/anjuta-plugin.h>

extern GType sorttab_display_plugin_get_type (GTypeModule *module);
#define SORTTAB_DISPLAY_TYPE_PLUGIN         (sorttab_display_plugin_get_type (NULL))
#define SORTTAB_DISPLAY_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), SORTTAB_DISPLAY_TYPE_PLUGIN, SorttabDisplayPlugin))
#define SORTTAB_DISPLAY_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), SORTTAB_DISPLAY_TYPE_PLUGIN, SorttabDisplayPluginClass))
#define SORTTAB_DISPLAY_IS_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), SORTTAB_DISPLAY_TYPE_PLUGIN))
#define SORTTAB_DISPLAY_IS_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), SORTTAB_DISPLAY_TYPE_PLUGIN))
#define SORTTAB_DISPLAY_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), SORTTAB_DISPLAY_TYPE_PLUGIN, SorttabDisplayPluginClass))

typedef struct _SorttabDisplayPlugin SorttabDisplayPlugin;
typedef struct _SorttabDisplayPluginClass SorttabDisplayPluginClass;

struct _SorttabDisplayPlugin {
    AnjutaPlugin parent;
    GtkWidget *sort_tab_widget_parent;
    gint uiid;
    GtkActionGroup *action_group;
    GtkWidget *prefs;
    GtkAction *more_filtertabs_action;
    GtkAction *fewer_filtertabs_action;
};

struct _SorttabDisplayPluginClass {
    AnjutaPluginClass parent_class;
};

#endif /* PLUGIN_H_ */
