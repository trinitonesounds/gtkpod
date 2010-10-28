/*
 |  Copyright (C) 2002-2010 Jorg Schuler <jcsjcs at users sourceforge net>
 |                                          Paul Richardson <phantom_sf at users.sourceforge.net>
 |  Part of the gtkpod project.
 |
 |  URL: http://www.gtkpod.org/
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
 |
 |  $Id$
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/interfaces/ianjuta-preferences.h>
#include "libgtkpod/gtkpod_app_iface.h"
#include "libgtkpod/prefs.h"
#include "libgtkpod/directories.h"
#include "libgtkpod/stock_icons.h"
#include "plugin.h"
#include "mserv.h"
#include "mserv_preferences.h"
#include "mserv_actions.h"

#define PREFERENCE_ICON "mserv-gtkpod-category"
#define PREFERENCE_ICON_STOCK_ID "mserv-preference-icon"
#define TAB_NAME "Mserv"

/* Parent class. Part of standard class definition */
static gpointer parent_class;

static GtkActionEntry mserv_actions[] =
    {
        {
            "ActionUpdateMservMenu",
            GTK_STOCK_REFRESH,
            N_("_Update mserv Data from File"),
            NULL,
            NULL,
            NULL
        },
        {
            "ActionUpdateMservPlaylist",
            GTK_STOCK_REFRESH,
            N_("Selected Playlist"),
            NULL,
            NULL,
            G_CALLBACK (on_update_mserv_selected_playlist)
        },
        {
            "ActionUpdateMservTracks",
            GTK_STOCK_REFRESH,
            N_("Selected Tracks"),
            NULL,
            NULL,
            G_CALLBACK (on_update_mserv_selected_tracks)
        }
    };

static gboolean activate_plugin(AnjutaPlugin *plugin) {
    AnjutaUI *ui;
    MservPlugin *mserv_plugin;
    GtkActionGroup* action_group;

    register_icon_path(get_plugin_dir(), "mserv");
    register_stock_icon(PREFERENCE_ICON, PREFERENCE_ICON_STOCK_ID);

    /* Set preferences */
    set_default_preferences();

    mserv_plugin = (MservPlugin*) plugin;
    ui = anjuta_shell_get_ui(plugin->shell, NULL);

    /* Add our cover_actions */
    action_group
            = anjuta_ui_add_action_group_entries(ui, "ActionGroupMserv", _("Mserv"), mserv_actions, G_N_ELEMENTS (mserv_actions), GETTEXT_PACKAGE, TRUE, plugin);
    mserv_plugin->action_group = action_group;

    /* Merge UI */
    gchar *uipath = g_build_filename(get_ui_dir(), "mserv.ui", NULL);
    mserv_plugin->uiid = anjuta_ui_merge(ui, uipath);
    g_free(uipath);

    /* Set preferences */
    set_default_preferences();

    // FIXME
    // Track updated signal should call update_mserv_data_from_file


    return TRUE; /* FALSE if activation failed */
}

static gboolean deactivate_plugin(AnjutaPlugin *plugin) {
    AnjutaUI *ui;
    MservPlugin *mserv_plugin;

    mserv_plugin = (MservPlugin*) plugin;
    ui = anjuta_shell_get_ui(plugin->shell, NULL);

    /* Unmerge UI */
    anjuta_ui_unmerge(ui, mserv_plugin->uiid);

    /* Remove Action groups */
    anjuta_ui_remove_action_group(ui, mserv_plugin->action_group);

    /* FALSE if plugin doesn't want to deactivate */
    return TRUE;
}

static void mserv_plugin_instance_init(GObject *obj) {
    MservPlugin *plugin = (MservPlugin*) obj;
    plugin->uiid = 0;
    plugin->action_group = NULL;
}

static void mserv_plugin_class_init(GObjectClass *klass) {
    AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

    parent_class = g_type_class_peek_parent(klass);

    plugin_class->activate = activate_plugin;
    plugin_class->deactivate = deactivate_plugin;
}

static void ipreferences_merge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e) {
    GdkPixbuf *pixbuf;
    GError *error = NULL;

    MservPlugin* plugin = MSERV_PLUGIN(ipref);
    plugin->prefs = init_mserv_preferences();
    if (plugin->prefs == NULL)
        return;

    pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), PREFERENCE_ICON, 48, 0, &error);

    if (!pixbuf) {
        g_warning ("Couldn't load icon: %s", error->message);
        g_error_free(error);
    }
    anjuta_preferences_dialog_add_page(ANJUTA_PREFERENCES_DIALOG (anjuta_preferences_get_dialog (prefs)), "gtkpod-mserv-settings", _(TAB_NAME), pixbuf, plugin->prefs);
    g_object_unref(pixbuf);
}

static void ipreferences_unmerge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e) {
    anjuta_preferences_remove_page(prefs, _(TAB_NAME));
    MservPlugin* plugin = MSERV_PLUGIN(ipref);
    gtk_widget_destroy(plugin->prefs);
}

static void ipreferences_iface_init(IAnjutaPreferencesIface* iface) {
    iface->merge = ipreferences_merge;
    iface->unmerge = ipreferences_unmerge;
}

ANJUTA_PLUGIN_BEGIN (MservPlugin, mserv_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE(ipreferences, IANJUTA_TYPE_PREFERENCES);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (MservPlugin, mserv_plugin);
