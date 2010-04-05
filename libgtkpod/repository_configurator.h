/*
 |  Copyright (C) 2002-2009 Paul Richardson <phantom_sf at users sourceforge net>
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
#ifndef REPOSITORY_EDITOR_IFACE_H_
#define REPOSITORY_EDITOR_IFACE_H_

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#include <gtk/gtk.h>
#include "itdb.h"

typedef struct _RepositoryEditor RepositoryEditor;
typedef struct _RepositoryEditorInterface RepositoryEditorInterface;

struct _RepositoryEditorInterface {
    GTypeInterface g_iface;

    void (*edit_repository)(iTunesDB *itdb, Playlist *playlist);
    gboolean (*init_repository)(iTunesDB *itdb);
    void (*set_repository_model)(iTunesDB *itdb, const gchar *old_model);
};

GType repository_editor_get_type(void);

#define REPOSITORY_EDITOR_TYPE                (repository_editor_get_type ())
#define REPOSITORY_EDITOR(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), REPOSITORY_EDITOR_TYPE, RepositoryEditor))
#define REPOSITORY_EDITOR_IS_EDITOR(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REPOSITORY_EDITOR_TYPE))
#define REPOSITORY_EDITOR_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), REPOSITORY_EDITOR_TYPE, RepositoryEditorInterface))

#endif /* REPOSITORY_EDITOR_IFACE_H_ */
