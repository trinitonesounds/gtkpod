/*
 *  Copyright (C) 2010  Jonathan Matthew  <jonathan@d14n.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  The Rhythmbox authors hereby grant permission for non-GPL compatible
 *  GStreamer plugins to be used and distributed together with GStreamer
 *  and Rhythmbox. This permission is above and beyond the permissions granted
 *  by the GPL license by which Rhythmbox is covered. If you modify this code
 *  you may extend this exception to your version of the code, but you are not
 *  obligated to do so. If you do not wish to do so, delete this exception
 *  statement from your version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA.
 *
 */

#include "config.h"

#include <memory.h>

#include <gst/pbutils/encoding-target.h>
#include <gst/pbutils/missing-plugins.h>

#include "rb-gst-media-types.h"

#define SOURCE_ENCODING_TARGET_FILE "../data/rhythmbox.gep"
#define INSTALLED_ENCODING_TARGET_FILE DATADIR"/sound-juicer/rhythmbox.gep"
static GstEncodingTarget *default_target = NULL;

char *
rb_gst_caps_to_media_type (const GstCaps *caps)
{
	GstStructure *s;
	const char *media_type;

	/* the aim here is to reduce the caps to a single mimetype-like
	 * string, describing the audio encoding (for audio files) or the
	 * file type (for everything else).  raw media types are ignored.
	 *
	 * there are only a couple of special cases.
	 */

	if (gst_caps_get_size (caps) == 0)
		return NULL;

	s = gst_caps_get_structure (caps, 0);
	media_type = gst_structure_get_name (s);
	if (media_type == NULL) {
		return NULL;
	} else if (g_str_has_prefix (media_type, "audio/x-raw-") ||
	    g_str_has_prefix (media_type, "video/x-raw-")) {
		/* ignore raw types */
		return NULL;
	} else if (g_str_equal (media_type, "audio/mpeg")) {
		/* need to distinguish between mpeg 1 layer 3 and
		 * mpeg 2 or 4 here.
		 */
		int mpegversion = 0;
		gst_structure_get_int (s, "mpegversion", &mpegversion);
		switch (mpegversion) {
		case 2:
		case 4:
			return g_strdup ("audio/x-aac");		/* hmm. */

		case 1:
		default:
			return g_strdup ("audio/mpeg");
		}
	} else {
		/* everything else is fine as-is */
		return g_strdup (media_type);
	}
}

GstCaps *
rb_gst_media_type_to_caps (const char *media_type)
{
	/* special cases: */
	if (strcmp (media_type, "audio/mpeg") == 0) {
		return gst_caps_from_string ("audio/mpeg, mpegversion=(int)1");
	} else if (strcmp (media_type, "audio/x-aac") == 0) {
		return gst_caps_from_string ("audio/mpeg, mpegversion=(int){ 2, 4 }");
	} else {
		/* otherwise, the media type is enough */
		return gst_caps_from_string (media_type);
	}
}

const char *
rb_gst_media_type_to_extension (const char *media_type)
{
	if (media_type == NULL) {
		return NULL;
	} else if (!strcmp (media_type, "audio/mpeg")) {
		return "mp3";
	} else if (!strcmp (media_type, "audio/x-vorbis") || !strcmp (media_type, "application/ogg")) {
		return "ogg";
	} else if (!strcmp (media_type, "audio/x-flac") || !strcmp (media_type, "audio/flac")) {
		return "flac";
	} else if (!strcmp (media_type, "audio/x-aac") || !strcmp (media_type, "audio/aac") || !strcmp (media_type, "audio/x-alac")) {
		return "m4a";
	} else if (!strcmp (media_type, "audio/x-wavpack")) {
		return "wv";
	} else {
		return NULL;
	}
}

const char *
rb_gst_mime_type_to_media_type (const char *mime_type)
{
	if (!strcmp (mime_type, "application/x-id3") || !strcmp (mime_type, "audio/mpeg")) {
		return "audio/mpeg";
	} else if (!strcmp (mime_type, "application/ogg") || !strcmp (mime_type, "audio/x-vorbis")) {
		return "audio/x-vorbis";
	} else if (!strcmp (mime_type, "audio/flac")) {
		return "audio/x-flac";
	} else if (!strcmp (mime_type, "audio/aac") || !strcmp (mime_type, "audio/mp4") || !strcmp (mime_type, "audio/m4a")) {
		return "audio/x-aac";
	}
	return mime_type;
}

const char *
rb_gst_media_type_to_mime_type (const char *media_type)
{
	if (!strcmp (media_type, "audio/x-vorbis")) {
		return "application/ogg";
	} else if (!strcmp (media_type, "audio/x-flac")) {
		return "audio/flac";
	} else if (!strcmp (media_type, "audio/x-aac")) {
		return "audio/mp4";	/* probably */
	} else {
		return media_type;
	}
}

gboolean
rb_gst_media_type_matches_profile (GstEncodingProfile *profile, const char *media_type)
{
	const GstCaps *pcaps;
	const GList *cl;
	GstCaps *caps;
	gboolean matches = FALSE;

	caps = rb_gst_media_type_to_caps (media_type);
	if (caps == NULL) {
		return FALSE;
	}

	pcaps = gst_encoding_profile_get_format (profile);
	if (gst_caps_can_intersect (caps, pcaps)) {
		matches = TRUE;
	}

	if (matches == FALSE && GST_IS_ENCODING_CONTAINER_PROFILE (profile)) {
		for (cl = gst_encoding_container_profile_get_profiles (GST_ENCODING_CONTAINER_PROFILE (profile)); cl != NULL; cl = cl->next) {
			GstEncodingProfile *cp = cl->data;
			pcaps = gst_encoding_profile_get_format (cp);
			if (gst_caps_can_intersect (caps, pcaps)) {
				matches = TRUE;
				break;
			}
		}
	}
	return matches;
}

char *
rb_gst_encoding_profile_get_media_type (GstEncodingProfile *profile)
{
	if (GST_IS_ENCODING_CONTAINER_PROFILE (profile)) {
		const GList *cl = gst_encoding_container_profile_get_profiles (GST_ENCODING_CONTAINER_PROFILE (profile));
		for (; cl != NULL; cl = cl->next) {
			GstEncodingProfile *p = cl->data;
			if (GST_IS_ENCODING_AUDIO_PROFILE (p)) {
				return rb_gst_caps_to_media_type (gst_encoding_profile_get_format (p));
			}

		}

		/* now what? */
		return NULL;
	} else {
		return rb_gst_caps_to_media_type (gst_encoding_profile_get_format (profile));
	}
}

GstEncodingTarget *
rb_gst_get_default_encoding_target ()
{
	if (default_target == NULL) {
		char *target_file;
		GError *error = NULL;

		if (g_file_test (SOURCE_ENCODING_TARGET_FILE,
			         G_FILE_TEST_EXISTS) != FALSE) {
			target_file = SOURCE_ENCODING_TARGET_FILE;
		} else {
			target_file = INSTALLED_ENCODING_TARGET_FILE;
		}

		default_target = gst_encoding_target_load_from_file (target_file, &error);
		if (default_target == NULL) {
			g_warning ("Unable to load encoding profiles from %s: %s", target_file, error ? error->message : "no error");
			g_clear_error (&error);
			return NULL;
		}
	}

	return default_target;
}

GstEncodingProfile *
rb_gst_get_encoding_profile (const char *media_type)
{
	const GList *l;
	GstEncodingTarget *target;
	target = rb_gst_get_default_encoding_target ();

	for (l = gst_encoding_target_get_profiles (target); l != NULL; l = l->next) {
		GstEncodingProfile *profile = l->data;
		if (rb_gst_media_type_matches_profile (profile, media_type)) {
			gst_encoding_profile_ref (profile);
			return profile;
		}
	}

	return NULL;
}

gboolean
rb_gst_media_type_is_lossless (const char *media_type)
{
	int i;
	const char *lossless_types[] = {
		"audio/x-flac",
		"audio/x-alac",
		"audio/x-shorten",
		"audio/x-wavpack"	/* not completely sure */
	};

	for (i = 0; i < G_N_ELEMENTS (lossless_types); i++) {
		if (strcmp (media_type, lossless_types[i]) == 0) {
			return TRUE;
		}
	}
	return FALSE;
}

gboolean
rb_gst_check_missing_plugins (GstEncodingProfile *profile,
			      char ***details,
			      char ***descriptions)
{
	GstElement *encodebin;
	GstBus *bus;
	GstPad *pad;
	gboolean ret;

	ret = FALSE;

	encodebin = gst_element_factory_make ("encodebin", NULL);
	if (encodebin == NULL) {
		g_warning ("Unable to create encodebin");
		return TRUE;
	}

	bus = gst_bus_new ();
	gst_element_set_bus (encodebin, bus);
	gst_bus_set_flushing (bus, FALSE);		/* necessary? */

	g_object_set (encodebin, "profile", profile, NULL);
	pad = gst_element_get_static_pad (encodebin, "audio_0");
	if (pad == NULL) {
		GstMessage *message;
		GList *messages = NULL;
		GList *m;
		int i;

		message = gst_bus_pop (bus);
		while (message != NULL) {
			if (gst_is_missing_plugin_message (message)) {
				messages = g_list_append (messages, message);
			} else {
				gst_message_unref (message);
			}
			message = gst_bus_pop (bus);
		}

		if (messages != NULL) {
			if (details != NULL) {
				*details = g_new0(char *, g_list_length (messages)+1);
			}
			if (descriptions != NULL) {
				*descriptions = g_new0(char *, g_list_length (messages)+1);
			}
			i = 0;
			for (m = messages; m != NULL; m = m->next) {
				char *str;
				if (details != NULL) {
					str = gst_missing_plugin_message_get_installer_detail (m->data);
					(*details)[i] = str;
				}
				if (descriptions != NULL) {
					str = gst_missing_plugin_message_get_description (m->data);
					(*descriptions)[i] = str;
				}
				i++;
			}

			ret = TRUE;
			g_list_foreach (messages, (GFunc)gst_message_unref, NULL);
			g_list_free (messages);
		}

	} else {
		gst_element_release_request_pad (encodebin, pad);
		gst_object_unref (pad);
	}

	gst_object_unref (encodebin);
	gst_object_unref (bus);
	return ret;
}
