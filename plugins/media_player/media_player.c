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
#include <gst/gst.h>
#include <gst/interfaces/xoverlay.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include "libgtkpod/itdb.h"
#include "libgtkpod/file.h"
#include "libgtkpod/directories.h"
#include "libgtkpod/misc.h"
#include "plugin.h"
#include "media_player.h"

static MediaPlayer *player;

GMainLoop *loop;
GstElement *pipeline, *audio, *audiomixer, *volume;
GstElement *src, *dec, *conv, *sink;
GstPad *audiopad;

GstElement *video, *videosink, *videoconv;
GstPad *videopad;

GstBus *bus;
static gboolean prevbut = FALSE;
static gboolean stopall = FALSE;
static gboolean shuffle = TRUE;
static GThread *thread = NULL;
gboolean hasvideo = FALSE;

static int my_bus_callback(GstBus *bus, GstMessage *msg, gpointer data) {
    switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_EOS:
        gtk_range_set_range(GTK_RANGE(player->song_scale), 0, 1);
        gtk_range_set_value(GTK_RANGE(player->song_scale), 0);
        gtk_label_set_text(GTK_LABEL(player->song_label), "");
        g_main_loop_quit(data);
        break;
    case GST_MESSAGE_ERROR: {
        gchar *debug;
        GError *err;

        gst_message_parse_error(msg, &err, &debug);
        g_free(debug);

        g_print("Error: %s\n", err->message);
        g_error_free(err);

        g_main_loop_quit(data);
        break;
    }
    default:
        break;
    }

    return TRUE;
}

static gboolean set_scale_range(GstElement *pipeline) {
    GstFormat fmt = GST_FORMAT_TIME;
    gint64 len;

    if (loop == NULL || GST_OBJECT_IS_DISPOSING(pipeline))
        return FALSE;

    if (loop != NULL || g_main_loop_is_running(loop)) {
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

    if (loop == NULL || thread == NULL || !g_main_loop_is_running(loop))
        return FALSE;

    if (loop != NULL || g_main_loop_is_running(loop)) {
        if (gst_element_query_position(pipeline, &fmt, &pos)) {
            gint seconds = ((GstClockTime) pos) / GST_SECOND;
            gint length;
            gchar *label;

            gst_element_query_duration(pipeline, &fmt, &len);
            length = ((GstClockTime) len) / GST_SECOND;

            label
                    = g_strdup_printf(_("%s (%d:%02d of %d:%02d)"), (gchar *) g_object_get_data(G_OBJECT (player->song_label), "tr_title"), seconds
                            / 60, seconds % 60, length / 60, length % 60);
            gtk_range_set_value(GTK_RANGE(player->song_scale), seconds);
            gtk_label_set_text(GTK_LABEL(player->song_label), label);
            g_free(label);
            return TRUE;
        }
    }

    return FALSE;
}
#ifndef M_LN10
#define M_LN10 (log(10.0))
#endif

static void volume_changed_callback(GtkWidget *widget, GstElement* volume) {
    gdouble value;
    gdouble level;

    value = gtk_range_get_value(GTK_RANGE(widget));
    level = exp(value / 20.0 * M_LN10);
    g_object_set(volume, "volume", level, NULL);
}
static void cb_newpad(GstElement *decodebin, GstPad *pad, gboolean last, gpointer data) {
    GstCaps *caps;
    GstStructure *str;
    GstPad *audiopad2, *videopad2;

    /* check media type */
    caps = gst_pad_get_caps(pad);
    str = gst_caps_get_structure(caps, 0);
    printf("string = %s\n", gst_structure_get_name(str));
    const gchar *name = gst_structure_get_name(str);
    if (g_strrstr(name, "audio")) {
        /* only link once */
        audiopad2 = gst_element_get_pad(audio, "sink");
        if (GST_PAD_IS_LINKED (audiopad2)) {
            printf("audio pad is linked!unreffing\n");
            g_object_unref(audiopad2);
            return;
        }

        /* link'n'play */
        gst_pad_link(pad, audiopad2); // Link audiopad to pad or other way around, dunno
        //gst_element_link (volume, dec); //Doesn't seem to work...
        //volume_changed_callback (vol_scale, volume); //Change volume to default
    }

    if (g_strrstr(name, "video")) {
        // only link once

        videopad2 = gst_element_get_pad(videoconv, "sink");
        if (GST_PAD_IS_LINKED (videopad2)) {
            printf("video pad is linked!unreffing\n");
            g_object_unref(videopad2);
            return;
        }
        // link'n'play
        gst_pad_link(pad, videopad2);
        //set_video_mode (TRUE);//Not needed since We can't actually SEE the video
    }

    gst_caps_unref(caps);

}

static void playsong_real() {
    GstStateChangeReturn sret;
    GstState state;
    gchar *str;

    while (player->tracks) {
        Track *tr = player->tracks->data;
        g_return_if_fail(tr);
        str = get_file_name_from_source(tr, SOURCE_PREFER_LOCAL);
        if (str) {

            gtk_label_set_text(GTK_LABEL(player->song_label), tr->title); // set label to title
            g_object_set_data(G_OBJECT (player->song_label), "tr_title", tr->title);
            g_object_set_data(G_OBJECT (player->song_label), "tr_artist", tr->artist);

            /* init GStreamer */
            loop = g_main_loop_new(NULL, FALSE); // make new loop
            /* setup */
            pipeline = gst_pipeline_new("pipeline"); //Create our pipeline
            volume = gst_element_factory_make("volume", "volume"); // Create volume element
            bus = gst_pipeline_get_bus(GST_PIPELINE (pipeline)); // get pipeline's bus
            gst_bus_add_watch(bus, my_bus_callback, loop); //Add a watch to the bus
            gst_object_unref(bus); //unref the bus

            src = gst_element_factory_make("filesrc", "source"); //create the file source
            g_object_set(G_OBJECT (src), "location", str, NULL); //set location to the file location
            dec = gst_element_factory_make("decodebin", "decoder"); //create our decodebing
            g_signal_connect (dec, "new-decoded-pad", G_CALLBACK (cb_newpad), NULL); //signal to the new-decoded-pad(same as new-pad)
            gst_bin_add_many(GST_BIN (pipeline), src, dec, NULL); //add src and dec to pipeline
            gst_element_link(src, dec); //link src and dec together

            /* create audio output */
            audio = gst_bin_new("audiobin"); //create our audio bin
            conv = gst_element_factory_make("audioconvert", "aconv"); //Create audioconvert element
            audiopad = gst_element_get_pad(conv, "sink"); //get the audioconvert pad
            sink = gst_element_factory_make("alsasink", "sink"); //create our alsasink

            gst_bin_add_many(GST_BIN (audio), conv, sink, volume, NULL); //add volume, conv, and sink to audio
            gst_element_link(conv, sink); // link sink and conv
            gst_element_add_pad(audio, gst_ghost_pad_new("sink", audiopad)); // add pad to audio...?
            gst_object_unref(audiopad); //unref audiopad
            gst_bin_add(GST_BIN (pipeline), audio); //add audio to pipeline

            /*create video output*/

            video = gst_bin_new("videobin"); //create videobin
            videoconv = gst_element_factory_make("ffmpegcolorspace", "vconv"); //make ffmpegcolorspace
            //videopad = gst_element_get_pad (videoconv, "sink"); //get videoconv pad, why the hell am I getting
            //videosink = gst_element_factory_make ("ximagesink", "sink"); //create video sink
            //g_object_set (G_OBJECT (videosink), "force-aspect-ratio", TRUE, NULL); //force aspect ratio
            // gst_x_overlay_set_xwindow_id (GST_X_OVERLAY (videosink), gdk_x11_drawable_get_xid (drawing_area->window)); //set the overlay to our drawing area
            //gst_element_link (videoconv, videosink); //link videoconv and videosink
            //gst_bin_add_many (GST_BIN (video),videoconv, videosink, NULL); //add videoconv and videosink to video
            gst_bin_add(GST_BIN (video), videoconv);
            //gst_object_unref (videopad); //unref videopad
            gst_bin_add(GST_BIN (pipeline), video);
            //[NOTE] If file doesn't contain video, it crashes the program, so commented out

            /* Volume Control */

            //volume_changed_callback (vol_scale, volume);
            //g_signal_connect (vol_scale, "value-changed", G_CALLBACK (volume_changed_callback), volume); //connect volume-changed signal

            /* run */
            gst_element_set_state(pipeline, GST_STATE_PLAYING);// set state
            g_timeout_add(250, (GSourceFunc) set_scale_range, GST_PIPELINE (pipeline));
            g_timeout_add(1000, (GSourceFunc) set_scale_position, GST_PIPELINE (pipeline));
            //g_timeout_add (500, (GSourceFunc) checkinfo, gst_pipeline_get_bus (GST_PIPELINE (pipeline)));
            g_main_loop_run(loop);

            /* cleanup */
            sret = gst_element_set_state(pipeline, GST_STATE_NULL);
#ifndef NEW_PIPE_PER_FILE
            printf("New_pipe_per_file is NOT defined\n");
            if (GST_STATE_CHANGE_ASYNC == sret) {
                if (gst_element_get_state(GST_ELEMENT (pipeline), &state, NULL, GST_CLOCK_TIME_NONE)
                        == GST_STATE_CHANGE_FAILURE) {
                    g_print("State change failed. Aborting");
                    break;
                }
            }
#endif
            gst_element_set_state(pipeline, GST_STATE_NULL);
            g_free(str);//Free it since it is no longer needed.
        }
        if (stopall)
            break;
        if (!prevbut)
            player->tracks = g_list_next(player->tracks);
        else
            prevbut = FALSE;
    }
    thread = NULL;
    stopall = FALSE;
    g_thread_exit(0);
}

void seek_to_time(gint64 time_seconds) {
    if (loop == NULL || pipeline == NULL || thread == NULL || !g_main_loop_is_running(loop))
        return;
    if (!gst_element_seek(pipeline, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, time_seconds
            * 1000000000, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE))
        g_print("Seek failed!\n");
}

static void waitforpipeline(int state) {
    if (pipeline == NULL || thread == NULL)
        return;

    GstState istate, ipending;
    gst_element_get_state(pipeline, &istate, &ipending, GST_CLOCK_TIME_NONE);

    if (istate == GST_STATE_VOID_PENDING) {
        return;
    }
    gst_element_set_state(pipeline, state);

    do {
        gst_element_get_state(pipeline, &istate, &ipending, GST_CLOCK_TIME_NONE);

        if (istate == GST_STATE_VOID_PENDING) {
            return;
        }
    }
    while (istate != state);

    return;
}

static void stop_song() {
    stopall = FALSE;
    if (loop != NULL && g_main_loop_is_running(loop))
        g_main_loop_quit(loop);
    waitforpipeline(1);
    gtk_range_set_range(GTK_RANGE(player->song_scale), 0, 1);
    gtk_range_set_value(GTK_RANGE(player->song_scale), 0);
    gtk_label_set_text(GTK_LABEL(player->song_label), "");
}

void stop_all_songs() {
    stopall = TRUE;
    if (loop != NULL && g_main_loop_is_running(loop)) {
        g_main_loop_quit(loop);
    }
    else
        stopall = FALSE;
    waitforpipeline(1);
    gtk_range_set_range(GTK_RANGE(player->song_scale), 0, 1);
    gtk_range_set_value(GTK_RANGE(player->song_scale), 0);
    gtk_label_set_text(GTK_LABEL(player->song_label), "");
    gtk_tool_button_set_stock_id(GTK_TOOL_BUTTON(player->play_button), GTK_STOCK_MEDIA_PLAY);
    gtk_widget_set_sensitive(player->song_scale, FALSE);
}

void previous_song() {
    prevbut = TRUE;
    player->tracks = g_list_previous (player->tracks);
    stop_song();
    //waitforpipeline (1);
}

void next_song() {
    stop_song();
    //waitforpipeline (1);
}

int play_song() {
    stop_all_songs();
    GError *err1 = NULL;
    if (!g_thread_supported ()) {
        g_thread_init(NULL);
        gdk_threads_init();
    }
    thread = g_thread_create ((GThreadFunc)playsong_real, NULL, TRUE, &err1);
    if (!thread) {
        printf("GStreamer thread creation failed: %s\n", err1->message);
        g_error_free(err1);
    }
    gtk_tool_button_set_stock_id(GTK_TOOL_BUTTON(player->play_button), GTK_STOCK_MEDIA_PAUSE);
    return 0;
}

void pause_song() {
    if (loop == NULL || pipeline == NULL || thread == NULL || !g_main_loop_is_running(loop)) {
        play_song();
    }

    GstState state, pending;
    gst_element_get_state(pipeline, &state, &pending, GST_CLOCK_TIME_NONE);

    if (state == GST_STATE_PLAYING) {
        gst_element_set_state(pipeline, GST_STATE_PAUSED);
        gtk_tool_button_set_stock_id(GTK_TOOL_BUTTON(player->play_button), GTK_STOCK_MEDIA_PLAY);
    }
    else if (state == GST_STATE_PAUSED) {
        gst_element_set_state(pipeline, GST_STATE_PLAYING);
        gtk_tool_button_set_stock_id(GTK_TOOL_BUTTON(player->play_button), GTK_STOCK_MEDIA_PAUSE);
    }
}

void set_selected_tracks(GList *tracks) {
    if (!player)
        return;

    if (player->tracks) {
        g_list_free(player->tracks);
        player->tracks = NULL;
    }

    GList *l = g_list_copy(tracks);
    //Does the same thing as generate_random_playlist()
    if (shuffle) {
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

    Track *tr = player->tracks->data;
    gchar *str = get_file_name_from_source(tr, SOURCE_PREFER_LOCAL);
    if (str) {
        gtk_label_set_text(GTK_LABEL(player->song_label), tr->title); // set label to title
        g_object_set_data(G_OBJECT (player->song_label), "tr_title", tr->title);
        g_object_set_data(G_OBJECT (player->song_label), "tr_artist", tr->artist);
    }
    g_free(str);
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
    player->video_widget = gtkpod_xml_get_widget(xml, "video_widget");
    player->media_panel = gtkpod_xml_get_widget(xml, "media_panel");
    player->song_label = gtkpod_xml_get_widget(xml, "song_label");
    player->song_time_label = gtkpod_xml_get_widget(xml, "song_time_label");
    player->media_toolbar = gtkpod_xml_get_widget(xml, "media_toolbar");

    player->play_button = gtkpod_xml_get_widget(xml, "play_button");
    player->stop_button = gtkpod_xml_get_widget(xml, "stop_button");
    player->previous_button = gtkpod_xml_get_widget(xml, "previous_button");
    player->next_button = gtkpod_xml_get_widget(xml, "next_button");
    player->song_scale = gtkpod_xml_get_widget(xml, "song_scale");

    player->volume = 5;

    glade_xml_signal_autoconnect(xml);

    g_object_ref(player->media_panel);
    gtk_container_remove(GTK_CONTAINER (window), player->media_panel);
    gtk_widget_destroy(window);

    if (GTK_IS_SCROLLED_WINDOW(parent))
        gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(parent), player->media_panel);
    else
        gtk_container_add(GTK_CONTAINER (parent), player->media_panel);

    gtk_widget_show(player->song_label);
    gtk_widget_show_all(player->media_toolbar);

    g_object_unref(xml);
}

void destroy_media_player() {
    gtk_widget_destroy(player->media_panel);
    g_free(player->glade_path);
    player = NULL;
}

static void update_volume(gint volume) {
    player->volume = volume;

}

static gboolean volume_changed_cb(GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data) {
    update_volume(value);
    return FALSE;
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

    gtk_range_set_value(GTK_RANGE(vol_scale), player->volume);
    g_signal_connect(G_OBJECT (vol_scale),
            "change-value",
            G_CALLBACK(volume_changed_cb),
            NULL);


    //    gtk_widget_set_events(vol_window, GDK_FOCUS_CHANGE_MASK);
    g_signal_connect (G_OBJECT (vol_window),
            "focus-out-event",
            G_CALLBACK (on_volume_window_focus_out),
            NULL);

    gtk_widget_show_all(vol_window);
    gtk_widget_grab_focus(vol_window);

    g_object_unref(xml);
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

