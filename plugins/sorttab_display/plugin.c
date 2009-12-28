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
#include "libgtkpod/stock_icons.h"
#include "libgtkpod/gtkpod_app_iface.h"
#include "plugin.h"
#include "display_sorttabs.h"

/* Parent class. Part of standard class definition */
static gpointer parent_class;

static GtkActionEntry sorttab_actions[] =
    {
    };

static gboolean activate_plugin(AnjutaPlugin *plugin) {
    AnjutaUI *ui;
    SorttabDisplayPlugin *sorttab_display_plugin;
    GtkActionGroup* action_group;

    /* Prepare the icons for the sorttab */

    sorttab_display_plugin = (SorttabDisplayPlugin*) plugin;
    ui = anjuta_shell_get_ui(plugin->shell, NULL);

    /* Add our sorttab_actions */
    action_group
            = anjuta_ui_add_action_group_entries(ui, "ActionGroupSorttabDisplay", _("Sorttab Display"), sorttab_actions, G_N_ELEMENTS (sorttab_actions), GETTEXT_PACKAGE, TRUE, plugin);
    sorttab_display_plugin->action_group = action_group;

    /* Merge UI */
    sorttab_display_plugin->uiid = anjuta_ui_merge(ui, UI_FILE);

    /* Add widget in Shell. Any number of widgets can be added */

    sorttab_display_plugin->st_paned = gtk_hpaned_new();
    st_create_tabs(sorttab_display_plugin->st_paned);

//    g_signal_connect (gtkpod_app, "sorttab_selected", G_CALLBACK (sorttab_display_select_sorttab_cb), NULL);
//    g_signal_connect (gtkpod_app, "itdb_updated", G_CALLBACK (sorttab_display_update_itdb_cb), NULL);

    gtk_widget_show_all(sorttab_display_plugin->st_paned);
    anjuta_shell_add_widget(plugin->shell, sorttab_display_plugin->st_paned, "SorttabDisplayPlugin", "", NULL, ANJUTA_SHELL_PLACEMENT_BOTTOM, NULL);

    return TRUE; /* FALSE if activation failed */
}

static gboolean deactivate_plugin(AnjutaPlugin *plugin) {
    AnjutaUI *ui;
    SorttabDisplayPlugin *sorttab_display_plugin;

    sorttab_display_plugin = (SorttabDisplayPlugin*) plugin;
    ui = anjuta_shell_get_ui(plugin->shell, NULL);

    /* Remove widgets from Shell */
    anjuta_shell_remove_widget(plugin->shell, sorttab_display_plugin->st_paned, NULL);

    /* Destroy the treeview */
    sorttab_display_plugin->st_paned = NULL;

    /* Unmerge UI */
    anjuta_ui_unmerge(ui, sorttab_display_plugin->uiid);

    /* Remove Action groups */
    anjuta_ui_remove_action_group(ui, sorttab_display_plugin->action_group);

    /* FALSE if plugin doesn't want to deactivate */
    return TRUE;
}

static void sorttab_display_plugin_instance_init(GObject *obj) {
    SorttabDisplayPlugin *plugin = (SorttabDisplayPlugin*) obj;
    plugin->uiid = 0;
    plugin->st_paned = NULL;
    plugin->action_group = NULL;
}

static void sorttab_display_plugin_class_init(GObjectClass *klass) {
    AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

    parent_class = g_type_class_peek_parent(klass);

    plugin_class->activate = activate_plugin;
    plugin_class->deactivate = deactivate_plugin;
}

ANJUTA_PLUGIN_BEGIN (SorttabDisplayPlugin, sorttab_display_plugin);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (SorttabDisplayPlugin, sorttab_display_plugin)
;
