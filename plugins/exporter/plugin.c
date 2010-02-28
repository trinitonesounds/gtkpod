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
#include "plugin.h"
#include "file_export.h"
#include "exporter_actions.h"

/* Parent class. Part of standard class definition */
static gpointer parent_class;

static GtkActionEntry exporter_actions[] =
    {
        {
            "ActionToolsExportMenu",
            GTK_STOCK_SAVE_AS,
            N_("_Export Tracks"),
            NULL,
            NULL,
            NULL
        },
        {
            "ActionExportToPlaylistFile", /* Action name */
            NULL, /* Stock icon */
            N_("Export Tracks To Playlist File"), /* Display label */
            NULL, /* short-cut */
            NULL, /* Tooltip */
            G_CALLBACK (on_export_tracks_to_playlist_file) /* callback */
        },
        {
            "ActionExportToFilesystem", /* Action name */
            NULL, /* Stock icon */
            N_("Export Tracks To Filesystem"), /* Display label */
            NULL, /* short-cut */
            NULL, /* Tooltip */
            G_CALLBACK (on_export_tracks_to_filesystem) /* callback */
        },
    };

static gboolean activate_plugin(AnjutaPlugin *plugin) {
    AnjutaUI *ui;
    ExporterPlugin *exporter_plugin;
    GtkActionGroup* action_group;

    exporter_plugin = (ExporterPlugin*) plugin;
    ui = anjuta_shell_get_ui(plugin->shell, NULL);

    /* Add our playlist_actions */
    action_group
            = anjuta_ui_add_action_group_entries(ui, "ActionGroupExporter", _("Exporter"), exporter_actions, G_N_ELEMENTS (exporter_actions), GETTEXT_PACKAGE, TRUE, plugin);
    exporter_plugin->action_group = action_group;

    /* Merge UI */
    exporter_plugin->uiid = anjuta_ui_merge(ui, UI_FILE);

    gtkpod_register_exporter(export_tracklist_when_necessary, export_trackglist_when_necessary);

//    g_signal_connect (gtkpod_app, SIGNAL_TRACK_REMOVED, G_CALLBACK (exporter_track_removed_cb), NULL);

    return TRUE; /* FALSE if activation failed */
}

static gboolean deactivate_plugin(AnjutaPlugin *plugin) {
    AnjutaUI *ui;
    ExporterPlugin *exporter_plugin;

    gtkpod_unregister_exporter();

    exporter_plugin = (ExporterPlugin*) plugin;
    ui = anjuta_shell_get_ui(plugin->shell, NULL);

    /* Unmerge UI */
    anjuta_ui_unmerge(ui, exporter_plugin->uiid);

    /* Remove Action groups */
    anjuta_ui_remove_action_group(ui, exporter_plugin->action_group);

    /* FALSE if plugin doesn't want to deactivate */
    return TRUE;
}

static void exporter_plugin_instance_init(GObject *obj) {
    ExporterPlugin *plugin = (ExporterPlugin*) obj;
    plugin->uiid = 0;
    plugin->action_group = NULL;
}

static void exporter_plugin_class_init(GObjectClass *klass) {
    AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

    parent_class = g_type_class_peek_parent(klass);

    plugin_class->activate = activate_plugin;
    plugin_class->deactivate = deactivate_plugin;
}

ANJUTA_PLUGIN_BEGIN (ExporterPlugin, exporter_plugin);ANJUTA_PLUGIN_END
;

ANJUTA_SIMPLE_PLUGIN (ExporterPlugin, exporter_plugin)
;
