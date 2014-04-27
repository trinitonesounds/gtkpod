/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

/*
 * EggPlayPreview GTK+ Widget - egg-play-preview.c
 * 
 * Copyright (C) 2008 Luca Cavalli <luca.cavalli@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Authors: Luca Cavalli <luca.cavalli@gmail.com>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gst/gst.h>

#include "egg-play-preview.h"

enum {
	PROP_NONE,

	PROP_URI,
	PROP_TITLE,
	PROP_ARTIST,
	PROP_ALBUM,
	PROP_DURATION,
	PROP_POSITION
};

enum {
	PLAY_STARTED_SIGNAL,
	PAUSED_SIGNAL,
	STOPPED_SIGNAL,

	LAST_SIGNAL
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), EGG_TYPE_PLAY_PREVIEW, EggPlayPreviewPrivate))

struct _EggPlayPreviewPrivate {

	GtkWidget *title_label;
	GtkWidget *artist_album_label;

	GtkWidget *play_button;
	GtkWidget *play_button_image;

	GtkWidget *time_scale;
	GtkWidget *time_label;

	GstElement *playbin;
	GstState state;

	gchar *play_icon_name;

	gchar *title;
	gchar *artist;
	gchar *album;

	gint duration;
	gint position;

	gint timeout_id;

	gboolean is_seekable;

	gchar *uri;
};

static void	egg_play_preview_class_init   (EggPlayPreviewClass *klass);
static void	egg_play_preview_init         (EggPlayPreview      *play_preview);
static void egg_play_preview_finalize     (GObject             *object);
static void egg_play_preview_dispose      (GObject             *object);
static void	egg_play_preview_set_property (GObject             *object,
										   guint                prop_id,
										   const GValue        *value,
										   GParamSpec          *pspec);
static void	egg_play_preview_get_property (GObject             *object,
										   guint                prop_id,
										   GValue              *value,
										   GParamSpec          *pspec);

static gboolean _timeout_cb                  (EggPlayPreview *play_preview);
static void _ui_update_duration              (EggPlayPreview *play_preview);
static void _ui_update_tags                  (EggPlayPreview *play_preview);
static void _ui_set_sensitive                (EggPlayPreview *play_preview,
											  gboolean sensitive);
static gboolean _change_value_cb             (GtkRange *range,
											  GtkScrollType scroll,
											  gdouble value,
											  EggPlayPreview *play_preview);
static void _clicked_cb                      (GtkButton      *button,
											  EggPlayPreview *play_preview);
static void _setup_pipeline                  (EggPlayPreview *play_preview);
static void _clear_pipeline                  (EggPlayPreview *play_preview);
static gboolean _process_bus_messages        (GstBus         *bus,
											  GstMessage     *msg,
											  EggPlayPreview *play_preview);
static gboolean _query_seeking               (GstElement *element);
static gint _query_duration                  (GstElement *element);
static gint _query_position                  (GstElement *element);
static void _seek                            (GstElement *element,
											  gint position);
static gboolean _is_playing                  (GstState state);
static void _play                            (EggPlayPreview *play_preview);
static void _pause                           (EggPlayPreview *play_preview);
static void _stop                            (EggPlayPreview *play_preview);

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (EggPlayPreview, egg_play_preview, GTK_TYPE_BOX)

static void
egg_play_preview_class_init (EggPlayPreviewClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EggPlayPreviewPrivate));

	gobject_class->finalize = egg_play_preview_finalize;
	gobject_class->dispose = egg_play_preview_dispose;
	gobject_class->set_property = egg_play_preview_set_property;
	gobject_class->get_property = egg_play_preview_get_property;

	signals[PLAY_STARTED_SIGNAL] =
		g_signal_new ("play-started",
					  G_TYPE_FROM_CLASS (klass),
					  G_SIGNAL_RUN_LAST,
					  G_STRUCT_OFFSET (EggPlayPreviewClass, play),
					  NULL, NULL,
					  g_cclosure_marshal_VOID__VOID,
					  G_TYPE_NONE, 0);

	signals[PAUSED_SIGNAL] =
		g_signal_new ("paused",
					  G_TYPE_FROM_CLASS (klass),
					  G_SIGNAL_RUN_LAST,
					  G_STRUCT_OFFSET (EggPlayPreviewClass, pause),
					  NULL, NULL,
					  g_cclosure_marshal_VOID__VOID,
					  G_TYPE_NONE, 0);

	signals[STOPPED_SIGNAL] =
		g_signal_new ("stopped",
					  G_TYPE_FROM_CLASS (klass),
					  G_SIGNAL_RUN_LAST,
					  G_STRUCT_OFFSET (EggPlayPreviewClass, stop),
					  NULL, NULL,
					  g_cclosure_marshal_VOID__VOID,
					  G_TYPE_NONE, 0);

	g_object_class_install_property (gobject_class,
									 PROP_URI,
									 g_param_spec_string ("uri",
														  _("URI"),
														  _("The URI of the audio file"),
														  NULL,
														  G_PARAM_READWRITE |
														  G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
														  G_PARAM_STATIC_BLURB));

	g_object_class_install_property (gobject_class,
									 PROP_TITLE,
									 g_param_spec_string ("title",
														  _("Title"),
														  _("The title of the current stream."),
														  NULL,
														  G_PARAM_READABLE |
														  G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
														  G_PARAM_STATIC_BLURB));

	g_object_class_install_property (gobject_class,
									 PROP_TITLE,
									 g_param_spec_string ("artist",
														  _("Artist"),
														  _("The artist of the current stream."),
														  NULL,
														  G_PARAM_READABLE |
														  G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
														  G_PARAM_STATIC_BLURB));

	g_object_class_install_property (gobject_class,
									 PROP_ALBUM,
									 g_param_spec_string ("album",
														  _("Album"),
														  _("The album of the current stream."),
														  NULL,
														  G_PARAM_READABLE |
														  G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
														  G_PARAM_STATIC_BLURB));

	g_object_class_install_property (gobject_class,
									 PROP_POSITION,
									 g_param_spec_int ("position",
													   _("Position"),
													   _("The position in the current stream in seconds."),
													   0, G_MAXINT, 0,
													   G_PARAM_READWRITE |
													   G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
													   G_PARAM_STATIC_BLURB));

	g_object_class_install_property (gobject_class,
									 PROP_DURATION,
									 g_param_spec_int ("duration",
													   _("Duration"),
													   _("The duration of the current stream in seconds."),
													   0, G_MAXINT, 0,
													   G_PARAM_READABLE |
													   G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
													   G_PARAM_STATIC_BLURB));

	gst_init (NULL, NULL);
}

static void
egg_play_preview_init (EggPlayPreview *play_preview)
{
	EggPlayPreviewPrivate *priv;
	PangoAttribute *bold;
	PangoAttrList *attrs;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *align;

	play_preview->priv = priv = GET_PRIVATE (play_preview);

	_setup_pipeline (play_preview);

	priv->title = NULL;
	priv->album = NULL;

	priv->duration = 0;
	priv->position = 0;
	priv->timeout_id = 0;

	priv->is_seekable = FALSE;

	priv->uri = NULL;

	gtk_box_set_homogeneous (GTK_BOX (play_preview), FALSE);
	gtk_box_set_spacing (GTK_BOX (play_preview), 6);

	/* track info */
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

	priv->title_label = gtk_label_new (NULL);
	gtk_label_set_justify (GTK_LABEL (priv->title_label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment (GTK_MISC (priv->title_label), 0.0, 0.5);
	bold = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
	attrs = pango_attr_list_new ();
	bold->start_index = 0;
	bold->end_index = G_MAXINT;
	pango_attr_list_insert (attrs, bold);
	gtk_label_set_attributes (GTK_LABEL (priv->title_label), attrs);
	pango_attr_list_unref(attrs);
	gtk_box_pack_start (GTK_BOX (vbox), priv->title_label, TRUE, TRUE, 0);

	priv->artist_album_label = gtk_label_new (NULL);
	gtk_label_set_justify (GTK_LABEL (priv->artist_album_label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment (GTK_MISC (priv->artist_album_label), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (vbox), priv->artist_album_label, TRUE, TRUE, 0);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);

	/* play button */
	priv->play_button = gtk_button_new ();
	if (gtk_widget_get_direction (GTK_WIDGET (priv->play_button)) == GTK_TEXT_DIR_RTL)
	    priv->play_icon_name = "media-playback-start-rtl";
	else
	    priv->play_icon_name = "media-playback-start";
	priv->play_button_image = gtk_image_new_from_icon_name (priv->play_icon_name, GTK_ICON_SIZE_BUTTON);
	gtk_container_add (GTK_CONTAINER (priv->play_button), priv->play_button_image);
	align = gtk_alignment_new (0.5, 0.5, 1.0, 0.0);
	gtk_container_add (GTK_CONTAINER (align), priv->play_button);

	/* time scale */
	priv->time_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 100.0, 1.0);
	gtk_scale_set_draw_value (GTK_SCALE (priv->time_scale), FALSE);
	gtk_widget_set_size_request (priv->time_scale, EGG_PLAYER_PREVIEW_WIDTH, -1);
	priv->time_label = gtk_label_new ("0:00");
	gtk_misc_set_alignment (GTK_MISC (priv->time_label), 0.0, 0.5);

	gtk_box_pack_start (GTK_BOX (hbox), align, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), priv->time_scale, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), priv->time_label, FALSE, FALSE, 0);

	g_signal_connect (G_OBJECT (priv->play_button), "clicked",
					  G_CALLBACK (_clicked_cb), play_preview);
	g_signal_connect (G_OBJECT (priv->time_scale), "change-value",
					  G_CALLBACK (_change_value_cb), play_preview);

	gtk_box_pack_start (GTK_BOX (play_preview), vbox, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (play_preview), hbox, FALSE, FALSE, 0);

	_ui_set_sensitive (play_preview, FALSE);

	gtk_widget_show_all (GTK_WIDGET (play_preview));
}

static void
egg_play_preview_finalize (GObject *object)
{
	gst_deinit ();

	G_OBJECT_CLASS (egg_play_preview_parent_class)->finalize (object);
}

static void
egg_play_preview_dispose (GObject *object)
{
	EggPlayPreview *play_preview;
	EggPlayPreviewPrivate *priv;

	play_preview = EGG_PLAY_PREVIEW (object);
	priv = GET_PRIVATE (play_preview);

	_clear_pipeline (play_preview);

	if (priv->title) {
		g_free (priv->title);
		priv->title = NULL;
	}

	if (priv->album) {
		g_free (priv->album);
		priv->album = NULL;
	}

	if (priv->uri) {
		g_free (priv->uri);
		priv->uri = NULL;
	}

	if (priv->timeout_id != 0) {
		g_source_remove (priv->timeout_id);
		priv->timeout_id = 0;
	}
}

static void
egg_play_preview_set_property (GObject       *object,
							   guint          prop_id,
							   const GValue  *value,
							   GParamSpec    *pspec)
{
	EggPlayPreview *play_preview;

	play_preview = EGG_PLAY_PREVIEW (object);

	switch (prop_id) {
    case PROP_URI:
		egg_play_preview_set_uri (play_preview,
								  g_value_get_string (value));
		break;

	case PROP_POSITION:
		egg_play_preview_set_position (play_preview,
									   g_value_get_int (value));
		break;

    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
    }
}

static void 
egg_play_preview_get_property (GObject     *object,
							   guint        prop_id,
							   GValue      *value,
							   GParamSpec  *pspec)
{
	EggPlayPreview *play_preview;

	play_preview = EGG_PLAY_PREVIEW (object);

	switch (prop_id) {
    case PROP_URI:
		g_value_set_string (value,
							egg_play_preview_get_uri (play_preview));
		break;

    case PROP_TITLE:
		g_value_set_string (value,
							egg_play_preview_get_title (play_preview));
		break;

    case PROP_ARTIST:
		g_value_set_string (value,
							egg_play_preview_get_artist (play_preview));
		break;

    case PROP_ALBUM:
		g_value_set_string (value,
							egg_play_preview_get_album (play_preview));
		break;

	case PROP_POSITION:
		g_value_set_int (value,
                         egg_play_preview_get_position (play_preview));
		break;

	case PROP_DURATION:
		g_value_set_int (value,
                         egg_play_preview_get_duration (play_preview));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static gboolean
_timeout_cb (EggPlayPreview *play_preview)
{
	EggPlayPreviewPrivate *priv;

	priv = GET_PRIVATE (play_preview);

	priv->position = _query_position (priv->playbin);
	g_object_notify (G_OBJECT (play_preview), "position");

	gtk_range_set_value (GTK_RANGE (priv->time_scale), priv->position * (100.0 / priv->duration));

	return TRUE;
}

static void
_ui_update_duration (EggPlayPreview *play_preview)
{
	EggPlayPreviewPrivate *priv;
	gchar *str;

	priv = GET_PRIVATE (play_preview);

	str = g_strdup_printf ("%u:%02u",
						   priv->duration / 60,
						   priv->duration % 60);

	gtk_label_set_text (GTK_LABEL (priv->time_label), str);
	g_free (str);
}

static void
_ui_update_tags (EggPlayPreview *play_preview)
{
	EggPlayPreviewPrivate *priv;
	gchar *str;

	priv = GET_PRIVATE (play_preview);

	str = g_strdup_printf ("%s", priv->title ? priv->title : _("Unknown Title"));
	gtk_label_set_text (GTK_LABEL (priv->title_label), str);
	g_free (str);

	str = g_strdup_printf ("%s - %s",
						   priv->artist ? priv->artist : _("Unknown Artist"),
						   priv->album ? priv->album : _("Unknown Album"));
	gtk_label_set_text (GTK_LABEL (priv->artist_album_label), str);
	g_free (str);
}

static void
_ui_set_sensitive (EggPlayPreview *play_preview, gboolean sensitive)
{
	EggPlayPreviewPrivate *priv;

	priv = GET_PRIVATE (play_preview);

	gtk_widget_set_sensitive (priv->play_button, sensitive);
	gtk_widget_set_sensitive (priv->time_scale, sensitive && priv->is_seekable);
}

static gboolean
_change_value_cb (GtkRange *range, GtkScrollType scroll, gdouble value, EggPlayPreview *play_preview)
{
	EggPlayPreviewPrivate *priv;

	priv = GET_PRIVATE (play_preview);

	if (priv->is_seekable)
		_seek (priv->playbin, (gint) ((value / 100.0) * priv->duration));

	return FALSE;
}

static void
_clicked_cb (GtkButton *button, EggPlayPreview *play_preview)
{
	EggPlayPreviewPrivate *priv;

	priv = GET_PRIVATE (play_preview);

	if (priv->playbin == NULL)
		return;

	if (_is_playing (GST_STATE (priv->playbin))) {		
		_pause (play_preview);
		g_signal_emit (play_preview, signals[PAUSED_SIGNAL], 0);
	} else {
		_play (play_preview);
		g_signal_emit (play_preview, signals[PLAY_STARTED_SIGNAL], 0);
	}
}

static void
_setup_pipeline (EggPlayPreview *play_preview)
{
	EggPlayPreviewPrivate *priv;
	GstBus *bus = NULL;
        guint flags;

	priv = GET_PRIVATE (play_preview);

	priv->state = GST_STATE_NULL;

	priv->playbin = gst_element_factory_make ("playbin", "playbin");
	if (!priv->playbin)
		return;

        /* Disable video output */
        g_object_get (G_OBJECT (priv->playbin),
                      "flags", &flags,
                      NULL);
        flags &= ~0x00000001;

	g_object_set (G_OBJECT (priv->playbin),
                      "flags", flags,
				  NULL);

	bus = gst_pipeline_get_bus (GST_PIPELINE (priv->playbin));
	gst_bus_add_watch (bus, (GstBusFunc) _process_bus_messages, play_preview);
	gst_object_unref (bus);

	gst_element_set_state (priv->playbin, GST_STATE_NULL);
}

static void
_clear_pipeline (EggPlayPreview *play_preview)
{
	EggPlayPreviewPrivate *priv;
	GstBus *bus;

	priv = GET_PRIVATE (play_preview);

	if (priv->playbin) {
		bus = gst_pipeline_get_bus (GST_PIPELINE (priv->playbin));
		gst_bus_set_flushing (bus, TRUE);
		gst_object_unref (bus);

		gst_element_set_state (priv->playbin, GST_STATE_NULL);
        gst_object_unref (GST_OBJECT (priv->playbin));
		priv->playbin = NULL;
	}
}

static gboolean
_process_bus_messages (GstBus *bus, GstMessage *msg, EggPlayPreview *play_preview)
{
	EggPlayPreviewPrivate *priv;
	GstFormat format = GST_FORMAT_TIME;
	GstTagList *tag_list;
	gint64 duration;
	GstState state;
	GstStateChangeReturn result;

	priv = GET_PRIVATE (play_preview);

	switch (GST_MESSAGE_TYPE (msg)) {
	case GST_MESSAGE_DURATION:
		gst_message_parse_duration (msg, &format, (gint64*) &duration);

		if (format != GST_FORMAT_TIME)
			break;

		priv->duration = duration / GST_SECOND;

		g_object_notify (G_OBJECT (play_preview), "duration");

		_ui_update_duration (play_preview);
		break;

	case GST_MESSAGE_EOS:
		_stop (play_preview);
		break;

	case GST_MESSAGE_TAG:
		gst_message_parse_tag (msg, &tag_list);
		gst_tag_list_get_string (tag_list, GST_TAG_TITLE, &priv->title);
		gst_tag_list_get_string (tag_list, GST_TAG_ARTIST, &priv->artist);
		gst_tag_list_get_string (tag_list, GST_TAG_ALBUM, &priv->album);

		g_object_notify (G_OBJECT (play_preview), "title");
		g_object_notify (G_OBJECT (play_preview), "artist");
		g_object_notify (G_OBJECT (play_preview), "album");

		_ui_update_tags (play_preview);
		break;

	case GST_MESSAGE_STATE_CHANGED:
		result = gst_element_get_state (GST_ELEMENT (priv->playbin), &state, NULL, 500);

		if (result != GST_STATE_CHANGE_SUCCESS)
			break;

		if (priv->state == state || state < GST_STATE_PAUSED)
			break;

		if (state == GST_STATE_PLAYING) {
			g_signal_emit (G_OBJECT (play_preview), signals[PLAY_STARTED_SIGNAL], 0);
		} else if (state == GST_STATE_PAUSED) {
			g_signal_emit (G_OBJECT (play_preview), signals[PAUSED_SIGNAL], 0);
		} else {
			g_signal_emit (G_OBJECT (play_preview), signals[STOPPED_SIGNAL], 0);
		}

		priv->state = state;
		break;

	default:
		break;
	}

	return TRUE;
}

static gboolean
_query_seeking (GstElement *element)
{
	GstStateChangeReturn result;
	GstState state;
	GstState pending;
	GstQuery *query;
	gboolean seekable;

	seekable = _query_duration (element) > 0;

	result = gst_element_get_state (element, &state, &pending, GST_CLOCK_TIME_NONE);

	if (result == GST_STATE_CHANGE_FAILURE)
		return FALSE;

	if (pending)
		state = pending;

	result = gst_element_set_state (element, GST_STATE_PAUSED);

	if (result == GST_STATE_CHANGE_ASYNC)
		gst_element_get_state (element, NULL, NULL, GST_CLOCK_TIME_NONE);

	query = gst_query_new_seeking (GST_FORMAT_TIME);

	if (gst_element_query (element, query))
		gst_query_parse_seeking (query, NULL, &seekable, NULL, NULL);

	gst_query_unref (query);

	gst_element_set_state (element, state);

	return seekable;
}

static gint
_query_duration (GstElement *element)
{
	GstStateChangeReturn result;
	GstState state;
	GstState pending;
	gint64 duration;

	duration = 0;

	result = gst_element_get_state (element, &state, &pending, GST_CLOCK_TIME_NONE);

	if (result == GST_STATE_CHANGE_FAILURE)
		return 0;

	if (pending)
		state = pending;

	result = gst_element_set_state (element, GST_STATE_PAUSED);

	if (result == GST_STATE_CHANGE_ASYNC)
		gst_element_get_state (element, NULL, NULL, GST_CLOCK_TIME_NONE);

	gst_element_query_duration (element, GST_FORMAT_TIME, &duration);

	gst_element_set_state (element, state);

	return (gint) (duration / GST_SECOND);
}

static gint
_query_position (GstElement *element)
{
	gint64 position;

	position = 0;

	gst_element_query_position (element, GST_FORMAT_TIME, &position);

	return (gint) (position / GST_SECOND);
}

static void
_seek (GstElement *element, gint position)
{
	GstStateChangeReturn result;
	GstState state;

	result = gst_element_get_state (element, &state, NULL, GST_CLOCK_TIME_NONE);

	if (result == GST_STATE_CHANGE_FAILURE)
		return;

	result = gst_element_set_state (element, GST_STATE_PAUSED);

	if (result == GST_STATE_CHANGE_ASYNC) {
		gst_element_get_state (element, NULL, NULL, GST_CLOCK_TIME_NONE);
	}

	gst_element_seek (element, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
					  GST_SEEK_TYPE_SET, position * GST_SECOND,
					  GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);

	gst_element_set_state (element, state);
}

static gboolean
_is_playing (GstState state)
{
	return (state == GST_STATE_PLAYING);
}

static void
_play (EggPlayPreview *play_preview)
{
	EggPlayPreviewPrivate *priv;

	priv = GET_PRIVATE (play_preview);

	gst_element_set_state (priv->playbin, GST_STATE_PLAYING);

	gtk_image_set_from_icon_name (GTK_IMAGE (priv->play_button_image),
	                              "media-playback-pause",
							  GTK_ICON_SIZE_BUTTON);
}

static void
_pause (EggPlayPreview *play_preview)
{
	EggPlayPreviewPrivate *priv;

	priv = GET_PRIVATE (play_preview);

	gst_element_set_state (priv->playbin, GST_STATE_PAUSED);

	gtk_image_set_from_icon_name (GTK_IMAGE (priv->play_button_image),
	                              priv->play_icon_name,
							      GTK_ICON_SIZE_BUTTON);
}

static void
_stop (EggPlayPreview *play_preview)
{
	EggPlayPreviewPrivate *priv;

	priv = GET_PRIVATE (play_preview);

	gst_element_set_state (priv->playbin, GST_STATE_READY);

	gtk_image_set_from_icon_name (GTK_IMAGE (priv->play_button_image),
	                              priv->play_icon_name,
							      GTK_ICON_SIZE_BUTTON);
}

GtkWidget *
egg_play_preview_new (void)
{
	EggPlayPreview *play_preview;

	play_preview = g_object_new (EGG_TYPE_PLAY_PREVIEW, NULL);

	return GTK_WIDGET (play_preview);
}

GtkWidget *
egg_play_preview_new_with_uri (const gchar *uri)
{
	EggPlayPreview *play_preview;

	play_preview = g_object_new (EGG_TYPE_PLAY_PREVIEW, NULL);
	egg_play_preview_set_uri (play_preview, uri);

	return GTK_WIDGET (play_preview);
}

void
egg_play_preview_set_uri (EggPlayPreview *play_preview, const gchar *uri)
{
	EggPlayPreviewPrivate *priv;

	g_return_if_fail (EGG_IS_PLAY_PREVIEW (play_preview));

	priv = GET_PRIVATE (play_preview);

	if (priv->uri) {
		g_free (priv->uri);
		priv->uri = NULL;
		priv->duration = 0;
	}

	if (priv->timeout_id != 0) {
		g_source_remove (priv->timeout_id);
		priv->timeout_id = 0;
	}

	_stop (play_preview);
	priv->is_seekable = FALSE;

	if (gst_uri_is_valid (uri)) {
		priv->uri = g_strdup (uri);

		g_object_set (G_OBJECT (priv->playbin), "uri", uri, NULL);

		priv->duration = _query_duration (priv->playbin);
		priv->is_seekable = _query_seeking (priv->playbin);

		g_object_notify (G_OBJECT (play_preview), "duration");

		_pause (play_preview);

		_ui_set_sensitive (play_preview, TRUE);
		_ui_update_duration (play_preview);
		_ui_update_tags (play_preview);
		priv->timeout_id = g_timeout_add_seconds (1, (GSourceFunc) _timeout_cb, play_preview);
	}

	g_object_notify (G_OBJECT (play_preview), "uri");
}

void
egg_play_preview_set_position (EggPlayPreview *play_preview, gint position)
{
	EggPlayPreviewPrivate *priv;

	g_return_if_fail (EGG_IS_PLAY_PREVIEW (play_preview));

	priv = GET_PRIVATE (play_preview);

	/* FIXME: write function content */
	if (priv->is_seekable) {
		_seek (priv->playbin, MIN (position, priv->duration));

		g_object_notify (G_OBJECT (play_preview), "position");
	}
}

gchar *
egg_play_preview_get_uri (EggPlayPreview *play_preview)
{
	EggPlayPreviewPrivate *priv;

	g_return_val_if_fail (EGG_IS_PLAY_PREVIEW (play_preview), NULL);

	priv = GET_PRIVATE (play_preview);

	return priv->uri;
}

gchar *
egg_play_preview_get_title (EggPlayPreview *play_preview)
{
	EggPlayPreviewPrivate *priv;

	g_return_val_if_fail (EGG_IS_PLAY_PREVIEW (play_preview), NULL);

	priv = GET_PRIVATE (play_preview);

	return priv->title;
}

gchar *
egg_play_preview_get_artist (EggPlayPreview *play_preview)
{
	EggPlayPreviewPrivate *priv;

	g_return_val_if_fail (EGG_IS_PLAY_PREVIEW (play_preview), NULL);

	priv = GET_PRIVATE (play_preview);

	return priv->artist;
}

gchar *
egg_play_preview_get_album (EggPlayPreview *play_preview)
{
	EggPlayPreviewPrivate *priv;

	g_return_val_if_fail (EGG_IS_PLAY_PREVIEW (play_preview), NULL);

	priv = GET_PRIVATE (play_preview);

	return priv->album;
}

gint
egg_play_preview_get_position (EggPlayPreview *play_preview)
{
	EggPlayPreviewPrivate *priv;

	g_return_val_if_fail (EGG_IS_PLAY_PREVIEW (play_preview), 0);

	priv = GET_PRIVATE (play_preview);

	return priv->position;
}

gint
egg_play_preview_get_duration (EggPlayPreview *play_preview)
{
	EggPlayPreviewPrivate *priv;

	g_return_val_if_fail (EGG_IS_PLAY_PREVIEW (play_preview), -1);

	priv = GET_PRIVATE (play_preview);

	return priv->duration;
}
