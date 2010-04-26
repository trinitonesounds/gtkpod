/*
 |  Copyright (C) 2002-2009 Paul Richardson <phantom_sf at users sourceforge net>
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
#ifndef DETAILS_EDITOR_IFACE_H_
#define DETAILS_EDITOR_IFACE_H_

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#include <gtk/gtk.h>
#include "itdb.h"

typedef struct _DetailsEditor DetailsEditor;
typedef struct _DetailsEditorInterface DetailsEditorInterface;

struct _DetailsEditorInterface {
    GTypeInterface g_iface;

    void (*edit_details)(GList *selected_tracks);
};

GType details_editor_get_type(void);

#define DETAILS_EDITOR_TYPE                (details_editor_get_type ())
#define DETAILS_EDITOR(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), DETAILS_EDITOR_TYPE, DetailsEditor))
#define DETAILS_EDITOR_IS_EDITOR(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DETAILS_EDITOR_TYPE))
#define DETAILS_EDITOR_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), DETAILS_EDITOR_TYPE, DetailsEditorInterface))

#endif /* DETAILS_EDITOR_IFACE_H_ */
