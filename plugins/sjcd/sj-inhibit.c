/* 
 * Copyright (C) 2007 Carl-Anton Ingmarsson <ca.ingmarsson@gmail.com>
 * Copyright (C) 2006-2007 Richard Hughes <richard@hughsie.com>
 *
 * Sound Juicer - sj-inhibit.c
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
 * Authors: Carl-Anton Ingmarsson <ca.ingmarsson@gmail.com>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gio/gio.h>
 
#include "sj-inhibit.h"
 
/* PowerManagent defines */
#define	PM_DBUS_SERVICE    "org.gnome.SessionManager"
#define	PM_DBUS_INHIBIT_PATH   "/org/gnome/SessionManager"
#define	PM_DBUS_INHIBIT_INTERFACE    "org.gnome.SessionManager"

/** cookie is returned as an unsigned integer */
guint
sj_inhibit (const gchar * appname, const gchar * reason, guint xid)
{
  guint cookie;
  GError *error = NULL;
  GDBusProxy *proxy = NULL;
  GVariant *variant;


  proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                         G_DBUS_PROXY_FLAGS_NONE,
                                         NULL,
                                         PM_DBUS_SERVICE,
                                         PM_DBUS_INHIBIT_PATH,
                                         PM_DBUS_INHIBIT_INTERFACE,
                                         NULL,
                                         &error);

  if (!proxy)
  {
    g_warning ("Could not get DBUS proxy: %s", error->message);
    g_clear_error (&error);
    return 0;
  }


  variant = g_dbus_proxy_call_sync (proxy,
                                    "Inhibit",
                                    g_variant_new ("(susu)",
                                                   appname,
                                                   xid,
                                                   reason,
                                                   4+8),
                                    G_DBUS_CALL_FLAGS_NONE, -1, NULL,
                                    &error);
  if (variant)
    {
      g_variant_get (variant, "(u)", &cookie);
      g_variant_unref (variant);
    }
  else
    {
      g_warning ("Problem calling inhibit %s", error->message);
    }

  g_object_unref (proxy);

  return cookie;
}

void
sj_uninhibit (guint cookie)
{
  GError *error = NULL;
  GDBusProxy *proxy = NULL;

  /* check if the cookie is valid */
  if (cookie <= 0)
    {
      g_warning ("Invalid cookie");
      return;
    }


  proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                         G_DBUS_PROXY_FLAGS_NONE,
                                         NULL,
                                         PM_DBUS_SERVICE,
                                         PM_DBUS_INHIBIT_PATH,
                                         PM_DBUS_INHIBIT_INTERFACE,
                                         NULL,
                                         &error);

  if (!proxy)
    {
      g_warning ("Could not get DBUS proxy: %s", error->message);
      g_clear_error (&error);
      return;
    }

  g_dbus_proxy_call_sync (proxy, "Uninhibit",
                          g_variant_new ("(u)", cookie),
                          G_DBUS_CALL_FLAGS_NONE, -1, NULL,
                          &error);

  if (error)
    {
      g_warning ("Problem uninhibiting: %s", error->message);
      g_clear_error (&error);
    }

  g_object_unref (proxy);
}

