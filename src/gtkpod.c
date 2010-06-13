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

#include <gtk/gtk.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-shell.h>

#include "gtkpod.h"
#include "libgtkpod/directories.h"
#include "libgtkpod/gtkpod_app_iface.h"
#include "libgtkpod/misc.h"
#include "libgtkpod/stock_icons.h"
#include "anjuta-app.h"

#define GTKPOD_REMEMBERED_PLUGINS "gtkpod.remembered.plugins"

static gboolean on_gtkpod_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data);
static void on_gtkpod_destroy(GtkWidget * w, gpointer data);

GladeXML *gtkpod_core_xml_new(const gchar *name) {
    return gtkpod_xml_new(gtkpod_get_glade_xml(), name);
}

void gtkpod_init(int argc, char *argv[]) {
    AnjutaPluginManager *plugin_manager;
    AnjutaProfileManager *profile_manager;
    AnjutaApp *app;
    AnjutaStatus *status;
    AnjutaProfile *profile;
    GFile *session_profile;
    GError *error = NULL;

    gchar *glade_xml_file = NULL;
    gchar *default_profile_file = NULL;
    gchar *user_profile_file = NULL;

    gchar *ui_file = NULL;
    gchar *remembered_plugins = NULL;
    gchar *session_dir = NULL;

    /* Initialise important directories */
    init_directories(argv);

    /* Initialise the ui file */
    ui_file = g_build_filename(get_ui_dir(), "gtkpod.ui", NULL);
    anjuta_set_ui_file_path(ui_file);

    /* Register the application icon */
    register_stock_icon("gtkpod", GTKPOD_APP_ICON_STOCK_ID);

    /* Initialize application class instance*/
    app = ANJUTA_APP(anjuta_app_new());

    /* initialise gtkpod library items depedent on path of executable*/
    gp_init(GTKPOD_APP(app), argc, argv);

    /* Set the glade xml file of the app */
    glade_xml_file = g_build_filename(get_glade_dir(), "gtkpod.glade", NULL);
    gtkpod_app_set_glade_xml(glade_xml_file);
    g_free(glade_xml_file);

    /* Add blocking widgets from the framework */
    add_blocked_widget(app->toolbar);
    add_blocked_widget(app->view_menu);

    /* Show some progress as the app is initialised */
    status = anjuta_shell_get_status(ANJUTA_SHELL(app), NULL);
    anjuta_status_progress_add_ticks(status, 1);

    /* Set up shutdown signals */
    g_object_set_data(G_OBJECT(app), "__proper_shutdown", "1");
    g_signal_connect(G_OBJECT(app), "delete_event", G_CALLBACK(
                    on_gtkpod_delete_event), NULL);
    g_signal_connect(G_OBJECT(app), "destroy", G_CALLBACK(on_gtkpod_destroy),
            NULL);

    plugin_manager = anjuta_shell_get_plugin_manager(ANJUTA_SHELL(app), NULL);
    profile_manager = anjuta_shell_get_profile_manager(ANJUTA_SHELL(app), NULL);

    /* Restore remembered plugins */
    remembered_plugins = anjuta_preferences_get(app->preferences, GTKPOD_REMEMBERED_PLUGINS);
    g_message("REMEMBERED PLUGINS %s", remembered_plugins);
    if (remembered_plugins)
        anjuta_plugin_manager_set_remembered_plugins(plugin_manager, remembered_plugins);
    g_free(remembered_plugins);

    /* Load default profile */
    default_profile_file = g_build_filename(get_data_dir(), "default.profile", NULL);
    g_message("DEFAULT PROFILE %s", default_profile_file);
    profile = anjuta_profile_new(USER_PROFILE_NAME, plugin_manager);
    session_profile = g_file_new_for_path(default_profile_file);
    anjuta_profile_add_plugins_from_xml(profile, session_profile, TRUE, &error);
    if (error) {
        anjuta_util_dialog_error(GTK_WINDOW(app), "%s", error->message);
        g_error_free(error);
        error = NULL;
    }
    g_object_unref(session_profile);
    g_free(default_profile_file);

    /* Load user session profile */
    user_profile_file = g_build_filename(g_get_home_dir(), ".gtkpod", "gtkpod.profile", NULL);
    g_message("User profile %s", user_profile_file);
    session_profile = g_file_new_for_path(user_profile_file);
    if (g_file_query_exists(session_profile, NULL)) {
        g_message("user session profile exists so adding plugins from it");
        anjuta_profile_add_plugins_from_xml(profile, session_profile, FALSE, &error);
        if (error) {
            anjuta_util_dialog_error(GTK_WINDOW(app), "%s", error->message);
            g_error_free(error);
            error = NULL;
        }
    }
    anjuta_profile_set_sync_file(profile, session_profile);
    g_object_unref(session_profile);
    g_free(user_profile_file);

    /* Load profile */
    anjuta_profile_manager_freeze(profile_manager);
    anjuta_profile_manager_push(profile_manager, profile, &error);
    if (error) {
        anjuta_util_dialog_error(GTK_WINDOW(app), "%s", error->message);
        g_error_free(error);
        error = NULL;
    }

    anjuta_profile_manager_thaw(profile_manager, &error);

    if (error) {
        anjuta_util_dialog_error(GTK_WINDOW(app), "%s", error->message);
        g_error_free(error);
        error = NULL;
    }

    /* Load layout.*/
    session_dir = g_build_filename(g_get_home_dir(), ".gtkpod", "session", NULL);
    if (! g_file_test(session_dir, G_FILE_TEST_IS_DIR))
        session_dir = get_data_dir();

    /* Restore session */
    anjuta_shell_session_load(ANJUTA_SHELL (app), session_dir, NULL);
    g_free(session_dir);

    anjuta_status_progress_tick(status, NULL, _("Loaded Session..."));
    anjuta_status_disable_splash(status, TRUE);

    g_set_application_name(_("GtkPod"));
    gtk_window_set_default_icon_name("gtkpod");
    gtk_window_set_auto_startup_notification(FALSE);

    gtk_window_set_role(GTK_WINDOW(app), "gtkpod-app");
    gtk_widget_show(GTK_WIDGET(app));

    GList *plugins = anjuta_plugin_manager_get_active_plugins(plugin_manager);
    g_printf("Number of active plugins: %d\n", g_list_length(plugins));
}

/* callback for gtkpod window's close button */
static gboolean on_gtkpod_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data) {

    if (!gtkpod_cleanup_quit()) {
        // Dont want to quit so avoid signalling any destroy event
        return TRUE;
    }

    AnjutaPluginManager *plugin_manager;
    AnjutaProfileManager *profile_manager;
    AnjutaProfile *current_profile;
    AnjutaApp *app;
    gchar *remembered_plugins;
    gchar *session_dir;

    app = ANJUTA_APP(widget);
    plugin_manager = anjuta_shell_get_plugin_manager(ANJUTA_SHELL (app), NULL);
    profile_manager = anjuta_shell_get_profile_manager(ANJUTA_SHELL (app), NULL);

    /* Save remembered plugins */
    remembered_plugins = anjuta_plugin_manager_get_remembered_plugins(plugin_manager);
    anjuta_preferences_set(app->preferences, GTKPOD_REMEMBERED_PLUGINS, remembered_plugins);
    g_free(remembered_plugins);

    current_profile = anjuta_profile_manager_get_current(profile_manager);
    anjuta_profile_sync(current_profile, NULL);

    /*
     * Workaround - FIXME
     * Seems that when the plugins are unloaded in the last line below, the changed
     * signal is emitted onto the current_profile, which has the effect of wiping the
     * user profile. This line avoids this by setting the profile file to null. Anjuta does
     * not do this but gtkpod does.
     */
    anjuta_profile_set_sync_file(current_profile, NULL);

    session_dir = g_build_filename(g_get_home_dir(), ".gtkpod", "session", NULL);
    anjuta_shell_session_save(ANJUTA_SHELL (app), session_dir, NULL);
    g_free(session_dir);

    anjuta_shell_notify_exit(ANJUTA_SHELL (app), NULL);

    /* Shutdown */
    if (g_object_get_data(G_OBJECT (app), "__proper_shutdown")) {
        gtk_widget_hide(GTK_WIDGET (app));
        anjuta_plugin_manager_unload_all_plugins(plugin_manager);
    }
    return FALSE;
}

static void on_gtkpod_destroy(GtkWidget * w, gpointer data) {
    gtk_widget_hide(w);
    gtk_main_quit();
}
