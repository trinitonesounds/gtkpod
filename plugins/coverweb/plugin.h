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

extern GType cover_web_plugin_get_type (GTypeModule *module);
#define COVER_WEB_TYPE_PLUGIN         (cover_web_plugin_get_type (NULL))
#define COVER_WEB_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), COVER_WEB_TYPE_PLUGIN, CoverWebPlugin))
#define COVER_WEB_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), COVER_WEB_TYPE_PLUGIN, CoverWebPluginClass))
#define COVER_WEB_IS_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), COVER_WEB_TYPE_PLUGIN))
#define COVER_WEB_IS_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), COVER_WEB_TYPE_PLUGIN))
#define COVER_WEB_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), COVER_WEB_TYPE_PLUGIN, CoverWebPluginClass))

typedef struct _CoverWebPlugin CoverWebPlugin;
typedef struct _CoverWebPluginClass CoverWebPluginClass;

struct _CoverWebPlugin {
    AnjutaPlugin parent;
    GtkWidget *coverweb_window;
    gint uiid;
    GtkActionGroup *action_group;
    GtkWidget *prefs;
    gchar *glade_path;
};

struct _CoverWebPluginClass {
    AnjutaPluginClass parent_class;
};

#endif /* PLUGIN_H_ */
