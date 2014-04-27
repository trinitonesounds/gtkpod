/*
 * sj-metadata.c
 * Copyright (C) 2003 Ross Burton <ross@burtonini.com>
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

#include "config.h"

#include <gio/gio.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include "sj-structures.h"
#include "sj-metadata-getter.h"
#include "sj-metadata-marshal.h"
#include "sj-metadata.h"
#ifdef HAVE_MUSICBRAINZ5
#include "sj-metadata-musicbrainz5.h"
#endif /* HAVE_MUSICBRAINZ5 */
#include "sj-metadata-gvfs.h"
#include "sj-error.h"

#define SJ_SETTINGS_PROXY_HOST "host"
#define SJ_SETTINGS_PROXY_PORT "port"
#define SJ_SETTINGS_PROXY_USE_AUTHENTICATION "use-authentication"
#define SJ_SETTINGS_PROXY_USERNAME "authentication-user"
#define SJ_SETTINGS_PROXY_PASSWORD "authentication-password"

enum {
  METADATA,
  LAST_SIGNAL
};

struct SjMetadataGetterPrivate {
  char *url;
  char *cdrom;
};

struct SjMetadataGetterSignal {
  SjMetadataGetter *mdg;
  SjMetadata *metadata;
  GList *albums;
  GError *error;
};

typedef struct SjMetadataGetterPrivate SjMetadataGetterPrivate;
typedef struct SjMetadataGetterSignal SjMetadataGetterSignal;

static int signals[LAST_SIGNAL] = { 0 };

static void sj_metadata_getter_finalize (GObject *object);
static void sj_metadata_getter_init (SjMetadataGetter *mdg);

G_DEFINE_TYPE(SjMetadataGetter, sj_metadata_getter, G_TYPE_OBJECT);

#define GETTER_PRIVATE(o)                                            \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), SJ_TYPE_METADATA_GETTER, SjMetadataGetterPrivate))

static void
sj_metadata_getter_class_init (SjMetadataGetterClass *klass)
{
  GObjectClass *object_class;
  object_class = (GObjectClass *)klass;

  g_type_class_add_private (klass, sizeof (SjMetadataGetterPrivate));

  object_class->finalize = sj_metadata_getter_finalize;

  /* Properties */
  signals[METADATA] = g_signal_new ("metadata",
				    G_TYPE_FROM_CLASS (object_class),
				    G_SIGNAL_RUN_LAST,
				    G_STRUCT_OFFSET (SjMetadataGetterClass, metadata),
				    NULL, NULL,
				    metadata_marshal_VOID__POINTER_POINTER,
				    G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_POINTER);
}

static void
sj_metadata_getter_finalize (GObject *object)
{
  SjMetadataGetterPrivate *priv = GETTER_PRIVATE (object);

  g_free (priv->url);
  g_free (priv->cdrom);

  G_OBJECT_CLASS (sj_metadata_getter_parent_class)->finalize (object);
}

static void
sj_metadata_getter_init (SjMetadataGetter *mdg)
{
}

SjMetadataGetter *
sj_metadata_getter_new (void)
{
  return SJ_METADATA_GETTER (g_object_new (SJ_TYPE_METADATA_GETTER, NULL));
}

void
sj_metadata_getter_set_cdrom (SjMetadataGetter *mdg, const char* device)
{
  SjMetadataGetterPrivate *priv;

  priv = GETTER_PRIVATE (mdg);

  g_free (priv->cdrom);

#ifdef __sun
  if (g_str_has_prefix (device, "/dev/dsk/")) {
    priv->cdrom = g_strdup_printf ("/dev/rdsk/%s", device + strlen ("/dev/dsk/"));
    return;
  }
#endif
  priv->cdrom = g_strdup (device);
}

static void
bind_http_proxy_settings (SjMetadata *metadata)
{
  GSettings *settings = g_settings_new ("org.gnome.system.proxy.http");
  /* bind with G_SETTINGS_BIND_GET_NO_CHANGES to avoid occasional
     segfaults in g_object_set_property called with an invalid pointer
     which I think were caused by the update being scheduled before
     metadata was destroy but happening afterwards (g_settings_bind is
     not called from the main thread). metadata is a short lived
     object so there shouldn't be a problem in practice, as the setting
     are unlikely to change while it exists. If the settings change
     between ripping one CD and the next then as a new metadata object
     is created for the second query it will have the updated
     settings. */
  g_settings_bind (settings, SJ_SETTINGS_PROXY_HOST,
                   metadata, "proxy-host",
                   G_SETTINGS_BIND_GET_NO_CHANGES);

  g_settings_bind (settings, SJ_SETTINGS_PROXY_PORT,
                   metadata, "proxy-port",
                   G_SETTINGS_BIND_GET_NO_CHANGES);

  g_settings_bind (settings, SJ_SETTINGS_PROXY_USERNAME,
                   metadata, "proxy-username",
                   G_SETTINGS_BIND_GET_NO_CHANGES);

  g_settings_bind (settings, SJ_SETTINGS_PROXY_PASSWORD,
                   metadata, "proxy-password",
                   G_SETTINGS_BIND_GET_NO_CHANGES);

  g_settings_bind (settings, SJ_SETTINGS_PROXY_USE_AUTHENTICATION,
                   metadata, "proxy-use-authentication",
                   G_SETTINGS_BIND_GET_NO_CHANGES);

  g_object_unref (settings);
}

static gboolean
fire_signal_idle (SjMetadataGetterSignal *signal)
{
  /* The callback is the sucker, and now owns the albums list */
  g_signal_emit_by_name (G_OBJECT (signal->mdg), "metadata",
  			 signal->albums, signal->error);

  if (signal->metadata)
    g_object_unref (signal->metadata);
  if (signal->error != NULL)
    g_error_free (signal->error);
  g_object_unref (signal->mdg);
  g_free (signal);

  return FALSE;
}

static gpointer
lookup_cd (SjMetadataGetter *mdg)
{
  guint i;
  SjMetadataGetterPrivate *priv;
  GError *error = NULL;
  gboolean found = FALSE;
  GType types[] = {
#ifdef HAVE_MUSICBRAINZ5
    SJ_TYPE_METADATA_MUSICBRAINZ5,
#endif /* HAVE_MUSICBRAINZ5 */
    SJ_TYPE_METADATA_GVFS
  };

  priv = GETTER_PRIVATE (mdg);

  g_free (priv->url);
  priv->url = NULL;
  
  for (i = 0; i < G_N_ELEMENTS (types); i++) {
    SjMetadata *metadata;
    GList *albums;

    metadata = g_object_new (types[i],
    			     "device", priv->cdrom,
    			     NULL);

    bind_http_proxy_settings (metadata);
    if (priv->url == NULL)
      albums = sj_metadata_list_albums (metadata, &priv->url, &error);
    else
      albums = sj_metadata_list_albums (metadata, NULL, &error);

    if (albums != NULL) {
      SjMetadataGetterSignal *signal;

      signal = g_new0 (SjMetadataGetterSignal, 1);
      signal->albums = albums;
      signal->mdg = g_object_ref (mdg);
      signal->metadata = metadata;
      g_idle_add ((GSourceFunc)fire_signal_idle, signal);
      break;
    }

    g_object_unref (metadata);

    if (error != NULL) {
      SjMetadataGetterSignal *signal;

      g_assert (found == FALSE);

      signal = g_new0 (SjMetadataGetterSignal, 1);
      signal->error = error;
      signal->mdg = g_object_ref (mdg);
      g_idle_add ((GSourceFunc)fire_signal_idle, signal);
      break;
    }
  }

  g_object_unref (mdg);

  return NULL;
}

gboolean
sj_metadata_getter_list_albums (SjMetadataGetter *mdg, GError **error)
{
  g_object_ref (mdg);
  g_thread_new ("sj-list-albums", (GThreadFunc)lookup_cd, mdg);

  return TRUE;
}

char *
sj_metadata_getter_get_submit_url (SjMetadataGetter *mdg)
{
  SjMetadataGetterPrivate *priv;

  priv = GETTER_PRIVATE (mdg);

  if (priv->url)
    return g_strdup (priv->url);
  return NULL;
}

