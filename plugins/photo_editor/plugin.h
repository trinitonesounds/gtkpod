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

#define DEFAULT_PHOTO_EDITOR_ICON "playlist-photo"
#define DEFAULT_PHOTO_EDITOR_STOCK_ID "photo-editor-editor-icon"

#define PHOTO_TOOLBAR_ALBUM_ICON "photo-toolbar-album"
#define PHOTO_TOOLBAR_ALBUM_STOCK_ID "photo-editor-toolbar-album"

#define PHOTO_TOOLBAR_PHOTOS_ICON "photo-toolbar-photos"
#define PHOTO_TOOLBAR_PHOTOS_STOCK_ID "photo-editor-toolbar-photos"

typedef struct _PhotoEditorPlugin PhotoEditorPlugin;
typedef struct _PhotoEditorPluginClass PhotoEditorPluginClass;

struct _PhotoEditorPlugin {
    AnjutaPlugin parent;
    gint uiid;
    GtkWidget *photo_window;
    GtkActionGroup *action_group;
};

struct _PhotoEditorPluginClass {
    AnjutaPluginClass parent_class;
};

PhotoEditorPlugin *photo_editor_plugin;

#endif /* PLUGIN_H_ */
