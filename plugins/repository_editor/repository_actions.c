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

#include "libgtkpod/itdb.h"
#include "libgtkpod/gtkpod_app_iface.h"
#include "libgtkpod/gp_itdb.h"
#include "libgtkpod/prefs.h"
#include "libgtkpod/misc.h"
#include "libgtkpod/misc_playlist.h"
#include "repository_actions.h"
#include "repository.h"

void on_create_ipod_directories(GtkAction* action, RepositoryEditorPlugin* plugin) {
    iTunesDB *itdb = gtkpod_get_current_itdb();
    if (!itdb) {
        message_sb_no_ipod_itdb_selected();
        return;
    }

    ExtraiTunesDBData *eitdb = itdb->userdata;

    g_return_if_fail (eitdb);

    if (!eitdb->itdb_imported) {
        gchar *mountpoint = get_itdb_prefs_string(itdb, KEY_MOUNTPOINT);
        gchar *str = g_strdup_printf(_("iPod at '%s' is not loaded.\nPlease load it first."), mountpoint);
        gtkpod_warning(str);
        g_free(str);
        g_free(mountpoint);
    }
    else {
        repository_ipod_init(itdb);
    }
}

void on_check_ipod_files(GtkAction* action, RepositoryEditorPlugin* plugin) {
    iTunesDB *itdb = gtkpod_get_current_itdb();
    if (!itdb) {
        message_sb_no_ipod_itdb_selected();
        return;
    }

    ExtraiTunesDBData *eitdb = itdb->userdata;

    g_return_if_fail (eitdb);

    if (!eitdb->itdb_imported) {
        gchar *mountpoint = get_itdb_prefs_string(itdb, KEY_MOUNTPOINT);
        gchar *str = g_strdup_printf(_("iPod at '%s' is not loaded.\nPlease load it first."), mountpoint);
        gtkpod_warning(str);
        g_free(str);
        g_free(mountpoint);
    }
    else {
        check_db(itdb);
    }
}

void on_configure_repositories(GtkAction* action, RepositoryEditorPlugin* plugin) {
    open_repository_editor(gtkpod_get_current_itdb(), gtkpod_get_current_playlist());
}
