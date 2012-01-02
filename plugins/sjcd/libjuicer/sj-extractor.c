/* 
 * Copyright (C) 2003-2007 Ross Burton <ross@burtonini.com>
 *
 * Sound Juicer - sj-extractor.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors: Ross Burton <ross@burtonini.com>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <gst/gst.h>
#include <gst/tag/tag.h>
#include "sj-extractor.h"
#include "sj-structures.h"
#include "sj-error.h"
#include "sj-util.h"


/* Properties */
enum {
  PROP_0,
  PROP_PROFILE,
  PROP_PARANOIA,
  PROP_DEVICE,
};

/* Signals */
enum {
  PROGRESS,
  COMPLETION,
  ERROR,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

/* Default profile name */
#define DEFAULT_MEDIA_TYPE "audio/x-vorbis"

/* Element names */
#define FILE_SINK "giosink"

struct SjExtractorPrivate {
  /** The current audio profile */
  GstEncodingProfile *profile;
  /** If the pipeline needs to be re-created */
  gboolean rebuild_pipeline;
  /* The gstreamer pipeline elements */
  GstElement *pipeline, *cdsrc, *encodebin, *filesink;
  GstFormat track_format;
  char *device_path;
  int paranoia_mode;
  int seconds;
  GError *construct_error;
  guint tick_id;
};

/*
 * GObject methods
 */

G_DEFINE_TYPE (SjExtractor, sj_extractor, G_TYPE_OBJECT);

#define EXTRACTOR_PRIVATE(o)                                            \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), SJ_TYPE_EXTRACTOR, SjExtractorPrivate))

static void
sj_extractor_set_property (GObject *object, guint property_id,
                                       const GValue *value, GParamSpec *pspec)
{
  GstEncodingProfile *profile;
  SjExtractorPrivate *priv = SJ_EXTRACTOR (object)->priv;
  switch (property_id) {
  case PROP_PROFILE:
    if (priv->profile)
      gst_encoding_profile_unref (priv->profile);
    profile = GST_ENCODING_PROFILE (g_value_get_pointer (value));
    priv->profile = GST_ENCODING_PROFILE(gst_encoding_profile_ref (profile));
    priv->rebuild_pipeline = TRUE;
    g_object_notify (object, "profile");
    break;
  case PROP_PARANOIA:
    priv->paranoia_mode = g_value_get_int (value);
    if (priv->cdsrc)
      g_object_set (G_OBJECT (priv->cdsrc),
                    "paranoia-mode", priv->paranoia_mode,
                    NULL);
    break;
  case PROP_DEVICE:
    /* We need to cache this as the source will be recreated */
    g_free (priv->device_path);
    priv->device_path = g_value_dup_string (value);
    if (priv->cdsrc != NULL)
      g_object_set (G_OBJECT (priv->cdsrc),
                    "device", priv->device_path,
                    NULL);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
sj_extractor_get_property (GObject *object, guint property_id,
                                       GValue *value, GParamSpec *pspec)
{
  SjExtractorPrivate *priv = SJ_EXTRACTOR (object)->priv;
  switch (property_id) {
  case PROP_PROFILE:
    g_value_set_pointer (value, gst_encoding_profile_ref (priv->profile));
    break;
  case PROP_DEVICE:
    g_value_set_string (value, priv->device_path);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
sj_extractor_dispose (GObject *object)
{
  SjExtractorPrivate *priv = SJ_EXTRACTOR (object)->priv;

  if (priv->profile) {
    gst_encoding_profile_unref (priv->profile);
    priv->profile = NULL;
  }

  if (priv->pipeline) {
    gst_element_set_state (priv->pipeline, GST_STATE_NULL);
    g_object_unref (priv->pipeline);
    priv->pipeline = NULL;
  }

  G_OBJECT_CLASS (sj_extractor_parent_class)->dispose (object);
}

static void
sj_extractor_finalize (GObject *object)
{
  SjExtractorPrivate *priv = SJ_EXTRACTOR (object)->priv;
  
  if (priv->tick_id)
    g_source_remove (priv->tick_id);

  g_free (priv->device_path);
  
  if (priv->construct_error)
    g_error_free (priv->construct_error);

  G_OBJECT_CLASS (sj_extractor_parent_class)->finalize (object);
}

static void
sj_extractor_class_init (SjExtractorClass *klass)
{
  GObjectClass *object_class;
  object_class = (GObjectClass *)klass;

  g_type_class_add_private (klass, sizeof (SjExtractorPrivate));

  /* GObject */
  object_class->set_property = sj_extractor_set_property;
  object_class->get_property = sj_extractor_get_property;
  object_class->dispose = sj_extractor_dispose;
  object_class->finalize = sj_extractor_finalize;

  /* Properties */
  /* TODO: make these constructors */
  g_object_class_install_property (object_class, PROP_PROFILE,
                                   g_param_spec_pointer ("profile",
                                                         _("Audio Profile"),
                                                         _("The GStreamer Encoding Profile used for encoding audio"),
                                                         G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_PARANOIA,
                                   g_param_spec_int ("paranoia",
                                                     _("Paranoia Level"),
                                                     _("The paranoia level"),
                                                      0, 255, 8, G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_DEVICE,
                                   g_param_spec_string ("device",
                                                        _("device"),
                                                        _("The device"),
                                                        "",
                                                        G_PARAM_READWRITE));
							
  /* Signals */
  signals[PROGRESS] =
    g_signal_new ("progress",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (SjExtractorClass, progress),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__INT,
                  G_TYPE_NONE, 1, G_TYPE_INT);
  signals[COMPLETION] =
    g_signal_new ("completion",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (SjExtractorClass, completion),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
  signals[ERROR] =
    g_signal_new ("error",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (SjExtractorClass, error),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1, G_TYPE_POINTER);
}


static void
sj_extractor_init (SjExtractor *extractor)
{
  extractor->priv = EXTRACTOR_PRIVATE (extractor);
  extractor->priv->profile = rb_gst_get_encoding_profile (DEFAULT_MEDIA_TYPE);
  extractor->priv->rebuild_pipeline = TRUE;
  extractor->priv->paranoia_mode = 8; /* TODO: replace with construct params */
}

/*
 * Private Methods
 */

static void
eos_cb (GstBus *bus, GstMessage *message, gpointer user_data)
{
  SjExtractor *extractor = SJ_EXTRACTOR (user_data);
  SjExtractorPrivate *priv = extractor->priv;

  gst_element_set_state (priv->pipeline, GST_STATE_NULL);

  if (priv->tick_id) {
    g_source_remove (priv->tick_id);
    priv->tick_id = 0;
  }

  /* TODO: shouldn't need to do this, see #327197 */
  priv->rebuild_pipeline = TRUE;

  g_signal_emit (extractor, signals[COMPLETION], 0);
}

static GstElement*
build_encoder (SjExtractor *extractor)
{
  SjExtractorPrivate *priv;
  GstElement *encodebin;

  g_return_val_if_fail (SJ_IS_EXTRACTOR (extractor), NULL);
  priv = (SjExtractorPrivate*)extractor->priv;
  g_return_val_if_fail (priv->profile != NULL, NULL);

  encodebin = gst_element_factory_make ("encodebin", NULL);
  if (encodebin == NULL)
    return NULL;
  g_object_set (encodebin, "profile", priv->profile, NULL);
  /* Nice big buffers... */
  g_object_set (encodebin, "queue-time-max", 120 * GST_SECOND, NULL);

  return encodebin;
}

static void
error_cb (GstBus *bus, GstMessage *message, gpointer user_data)
{
  SjExtractor *extractor = SJ_EXTRACTOR (user_data);
  SjExtractorPrivate *priv = extractor->priv;
  GError *error = NULL;

  /* Make sure the pipeline is not running any more */
  gst_element_set_state (priv->pipeline, GST_STATE_NULL);
  extractor->priv->rebuild_pipeline = TRUE;

  if (priv->tick_id) {
    g_source_remove (priv->tick_id);
    priv->tick_id = 0;
  }

  gst_message_parse_error (message, &error, NULL);
  g_signal_emit (extractor, signals[ERROR], 0, error);
  g_error_free (error);
}

#if 0
/**
 * Callback from the giosink to say that its about to overwrite a file.
 * For now, Just Say Yes.  If this API will stay in 0.9, then rewrite
 * SjExtractor.
 */
static gboolean
just_say_yes (GstElement *element, gpointer filename, gpointer user_data)
{
  return TRUE;
}
#endif

static void
build_pipeline (SjExtractor *extractor)
{
  SjExtractorPrivate *priv;
  GstBus *bus;

  g_return_if_fail (SJ_IS_EXTRACTOR (extractor));

  priv = extractor->priv;

  if (priv->pipeline != NULL) {
    gst_object_unref (GST_OBJECT (priv->pipeline));
  }
  priv->pipeline = gst_pipeline_new ("pipeline");
  bus = gst_element_get_bus (priv->pipeline);
  gst_bus_add_signal_watch (bus);

  g_signal_connect (G_OBJECT (bus), "message::error", G_CALLBACK (error_cb), extractor);

  /* Read from CD */
  priv->cdsrc = gst_element_make_from_uri (GST_URI_SRC, "cdda://1", "cd_src");
  if (priv->cdsrc == NULL) {
    g_set_error (&priv->construct_error,
                 SJ_ERROR, SJ_ERROR_INTERNAL_ERROR,
                 _("Could not create GStreamer CD reader"));
    return;
  }

  g_object_set (G_OBJECT (priv->cdsrc), "device", priv->device_path, NULL);
  if (g_object_class_find_property (G_OBJECT_GET_CLASS (priv->cdsrc), "paranoia-mode")) {
	  g_object_set (G_OBJECT (priv->cdsrc), "paranoia-mode", priv->paranoia_mode, NULL);
  }

  /* Get the track format for seeking later */
  priv->track_format = gst_format_get_by_nick ("track");
  g_assert (priv->track_format != 0);

  /* Encode */
  priv->encodebin = build_encoder (extractor);
  if (priv->encodebin == NULL) {
    g_set_error (&priv->construct_error,
                 SJ_ERROR, SJ_ERROR_INTERNAL_ERROR,
                 _("Could not create GStreamer encoders for %s"),
                 gst_encoding_profile_get_name (priv->profile));
    return;
  }
  /* Connect to the eos so we know when its finished */
  g_signal_connect (bus, "message::eos", G_CALLBACK (eos_cb), extractor);

  /* Write to disk */
  priv->filesink = gst_element_factory_make (FILE_SINK, "file_sink");
  if (priv->filesink == NULL) {
    g_set_error (&priv->construct_error,
                 SJ_ERROR, SJ_ERROR_INTERNAL_ERROR,
                 _("Could not create GStreamer file output"));
    return;
  }
#if 0
  g_signal_connect (G_OBJECT (priv->filesink), "allow-overwrite", G_CALLBACK (just_say_yes), extractor);
#endif

  /* Add the elements to the pipeline */
  gst_bin_add_many (GST_BIN (priv->pipeline), priv->cdsrc, priv->encodebin, priv->filesink, NULL);

  /* Link it all together */
  if (!gst_element_link_many (priv->cdsrc, priv->encodebin, priv->filesink, NULL)) {
    g_set_error (&priv->construct_error,
                 SJ_ERROR, SJ_ERROR_INTERNAL_ERROR,
                 _("Could not link pipeline"));
    return;
  }

  priv->rebuild_pipeline = FALSE;
}

static gboolean
tick_timeout_cb(SjExtractor *extractor)
{
  gint64 nanos;
  gint secs;
  GstState state, pending_state;
  static GstFormat format = GST_FORMAT_TIME;

  g_return_val_if_fail (SJ_IS_EXTRACTOR (extractor), FALSE);

  gst_element_get_state (extractor->priv->pipeline, &state, &pending_state, 0);
  if (state != GST_STATE_PLAYING && pending_state != GST_STATE_PLAYING) {
    extractor->priv->tick_id = 0;
    return FALSE;
  }

  if (!gst_element_query_position (extractor->priv->cdsrc, &format, &nanos)) {
    g_warning (_("Could not get current track position"));
    return TRUE;
  }

  secs = nanos / GST_SECOND;
  if (secs != extractor->priv->seconds) {
    g_signal_emit (extractor, signals[PROGRESS], 0, secs);
  }

  return TRUE;
}

/*
 * Public Methods
 */

GObject *
sj_extractor_new (void)
{
  return g_object_new (SJ_TYPE_EXTRACTOR, NULL);
}

GError *
sj_extractor_get_new_error (SjExtractor *extractor)
{
  GError *error;
  if (extractor == NULL || extractor->priv == NULL) {
    g_set_error (&error,
                 SJ_ERROR, SJ_ERROR_INTERNAL_ERROR,
                 _("Extractor object is not valid. This is bad, check your console for errors."));
    return error;
  }
  return extractor->priv->construct_error;
}

void
sj_extractor_set_device (SjExtractor *extractor, const char* device)
{
  g_return_if_fail (SJ_IS_EXTRACTOR (extractor));
  g_return_if_fail (device != NULL);
  
  g_object_set (extractor, "device", device, NULL);
}

void
sj_extractor_set_paranoia (SjExtractor *extractor, const int paranoia_mode)
{
  g_return_if_fail (SJ_IS_EXTRACTOR (extractor));
  
  g_object_set (extractor, "paranoia", paranoia_mode, NULL);
}

void
sj_extractor_extract_track (SjExtractor *extractor, const TrackDetails *track, GFile *file, GError **error)
{
  GParamSpec *spec;
  GstStateChangeReturn state_ret;
  SjExtractorPrivate *priv;
  GstIterator *iter;
  GstTagSetter *tagger;
  gboolean done;
  char *uri;

  g_return_if_fail (SJ_IS_EXTRACTOR (extractor));

  g_return_if_fail (file != NULL);
  g_return_if_fail (track != NULL);

  priv = extractor->priv;

  /* See if we need to rebuild the pipeline */
  if (priv->rebuild_pipeline != FALSE) {
    build_pipeline (extractor);
    if (priv->construct_error != NULL) {
      g_propagate_error (error, priv->construct_error);
      priv->construct_error = NULL;
      return;
    }
  }

  /* Need to do this, as playback will have locked the read speed to 2x previously */
  spec = g_object_class_find_property (G_OBJECT_GET_CLASS (priv->cdsrc), "read-speed");
  if (spec && spec->value_type == G_TYPE_INT) {
    g_object_set (G_OBJECT (priv->cdsrc), "read-speed", ((GParamSpecInt*)spec)->maximum, NULL);
  }

  /* Set the output filename */
  gst_element_set_state (priv->filesink, GST_STATE_NULL);
  uri = g_file_get_uri (file);
  g_object_set (G_OBJECT (priv->filesink), "location", uri, NULL);
  g_free (uri);

  /* Set the metadata */
  iter = gst_bin_iterate_all_by_interface (GST_BIN (priv->pipeline), GST_TYPE_TAG_SETTER);
  done = FALSE;
  while (!done) {
    switch (gst_iterator_next (iter, (gpointer)&tagger)) {
    case GST_ITERATOR_OK:
      /* TODO: generate this as a taglist once, and apply it to all elements */
      gst_tag_setter_add_tags (tagger,
                               GST_TAG_MERGE_REPLACE_ALL,
                               GST_TAG_TITLE, track->title,
                               GST_TAG_ARTIST, track->artist,
                               GST_TAG_TRACK_NUMBER, track->number,
                               GST_TAG_TRACK_COUNT, track->album->number,
                               GST_TAG_ALBUM, track->album->title,
                               GST_TAG_DURATION, track->duration * GST_SECOND,
                               NULL);

      if (track->album->album_id != NULL && strcmp (track->album->album_id, "") != 0) {
        gst_tag_setter_add_tags (tagger,
                            GST_TAG_MERGE_APPEND,
                            GST_TAG_MUSICBRAINZ_ALBUMID, track->album->album_id,
                            NULL);
      }
      if (track->album->artist_id != NULL && strcmp (track->album->artist_id, "") != 0) {
        gst_tag_setter_add_tags (tagger,
                            GST_TAG_MERGE_APPEND,
                            GST_TAG_MUSICBRAINZ_ALBUMARTISTID, track->album->artist_id,
                            NULL);
      }
      if (track->album->artist != NULL && strcmp (track->album->artist, "") != 0) {
        gst_tag_setter_add_tags (tagger,
                            GST_TAG_MERGE_APPEND,
                            GST_TAG_ALBUM_ARTIST, track->album->artist,
                            NULL);
      }
      if (track->album->artist_sortname != NULL && strcmp (track->album->artist_sortname, "") != 0) {
        gst_tag_setter_add_tags (tagger,
                            GST_TAG_MERGE_APPEND,
                            GST_TAG_ALBUM_ARTIST_SORTNAME, track->album->artist_sortname,
                            NULL);
      }
      if (track->artist_id != NULL && strcmp (track->artist_id, "") != 0) {
        gst_tag_setter_add_tags (tagger,
                            GST_TAG_MERGE_APPEND,
                            GST_TAG_MUSICBRAINZ_ARTISTID, track->artist_id,
                            NULL);
      }
      if (track->track_id != NULL && strcmp (track->track_id, "") != 0) {
        gst_tag_setter_add_tags (tagger,
                            GST_TAG_MERGE_APPEND,
                            GST_TAG_MUSICBRAINZ_TRACKID, track->track_id,
                            NULL);
      }
      if (track->artist_sortname != NULL && strcmp (track->artist_sortname, "") != 0) {
        gst_tag_setter_add_tags (tagger,
                            GST_TAG_MERGE_APPEND,
                            GST_TAG_ARTIST_SORTNAME, track->artist_sortname,
                            NULL);
      }

      if (track->album->genre != NULL && strcmp (track->album->genre, "") != 0) {
        char **values, **l;
        values = g_strsplit (track->album->genre, ",", 0);
        for (l = values; *l; l++) {
          g_strstrip (*l);
          gst_tag_setter_add_tags (tagger,
                                   GST_TAG_MERGE_APPEND,
                                   GST_TAG_GENRE, *l,
                                   NULL);
        }
        g_strfreev (values);
      }
      if (track->album->release_date) {
        gst_tag_setter_add_tags (tagger,
                                 GST_TAG_MERGE_APPEND,
                                 GST_TAG_DATE, track->album->release_date,
                                 NULL);
      }
      if (track->album->disc_number > 0) {
        gst_tag_setter_add_tags (tagger,
                                 GST_TAG_MERGE_APPEND,
                                 GST_TAG_ALBUM_VOLUME_NUMBER, track->album->disc_number,
                                 NULL);
      }
      gst_object_unref (tagger);
      break;
    case GST_ITERATOR_RESYNC:
      /* TODO? */
      g_warning ("Got GST_ITERATOR_RESYNC, not sure what to do");
      gst_iterator_resync (iter);
      break;
    case GST_ITERATOR_ERROR:
      done = TRUE;
      break;
    case GST_ITERATOR_DONE:
      done = TRUE;
      break;
    }
  }
  gst_iterator_free (iter);

  /* Seek to the right track */
  g_object_set (G_OBJECT (priv->cdsrc), "track", track->number, NULL);
  
  /* Let's get ready to rumble! */
  state_ret = gst_element_set_state (priv->pipeline, GST_STATE_PLAYING);

  if (state_ret == GST_STATE_CHANGE_ASYNC) {
    /* Wait for state change to either complete or fail, but not for too long,
     * just to catch immediate errors. The rest we'll handle asynchronously */
    state_ret = gst_element_get_state (priv->pipeline, NULL, NULL, GST_SECOND / 2);
  }

  if (state_ret == GST_STATE_CHANGE_FAILURE) {
    GstMessage *msg;

    msg = gst_bus_poll (GST_ELEMENT_BUS (priv->pipeline), GST_MESSAGE_ERROR, 0);
    if (msg) {
      gst_message_parse_error (msg, error, NULL);
      gst_message_unref (msg);
    } else if (error) {
      /* this should never happen, create generic error just in case */
      *error = g_error_new (SJ_ERROR, SJ_ERROR_INTERNAL_ERROR,
                            "Error starting ripping pipeline");
    }
    gst_element_set_state (priv->pipeline, GST_STATE_NULL);
    priv->rebuild_pipeline = TRUE;
    return;
  }

  priv->tick_id = g_timeout_add (250, (GSourceFunc)tick_timeout_cb, extractor);
}

void
sj_extractor_cancel_extract (SjExtractor *extractor)
{
  GstState state;

  g_return_if_fail (SJ_IS_EXTRACTOR (extractor));

  gst_element_get_state (extractor->priv->pipeline, &state, NULL, GST_CLOCK_TIME_NONE);
  if (state != GST_STATE_PLAYING) {
    return;
  }
  gst_element_set_state (extractor->priv->pipeline, GST_STATE_NULL);

  extractor->priv->rebuild_pipeline = TRUE;
}

gboolean
sj_extractor_supports_encoding (GError **error)
{
  GstElement *element = NULL;

  element = gst_element_make_from_uri (GST_URI_SRC, "cdda://1", "test");
  if (element == NULL) {
    g_set_error (error, SJ_ERROR, SJ_ERROR_INTERNAL_ERROR,
                 _("The plugin necessary for CD access was not found"));
    return FALSE;
  }
  g_object_unref (element);

  element = gst_element_factory_make (FILE_SINK, "test");
  if (element == NULL) {
    g_set_error (error, SJ_ERROR, SJ_ERROR_INTERNAL_ERROR,
                 _("The plugin necessary for file access was not found"));
    return FALSE;
  }
  g_object_unref (element);

  return TRUE;
}

gboolean
sj_extractor_supports_profile (GstEncodingProfile *profile)
{
  /* TODO: take a GError to return a message if the profile isn't supported */
  return !rb_gst_check_missing_plugins(profile, NULL, NULL);
}
