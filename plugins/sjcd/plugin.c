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
#include "sj-main.h"

/* Parent class. Part of standard class definition */
static gpointer parent_class;

static GtkActionEntry sjcd_actions[] =
    {};

static gboolean activate_plugin(AnjutaPlugin *plugin) {
    AnjutaUI *ui;
    GtkActionGroup* action_group;

    /* Prepare the icons for the playlist */
//    register_icon_path(get_plugin_dir(), "sjcd");
//    register_stock_icon(PREFERENCE_ICON, PREFERENCE_ICON_STOCK_ID);

    sjcd_plugin = (SJCDPlugin*) plugin;
    ui = anjuta_shell_get_ui(plugin->shell, NULL);

    /* Add actions */
    action_group
            = anjuta_ui_add_action_group_entries(ui, "ActionGroupSjCd", _("Sound Juicer"), sjcd_actions, G_N_ELEMENTS (sjcd_actions), GETTEXT_PACKAGE, TRUE, plugin);
    sjcd_plugin->action_group = action_group;

    /* Merge UI */
    gchar *uipath = g_build_filename(get_ui_dir(), "sjcd.ui", NULL);
    sjcd_plugin->uiid = anjuta_ui_merge(ui, uipath);
    g_free(uipath);

    sjcd_plugin->sj_view = sj_create_sound_juicer();
    gtk_widget_show_all(sjcd_plugin->sj_view);
    // Add widget directly as scrolling is handled internally by the widget
    anjuta_shell_add_widget(plugin->shell, sjcd_plugin->sj_view, "SJCDPlugin", _("  Sound Juicer"), NULL, ANJUTA_SHELL_PLACEMENT_TOP, NULL);


    return TRUE; /* FALSE if activation failed */
}

static gboolean deactivate_plugin(AnjutaPlugin *plugin) {
    AnjutaUI *ui;

    ui = anjuta_shell_get_ui(plugin->shell, NULL);

    /* Unmerge UI */
    anjuta_ui_unmerge(ui, sjcd_plugin->uiid);

    /* Remove Action groups */
    anjuta_ui_remove_action_group(ui, sjcd_plugin->action_group);

    /* FALSE if plugin doesn't want to deactivate */
    return TRUE;
}

static void sjcd_plugin_instance_init(GObject *obj) {
    SJCDPlugin *plugin = (SJCDPlugin*) obj;
    plugin->uiid = 0;
    plugin->action_group = NULL;
}

static void sjcd_plugin_class_init(GObjectClass *klass) {
    AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

    parent_class = g_type_class_peek_parent(klass);

    plugin_class->activate = activate_plugin;
    plugin_class->deactivate = deactivate_plugin;
}

ANJUTA_PLUGIN_BEGIN (SJCDPlugin, sjcd_plugin);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (SJCDPlugin, sjcd_plugin)
;
