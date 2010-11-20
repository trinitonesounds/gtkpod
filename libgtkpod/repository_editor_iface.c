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
 */

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#include "repository_editor_iface.h"

static void repository_editor_base_init(RepositoryEditorInterface *klass) {
    static gboolean initialized = FALSE;

    if (!initialized) {
        klass->edit_repository = NULL;
        klass->init_repository = NULL;
        klass->set_repository_model = NULL;
        initialized = TRUE;
    }
}

GType repository_editor_get_type(void) {
    static GType type = 0;
    if (!type) {
        static const GTypeInfo info =
            { sizeof(RepositoryEditorInterface), (GBaseInitFunc) repository_editor_base_init, NULL, NULL, NULL, NULL, 0, 0, NULL };
        type = g_type_register_static(G_TYPE_INTERFACE, "RepositoryEditorInterface", &info, 0);
        g_type_interface_add_prerequisite(type, G_TYPE_OBJECT);
    }
    return type;
}

void repository_editor_edit_repository(RepositoryEditor *editor, iTunesDB *itdb, Playlist *playlist) {
    if (! REPOSITORY_EDITOR_IS_EDITOR(editor))
           return;

       REPOSITORY_EDITOR_GET_INTERFACE(editor)->edit_repository(itdb, playlist);
}

gboolean repository_editor_init_repository(RepositoryEditor *editor, iTunesDB *itdb) {
    if (! REPOSITORY_EDITOR_IS_EDITOR(editor))
           return FALSE;

    return REPOSITORY_EDITOR_GET_INTERFACE(editor)->init_repository(itdb);
}

void repository_edtor_set_repository_model(RepositoryEditor *editor, iTunesDB *itdb, const gchar *old_model) {
    if (! REPOSITORY_EDITOR_IS_EDITOR(editor))
           return;

    REPOSITORY_EDITOR_GET_INTERFACE(editor)->set_repository_model(itdb, old_model);
}
