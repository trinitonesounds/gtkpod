/*
 * sj-metadata-musicbrainz5.c
 * Copyright (C) 2008 Ross Burton <ross@burtonini.com>
 * Copyright (C) 2008 Bastien Nocera <hadess@hadess.net>
 * Copyright (C) 2011 Christophe Fergeau <cfergeau@redhat.com>
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
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <glib-object.h>
#include <discid/discid.h>
#include <musicbrainz5/mb5_c.h>

#include "sj-metadata-musicbrainz5.h"
#include "sj-structures.h"
#include "sj-error.h"

static char language[3];

#define GET(field, function, obj) {						\
	function (obj, buffer, sizeof (buffer));				\
	if (field)								\
		g_free (field);							\
	if (*buffer == '\0')							\
		field = NULL;							\
	else									\
		field = g_strdup (buffer);					\
}

#ifndef DISCID_HAVE_SPARSE_READ
#define discid_read_sparse(disc, dev, i) discid_read(disc, dev)
#endif

#define SJ_MUSICBRAINZ_USER_AGENT "libjuicer-"VERSION

typedef struct {
  Mb5Query mb;
  DiscId  *disc;
  char    *cdrom;
  GHashTable *artist_cache;
  /* Proxy */
  char    *proxy_host;
  char    *proxy_username;
  char    *proxy_password;
  int      proxy_port;
  gboolean proxy_use_authentication;
} SjMetadataMusicbrainz5Private;

#define GET_PRIVATE(o)  \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), SJ_TYPE_METADATA_MUSICBRAINZ5, SjMetadataMusicbrainz5Private))

enum {
  PROP_0,
  PROP_DEVICE,
  PROP_PROXY_USE_AUTHENTICATION,
  PROP_PROXY_HOST,
  PROP_PROXY_PORT,
  PROP_PROXY_USERNAME,
  PROP_PROXY_PASSWORD
};

static void metadata_interface_init (gpointer g_iface, gpointer iface_data);

G_DEFINE_TYPE_WITH_CODE (SjMetadataMusicbrainz5,
                         sj_metadata_musicbrainz5,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (SJ_TYPE_METADATA,
                                                metadata_interface_init));


/*
 * Private methods
 */
#ifdef DUMP_DETAILS
static void
sj_mb5_album_details_dump (AlbumDetails *details)
{
  if (details->country)
    g_print ("Country: %s\n", details->country);
  if (details->type)
    g_print ("Type: %s\n", details->type);
  if (details->lyrics_url)
    g_print ("Lyrics URL: %s\n", details->lyrics_url);
}
#else
#define sj_mb5_album_details_dump(...)
#endif


static void
parse_artist_aliases (Mb5Artist   artist,
                      char      **name,
                      char      **sortname)
{
  Mb5AliasList alias_list;
  int i;
  char buffer[512]; /* for the GET macro */
  char locale[512];

  GET (*name, mb5_artist_get_name, artist);
  GET (*sortname, mb5_artist_get_sortname, artist);

  if (*language == 0)
    return;

  alias_list = mb5_artist_get_aliaslist (artist);
  if (alias_list == NULL)
    return;

  for (i = 0; i < mb5_alias_list_size (alias_list); i++) {
    Mb5Alias alias = mb5_alias_list_item (alias_list, i);

    if (alias == NULL)
      continue;

    if (mb5_alias_get_locale (alias, locale, sizeof(locale)) > 0 &&
        strcmp (locale, language) == 0 &&
        mb5_alias_get_primary (alias, buffer, sizeof(buffer)) > 0 &&
        strcmp (buffer, "primary") == 0) {
      GET (*name, mb5_alias_get_text, alias);
      GET (*sortname, mb5_alias_get_sortname, alias);
    }
  }
}

static ArtistDetails*
make_artist_details (SjMetadataMusicbrainz5 *self,
                     Mb5Artist               artist)
{
  char buffer[512]; /* For the GET macro */
  ArtistDetails *details;
  SjMetadataMusicbrainz5Private *priv = GET_PRIVATE (self);

  mb5_artist_get_id (artist, buffer, sizeof(buffer));
  details = g_hash_table_lookup (priv->artist_cache, buffer);
  if (details == NULL) {
    details = g_new0 (ArtistDetails, 1);
    details->id = g_strdup (buffer);
    parse_artist_aliases (artist, &details->name, &details->sortname);
    GET (details->disambiguation, mb5_artist_get_disambiguation, artist);
    GET (details->gender, mb5_artist_get_gender, artist);
    GET (details->country, mb5_artist_get_country, artist);
    g_hash_table_insert (priv->artist_cache, details->id, details);
  }
  return details;
}

static void
print_musicbrainz_query_error (SjMetadataMusicbrainz5 *self,
                               const char             *entity,
                               const char             *id)
{
  SjMetadataMusicbrainz5Private *priv = GET_PRIVATE (self);
  int len;
  char *message;
  int code =  mb5_query_get_lasthttpcode (priv->mb);
  /* No error if the discid isn't found */
  if (strcmp (entity, "discid") == 0 && code == 404)
    return;
  len =  mb5_query_get_lasterrormessage (priv->mb, NULL, 0) + 1;
  message = g_malloc (len);
  mb5_query_get_lasterrormessage (priv->mb, message, len);
  g_warning ("No Musicbrainz metadata for %s %s, http code %d, %s",
             entity, id, code, message);
  g_free (message);
}


static Mb5Metadata
query_musicbrainz (SjMetadataMusicbrainz5 *self,
                   const char             *entity,
                   const char             *id,
                   char                   *includes)
{
  Mb5Metadata metadata;
  char *inc[] = { "inc" };
  SjMetadataMusicbrainz5Private *priv = GET_PRIVATE (self);

  if (includes == NULL)
    metadata = mb5_query_query (priv->mb, entity, id, "",
                                0, NULL, NULL);
  else
    metadata = mb5_query_query (priv->mb, entity, id, "",
                                1, inc, &includes);
  if (metadata == NULL)
    print_musicbrainz_query_error (self, entity, id);

  return metadata;
}

static ArtistDetails*
query_artist (SjMetadataMusicbrainz5 *self,
              const gchar            *id)
{
  SjMetadataMusicbrainz5Private *priv = GET_PRIVATE (self);
  ArtistDetails *details = g_hash_table_lookup (priv->artist_cache, id);

  if (details == NULL) {
    Mb5Artist artist;
    Mb5Metadata metadata = query_musicbrainz (self, "artist", id, "aliases");
    if (metadata == NULL)
      return NULL;
    artist = mb5_metadata_get_artist (metadata);
    if (artist != NULL)
      details = make_artist_details (self, artist);
    mb5_metadata_delete (metadata);
  }
  return details;
}

static GList *
get_artist_list (SjMetadataMusicbrainz5 *self,
                 Mb5ArtistCredit         credit)
{
  Mb5NameCreditList name_list;
  GList *artists;
  unsigned int i;
  char buffer[512]; /* for the GET macro */

  if (credit == NULL)
    return NULL;

  name_list = mb5_artistcredit_get_namecreditlist (credit);
  if (name_list == NULL) {
    return NULL;
  }

  artists = NULL;
  for (i = 0; i < mb5_namecredit_list_size (name_list); i++) {
    Mb5NameCredit name_credit;
    Mb5Artist artist;
    ArtistCredit *artist_credit = g_slice_new0 (ArtistCredit);

    name_credit = mb5_namecredit_list_item (name_list, i);
    artist = mb5_namecredit_get_artist (name_credit);
    if (artist != NULL) {
      artist_credit->details = make_artist_details (self, artist);
    } else {
      g_warning ("no Mb5Artist associated with Mb5NameCredit, falling back to Mb5NameCredit::name");
      artist_credit->details = g_new0 (ArtistDetails, 1);
      GET (artist_credit->details->name, mb5_namecredit_get_name, name_credit);
    }
    GET (artist_credit->joinphrase, mb5_namecredit_get_joinphrase, name_credit);
    artists = g_list_prepend (artists, artist_credit);
  }

  return g_list_reverse(artists);
}

static void
get_artist_info (GList *artists, char **name, char **sortname, char **id)
{
  GString *artist_name;
  GList *it;
  unsigned int artist_count;

  artist_name = g_string_new (NULL);
  artist_count = 0;
  for (it = artists; it != NULL; it = it->next) {
    ArtistCredit *artist_credit = (ArtistCredit *)it->data;
    artist_count++;
    g_string_append (artist_name, artist_credit->details->name);
    if (artist_credit->joinphrase != NULL)
      g_string_append (artist_name, artist_credit->joinphrase);
  }

  if (artist_count != 1) {
      g_warning ("multiple artists");
      if (sortname != NULL)
        *sortname = NULL;
      if (id != NULL)
        *id = NULL;
  } else {
      ArtistDetails *details = ((ArtistCredit *)artists->data)->details;
      if (sortname != NULL)
        *sortname = g_strdup (details->sortname);
      if (id != NULL)
        *id = g_strdup (details->id);
  }

  if (name != NULL)
    *name = artist_name->str;

  g_string_free (artist_name, FALSE);
}

static GList*
get_release_labels (Mb5Release *release)
{
  Mb5LabelInfoList list;
  int i;
  char buffer[512]; /* for the GET() macro */
  GList *label_list = NULL;

  list = mb5_release_get_labelinfolist (release);
  if (list == NULL)
    return NULL;

  for (i = 0; i < mb5_labelinfo_list_size (list); i++) {
    Mb5LabelInfo info;
    Mb5Label label;
    LabelDetails *label_data;

    info = mb5_labelinfo_list_item (list, i);
    if (info == NULL)
      continue;

    label = mb5_labelinfo_get_label (info);
    if (label == NULL)
      continue;

    label_data = g_new0 (LabelDetails, 1);
    GET (label_data->name,     mb5_label_get_name,     label);
    GET (label_data->sortname, mb5_label_get_sortname, label);
    label_list = g_list_prepend (label_list, label_data);
  }
  label_list = g_list_reverse (label_list);
  return label_list;
}

static void
fill_album_composer (AlbumDetails *album)
{
  char *old_composer;
  GList *l;
  TrackDetails *track_one; /* The first track on the album */
  gboolean one_composer = TRUE; /* TRUE if all tracks have the same composer */

  if (album->composer != NULL)
    return;

  if (album->tracks == NULL)
    return;

  l = album->tracks;
  track_one = (TrackDetails*)l->data;
  old_composer = track_one->composer;

  for (l = l->next; l != NULL; l = l->next) {
    char *new_composer;
    TrackDetails *track = (TrackDetails*)l->data;
    new_composer = track->composer;
    if (g_strcmp0 (old_composer, new_composer) != 0) {
      one_composer = FALSE;
      break;
    }
  }

  if (one_composer) {
    album->composer = g_strdup (track_one->composer);
    album->composer_sortname = g_strdup (track_one->composer_sortname);
  } else {
    album->composer = g_strdup ("Various Composers");
    album->composer_sortname = g_strdup ("Various Composers");
  }
}

typedef void (*RelationForeachFunc)(SjMetadataMusicbrainz5 *self, Mb5Relation relation, gpointer user_data);

static void relationlist_list_foreach_relation(SjMetadataMusicbrainz5 *self,
                                               Mb5RelationListList     relation_lists,
                                               const char             *targettype,
                                               const char             *relation_type,
                                               RelationForeachFunc     callback,
                                               gpointer                user_data)
{
  unsigned int j;

  if (relation_lists == NULL)
    return;

  for (j = 0; j < mb5_relationlist_list_size (relation_lists); j++) {
    Mb5RelationList relations;
    char type[512]; /* To hold relationlist target-type and relation type */
    unsigned int i;

    relations = mb5_relationlist_list_item (relation_lists, j);
    if (relations == NULL)
      return;

    mb5_relation_list_get_targettype (relations, type, sizeof (type));
    if (!g_str_equal (type, targettype))
      continue;

    for (i = 0; i < mb5_relation_list_size (relations); i++) {
      Mb5Relation relation;

      relation = mb5_relation_list_item (relations, i);
      if (relation == NULL)
        continue;

      mb5_relation_get_type (relation, type, sizeof (type));
      if (!g_str_equal (type, relation_type))
        continue;

      callback(self, relation, user_data);
    }
  }
}

static void composer_cb (SjMetadataMusicbrainz5 *self,
                         Mb5Relation             relation,
                         gpointer                user_data)
{
  Mb5Artist composer;
  TrackDetails *track = (TrackDetails *)user_data;
  char buffer[512]; /* For the GET macro */
  ArtistDetails *details;

  composer = mb5_relation_get_artist (relation);
  if (composer == NULL)
    return;
  /* NB work-level-rels only returns artist name, sortname & id so
     we need to perform another query if we want the alias list
     therefore use query_artist rather than make_artist_details. */
  mb5_artist_get_id (composer, buffer, sizeof (buffer));
  details = query_artist (self, buffer);
  if (details != NULL) {
    track->composer = g_strdup (details->name);
    track->composer_sortname = g_strdup (details->sortname);
  } else {
    GET (track->composer, mb5_artist_get_name, composer);
    GET (track->composer_sortname, mb5_artist_get_sortname, composer);
  }
}

static void work_cb (SjMetadataMusicbrainz5 *self,
                     Mb5Relation             relation,
                     gpointer                user_data)
{
    Mb5RelationListList relation_lists;
    Mb5Work work;

    work = mb5_relation_get_work (relation);
    if (work == NULL)
        return;
    relation_lists = mb5_work_get_relationlistlist (work);
    relationlist_list_foreach_relation (self, relation_lists,
                                        "artist", "composer",
                                        composer_cb, user_data);
}

static void
fill_recording_relations (SjMetadataMusicbrainz5 *self,
                          Mb5Recording            recording,
                          TrackDetails           *track)
{
  Mb5RelationListList relation_lists;

  relation_lists = mb5_recording_get_relationlistlist (recording);
  relationlist_list_foreach_relation(self, relation_lists,
                                     "work", "performance",
                                     work_cb, track);
}

static void
parse_artist_credit (SjMetadataMusicbrainz5  *self,
                     Mb5ArtistCredit          credit,
                     char                   **name,
                     char                   **sortname,
                     char                   **id)
{
  if (credit) {
    GList *artists;
    artists = get_artist_list (self, credit);
    if (artists) {
      get_artist_info (artists, name, sortname, id);
      g_list_free_full (artists, artist_credit_destroy);
    }
  }
}

static void
fill_tracks_from_medium (SjMetadataMusicbrainz5 *self,
                         Mb5Medium               medium,
                         AlbumDetails           *album)
{
  Mb5TrackList track_list;
  GList *tracks;
  unsigned int i;
  char buffer[512]; /* for the GET() macro */

  track_list = mb5_medium_get_tracklist (medium);
  if (!track_list)
    return;

  album->number = mb5_track_list_size (track_list);

  tracks = NULL;

  for (i = 0; i < mb5_track_list_size (track_list); i++) {
    Mb5Track mbt;
    Mb5ArtistCredit credit;
    Mb5Recording recording;
    TrackDetails *track;

    mbt = mb5_track_list_item (track_list, i);
    if (!mbt)
      continue;

    track = g_new0 (TrackDetails, 1);

    track->album = album;

    track->number = mb5_track_get_position (mbt);

    /* Prefer metadata from Mb5Track over metadata from Mb5Recording
     * https://bugzilla.gnome.org/show_bug.cgi?id=690903#c8
     */
    track->duration = mb5_track_get_length (mbt) / 1000;
    GET (track->title, mb5_track_get_title, mbt);
    credit = mb5_track_get_artistcredit (mbt);

    recording = mb5_track_get_recording (mbt);
    if (recording != NULL) {
      GET (track->track_id, mb5_recording_get_id, recording);
      fill_recording_relations (self, recording, track);

      if (track->duration == 0)
        track->duration = mb5_recording_get_length (recording) / 1000;
      if (track->title == NULL)
        GET (track->title, mb5_recording_get_title, recording);
      if (credit == NULL)
          credit = mb5_recording_get_artistcredit (recording);
    }

      parse_artist_credit(self, credit,
                          &track->artist,
                          &track->artist_sortname,
                          &track->artist_id);
    if (track->artist == NULL)
      track->artist = g_strdup (album->artist);
    if (track->artist_sortname == NULL)
      track->artist_sortname = g_strdup (album->artist_sortname);
    if (track->artist_id == NULL)
      track->artist_id = g_strdup (album->artist_id);

    tracks = g_list_prepend (tracks, track);
  }
  album->tracks = g_list_reverse (tracks);
}

static void wikipedia_cb (SjMetadataMusicbrainz5 *self,
                          Mb5Relation             relation,
                          gpointer                user_data)
{
  AlbumDetails *album = (AlbumDetails *)user_data;
  char buffer[512]; /* for the GET() macro */
  char *wikipedia = NULL;

  GET (wikipedia, mb5_relation_get_target, relation);
  if (wikipedia != NULL) {
    g_free (album->wikipedia);
    album->wikipedia = wikipedia;
  }
}

static void discogs_cb (SjMetadataMusicbrainz5 *self,
                        Mb5Relation             relation,
                        gpointer                user_data)
{
  AlbumDetails *album = (AlbumDetails *)user_data;
  char buffer[512]; /* for the GET() macro */
  char *discogs = NULL;

  GET (discogs, mb5_relation_get_target, relation);
  if (discogs != NULL) {
    g_free (album->discogs);
    album->discogs = discogs;
  }
}

static void lyrics_cb (SjMetadataMusicbrainz5 *self,
                       Mb5Relation             relation,
                       gpointer                user_data)
{
  AlbumDetails *album = (AlbumDetails *)user_data;
  char buffer[512]; /* for the GET() macro */
  char *lyrics = NULL;

  GET (lyrics, mb5_relation_get_target, relation);
  if (lyrics != NULL) {
    g_free (album->lyrics_url);
    album->lyrics_url = lyrics;
  }
}

static void
fill_relations (SjMetadataMusicbrainz5 *self,
                Mb5RelationListList     relation_lists,
                AlbumDetails           *album)
{
  relationlist_list_foreach_relation (self, relation_lists,
                                      "url", "wikipedia",
                                      wikipedia_cb, album);
  relationlist_list_foreach_relation (self, relation_lists,
                                      "url", "discogs",
                                      discogs_cb, album);
  relationlist_list_foreach_relation (self, relation_lists,
                                      "url", "lyrics",
                                      lyrics_cb, album);
}

static AlbumDetails *
make_album_from_release (SjMetadataMusicbrainz5 *self,
                         Mb5ReleaseGroup         group,
                         Mb5Release              release,
                         Mb5Medium               medium)
{
  AlbumDetails *album;
  Mb5ArtistCredit credit;
  Mb5RelationListList relationlists;
  Mb5MediumList media;
  char *date = NULL;
  char buffer[512]; /* for the GET macro */

  g_assert (release);
  g_return_val_if_fail (medium != NULL, NULL);

  album = g_new0 (AlbumDetails, 1);

  media = mb5_release_get_mediumlist (release);
  if (media)
    album->disc_count = mb5_medium_list_size (media);

  GET (album->album_id, mb5_release_get_id, release);
  GET (album->title, mb5_medium_get_title, medium);
  if (album->title == NULL)
    GET (album->title, mb5_release_get_title, release);

  credit = mb5_release_get_artistcredit (release);

  parse_artist_credit (self, credit,
                       &album->artist,
                       &album->artist_sortname,
                       &album->artist_id);

  GET (date, mb5_release_get_date, release);
  if (date) {
    album->release_date = gst_date_time_new_from_iso8601_string (date);
    g_free (date);
  }

  GET (album->asin, mb5_release_get_asin, release);
  mb5_release_get_country (release, buffer, sizeof(buffer));
  album->country = sj_metadata_helper_lookup_country_code (buffer);
  if (group) {
    GET (album->type, mb5_releasegroup_get_primarytype, group);
    if (g_str_has_suffix (album->type, "Spokenword")
        || g_str_has_suffix (album->type, "Interview")
        || g_str_has_suffix (album->type, "Audiobook")) {
      album->is_spoken_word = TRUE;
    }
    relationlists = mb5_releasegroup_get_relationlistlist (group);
    fill_relations (self, relationlists, album);
  }

  album->disc_number = mb5_medium_get_position (medium);
  fill_tracks_from_medium (self, medium, album);
  fill_album_composer (album);
  relationlists = mb5_release_get_relationlistlist (release);
  fill_relations (self, relationlists, album);
  album->labels = get_release_labels (release);

  sj_mb5_album_details_dump (album);
  return album;
}

/*
 * Virtual methods
 */
static GList *
mb5_list_albums (SjMetadata *metadata, char **url, GError **error)
{
  SjMetadataMusicbrainz5 *self;
  SjMetadataMusicbrainz5Private *priv;
  GList *albums = NULL;
  Mb5ReleaseList releases;
  Mb5Release release;
  const char *discid = NULL;
  char buffer[1024];
  int i;
  g_return_val_if_fail (SJ_IS_METADATA_MUSICBRAINZ5 (metadata), NULL);

  self = SJ_METADATA_MUSICBRAINZ5 (metadata);
  priv = GET_PRIVATE (SJ_METADATA_MUSICBRAINZ5 (self));

  if (sj_metadata_helper_check_media (priv->cdrom, error) == FALSE) {
    return NULL;
  }

  priv->disc = discid_new ();
  if (priv->disc == NULL)
    return NULL;
  if (discid_read_sparse (priv->disc, priv->cdrom, 0) == 0)
    return NULL;

  if (url != NULL)
    *url = g_strdup (discid_get_submission_url (priv->disc));

  if (g_getenv("MUSICBRAINZ_FORCE_DISC_ID")) {
    discid = g_getenv("MUSICBRAINZ_FORCE_DISC_ID");
  } else {
    discid = discid_get_id (priv->disc);
  }

  releases = mb5_query_lookup_discid(priv->mb, discid);

  if (releases == NULL) {
    print_musicbrainz_query_error (self, "discid", discid);
    return NULL;
  }

  if (mb5_release_list_size (releases) == 0)
    return NULL;

  for (i = 0; i < mb5_release_list_size (releases); i++) {
    AlbumDetails *album;

    release = mb5_release_list_item (releases, i);
    if (release) {
      char *releaseid = NULL;
      Mb5Release full_release = NULL;
      Mb5Metadata release_md = NULL;
      char *includes = "aliases artists artist-credits labels recordings \
release-groups url-rels discids recording-level-rels work-level-rels work-rels \
artist-rels";

      releaseid = NULL;
      GET(releaseid, mb5_release_get_id, release);
      /* Inorder to get metadata for artist aliases & work / composer
       * relationships we need to perform a custom query
       */
      release_md = query_musicbrainz (self, "release", releaseid, includes);

      if (release_md && mb5_metadata_get_release (release_md))
        full_release = mb5_metadata_get_release (release_md);
      g_free (releaseid);

      if (full_release) {
        Mb5MediumList media;
        Mb5Metadata metadata = NULL;
        Mb5ReleaseGroup group;
        unsigned int j;

        group = mb5_release_get_releasegroup (full_release);
        if (group) {
          /* The release-group information we can extract from the
           * lookup_release query doesn't have the url relations for the
           * release-group, so run a separate query to get these urls
           */
          char *releasegroupid = NULL;
          char *includes = "artists url-rels";

          GET (releasegroupid, mb5_releasegroup_get_id, group);
          metadata = query_musicbrainz (self, "release-group", releasegroupid, includes);
          g_free (releasegroupid);
        }

        if (metadata && mb5_metadata_get_releasegroup (metadata))
          group = mb5_metadata_get_releasegroup (metadata);

        media = mb5_release_media_matching_discid (full_release, discid);
        for (j = 0; j < mb5_medium_list_size (media); j++) {
          Mb5Medium medium;
          medium = mb5_medium_list_item (media, j);
          if (medium) {
            album = make_album_from_release (self, group, full_release, medium);
            album->metadata_source = SOURCE_MUSICBRAINZ;
            albums = g_list_append (albums, album);
          }
        }
        mb5_metadata_delete (metadata);
        mb5_medium_list_delete (media);
      }
      mb5_metadata_delete (release_md);
    }
  }
  mb5_release_list_delete (releases);
  return albums;
}

static void
setup_http_proxy (SjMetadataMusicbrainz5Private *priv)
{
  if (priv->proxy_host == NULL || priv->proxy_port == 0) {
    mb5_query_set_proxyhost (priv->mb, NULL);
    mb5_query_set_proxyport (priv->mb, 0);
    mb5_query_set_proxyusername (priv->mb, NULL);
    mb5_query_set_proxypassword (priv->mb, NULL);
  } else {
    mb5_query_set_proxyhost (priv->mb, priv->proxy_host);
    mb5_query_set_proxyport (priv->mb, priv->proxy_port);
    if (priv->proxy_use_authentication &&
        priv->proxy_username != NULL && priv->proxy_password != NULL) {
      mb5_query_set_proxyusername (priv->mb, priv->proxy_username);
      mb5_query_set_proxypassword (priv->mb, priv->proxy_password);
    } else {
      mb5_query_set_proxyusername (priv->mb, NULL);
      mb5_query_set_proxypassword (priv->mb, NULL);
    }
  }
}

/*
 * GObject methods
 */

static void
metadata_interface_init (gpointer g_iface, gpointer iface_data)
{
  SjMetadataClass *klass = (SjMetadataClass*)g_iface;

  klass->list_albums = mb5_list_albums;
}

static void
sj_metadata_musicbrainz5_init (SjMetadataMusicbrainz5 *self)
{
  SjMetadataMusicbrainz5Private *priv;

  priv = GET_PRIVATE (self);
  priv->mb = mb5_query_new (SJ_MUSICBRAINZ_USER_AGENT, NULL, 0);

  priv->artist_cache = g_hash_table_new_full (g_str_hash, g_str_equal,
                                              NULL, artist_details_destroy);
}

static void
sj_metadata_musicbrainz5_get_property (GObject *object, guint property_id,
                                       GValue *value, GParamSpec *pspec)
{
  SjMetadataMusicbrainz5Private *priv = GET_PRIVATE (object);
  g_assert (priv);

  switch (property_id) {
  case PROP_DEVICE:
    g_value_set_string (value, priv->cdrom);
    break;
  case PROP_PROXY_USE_AUTHENTICATION:
    g_value_set_boolean (value, priv->proxy_use_authentication);
    break;
  case PROP_PROXY_HOST:
    g_value_set_string (value, priv->proxy_host);
    break;
  case PROP_PROXY_PORT:
    g_value_set_int (value, priv->proxy_port);
    break;
  case PROP_PROXY_USERNAME:
    g_value_set_string (value, priv->proxy_username);
    break;
  case PROP_PROXY_PASSWORD:
    g_value_set_string (value, priv->proxy_password);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
sj_metadata_musicbrainz5_set_property (GObject *object, guint property_id,
                                       const GValue *value, GParamSpec *pspec)
{
  SjMetadataMusicbrainz5Private *priv = GET_PRIVATE (object);
  g_assert (priv);

  switch (property_id) {
  case PROP_DEVICE:
    if (priv->cdrom)
      g_free (priv->cdrom);
    priv->cdrom = g_value_dup_string (value);
    break;
  case PROP_PROXY_USE_AUTHENTICATION:
    priv->proxy_use_authentication = g_value_get_boolean (value);
    setup_http_proxy (priv);
    break;
  case PROP_PROXY_HOST:
    g_free (priv->proxy_host);
    priv->proxy_host = g_value_dup_string (value);
    setup_http_proxy (priv);
    break;
  case PROP_PROXY_PORT:
    priv->proxy_port = g_value_get_int (value);
    setup_http_proxy (priv);
    break;
  case PROP_PROXY_USERNAME:
    g_free (priv->proxy_username);
    priv->proxy_username = g_value_dup_string (value);
    setup_http_proxy (priv);
    break;
  case PROP_PROXY_PASSWORD:
    g_free (priv->proxy_password);
    priv->proxy_password = g_value_dup_string (value);
    setup_http_proxy (priv);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
sj_metadata_musicbrainz5_finalize (GObject *object)
{
  SjMetadataMusicbrainz5Private *priv;

  priv = GET_PRIVATE (object);

  if (priv->mb != NULL) {
    mb5_query_delete (priv->mb);
    priv->mb = NULL;
  }
  if (priv->disc != NULL) {
    discid_free (priv->disc);
    priv->disc = NULL;
  }
  g_free (priv->cdrom);
  g_free (priv->proxy_host);
  g_free (priv->proxy_username);
  g_free (priv->proxy_password);
  g_hash_table_unref (priv->artist_cache);

  G_OBJECT_CLASS (sj_metadata_musicbrainz5_parent_class)->finalize (object);
}

static void
sj_metadata_musicbrainz5_class_init (SjMetadataMusicbrainz5Class *class)
{
  const gchar * const * l;
  GObjectClass *object_class = (GObjectClass*)class;

  g_type_class_add_private (class, sizeof (SjMetadataMusicbrainz5Private));

  object_class->get_property = sj_metadata_musicbrainz5_get_property;
  object_class->set_property = sj_metadata_musicbrainz5_set_property;
  object_class->finalize = sj_metadata_musicbrainz5_finalize;

  g_object_class_override_property (object_class,
                                    PROP_DEVICE, "device");
  g_object_class_override_property (object_class,
                                    PROP_PROXY_USE_AUTHENTICATION, "proxy-use-authentication");
  g_object_class_override_property (object_class,
                                    PROP_PROXY_HOST, "proxy-host");
  g_object_class_override_property (object_class,
                                    PROP_PROXY_PORT, "proxy-port");
  g_object_class_override_property (object_class,
                                    PROP_PROXY_USERNAME, "proxy-username");
  g_object_class_override_property (object_class,
                                    PROP_PROXY_PASSWORD, "proxy-password");

  /* Although GLib supports fallback locales we do not use them as
     MusicBrainz does not provide locale information for the canonical
     artist name, only it's aliases. This means that it is not
     possible to fallback to other locales as the canonical name may
     actually be in a higher priority locale than the alias that
     matches one of the fallback locales. */
  l = g_get_language_names ();
  if (strcmp (l[0], "C") == 0) { /* "C" locale should be "en" for MusicBrainz */
    memcpy (language, "en", 3);
  } else if (strlen (l[0]) > 1) {
    memcpy (language, l[0], 2);
    language[2] = '\0';
  }
}

/*
 * Public methods.
 */

GObject *
sj_metadata_musicbrainz5_new (void)
{
  return g_object_new (SJ_TYPE_METADATA_MUSICBRAINZ5, NULL);
}
