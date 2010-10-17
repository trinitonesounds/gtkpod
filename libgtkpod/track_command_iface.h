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

#ifndef TRACK_COMMAND_IFACE_H_
#define TRACK_COMMAND_IFACE_H_

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#include <gtk/gtk.h>
#include "itdb.h"

#define DEFAULT_TRACK_COMMAND_PREF_KEY "default_track_display_track_command"

typedef struct _TrackCommand TrackCommand;
typedef struct _TrackCommandInterface TrackCommandInterface;

struct _TrackCommandInterface {
    GTypeInterface g_iface;

    gchar *id;
    gchar *text;
    void (*execute)(GList *tracks);
};

GType track_command_get_type(void);

#define TRACK_COMMAND_TYPE                (track_command_get_type ())
#define TRACK_COMMAND(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), TRACK_COMMAND_TYPE, TrackCommand))
#define TRACK_IS_COMMAND(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TRACK_COMMAND_TYPE))
#define TRACK_COMMAND_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), TRACK_COMMAND_TYPE, TrackCommandInterface))

void on_track_command_menuitem_activate(GtkMenuItem *mi, gpointer data);

#endif /* TRACK_COMMAND_IFACE_H_ */
