/* 
 * Copyright (C) 2003 Ross Burton <ross@burtonini.com>
 *
 * Sound Juicer - sj-structures.h
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Ross Burton <ross@burtonini.com>
 */

#ifndef SJ_STRUCTURES_H
#define SJ_STRUCTURES_H

#include <glib.h>
#include <gst/gst.h>

typedef enum _MetadataSource MetadataSource;

typedef struct _AlbumDetails AlbumDetails;
typedef struct _ArtistCredit ArtistCredit;
typedef struct _ArtistDetails ArtistDetails;
typedef struct _LabelDetails LabelDetails;
typedef struct _TrackDetails TrackDetails;

enum _MetadataSource {
  SOURCE_UNKNOWN = 0,
  SOURCE_CDTEXT,
  SOURCE_FREEDB,
  SOURCE_MUSICBRAINZ,
  SOURCE_FALLBACK
};

struct _TrackDetails {
  AlbumDetails *album;
  int number; /* track number */
  char *title;
  char *artist;
  char* artist_sortname; /* Can be NULL, so fall back onto artist */
  char* composer;
  char* composer_sortname;
  int duration; /* seconds */
  char* track_id;
  char* artist_id;
};

struct _AlbumDetails {
  char* title;
  char* artist;
  char* artist_sortname;
  char* composer;
  char* composer_sortname;
  char *genre;
  int   number; /* number of tracks in the album */
  int   disc_number;
  int   disc_count; /* number of discs in the album */
  GList* tracks;
  GstDateTime *release_date; /* MusicBrainz support multiple releases per album */
  char* album_id;
  char* artist_id;
  GList* labels;
  char* asin;
  char* discogs;
  char* wikipedia;
  MetadataSource metadata_source;
  gboolean is_spoken_word;

  /* some of the new properties that we can get with the NGS musicbrainz
   * API
   */
  char *type;
  char *lyrics_url;
  char *country;
};

struct _ArtistDetails {
  char *id;
  char *name;
  char *sortname;
  char *disambiguation;
  char *gender;
  char *country;
};

struct _ArtistCredit
{
  ArtistDetails *details;
  char *joinphrase;
};

struct _LabelDetails {
  char *name;
  char *sortname;
};

void album_details_free(AlbumDetails *album);
ArtistDetails* artist_details_copy (const ArtistDetails *artist);
void artist_credit_free (ArtistCredit *credit, gboolean free_details);
void artist_credit_destroy (gpointer credit);
void artist_details_free(ArtistDetails *artist);
void artist_details_destroy (gpointer artist);
void label_details_free (LabelDetails *label);
void track_details_free(TrackDetails *track);

#endif
