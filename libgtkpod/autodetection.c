/*
 |  Copyright (C) 2007 Jorg Schuler <jcsjcs at users.sourceforge.net>
 |  Part of the gtkpod project.
 |
 |  URL: http://gtkpod.sourceforge.net/
 |  URL: http://www.gtkpod.org
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
 |
 |  $Id$
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <glib/gi18n-lib.h>
#include <gio/gio.h>
#include "autodetection.h"
#include "gp_itdb.h"
#include "gtkpod_app_iface.h"
#include "misc.h"
#include "prefs.h"

#undef DEBUG_AUTO
#ifdef DEBUG_AUTO
#   define _TO_STR(x) #x
#   define TO_STR(x) _TO_STR(x)
#   define debug(...) do { fprintf(stderr,  __FILE__ ":" TO_STR(__LINE__) ":" __VA_ARGS__); } while(0)
#else
#   define debug(...)
#endif

/* Find out if an itdb uses the mountpoint @mountpoint and return that
 itdb */
static iTunesDB *ad_find_repository_with_mountpoint(const gchar *mountpoint) {
    GList *gl;
    gchar *mp;
    gint lenmp;
    iTunesDB *result = NULL;
    struct itdbs_head *itdbs;

    g_return_val_if_fail (mountpoint, NULL);

    itdbs = gp_get_itdbs_head();
    g_return_val_if_fail (itdbs, NULL);

    /* eliminate trailing dir separators ('/') */
    mp = g_strdup(mountpoint);
    lenmp = strlen(mountpoint);
    if ((lenmp > 0) && (mp[lenmp - 1] == G_DIR_SEPARATOR)) {
        mp[lenmp - 1] = 0;
    }

    for (gl = itdbs->itdbs; gl; gl = gl->next) {
        iTunesDB *itdb = gl->data;
        g_return_val_if_fail (itdb, NULL);

        if (itdb->usertype & GP_ITDB_TYPE_IPOD) {
            gchar *imp = get_itdb_prefs_string(itdb, KEY_MOUNTPOINT);
            if (imp) {
                gint comp;
                gint lenimp = strlen(imp);

                /* eliminate trailing dir separators ('/') */
                if ((lenimp > 0) && (imp[lenimp - 1] == G_DIR_SEPARATOR)) {
                    imp[lenmp - 1] = 0;
                }

                comp = strcmp(mp, imp);

                g_free(imp);

                if (comp == 0) {
                    result = itdb;
                    break;
                }
            }
        }
    }

    g_free(mp);

    return result;
}

typedef struct _AutoDetect AutoDetect;

static gboolean ad_timeout_cb(gpointer data);

struct _AutoDetect {
    GMutex *mutex; /* shared lock */
    GList *new_ipod_uris; /* list of new mounts */
    guint timeout_id;
};

static AutoDetect *autodetect;

/* adapted from rb-ipod-plugin.c (rhythmbox ipod plugin) */
static gboolean ad_mount_is_ipod(GMount *mount) {
    gboolean result = FALSE;
    GFile *root = g_mount_get_root(mount);
    if (root != NULL) {
        gchar *mount_point;
        mount_point = g_file_get_path(root);
        if (mount_point != NULL) {
            gchar *itunes_dir;
            itunes_dir = itdb_get_itunes_dir(mount_point);
            if (itunes_dir != NULL) {
                result = g_file_test(itunes_dir, G_FILE_TEST_IS_DIR);
                g_free(itunes_dir);
            }
            g_free(mount_point);
        }
        g_object_unref(root);
    }
    return result;
}

static void ad_volume_mounted_cb(GVolumeMonitor *volumemonitor, GMount *mount, AutoDetect *ad) {
    g_return_if_fail (mount && ad);

    if (G_IS_MOUNT(mount) && ad_mount_is_ipod(mount)) {
        GFile *root;
        root = g_mount_get_root(mount);
        if (root) {
            gchar *uri;
            uri = g_file_get_path(root);
            if (uri) {
                debug ("mounted iPod: '%s'\n", uri);

                g_mutex_lock (ad->mutex);
                ad->new_ipod_uris = g_list_prepend(ad->new_ipod_uris, uri);
                g_mutex_unlock (ad->mutex);
            }
            else {
                fprintf(stderr, "ERROR: could not get activation root!\n");
            }
            g_object_unref(root);
        }
    }
}

void autodetection_init() {
    if (autodetect == NULL) {
        GList *mounts, *gl;

        static GOnce g_type_init_once =
        G_ONCE_INIT;
        g_once (&g_type_init_once, (GThreadFunc)g_type_init, NULL);

        autodetect = g_new0 (AutoDetect, 1);
        autodetect->mutex = g_mutex_new ();

        /* Check if an iPod is already mounted and add it to the list */
        mounts = g_volume_monitor_get_mounts(g_volume_monitor_get());

        for (gl = mounts; gl; gl = gl->next) {
            GMount *mount = gl->data;
            g_return_if_fail (mount);
            ad_volume_mounted_cb(NULL, mount, autodetect);
            g_object_unref(mount);
        }
        g_list_free(mounts);

        g_signal_connect (G_OBJECT (g_volume_monitor_get()),
                "mount_added",
                G_CALLBACK (ad_volume_mounted_cb),
                autodetect);

        /* start timeout function for the monitor */
        autodetect->timeout_id = g_timeout_add(100, /* every 100 ms */
        ad_timeout_cb, autodetect);
    }
}

static gboolean ad_timeout_cb(gpointer data) {
    AutoDetect *ad = data;
    g_return_val_if_fail (ad, FALSE);

    /* Don't interfere with a blocked display -- try again later */
    if (!widgets_blocked) {
        gdk_threads_enter();
        g_mutex_lock (ad->mutex);

        while (ad->new_ipod_uris) {
            iTunesDB *itdb = NULL, *loaded_itdb = NULL;
            gchar *mountpoint;
            struct itdbs_head *itdbs;
            GList *gl = ad->new_ipod_uris;
            gchar *mount_uri = gl->data;

            ad->new_ipod_uris = g_list_delete_link(ad->new_ipod_uris, gl);

            g_mutex_unlock (ad->mutex);

            g_return_val_if_fail (mount_uri, (gdk_threads_leave(), release_widgets(), TRUE));

            GFile *muri = g_file_parse_name(mount_uri);
            mountpoint = g_file_get_path(muri);
            g_object_unref(muri);
            g_free(mount_uri);

            if (mountpoint) {
                debug ("Mounted iPod at '%s'\n", mountpoint);
                itdb = ad_find_repository_with_mountpoint(mountpoint);
            }

            itdbs = gp_get_itdbs_head();
            g_return_val_if_fail (itdbs, (gdk_threads_leave(), release_widgets(), TRUE));

            block_widgets();

            if (itdb) {
                ExtraiTunesDBData *eitdb = itdb->userdata;
                g_return_val_if_fail (eitdb,(gdk_threads_leave(), release_widgets(), TRUE));

                debug ("...used by itdb %p\n", itdb);

                if (!eitdb->itdb_imported) {
                    loaded_itdb = gp_load_ipod(itdb);
                    if (loaded_itdb) {
                        loaded_itdb->usertype |= GP_ITDB_TYPE_AUTOMATIC;
                        set_itdb_prefs_int(loaded_itdb, "type", loaded_itdb->usertype);
                    }
                    else {
                        gtkpod_warning(_("Newly mounted iPod at '%s' could not be loaded into gtkpod.\n\n"), mountpoint);
                    }
                }
                else {
                    gtkpod_warning(_("Newly mounted iPod at '%s' appears to be already loaded!\n\n"), mountpoint);
                } debug ("...OK (used)\n");
            }
            else { /* Set up new itdb (which we'll add to the end of the list) */
                iTunesDB *new_itdb;
                gint index = g_list_length(itdbs->itdbs);
                debug ("...not used by any itdb.\n");
                set_itdb_index_prefs_string(index, KEY_MOUNTPOINT, mountpoint);
                set_itdb_index_prefs_string(index, "name", _("New iPod"));
                set_itdb_index_prefs_int(index, "type", GP_ITDB_TYPE_IPOD | GP_ITDB_TYPE_AUTOMATIC);
                new_itdb = setup_itdb_n(index);
                g_return_val_if_fail (new_itdb,
                        (gdk_threads_leave(), release_widgets(), TRUE));
                /* add to display */
                gp_itdb_add(new_itdb, -1);
                /* load prefs from iPod */
                loaded_itdb = gp_load_ipod(new_itdb);
                if (!loaded_itdb) { /* remove itdb and all associated keys again */
                    remove_itdb_prefs(itdb);
                    gp_itdb_remove(new_itdb);
                    gp_itdb_free(new_itdb);
                } debug ("...OK (new)\n");
            }

            release_widgets();

            g_free(mountpoint);

            g_mutex_lock (ad->mutex);
        }
        g_mutex_unlock (ad->mutex);
        gdk_threads_leave();
    }

    return TRUE;
}
