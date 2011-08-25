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
#include "clarity_utils.h"

extern GType clarity_plugin_get_type (GTypeModule *module);
#define CLARITY_TYPE_PLUGIN         (clarity_plugin_get_type (NULL))
#define CLARITY_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CLARITY_TYPE_PLUGIN, ClarityPlugin))
#define CLARITY_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), CLARITY_TYPE_PLUGIN, ClarityPluginClass))
#define CLARITY_IS_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CLARITY_TYPE_PLUGIN))
#define CLARITY_IS_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), CLARITY_TYPE_PLUGIN))
#define CLARITY_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CLARITY_TYPE_PLUGIN, ClarityPluginClass))

typedef struct _ClarityPlugin ClarityPlugin;
typedef struct _ClarityPluginClass ClarityPluginClass;

struct _ClarityPlugin {
    AnjutaPlugin parent;
    GtkScrolledWindow *cover_window;
    gint uiid;
    GtkActionGroup *action_group;
    GtkWidget *prefs;
    gchar *gladepath;
};

struct _ClarityPluginClass {
    AnjutaPluginClass parent_class;
};

#endif /* PLUGIN_H_ */
