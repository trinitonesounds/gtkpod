/* Time-stamp: <2008-01-07 07:07:33 Sonic1>
|
|  Copyright (C) 2002-2010 Ryan Houdek <Sonicadvance1 at users.sourceforge.net>
|  Modified by Paul Richardson <phantom_sf at users.sourceforge.net>
|  Part of the gtkpod project.
|
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
|  $Id: oggfile.h 954 2007-01-16 09:45:00Z jcsjcs $
*/

#ifndef __MEDIA_PLAYER_H__
#define __MEDIA_PLAYER_H__

#include <gtk/gtk.h>
#include <gst/gst.h>

typedef struct {
    GtkWidget *media_panel;
    GtkWidget *video_widget;
    GtkWidget *song_label;
    GtkWidget *song_time_label;
    GtkWidget *media_toolbar;

    GtkWidget *previous_button;
    GtkWidget *play_button;
    GtkWidget *stop_button;
    GtkWidget *next_button;
    GtkWidget *song_scale;

    gchar *glade_path;

    GList *tracks;
    GThread *thread;
    GMainLoop *loop;

    gboolean previousButtonPressed;
    gboolean stopButtonPressed;
    gboolean shuffle;

    gdouble volume_level;
    GstElement *volume_element;
    GstElement *pipeline;
    GstElement *audio;
    GstElement *audiomixer;
    GstElement *volume;
    GstElement *src;
    GstElement *dec;
    GstElement *conv;
    GstElement *sink;
    GstPad *audiopad;

} MediaPlayer;

void init_media_player (GtkWidget *parent);
void destroy_media_player();

void media_player_track_removed_cb(GtkPodApp *app, gpointer tk, gpointer data);
void media_player_set_tracks_cb(GtkPodApp *app, gpointer tks, gpointer data);
void media_player_track_updated_cb(GtkPodApp *app, gpointer tk, gpointer data);

#endif
