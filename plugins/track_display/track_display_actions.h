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
#ifndef __TRACK_DISPLAY_ACTIONS_H__
#define __TRACK_DISPLAY_ACTIONS_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include "plugin.h"

void on_delete_selected_tracks_from_playlist(GtkAction *action, TrackDisplayPlugin* plugin);
void on_delete_selected_tracks_from_database(GtkAction *action, TrackDisplayPlugin* plugin);
void on_delete_selected_tracks_from_harddisk(GtkAction *action, TrackDisplayPlugin* plugin);
void on_delete_selected_tracks_from_ipod(GtkAction *action, TrackDisplayPlugin* plugin);
void on_delete_selected_tracks_from_device(GtkAction *action, TrackDisplayPlugin* plugin);
void on_update_selected_tracks (GtkAction *action, TrackDisplayPlugin* plugin);
void on_update_mserv_selected_tracks (GtkAction *action, TrackDisplayPlugin* plugin);
#endif
