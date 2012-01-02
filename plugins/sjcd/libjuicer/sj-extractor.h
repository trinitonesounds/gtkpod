/* 
 * Copyright (C) 2003 Ross Burton <ross@burtonini.com>
 *
 * Sound Juicer - sj-extractor.h
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

#ifndef SJ_EXTRACTOR_H
#define SJ_EXTRACTOR_H

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include "rb-gst-media-types.h"
#include "sj-structures.h"

G_BEGIN_DECLS


#define SJ_TYPE_EXTRACTOR              (sj_extractor_get_type ())
#define SJ_EXTRACTOR(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), SJ_TYPE_EXTRACTOR, SjExtractor))
#define SJ_EXTRACTOR_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), SJ_TYPE_EXTRACTOR, SjExtractorClass))
#define SJ_IS_EXTRACTOR(obj)           (G_TYPE_CHECK_INSTANCE_TYPE (obj, SJ_TYPE_EXTRACTOR))
#define SJ_IS_EXTRACTOR_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), SJ_TYPE_EXTRACTOR))
#define SJ_EXTRACTOR_GET_CLASS(obj)    (G_TYPE_CHECK_CLASS_CAST ((obj), SJ_TYPE_EXTRACTOR, SjExtractorClass))

typedef struct SjExtractorPrivate SjExtractorPrivate;

typedef struct {
  GObject object;
  SjExtractorPrivate *priv;
} SjExtractor;

typedef struct {
  GObjectClass parent_class;
  void (*progress) (SjExtractor *extractor, const int seconds);
  void (*completion) (SjExtractor *extractor);
  void (*error) (SjExtractor *extractor, GError *error);
} SjExtractorClass;

GType sj_extractor_get_type (void);

/* TODO: should this call gst_init? How to pass arg[cv]? */
GObject *sj_extractor_new (void);

/**
 * Call after _new() to see if an GError was created
 */
GError *sj_extractor_get_new_error (SjExtractor *extractor);

void sj_extractor_set_device (SjExtractor *extractor, const char* device);

void sj_extractor_set_paranoia (SjExtractor *extractor, const int paranoia_mode);

void sj_extractor_extract_track (SjExtractor *extractor, const TrackDetails *track, GFile *file, GError **error);

void sj_extractor_cancel_extract (SjExtractor *extractor);

gboolean sj_extractor_supports_profile (GstEncodingProfile *profile);

gboolean sj_extractor_supports_encoding (GError **error);

G_END_DECLS

#endif /* SJ_EXTRACTOR_H */
