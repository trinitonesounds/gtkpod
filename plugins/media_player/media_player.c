/* Time-stamp: <2008-01-07 07:07:33 Sonic1>
 |
 |  Copyright (C) 2002-2003 Ryan Houdek <Sonicadvance1 at users.sourceforge.net>
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
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <math.h>
#include <gst/interfaces/xoverlay.h>
#include "libgtkpod/itdb.h"
#include "libgtkpod/file.h"
#include "libgtkpod/directories.h"
#include "libgtkpod/misc.h"
#include "plugin.h"
#include "media_player.h"

#ifndef M_LN10
#define M_LN10 (log(10.0))
#endif

static MediaPlayer *player;

static int pipeline_bus_watch_cb(GstBus *bus, GstMessage *msg, gpointer data);

static gboolean set_scale_range(GstElement *pipeline) {
    GstFormat fmt = GST_FORMAT_TIME;
    gint64 len;

    if (!player)
        return FALSE;

    if (!player->loop || GST_OBJECT_IS_DISPOSING(pipeline))
        return FALSE;

    if (g_main_loop_is_running(player->loop)) {
        if (gst_element_query_duration(pipeline, &fmt, &len)) {
            gtk_range_set_range(GTK_RANGE(player->song_scale), 0, (((GstClockTime) (len)) / GST_SECOND));
            //            if (icon)
            //                gtk_status_icon_set_tooltip(icon, (gchar *) g_object_get_data(G_OBJECT (song_label), "tr_title"));
            return FALSE;
        }
    }
    else
        return FALSE;

    return TRUE;
}

static gboolean set_scale_position(GstElement *pipeline) {
    GstFormat fmt = GST_FORMAT_TIME;
    gint64 pos;
    gint64 len;

    if (player == NULL)
        return FALSE;

    if (!player->loop || !player->thread)
        return FALSE;

    if (!g_main_loop_is_running(player->loop))
        return FALSE;

    if (gst_element_query_position(pipeline, &fmt, &pos)) {
        gint seconds = ((GstClockTime) pos) / GST_SECOND;
        gint length;
        gchar *label;

        gst_element_query_duration(pipeline, &fmt, &len);
        length = ((GstClockTime) len) / GST_SECOND;

        label = g_strdup_printf(_("%d:%02d of %d:%02d"), seconds / 60, seconds % 60, length / 60, length % 60);
        gtk_range_set_value(GTK_RANGE(player->song_scale), seconds);
        gtk_label_set_text(GTK_LABEL(player->song_time_label), label);
        g_free(label);
        return TRUE;
    }

    return FALSE;
}

static void update_volume(gboolean value) {
    if (!player)
        return;

    player->volume_level = exp(value / 20.0 * M_LN10);
    //    g_object_set(player->volume_element, "volume", player->volume_level, NULL);
}

static gboolean volume_changed_cb(GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data) {
    update_volume(value);
    return FALSE;
}

//static void new_decoded_pad_cb(GstElement *decodebin, GstPad *pad, gboolean last, gpointer data) {
//    GstCaps *caps;
//    GstStructure *str;
//    GstPad *audiopad2;
//    //    , *videopad2;
//
//    if (!player)
//        return;
//
//    /* check media type */
//    caps = gst_pad_get_caps(pad);
//    str = gst_caps_get_structure(caps, 0);
//    const gchar *name = gst_structure_get_name(str);
//    if (g_strrstr(name, "audio")) {
//        /* only link once */
//        audiopad2 = gst_element_get_pad(player->audio, "sink");
//        if (GST_PAD_IS_LINKED (audiopad2)) {
//            g_object_unref(audiopad2);
//            return;
//        }
//
//        /* link'n'play */
//        gst_pad_link(pad, audiopad2); // Link audiopad to pad or other way around, dunno
//        //gst_element_link (volume, dec); //Doesn't seem to work...
//        //volume_changed_callback (vol_scale, volume); //Change volume to default
//    }
//
//    if (g_strrstr(name, "video")) {
//        // only link once
//
//        //        videopad2 = gst_element_get_pad(videoconv, "sink");
//        //        if (GST_PAD_IS_LINKED (videopad2)) {
//        //            printf("video pad is linked!unreffing\n");
//        //            g_object_unref(videopad2);
//        //            return;
//        //        }
//        // link'n'play
//        //        gst_pad_link(pad, videopad2);
//        //set_video_mode (TRUE);//Not needed since We can't actually SEE the video
//    }
//
//    gst_caps_unref(caps);
//
//}

static void set_song_label(Track *track) {
    if (!track)
        return;

    gchar *label;

    // title by artist from album
    if (track->title)
        label = g_strdup(track->title);
    else
        label = _("No Track Title");

    if (track->artist && strlen(track->artist) > 0 )
        label = g_strconcat(label, " by ", track->artist, NULL);

    if (track->album && strlen(track->album) > 0)
        label = g_strconcat(label, " from ", track->album, NULL);

    gtk_label_set_text(GTK_LABEL(player->song_label), label);
    g_object_set_data(G_OBJECT (player->song_label), "tr_title", track->title);
    g_object_set_data(G_OBJECT (player->song_label), "tr_artist", track->artist);
    g_free(label);
}

static void thread_play_song() {
    GstStateChangeReturn sret;
    GstState state;
    gchar *track_name;
    gchar *uri;
    GstBus *bus;

    if (!player || !player->tracks)
        return;

    while (player->tracks) {
        Track *tr = player->tracks->data;
        g_return_if_fail(tr);
        track_name = get_file_name_from_source(tr, SOURCE_PREFER_LOCAL);
        if (!track_name)
            continue;

        set_song_label(tr);

        /* init GStreamer */
        player->loop = g_main_loop_new(NULL, FALSE); // make new loop

        uri = g_strconcat("file://", track_name, NULL);
        player->play_element = gst_element_factory_make("playbin2", "play");
        g_object_set(G_OBJECT (player->play_element), "uri", uri, NULL);

        bus = gst_pipeline_get_bus(GST_PIPELINE (player->play_element));
        gst_bus_add_watch(bus, pipeline_bus_watch_cb, player->loop); //Add a watch to the bus
        gst_object_unref(bus); //unref the bus

        /* run */
        gst_element_set_state(player->play_element, GST_STATE_PLAYING);// set state
        g_timeout_add(250, (GSourceFunc) set_scale_range, GST_PIPELINE (player->play_element));
        g_timeout_add(1000, (GSourceFunc) set_scale_position, GST_PIPELINE (player->play_element));
        g_main_loop_run(player->loop);

        /* cleanup */
        sret = gst_element_set_state(player->play_element, GST_STATE_NULL);
#ifndef NEW_PIPE_PER_FILE
        if (GST_STATE_CHANGE_ASYNC == sret) {
            if (gst_element_get_state(GST_ELEMENT (player->play_element), &state, NULL, GST_CLOCK_TIME_NONE)
                    == GST_STATE_CHANGE_FAILURE) {
                break;
            }
        }
#endif
        gst_element_set_state(player->play_element, GST_STATE_NULL);
        g_free(uri);
        g_free(track_name);//Free it since it is no longer needed.


        if (player->stopButtonPressed)
            break;

        if (!player->previousButtonPressed)
            player->tracks = g_list_next(player->tracks);
        else
            player->previousButtonPressed = FALSE;
    }

    player->thread = NULL;
    player->stopButtonPressed = FALSE;
    g_thread_exit(0);
}

static void waitforpipeline(int state) {
    if (!player)
        return;

    if (!player->loop || !player->thread)
        return;

    GstState istate, ipending;
    gst_element_get_state(player->play_element, &istate, &ipending, GST_CLOCK_TIME_NONE);

    if (istate == GST_STATE_VOID_PENDING) {
        return;
    }
    gst_element_set_state(player->play_element, state);

    do {
        gst_element_get_state(player->play_element, &istate, &ipending, GST_CLOCK_TIME_NONE);

        if (istate == GST_STATE_VOID_PENDING) {
            return;
        }
    }
    while (istate != state);

    return;
}

static void stop_song(gboolean stopButtonPressed) {
    if (!player)
        return;

    player->stopButtonPressed = stopButtonPressed;
    if (player->loop && g_main_loop_is_running(player->loop)) {
        g_main_loop_quit(player->loop);
    }

    waitforpipeline(1);
    gtk_range_set_range(GTK_RANGE(player->song_scale), 0, 1);
    gtk_range_set_value(GTK_RANGE(player->song_scale), 0);
    gtk_label_set_text(GTK_LABEL(player->song_time_label), "");
    gtk_tool_button_set_stock_id(GTK_TOOL_BUTTON(player->play_button), GTK_STOCK_MEDIA_PLAY);
}

static void previous_song() {
    if (!player)
        return;

    player->previousButtonPressed = TRUE;
    player->tracks = g_list_previous (player->tracks);
    stop_song(FALSE);
}

static void next_song() {
    stop_song(FALSE);
}

static void play_song() {
    GError *err1 = NULL;

    if (!player)
        return;

    if (!g_thread_supported ()) {
        g_thread_init(NULL);
        gdk_threads_init();
    }

    stop_song(TRUE);

    player->stopButtonPressed = FALSE;
    gtk_tool_button_set_stock_id(GTK_TOOL_BUTTON(player->play_button), GTK_STOCK_MEDIA_PLAY);

    player->thread = g_thread_create ((GThreadFunc)thread_play_song, NULL, TRUE, &err1);
    if (!player->thread) {
        gtkpod_statusbar_message("GStreamer thread creation failed: %s\n", err1->message);
        g_error_free(err1);
    }

    gtk_tool_button_set_stock_id(GTK_TOOL_BUTTON(player->play_button), GTK_STOCK_MEDIA_PAUSE);
}

static void pause_or_play_song() {
    if (!player)
        return;

    if (!player->loop || !player->play_element || !player->thread || !g_main_loop_is_running(player->loop)) {
        play_song();
        return;
    }

    GstState state, pending;
    gst_element_get_state(player->play_element, &state, &pending, GST_CLOCK_TIME_NONE);

    if (state == GST_STATE_PLAYING) {
        gtk_tool_button_set_stock_id(GTK_TOOL_BUTTON(player->play_button), GTK_STOCK_MEDIA_PLAY);
        gst_element_set_state(player->play_element, GST_STATE_PAUSED);
    }
    else if (state == GST_STATE_PAUSED) {
        gtk_tool_button_set_stock_id(GTK_TOOL_BUTTON(player->play_button), GTK_STOCK_MEDIA_PAUSE);
        gst_element_set_state(player->play_element, GST_STATE_PLAYING);
    }
}

void seek_to_time(gint64 time_seconds) {
    if (!player)
        return;

    if (!player->loop || !player->play_element || !player->thread)
        return;

    if (!g_main_loop_is_running(player->loop))
        return;

    if (!gst_element_seek(player->play_element, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, time_seconds
            * 1000000000, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE))
        g_print("Seek failed!\n");
}

static int pipeline_bus_watch_cb(GstBus *bus, GstMessage *msg, gpointer data) {
    switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_EOS:
        stop_song(FALSE);
        break;
    case GST_MESSAGE_ERROR: {
        stop_song(TRUE);
        break;
    }
    default:
        break;
    }

    return TRUE;
}

void set_selected_tracks(GList *tracks) {
    if (!player)
        return;

    stop_song(TRUE);

    if (player->tracks) {
        g_list_free(player->tracks);
        player->tracks = NULL;
    }

    GList *l = g_list_copy(tracks);
    //Does the same thing as generate_random_playlist()
    if (player->shuffle) {
        GRand *grand = g_rand_new();
        while (l || (g_list_length(l) != 0)) {
            /* get random number between 0 and g_list_length()-1 */
            gint rn = g_rand_int_range(grand, 0, g_list_length(l));
            GList *random = g_list_nth(l, rn);
            player->tracks = g_list_append(player->tracks, random->data);
            l = g_list_delete_link(l, random);
        }
        g_rand_free(grand);
    }
    else
        player->tracks = l;

    Track *track = player->tracks->data;
    set_song_label(track);
}

void init_media_player(GtkWidget *parent) {
    GtkWidget *window;
    GladeXML *xml;
    gst_init_check(0, NULL, NULL);
    srand(time(NULL));

    player = g_new0(MediaPlayer, 1);
    player->glade_path = g_build_filename(get_glade_dir(), "media_player.glade", NULL);
    xml = glade_xml_new(player->glade_path, "media_window", NULL);

    window = gtkpod_xml_get_widget(xml, "media_window");
    player->media_panel = gtkpod_xml_get_widget(xml, "media_panel");
    player->song_label = gtkpod_xml_get_widget(xml, "song_label");
    player->song_time_label = gtkpod_xml_get_widget(xml, "song_time_label");
    player->media_toolbar = gtkpod_xml_get_widget(xml, "media_toolbar");

    player->play_button = gtkpod_xml_get_widget(xml, "play_button");
    player->stop_button = gtkpod_xml_get_widget(xml, "stop_button");
    player->previous_button = gtkpod_xml_get_widget(xml, "previous_button");
    player->next_button = gtkpod_xml_get_widget(xml, "next_button");
    player->song_scale = gtkpod_xml_get_widget(xml, "song_scale");

    glade_xml_signal_autoconnect(xml);

    g_object_ref(player->media_panel);
    gtk_container_remove(GTK_CONTAINER (window), player->media_panel);
    gtk_widget_destroy(window);

    if (GTK_IS_SCROLLED_WINDOW(parent))
        gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(parent), player->media_panel);
    else
        gtk_container_add(GTK_CONTAINER (parent), player->media_panel);

    player->thread = NULL;
    player->loop = NULL;
    player->previousButtonPressed = FALSE;
    player->stopButtonPressed = FALSE;
    player->shuffle = FALSE;
    player->play_element = NULL;

    //    gtk_widget_show(player->song_label);
    gtk_widget_show_all(player->media_panel);
//    gtk_widget_realize(player->video_widget);

    g_object_unref(xml);
}

void destroy_media_player() {
    gtk_widget_destroy(player->media_panel);
    g_free(player->glade_path);
    player = NULL;
}

G_MODULE_EXPORT gboolean on_volume_window_focus_out(GtkWidget *widget, GdkEventFocus *event, gpointer data) {
    GtkWidget *vol_scale = (GtkWidget *) g_object_get_data(G_OBJECT(widget), "scale");
    update_volume(gtk_range_get_value(GTK_RANGE(vol_scale)));
    gtk_widget_destroy(widget);
    return TRUE;
}

G_MODULE_EXPORT void on_volume_button_clicked_cb(GtkToolButton *toolbutton, gpointer *userdata) {
    GladeXML *xml;
    GtkWidget *vol_window;
    GtkWidget *vol_scale;

    xml = glade_xml_new(player->glade_path, "volume_window", NULL);
    vol_window = gtkpod_xml_get_widget(xml, "volume_window");
    vol_scale = gtkpod_xml_get_widget(xml, "volume_scale");
    g_object_set_data(G_OBJECT(vol_window), "scale", vol_scale);

    g_message("Volume level: %f", player->volume_level);

    gtk_range_set_value(GTK_RANGE(vol_scale), player->volume_level);
    g_signal_connect(G_OBJECT (vol_scale),
            "change-value",
            G_CALLBACK(volume_changed_cb),
            NULL);

    g_signal_connect (G_OBJECT (vol_window),
            "focus-out-event",
            G_CALLBACK (on_volume_window_focus_out),
            NULL);

    gtk_widget_show_all(vol_window);
    gtk_widget_grab_focus(vol_window);

    g_object_unref(xml);
}

G_MODULE_EXPORT void on_previous_button_clicked_cb(GtkToolButton *toolbutton, gpointer *userdata) {
    previous_song();
}

G_MODULE_EXPORT void on_play_button_clicked_cb(GtkToolButton *toolbutton, gpointer *userdata) {
    pause_or_play_song();
}

G_MODULE_EXPORT void on_stop_button_clicked_cb(GtkToolButton *toolbutton, gpointer *userdata) {
    stop_song(TRUE);
}

G_MODULE_EXPORT void on_next_button_clicked_cb(GtkToolButton *toolbutton, gpointer *userdata) {
    next_song();
}

G_MODULE_EXPORT gboolean on_song_scale_change_value_cb(GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data) {
    seek_to_time(value);
    return FALSE;
}

void media_player_track_removed_cb(GtkPodApp *app, gpointer tk, gpointer data) {
    Track *old_track = tk;
    if (!player)
        return;

    if (g_list_index(player->tracks, old_track) == -1)
        return;

    set_selected_tracks(gtkpod_get_selected_tracks());
}

void media_player_set_tracks_cb(GtkPodApp *app, gpointer tks, gpointer data) {
    GList *tracks = tks;

    if (!player)
        return;

    set_selected_tracks(tracks);
}

void media_player_track_updated_cb(GtkPodApp *app, gpointer tk, gpointer data) {
    Track *track = tk;

    if (!player)
        return;

    if (g_list_index(player->tracks, track) == -1)
        return;

    set_selected_tracks(gtkpod_get_selected_tracks());
}

