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

#include <gtk/gtk.h>
#include "libgtkpod/misc.h"
#include "libgtkpod/prefs.h"
#include "libgtkpod/directories.h"
#include "libgtkpod/gtkpod_app_iface.h"
#include "plugin.h"
#include "mserv_preferences.h"

typedef enum {
    PATH_MSERV_MUSIC_ROOT, PATH_MSERV_TRACKINFO_ROOT
} MservPathType;

void set_default_preferences() {
    gchar *buf;

    /* clean up keys */
    /* MSERV music root */
    if (prefs_get_string_value_index("path", PATH_MSERV_MUSIC_ROOT, &buf)) {
        prefs_set_string("path_mserv_music_root", buf);
        g_free(buf);
        prefs_set_string_index("path", PATH_MSERV_MUSIC_ROOT, NULL);
    }

    if (prefs_get_string_value_index("toolpath", PATH_MSERV_MUSIC_ROOT, &buf)) {
        prefs_set_string("path_mserv_music_root", buf);
        g_free(buf);
        prefs_set_string_index("toolpath", PATH_MSERV_MUSIC_ROOT, NULL);
    }

    /* MSERV track info root */
    if (prefs_get_string_value_index("path", PATH_MSERV_TRACKINFO_ROOT, &buf)) {
        prefs_set_string("path_mserv_trackinfo_root", buf);
        g_free(buf);
        prefs_set_string_index("path", PATH_MSERV_TRACKINFO_ROOT, NULL);
    }

    if (prefs_get_string_value_index("toolpath", PATH_MSERV_TRACKINFO_ROOT, &buf)) {
        prefs_set_string("path_mserv_trackinfo_root", buf);
        g_free(buf);
        prefs_set_string_index("toolpath", PATH_MSERV_TRACKINFO_ROOT, NULL);
    }

    prefs_set_int("mserv_report_probs", TRUE);
    prefs_set_string("path_mserv_trackinfo_root", "/var/lib/mserv/trackinfo/");
    prefs_set_int("mserv_use", FALSE);
    prefs_set_string("mserv_username", "");
}

/*
    glade callback
*/
static void init_settings (GtkBuilder *builder) {
    gchar *temp = prefs_get_string ("mserv_username");
    if(temp) {
        gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object(builder, "mserv_username")), temp);
        g_free (temp);
    }

    temp = prefs_get_string ("path_mserv_music_root");

    if(temp) {
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (gtk_builder_get_object(builder, "music_root")), temp);
        g_free (temp);
    }

    temp = prefs_get_string ("path_mserv_trackinfo_root");

    if(temp) {
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (gtk_builder_get_object(builder, "mserv_root")), temp);
        g_free (temp);
    }

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (gtk_builder_get_object(builder, "use_mserv")),
                   prefs_get_int("mserv_use"));

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (gtk_builder_get_object(builder, "report_mserv_problems")),
                   prefs_get_int("mserv_report_probs"));
}

/*
    glade callback
*/
G_MODULE_EXPORT void on_mserv_username_changed (GtkEditable *sender, gpointer e) {
    prefs_set_string ("mserv_username", gtk_entry_get_text (GTK_ENTRY (sender)));
}

/*
    glade callback
*/
G_MODULE_EXPORT void on_music_root_current_folder_changed (GtkFileChooser *sender, gpointer e) {
    prefs_set_string ("path_mserv_music_root",
                      gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (sender)));
}

/*
    glade callback
*/
G_MODULE_EXPORT void on_mserv_root_current_folder_changed (GtkFileChooser *sender, gpointer e) {
    prefs_set_string ("path_mserv_trackinfo_root",
                      gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (sender)));
}

G_MODULE_EXPORT void on_fill_track_info_checkbox_toggled (GtkToggleButton *sender, gpointer e){
    prefs_set_int("mserv_use", gtk_toggle_button_get_active (sender));
}

G_MODULE_EXPORT void on_report_problems_checkbox_toggled (GtkToggleButton *sender, gpointer e) {
    prefs_set_int("mserv_report_probs", gtk_toggle_button_get_active (sender));
}

GtkWidget *init_mserv_preferences() {
    GError* error = NULL;
    GtkWidget *notebook;
    gchar *builderpath;
    GtkBuilder *builder;

    builderpath = g_build_filename(get_glade_dir(), "mserv.xml", NULL);
    builder = gtk_builder_new();
    gtk_builder_add_from_file(builder, builderpath, &error);
    if (error) {
        g_warning("Could not load mserv preferences: %s", error->message);
        g_error_free(error);
        g_free(builderpath);
        return NULL;
    }

    notebook = GTK_WIDGET (gtk_builder_get_object (builder, "mserv_settings_notebook"));
    GtkWidget *parent = gtk_widget_get_parent(notebook);
    g_object_ref(notebook);
    gtk_container_remove(GTK_CONTAINER(parent), notebook);
    gtk_widget_destroy(parent);

    init_settings(builder);

    gtk_builder_connect_signals(builder, NULL);

    g_free(builderpath);

    return notebook;
}
