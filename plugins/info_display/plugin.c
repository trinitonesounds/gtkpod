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
#include "libgtkpod/gtkpod_app_iface.h"
#include "libgtkpod/directories.h"
#include "plugin.h"
#include "info.h"
#include "infoview.h"

/* Parent class. Part of standard class definition */
static gpointer parent_class;

static GtkActionEntry info_actions[] =
    {
        {
            "ActionDisplayInfoView", /* Action name */
            GTK_STOCK_DIALOG_INFO, /* Stock icon */
            N_("_Info View"), /* Display label */
            NULL, /* short-cut */
            NULL, /* Tooltip */
            G_CALLBACK (on_info_view_open) /* callback */
        },
    };

static gboolean activate_plugin(AnjutaPlugin *plugin) {
    AnjutaUI *ui;
    GtkActionGroup* action_group;

    info_display_plugin = (InfoDisplayPlugin*) plugin;
    ui = anjuta_shell_get_ui(plugin->shell, NULL);

    /* Add our info_actions */
    action_group
            = anjuta_ui_add_action_group_entries(ui, "ActionGroupInfoDisplay", _("Info Display"), info_actions, G_N_ELEMENTS (info_actions), GETTEXT_PACKAGE, TRUE, plugin);
    info_display_plugin->action_group = action_group;

    /* Merge UI */
    gchar *uipath = g_build_filename(get_ui_dir(), "info_display.ui", NULL);
    info_display_plugin->uiid = anjuta_ui_merge(ui, uipath);
    g_free(uipath);

    info_display_init();

    g_signal_connect (gtkpod_app, SIGNAL_PLAYLIST_SELECTED, G_CALLBACK (info_display_playlist_selected_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_PLAYLIST_ADDED, G_CALLBACK (info_display_playlist_added_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_PLAYLIST_REMOVED, G_CALLBACK (info_display_playlist_removed_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_TRACK_UPDATED, G_CALLBACK (info_display_track_updated_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_TRACK_REMOVED, G_CALLBACK (info_display_track_removed_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_TRACKS_DISPLAYED, G_CALLBACK (info_display_tracks_displayed_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_TRACKS_SELECTED, G_CALLBACK (info_display_tracks_selected_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_ITDB_DATA_CHANGED, G_CALLBACK (info_display_itdb_changed_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_ITDB_DATA_SAVED, G_CALLBACK (info_display_itdb_changed_cb), NULL);

    return TRUE; /* FALSE if activation failed */
}

static gboolean deactivate_plugin(AnjutaPlugin *plugin) {
    AnjutaUI *ui;

    destroy_info_view ();

    info_display_plugin = (InfoDisplayPlugin*) plugin;
    ui = anjuta_shell_get_ui(plugin->shell, NULL);

    /* Unmerge UI */
    anjuta_ui_unmerge(ui, info_display_plugin->uiid);

    /* Remove Action groups */
    anjuta_ui_remove_action_group(ui, info_display_plugin->action_group);

    /* FALSE if plugin doesn't want to deactivate */
    return TRUE;
}

static void info_display_plugin_instance_init(GObject *obj) {
    InfoDisplayPlugin *plugin = (InfoDisplayPlugin*) obj;
    plugin->uiid = 0;
    plugin->action_group = NULL;
}

static void info_display_plugin_class_init(GObjectClass *klass) {
    AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

    parent_class = g_type_class_peek_parent(klass);

    plugin_class->activate = activate_plugin;
    plugin_class->deactivate = deactivate_plugin;
}

ANJUTA_PLUGIN_BEGIN (InfoDisplayPlugin, info_display_plugin);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (InfoDisplayPlugin, info_display_plugin);
