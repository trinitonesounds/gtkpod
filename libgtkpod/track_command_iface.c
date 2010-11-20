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
    #include <config.h>
#endif

#include "track_command_iface.h"

static void track_command_base_init(TrackCommandInterface *klass) {
    static gboolean initialized = FALSE;

    if (!initialized) {
        klass->execute = NULL;
        initialized = TRUE;
    }
}

GType track_command_get_type(void) {
    static GType type = 0;
    if (!type) {
        static const GTypeInfo info =
            { sizeof(TrackCommandInterface), (GBaseInitFunc) track_command_base_init, NULL, NULL, NULL, NULL, 0, 0, NULL };
        type = g_type_register_static(G_TYPE_INTERFACE, "TrackCommand", &info, 0);
        g_type_interface_add_prerequisite(type, G_TYPE_OBJECT);
    }
    return type;
}

gchar *track_command_get_id(TrackCommand *command) {
    if (!TRACK_IS_COMMAND(command))
            return NULL;

    return TRACK_COMMAND_GET_INTERFACE(command)->id;
}

gchar *track_command_get_text(TrackCommand *command) {
    if (!TRACK_IS_COMMAND(command))
            return NULL;

    return TRACK_COMMAND_GET_INTERFACE(command)->text;
}

void track_command_execute(TrackCommand *command, GList *tracks) {
    if (!tracks) {
        return;
    }

    if (!TRACK_IS_COMMAND(command))
        return;

    return TRACK_COMMAND_GET_INTERFACE(command)->execute(tracks);
}

void on_track_command_menuitem_activate(GtkMenuItem *mi, gpointer data) {
    GPtrArray *pairarr = (GPtrArray *) data;

    TrackCommand *cmd = g_ptr_array_index(pairarr, 0);
    GList *tracks = g_ptr_array_index(pairarr, 1);
    track_command_execute(cmd, tracks);
    g_ptr_array_free(pairarr, FALSE);
}
