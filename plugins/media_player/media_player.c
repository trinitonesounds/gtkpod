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
#include "libgtkpod/prefs.h"
#include "plugin.h"
#include "media_player.h"

#ifndef M_LN10
#define M_LN10 (log(10.0))
#endif

#define MEDIA_PLAYER_VOLUME_KEY "media_player_volume_key"
#define MEDIA_PLAYER_VOLUME_MUTE "media_player_volume_mute"

static MediaPlayer *player;

// Declarations
static void thread_play_song();
static gint thread_stop_song(void *data);
static gint thread_next_song(void *data);

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

static void update_volume(gdouble value) {
    if (!player)
        return;

    if (value < 0) {
        value = 0;
    }

    player->volume_level = value / 10;
    prefs_set_double(MEDIA_PLAYER_VOLUME_KEY, player->volume_level);
    prefs_set_double(MEDIA_PLAYER_VOLUME_MUTE, player->volume_level == 0 ? 1 : 0);

    if (player->play_element) {
        g_object_set(player->play_element, "volume", player->volume_level, NULL);
    }
}

static void set_song_label(Track *track) {
    if (!track) {
        gtk_label_set_markup(GTK_LABEL(player->song_label), "");
        return;
    }

    gchar *label;

    // title by artist from album
    const gchar *track_title = track->title ? track->title : _("No Track Title");
    gboolean have_track_artist = track->artist && strlen(track->artist) > 0;
    gboolean have_track_album = track->album && strlen(track->album) > 0;

    if (have_track_artist && have_track_album)
        label = g_markup_printf_escaped(_("<b>%s</b> by %s from %s"), track_title, track->artist, track->album);
    else if (have_track_artist)
        label = g_markup_printf_escaped(_("<b>%s</b> by %s"), track_title, track->artist);
    else if (have_track_album)
        label = g_markup_printf_escaped(_("<b>%s</b> from %s"), track_title, track->album);
    else
        label = g_markup_printf_escaped("<b>%s</b>", track_title);

    gtk_label_set_markup(GTK_LABEL(player->song_label), label);
    g_object_set_data(G_OBJECT (player->song_label), "tr_title", track->title);
    g_object_set_data(G_OBJECT (player->song_label), "tr_artist", track->artist);
    g_free(label);
}

static void set_control_state(GstState state) {

    Track *tr = g_list_nth_data(player->tracks, player->track_index);
    if (tr) {
        set_song_label(tr);
    }

    switch (state) {
    case GST_STATE_PLAYING:
        gtk_tool_button_set_stock_id(GTK_TOOL_BUTTON(player->play_button), GTK_STOCK_MEDIA_PAUSE);
        break;
    case GST_STATE_PAUSED:
        gtk_tool_button_set_stock_id(GTK_TOOL_BUTTON(player->play_button), GTK_STOCK_MEDIA_PLAY);
        break;
    default:
        gtk_tool_button_set_stock_id(GTK_TOOL_BUTTON(player->play_button), GTK_STOCK_MEDIA_PLAY);
        gtk_range_set_range(GTK_RANGE(player->song_scale), 0, 1);
        gtk_range_set_value(GTK_RANGE(player->song_scale), 0);
        gtk_label_set_text(GTK_LABEL(player->song_time_label), "");
    }
}

static void waitforpipeline(int state) {
    if (!player)
        return;

    if (!player->loop || !player->thread)
        return;

    if (!player->play_element)
        return;

    GstState istate, ipending;
    gst_element_get_state(player->play_element, &istate, &ipending, GST_CLOCK_TIME_NONE);

    if (istate == GST_STATE_VOID_PENDING) {
        return;
    }

    if (istate == state)
        return;

    gst_element_set_state(player->play_element, state);

    do {
        gst_element_get_state(player->play_element, &istate, &ipending, GST_CLOCK_TIME_NONE);

        if (istate == GST_STATE_VOID_PENDING) {
            return;
        }
    }
    while (istate != state);
}

static gboolean is_playing() {
    if (!player || !player->loop || !player->play_element || !player->thread || !g_main_loop_is_running(player->loop))
        return FALSE;

    GstState state, pending;
    gst_element_get_state(player->play_element, &state, &pending, GST_CLOCK_TIME_NONE);

    if (state == GST_STATE_PLAYING)
        return TRUE;

    return FALSE;
}

static gboolean is_paused() {
    if (!player || !player->loop || !player->play_element || !player->thread || !g_main_loop_is_running(player->loop))
        return FALSE;

    GstState state, pending;
    gst_element_get_state(player->play_element, &state, &pending, GST_CLOCK_TIME_NONE);

    if (state == GST_STATE_PAUSED)
        return TRUE;

    return FALSE;
}

static gboolean is_stopped() {
    if (!player || !player->loop || !player->play_element || !player->thread || !g_main_loop_is_running(player->loop)) {
        return TRUE;
    }

    GstState state, pending;
    gst_element_get_state(player->play_element, &state, &pending, GST_CLOCK_TIME_NONE);

    if (state == GST_STATE_NULL)
        return TRUE;

    return FALSE;
}

static void stop_song() {
    if (!player)
        return;

    if (player->loop && g_main_loop_is_running(player->loop)) {
        g_main_loop_quit(player->loop);
    }

    waitforpipeline(GST_STATE_NULL);

    player->thread = NULL;
}

static void pause_or_play_song() {
    if (!player || !player->tracks)
        return;

    if (is_stopped()) {
        GError *err1 = NULL;

        set_control_state(GST_STATE_PLAYING);

        player->thread = g_thread_create ((GThreadFunc)thread_play_song, NULL, TRUE, &err1);
        if (!player->thread) {
            gtkpod_statusbar_message("GStreamer thread creation failed: %s\n", err1->message);
            g_error_free(err1);
        }
    } else if (is_playing()) {
        waitforpipeline(GST_STATE_PAUSED);
        set_control_state(GST_STATE_PAUSED);
    } else if (is_paused()) {
        waitforpipeline(GST_STATE_PLAYING);
        set_control_state(GST_STATE_PLAYING);
    }
}

static void next_song() {
    gboolean playing = is_playing() || is_paused();

    if (playing) {
        stop_song();
    }

    if (player->track_index < g_list_length(player->tracks) - 1) {
        player->track_index++;
    } else {
        player->track_index = 0;
    }

    set_song_label(g_list_nth_data(player->tracks, player->track_index));

    if (playing) {
        pause_or_play_song();
    }
}

static void previous_song() {
    gboolean playing = is_playing() || is_paused();

    if (playing) {
        stop_song();
    }

    if (player->track_index > 0) {
        player->track_index--;
    } else {
        player->track_index = g_list_length(player->tracks) - 1;
    }

    set_song_label(g_list_nth_data(player->tracks, player->track_index));

    if (playing)
        pause_or_play_song();
}

void seek_to_time(gint64 time_seconds) {
    if (is_stopped())
        return;

    if (!gst_element_seek(player->play_element, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, time_seconds
            * 1000000000, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE))
        gtkpod_statusbar_message("Seek failed!\n");
}

static int pipeline_bus_watch_cb(GstBus *bus, GstMessage *msg, gpointer data) {
    switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_EOS:
        g_idle_add(thread_next_song, NULL);
        break;
    case GST_MESSAGE_ERROR: {
        g_idle_add(thread_stop_song, NULL);
        break;
    }
    default:
        break;
    }

    return TRUE;
}

static void thread_play_song() {
    gchar *track_name;
    gchar *uri;
    GstBus *bus;
    GError *error;

    if (!player || !player->tracks)
        return;

    Track *tr = g_list_nth_data(player->tracks, player->track_index);
    if (!tr)
        return;

    error = NULL;
    track_name = get_file_name_from_source(tr, SOURCE_PREFER_LOCAL);
    if (!track_name)
        return;

    /* init GStreamer */
    player->loop = g_main_loop_new(NULL, FALSE); // make new loop
    uri = g_filename_to_uri (track_name, NULL, &error);
    g_free(track_name);
    if (error) {
        gtkpod_statusbar_message("Failed to play track: %s", error->message);
        g_free(uri);
        return;
    }

    player->play_element = gst_element_factory_make("playbin2", "play");
    if (!player->play_element) {
        gtkpod_statusbar_message("Failed to play track: Cannot create a play element. Ensure that all gstreamer plugins are installed");
        return;
    }

    g_object_set(G_OBJECT (player->play_element), "uri", uri, NULL);
    g_object_set(player->play_element, "volume", player->volume_level, NULL);

    bus = gst_pipeline_get_bus(GST_PIPELINE (player->play_element));
    gst_bus_add_watch(bus, pipeline_bus_watch_cb, player->loop); //Add a watch to the bus
    gst_object_unref(bus); //unref the bus

    /* run */
    gst_element_set_state(player->play_element, GST_STATE_PLAYING);// set state
    g_timeout_add(250, (GSourceFunc) set_scale_range, GST_PIPELINE (player->play_element));
    g_timeout_add(1000, (GSourceFunc) set_scale_position, GST_PIPELINE (player->play_element));
    g_main_loop_run(player->loop);

    g_free(uri);

    gst_element_set_state(player->play_element, GST_STATE_NULL);
    g_thread_exit(0);
}

static gint thread_stop_song(void *data) {
    stop_song();
    return FALSE; // call only once
}

static gint thread_next_song(void *data) {
    next_song();
    return FALSE; // call only once
}

void set_selected_tracks(GList *tracks) {
    if (! tracks)
        return;

    if (is_playing() || is_paused())
        return;

    if (player->tracks) {
        g_list_free(player->tracks);
        player->tracks = NULL;
        set_song_label(NULL);
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
    GtkBuilder *builder;
    gst_init_check(0, NULL, NULL);
    srand(time(NULL));

    player = g_new0(MediaPlayer, 1);
    player->glade_path = g_build_filename(get_glade_dir(), "media_player.xml", NULL);
    builder = gtkpod_builder_xml_new(player->glade_path);

    window = gtkpod_builder_xml_get_widget(builder, "media_window");
    player->media_panel = gtkpod_builder_xml_get_widget(builder, "media_panel");
    player->song_label = gtkpod_builder_xml_get_widget(builder, "song_label");
    player->song_time_label = gtkpod_builder_xml_get_widget(builder, "song_time_label");
    player->media_toolbar = gtkpod_builder_xml_get_widget(builder, "media_toolbar");

    player->play_button = gtkpod_builder_xml_get_widget(builder, "play_button");
    player->stop_button = gtkpod_builder_xml_get_widget(builder, "stop_button");
    player->previous_button = gtkpod_builder_xml_get_widget(builder, "previous_button");
    player->next_button = gtkpod_builder_xml_get_widget(builder, "next_button");
    player->song_scale = gtkpod_builder_xml_get_widget(builder, "song_scale");

    g_object_ref(player->media_panel);
    gtk_container_remove(GTK_CONTAINER (window), player->media_panel);
    gtk_widget_destroy(window);

    if (GTK_IS_SCROLLED_WINDOW(parent))
        gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(parent), player->media_panel);
    else
        gtk_container_add(GTK_CONTAINER (parent), player->media_panel);

    gtk_builder_connect_signals(builder, NULL);

    player->thread = NULL;
    player->loop = NULL;
    player->shuffle = FALSE;
    player->play_element = NULL;
    player->track_index = 0;

    /* Set the volume based on preference */
    gint volume_mute = prefs_get_int(MEDIA_PLAYER_VOLUME_MUTE);
    if (volume_mute == 1) {
        player->volume_level = 0;
    }
    else {
        gdouble volume = prefs_get_double(MEDIA_PLAYER_VOLUME_KEY);
        if (volume == 0) {
            /*
             * The preference is at its default value so set it to
             * the default of level 0.5
             */
            player->volume_level = 0.5;
        }
        else {
            player->volume_level = volume;
        }
    }

    gtk_widget_show_all(player->media_panel);

    g_object_unref(builder);
}

void destroy_media_player() {
    gtk_widget_destroy(player->media_panel);
    g_free(player->glade_path);
    player = NULL;
}

static gboolean on_volume_changed_cb(GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data) {
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
    GtkBuilder *builder;
    GtkWidget *vol_window;
    GtkWidget *vol_scale;

    builder = gtkpod_builder_xml_new(player->glade_path);
    vol_window = gtkpod_builder_xml_get_widget(builder, "volume_window");
    vol_scale = gtkpod_builder_xml_get_widget(builder, "volume_scale");
    g_object_set_data(G_OBJECT(vol_window), "scale", vol_scale);

    gtk_range_set_value(GTK_RANGE(vol_scale), (player->volume_level * 10));
    g_signal_connect(G_OBJECT (vol_scale),
            "change-value",
            G_CALLBACK(on_volume_changed_cb),
            NULL);

    g_signal_connect (G_OBJECT (vol_window),
            "focus-out-event",
            G_CALLBACK (on_volume_window_focus_out),
            NULL);

    gtk_widget_show_all(vol_window);
    gtk_widget_grab_focus(vol_window);

    g_object_unref(builder);
}

G_MODULE_EXPORT void on_previous_button_clicked_cb(GtkToolButton *toolbutton, gpointer *userdata) {
    previous_song();
}

G_MODULE_EXPORT void on_play_button_clicked_cb(GtkToolButton *toolbutton, gpointer *userdata) {
    pause_or_play_song();
}

G_MODULE_EXPORT void on_stop_button_clicked_cb(GtkToolButton *toolbutton, gpointer *userdata) {
    stop_song();
}

G_MODULE_EXPORT void on_next_button_clicked_cb(GtkToolButton *toolbutton, gpointer *userdata) {
    next_song();
}

G_MODULE_EXPORT gboolean on_song_scale_change_value_cb(GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data) {
    seek_to_time(value);
    return FALSE;
}

void media_player_play_tracks(GList *tracks) {
    if (!player)
        return;

    if (is_playing())
        stop_song();

    set_selected_tracks(tracks);

    pause_or_play_song();
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

