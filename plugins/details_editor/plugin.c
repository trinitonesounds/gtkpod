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
#include "details.h"
#include "details_editor_actions.h"

/* Parent class. Part of standard class definition */
static gpointer parent_class;

static GtkActionEntry details_editor_actions[] =
    {
        {
            "ActionEditTrackDetails", /* Action name */
            GTK_STOCK_PREFERENCES, /* Stock icon */
            N_("Edit Track Details"), /* Display label */
            NULL, /* short-cut */
            NULL, /* Tooltip */
            G_CALLBACK (on_edit_details_selected_tracks)
        },
    };

static gboolean activate_plugin(AnjutaPlugin *plugin) {
    AnjutaUI *ui;
    GtkActionGroup* action_group;

    details_editor_plugin = (DetailsEditorPlugin*) plugin;
    ui = anjuta_shell_get_ui(plugin->shell, NULL);

    /* Add our playlist_actions */
    action_group
            = anjuta_ui_add_action_group_entries(ui, "ActionGroupDetailsEditor", _("DetailsEditor"), details_editor_actions, G_N_ELEMENTS (details_editor_actions), GETTEXT_PACKAGE, TRUE, plugin);
    details_editor_plugin->action_group = action_group;

    /* Merge UI */
    gchar *uipath = g_build_filename(get_ui_dir(), "details_editor.ui", NULL);
    details_editor_plugin->uiid = anjuta_ui_merge(ui, uipath);
    g_free(uipath);

    g_return_val_if_fail(DETAILS_EDITOR_IS_EDITOR(details_editor_plugin), TRUE);

    gtkpod_register_details_editor (DETAILS_EDITOR(details_editor_plugin));
    gtkpod_register_lyrics_editor(LYRICS_EDITOR(details_editor_plugin));

    g_signal_connect (gtkpod_app, SIGNAL_TRACK_REMOVED, G_CALLBACK (details_editor_track_removed_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_TRACKS_SELECTED, G_CALLBACK (details_editor_set_tracks_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_TRACKS_DISPLAYED, G_CALLBACK (details_editor_set_tracks_cb), NULL);

    return TRUE; /* FALSE if activation failed */
}

static gboolean deactivate_plugin(AnjutaPlugin *plugin) {
    AnjutaUI *ui;

    details_editor_plugin = (DetailsEditorPlugin*) plugin;

    destroy_details_editor();

    details_editor_plugin->details_window = NULL;
    details_editor_plugin->details_view = NULL;
    details_editor_plugin->details_notebook = NULL;

    gtkpod_unregister_lyrics_editor();
    gtkpod_unregister_details_editor();

    ui = anjuta_shell_get_ui(plugin->shell, NULL);

    /* Unmerge UI */
    anjuta_ui_unmerge(ui, details_editor_plugin->uiid);

    /* Remove Action groups */
    anjuta_ui_remove_action_group(ui, details_editor_plugin->action_group);

    /* FALSE if plugin doesn't want to deactivate */
    return TRUE;
}

static void details_editor_plugin_instance_init(GObject *obj) {
    DetailsEditorPlugin *plugin = (DetailsEditorPlugin*) obj;
    plugin->uiid = 0;
    plugin->action_group = NULL;
}

static void details_editor_plugin_class_init(GObjectClass *klass) {
    AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

    parent_class = g_type_class_peek_parent(klass);

    plugin_class->activate = activate_plugin;
    plugin_class->deactivate = deactivate_plugin;
}

static void details_editor_iface_init(DetailsEditorInterface *iface) {
    iface->edit_details = details_edit;
}

static void lyrics_editor_iface_init(LyricsEditorInterface *iface) {
    iface->edit_lyrics = lyrics_edit;
}

ANJUTA_PLUGIN_BEGIN (DetailsEditorPlugin, details_editor_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE(details_editor, DETAILS_EDITOR_TYPE);
ANJUTA_PLUGIN_ADD_INTERFACE(lyrics_editor, LYRICS_EDITOR_TYPE);
ANJUTA_PLUGIN_END;
ANJUTA_SIMPLE_PLUGIN (DetailsEditorPlugin, details_editor_plugin)
;
