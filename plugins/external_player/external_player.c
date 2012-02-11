/*
 |
 |  Copyright (C) 2002-2012 Paul Richardson <phantom_sf at users.sourceforge.net>
 |  Part of the gtkpod project.
 |
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
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "libgtkpod/directories.h"
#include "libgtkpod/prefs.h"
#include "libgtkpod/misc.h"
#include "libgtkpod/tools.h"
#include "plugin.h"
#include "external_player.h"

#define PLAY_NOW_PREF "path_play_now"

/*
 * play tracks with external player.
 *
 * @selected_tracks: list of tracks to be played
 */
void external_player_play_tracks(GList *tracks) {
    GError *error = NULL;
    gchar *cmd = prefs_get_string ("path_play_now");

    run_exec_on_tracks(cmd, tracks, &error);

    if (error) {
        gtkpod_warning_simple(error->message);
        g_error_free(error);
    }

    g_free(cmd);
}

GtkWidget *init_external_player_preferences() {
    GtkBuilder *prefbuilder;
    GtkWidget *w = NULL;
    GtkWidget *notebook;
    GtkWidget *play_entry;

    gchar *glade_path = g_build_filename(get_glade_dir(), "external_player.xml", NULL);
    prefbuilder = gtkpod_builder_xml_new(glade_path);
    w = gtkpod_builder_xml_get_widget(prefbuilder, "prefs_window");
    notebook = gtkpod_builder_xml_get_widget(prefbuilder, "external_player_notebook");
    play_entry = gtkpod_builder_xml_get_widget(prefbuilder, "play_command_entry");
    g_object_ref(notebook);
    gtk_container_remove(GTK_CONTAINER(w), notebook);
    gtk_widget_destroy(w);
    g_free(glade_path);

    gtk_entry_set_text(GTK_ENTRY(play_entry), prefs_get_string(PLAY_NOW_PREF));

    gtk_builder_connect_signals(prefbuilder, NULL);
    g_object_unref(prefbuilder);

    return notebook;
}

/**
 * Callback for play command entry in preference dialog
 */
G_MODULE_EXPORT void on_play_command_entry_changed (GtkEditable *sender, gpointer e) {
    prefs_set_string (PLAY_NOW_PREF, gtk_entry_get_text (GTK_ENTRY (sender)));
}

