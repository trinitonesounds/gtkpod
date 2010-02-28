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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "exporter_actions.h"
#include "file_export.h"
#include "libgtkpod/gtkpod_app_iface.h"
#include <gdk/gdk.h>

void on_export_tracks_to_playlist_file(GtkAction *action, ExporterPlugin* plugin) {
    GList *tracks = gtkpod_get_selected_tracks();
    g_return_if_fail(tracks);
    export_playlist_file_init(tracks);
}

void on_export_tracks_to_filesystem(GtkAction *action, ExporterPlugin* plugin) {
    GList *tracks = gtkpod_get_selected_tracks();
    g_return_if_fail(tracks);
    export_files_init(tracks, NULL, FALSE, NULL);
}
