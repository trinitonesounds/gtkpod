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
 |  GNU General Public License for more photo.
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
#include "libgtkpod/stock_icons.h"
#include "display_photo.h"
#include "photo_editor_actions.h"
#include "plugin.h"

/* Parent class. Part of standard class definition */
static gpointer parent_class;

static GtkActionEntry photo_editor_actions[] =
    {
        {
            "ActionEditTrackPhoto", /* Action name */
            DEFAULT_PHOTO_EDITOR_STOCK_ID, /* Stock icon */
            N_("Open Photo Editor"), /* Display label */
            NULL, /* short-cut */
            NULL, /* Tooltip */
            G_CALLBACK (on_open_photo_editor)
        },
    };

static gboolean activate_plugin(AnjutaPlugin *plugin) {
    AnjutaUI *ui;
    GtkActionGroup* action_group;

    photo_editor_plugin = (PhotoEditorPlugin*) plugin;

    register_icon_path(get_plugin_dir(), "photo_editor");
    register_stock_icon(DEFAULT_PHOTO_EDITOR_ICON, DEFAULT_PHOTO_EDITOR_STOCK_ID);
    register_stock_icon(PHOTO_TOOLBAR_ALBUM_ICON, PHOTO_TOOLBAR_ALBUM_STOCK_ID);
    register_stock_icon(PHOTO_TOOLBAR_PHOTOS_ICON, PHOTO_TOOLBAR_PHOTOS_STOCK_ID);

    ui = anjuta_shell_get_ui(plugin->shell, NULL);

    /* Add our playlist_actions */
    action_group
            = anjuta_ui_add_action_group_entries(ui, "ActionGroupPhotoEditor", _("Photo Editor"), photo_editor_actions, G_N_ELEMENTS (photo_editor_actions), GETTEXT_PACKAGE, TRUE, plugin);
    photo_editor_plugin->action_group = action_group;

    /* Merge UI */
    gchar *uipath = g_build_filename(get_ui_dir(), "photo_editor.ui", NULL);
    photo_editor_plugin->uiid = anjuta_ui_merge(ui, uipath);
    g_free(uipath);

    g_return_val_if_fail(PHOTO_EDITOR_IS_EDITOR(photo_editor_plugin), TRUE);

    gtkpod_register_photo_editor (PHOTO_EDITOR(photo_editor_plugin));

    g_signal_connect (gtkpod_app, SIGNAL_PLAYLIST_SELECTED, G_CALLBACK (photo_editor_select_playlist_cb), NULL);

    return TRUE; /* FALSE if activation failed */
}

static gboolean deactivate_plugin(AnjutaPlugin *plugin) {
    AnjutaUI *ui;

    photo_editor_plugin = (PhotoEditorPlugin*) plugin;

//    destroy_photo_editor();

    gtkpod_unregister_photo_editor();

    ui = anjuta_shell_get_ui(plugin->shell, NULL);

    /* Unmerge UI */
    anjuta_ui_unmerge(ui, photo_editor_plugin->uiid);

    /* Remove Action groups */
    anjuta_ui_remove_action_group(ui, photo_editor_plugin->action_group);

    photo_editor_plugin = NULL;

    /* FALSE if plugin doesn't want to deactivate */
    return TRUE;
}

static void photo_editor_plugin_instance_init(GObject *obj) {
    PhotoEditorPlugin *plugin = (PhotoEditorPlugin*) obj;
    plugin->uiid = 0;
    plugin->action_group = NULL;
}

static void photo_editor_plugin_class_init(GObjectClass *klass) {
    AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

    parent_class = g_type_class_peek_parent(klass);

    plugin_class->activate = activate_plugin;
    plugin_class->deactivate = deactivate_plugin;
}

static void photo_editor_iface_init(PhotoEditorInterface *iface) {
    iface->edit_photos = gphoto_display_photo_window;
}

ANJUTA_PLUGIN_BEGIN (PhotoEditorPlugin, photo_editor_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE(photo_editor, PHOTO_EDITOR_TYPE);
ANJUTA_PLUGIN_END;
ANJUTA_SIMPLE_PLUGIN (PhotoEditorPlugin, photo_editor_plugin)
;
