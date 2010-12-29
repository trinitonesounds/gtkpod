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
#include "repository.h"
#include "repository_actions.h"

/* Parent class. Part of standard class definition */
static gpointer parent_class;

static GtkActionEntry repository_editor_actions[] =
    {
        {
            "ActionInitRepository",
            GTK_STOCK_EXECUTE,
            N_("Create iPod's _Directories"),
            NULL,
            NULL,
            G_CALLBACK (on_create_ipod_directories)
        },
        {
            "ActionCheckiPodFiles",
            GTK_STOCK_FILE,
            N_("Check iPod's _Files"),
            NULL,
            NULL,
            G_CALLBACK (on_check_ipod_files)
        },
        {
            "ActionConfigRepositories",
            GTK_STOCK_PREFERENCES,
            N_("_Configure Repositories"),
            NULL,
            NULL,
            G_CALLBACK (on_configure_repositories)
        }
    };

static gboolean activate_plugin(AnjutaPlugin *plugin) {
    AnjutaUI *ui;;
    GtkActionGroup* action_group;

    repository_editor_plugin = (RepositoryEditorPlugin*) plugin;
    ui = anjuta_shell_get_ui(plugin->shell, NULL);

    /* Add our playlist_actions */
    action_group
            = anjuta_ui_add_action_group_entries(ui, "ActionGroupRepositoryEditor", _("Repository Editor"), repository_editor_actions, G_N_ELEMENTS (repository_editor_actions), GETTEXT_PACKAGE, TRUE, plugin);
    repository_editor_plugin->action_group = action_group;

    /* Merge UI */
    gchar *uipath = g_build_filename(get_ui_dir(), "repository_editor.ui", NULL);
    repository_editor_plugin->uiid = anjuta_ui_merge(ui, uipath);
    g_free(uipath);

    g_return_val_if_fail(REPOSITORY_EDITOR_IS_EDITOR(repository_editor_plugin), TRUE);

    gtkpod_register_repository_editor (REPOSITORY_EDITOR(repository_editor_plugin));

    return TRUE; /* FALSE if activation failed */
}

static gboolean deactivate_plugin(AnjutaPlugin *plugin) {
    AnjutaUI *ui;

    destroy_repository_editor();

    gtkpod_unregister_repository_editor();

    ui = anjuta_shell_get_ui(plugin->shell, NULL);

    /* Unmerge UI */
    anjuta_ui_unmerge(ui, repository_editor_plugin->uiid);

    /* Remove Action groups */
    anjuta_ui_remove_action_group(ui, repository_editor_plugin->action_group);

    /* FALSE if plugin doesn't want to deactivate */
    return TRUE;
}

static void repository_editor_plugin_instance_init(GObject *obj) {
    RepositoryEditorPlugin *plugin = (RepositoryEditorPlugin*) obj;
    plugin->uiid = 0;
    plugin->action_group = NULL;
}

static void repository_editor_plugin_class_init(GObjectClass *klass) {
    AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

    parent_class = g_type_class_peek_parent(klass);

    plugin_class->activate = activate_plugin;
    plugin_class->deactivate = deactivate_plugin;
}

static void repository_editor_iface_init(RepositoryEditorInterface *iface) {
    iface->edit_repository = open_repository_editor;
    iface->init_repository = repository_ipod_init;
    iface->set_repository_model = repository_ipod_init_set_model;
}

ANJUTA_PLUGIN_BEGIN (RepositoryEditorPlugin, repository_editor_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE(repository_editor, REPOSITORY_EDITOR_TYPE);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (RepositoryEditorPlugin, repository_editor_plugin)
;
