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

#include "gtkpod_app_iface.h"

static void gtkpod_app_base_init(GtkPodAppInterface* klass) {
    static gboolean initialized = FALSE;

    if (!initialized) {
        initialized = TRUE;
    }
}

GType gtkpod_app_get_type(void) {
    static GType type = 0;
    if (!type) {
        static const GTypeInfo
                info = { sizeof(GtkPodAppInterface), (GBaseInitFunc) gtkpod_app_base_init, NULL, NULL, NULL, NULL, 0, 0, NULL };
        type = g_type_register_static(G_TYPE_INTERFACE, "GtkPodAppInterface", &info, 0);
        g_type_interface_add_prerequisite(type, G_TYPE_OBJECT);
    }
    return type;
}

void gtkpod_app_statusbar_message(GtkPodApp *obj, gchar* message, ...) {
    g_return_if_fail (GTKPOD_IS_APP(obj));
    GTKPOD_APP_GET_INTERFACE (obj)->statusbar_message(obj, message, NULL);
}
