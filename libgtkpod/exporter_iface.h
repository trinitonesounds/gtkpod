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
#ifndef EXPORTER_IFACE_H_
#define EXPORTER_IFACE_H_

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#include <gtk/gtk.h>
#include "itdb.h"

#define EXPORTER_TYPE                (exporter_get_type ())
#define EXPORTER(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), EXPORTER_TYPE, Exporter))
#define EXPORTER_IS_EXPORTER(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EXPORTER_TYPE))
#define EXPORTER_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), EXPORTER_TYPE, ExporterInterface))

typedef struct _Exporter Exporter;
typedef struct _ExporterInterface ExporterInterface;

struct _ExporterInterface {
    GTypeInterface g_iface;

    void (*export_tracks_as_files) (GList *tracks, GList **filenames, gboolean display, gchar *message);
    void (*export_tracks_to_playlist_file) (GList *tracks);
    GList *(*transfer_track_glist_between_itdbs) (iTunesDB *itdb_s, iTunesDB *itdb_d, GList *tracks);
    GList *(*transfer_track_names_between_itdbs) (iTunesDB *itdb_s, iTunesDB *itdb_d, gchar *data);
};

GType exporter_get_type(void);

void exporter_export_tracks_as_files(Exporter *exporter, GList *tracks, GList **filenames, gboolean display, gchar *message);
void exporter_export_tracks_to_playlist_file (Exporter *exporter, GList *tracks);
GList *exporter_transfer_track_glist_between_itdbs (Exporter *exporter, iTunesDB *itdb_s, iTunesDB *itdb_d, GList *tracks);
GList *exporter_transfer_track_names_between_itdbs (Exporter *exporter, iTunesDB *itdb_s, iTunesDB *itdb_d, gchar *data);

#endif /* EXPORTER_IFACE_H_ */
