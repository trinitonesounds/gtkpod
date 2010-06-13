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
#include "libgtkpod/gtkpod_app_iface.h"
#include "plugin.h"
#include "display_coverart.h"

/* Parent class. Part of standard class definition */
static gpointer parent_class;

static GtkActionEntry cover_actions[] =
    {
    };

static gboolean activate_plugin(AnjutaPlugin *plugin) {
    AnjutaUI *ui;
    CoverDisplayPlugin *cover_display_plugin;
    GtkActionGroup* action_group;

    cover_display_plugin = (CoverDisplayPlugin*) plugin;
    ui = anjuta_shell_get_ui(plugin->shell, NULL);

    /* Add our cover_actions */
    action_group
            = anjuta_ui_add_action_group_entries(ui, "ActionGroupCoverDisplay", _("Cover Display"), cover_actions, G_N_ELEMENTS (cover_actions), GETTEXT_PACKAGE, TRUE, plugin);
    cover_display_plugin->action_group = action_group;

    /* Merge UI */
    cover_display_plugin->uiid = anjuta_ui_merge(ui, UI_FILE);

    /* Add widget in Shell. Any number of widgets can be added */
    cover_display_plugin->cover_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_ref(cover_display_plugin->cover_window);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (cover_display_plugin->cover_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW (cover_display_plugin->cover_window), GTK_SHADOW_IN);

    g_signal_connect (gtkpod_app, SIGNAL_PLAYLIST_SELECTED, G_CALLBACK (coverart_display_update_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_TRACK_REMOVED, G_CALLBACK (coverart_display_track_removed_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_TRACKS_SELECTED, G_CALLBACK (coverart_display_set_tracks_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_TRACK_UPDATED, G_CALLBACK (coverart_display_track_updated_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_TRACK_ADDED, G_CALLBACK (coverart_display_track_added_cb), NULL);

    coverart_init_display(cover_display_plugin->cover_window);
    anjuta_shell_add_widget(plugin->shell, cover_display_plugin->cover_window, "CoverDisplayPlugin", "Cover Artwork", NULL, ANJUTA_SHELL_PLACEMENT_CENTER, NULL);

    coverart_display_update(TRUE);

    return TRUE; /* FALSE if activation failed */
}

static gboolean deactivate_plugin(AnjutaPlugin *plugin) {
    AnjutaUI *ui;
    CoverDisplayPlugin *cover_display_plugin;

    cover_display_plugin = (CoverDisplayPlugin*) plugin;
    ui = anjuta_shell_get_ui(plugin->shell, NULL);

    /* Remove widgets from Shell */
    anjuta_shell_remove_widget(plugin->shell, cover_display_plugin->cover_window, NULL);

    /* Destroy the treeview */
    destroy_coverart_display();

    /* Unmerge UI */
    anjuta_ui_unmerge(ui, cover_display_plugin->uiid);

    /* Remove Action groups */
    anjuta_ui_remove_action_group(ui, cover_display_plugin->action_group);

    /* FALSE if plugin doesn't want to deactivate */
    return TRUE;
}

static void cover_display_plugin_instance_init(GObject *obj) {
    CoverDisplayPlugin *plugin = (CoverDisplayPlugin*) obj;
    plugin->uiid = 0;
    plugin->cover_window = NULL;
    plugin->action_group = NULL;
}

static void cover_display_plugin_class_init(GObjectClass *klass) {
    AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

    parent_class = g_type_class_peek_parent(klass);

    plugin_class->activate = activate_plugin;
    plugin_class->deactivate = deactivate_plugin;
}

ANJUTA_PLUGIN_BEGIN (CoverDisplayPlugin, cover_display_plugin);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (CoverDisplayPlugin, cover_display_plugin);
