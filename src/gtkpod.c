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
#include <libanjuta/anjuta-version.h>

#include "gtkpod.h"
#include "libgtkpod/directories.h"
#include "libgtkpod/gtkpod_app_iface.h"
#include "libgtkpod/misc.h"
#include "libgtkpod/stock_icons.h"
#include "libgtkpod/prefs.h"
#include "anjuta-window.h"

#define GTKPOD_REMEMBERED_PLUGINS "remembered-plugins"

static gchar *system_restore_session = NULL;

static gboolean on_gtkpod_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data);
static void on_gtkpod_destroy(GtkWidget * w, gpointer data);

#if (ANJUTA_CHECK_VERSION(3, 9, 3))
static void on_profile_scoped(AnjutaProfile *profile, AnjutaWindow *win);
static void on_profile_descoped (AnjutaProfile *profile, AnjutaWindow *win);
#else
static void on_profile_scoped(AnjutaProfileManager *profile_manager, AnjutaProfile *profile, AnjutaWindow *win);
static void on_profile_descoped(AnjutaProfileManager *profile_manager, AnjutaProfile *profile, AnjutaWindow *win);
#endif

static gchar *get_user_session_dir() {
    return g_build_filename(g_get_home_dir(), ".gtkpod", "session", NULL);
}

static gchar* get_user_profile_path() {
    return g_build_filename(g_get_home_dir(), ".gtkpod", "gtkpod.profile", NULL);
}

static gchar* get_default_profile_path() {
    return g_build_filename(get_data_dir(), "default.profile", NULL);
}

void gtkpod_init(int argc, char *argv[]) {
    AnjutaPluginManager *plugin_manager;
    AnjutaProfileManager *profile_manager;
    AnjutaWindow *win;
    AnjutaStatus *status;
    AnjutaProfile *profile;
    GFile *session_profile;
    GError *error = NULL;

    gchar *default_profile_file = NULL;
    gchar *user_profile_file = NULL;

    gchar *ui_file = NULL;
    gchar *remembered_plugins = NULL;
    gchar *session_dir = NULL;
    gchar *splash = NULL;

    /* Initialise important directories */
    init_directories(argv);

    register_stock_icon(GTKPOD_ICON, GTKPOD_ICON_STOCK_ID);

    /* Initialise the ui file */
    ui_file = g_build_filename(get_ui_dir(), "gtkpod.ui", NULL);
    anjuta_set_ui_file_path(ui_file);

    /* Register the application icon */
    register_stock_icon("gtkpod", GTKPOD_APP_ICON_STOCK_ID);

    /* Initialize application class instance*/
    win = ANJUTA_WINDOW(anjuta_window_new());
    gtkpod_app = GTKPOD_APP(win);

    /* Initialise the preferences as required for the display of the splash screen */
    prefs_init(argc, argv);

    /* Show some progress as the app is initialised */
    status = anjuta_shell_get_status(ANJUTA_SHELL(win), NULL);
    anjuta_status_progress_add_ticks(status, 1);

    /* Show the splash screen if user requires */
    if (! prefs_get_int(DISABLE_SPLASH_SCREEN)) {
        splash = g_build_filename(get_icon_dir(), "gtkpod-splash.png", NULL);
        if (g_file_test(splash, G_FILE_TEST_IS_REGULAR))
            anjuta_status_set_splash(status, splash, 100);
        else {
            anjuta_status_disable_splash(status, TRUE);
        }

        g_free(splash);
    }

    /*
     * initialise gtkpod library items. Needs to be safety threaded due
     * to splash screen.
     */
    gdk_threads_add_idle(gp_init, NULL);

    /* Add blocking widgets from the framework */
    add_blocked_widget(win->toolbar);
    add_blocked_widget(win->view_menu);

    /* Set up shutdown signals */
    g_signal_connect(G_OBJECT(win), "delete_event", G_CALLBACK(
                    on_gtkpod_delete_event), NULL);
    g_signal_connect(G_OBJECT(win), "destroy", G_CALLBACK(on_gtkpod_destroy),
            NULL);

    plugin_manager = anjuta_shell_get_plugin_manager(ANJUTA_SHELL(win), NULL);
    profile_manager = anjuta_shell_get_profile_manager(ANJUTA_SHELL(win), NULL);

    /* Restore remembered plugins */
    remembered_plugins = g_settings_get_string(win->settings, GTKPOD_REMEMBERED_PLUGINS);
    if (remembered_plugins)
        anjuta_plugin_manager_set_remembered_plugins(plugin_manager, remembered_plugins);
    g_free(remembered_plugins);

    /* Load default profile */
    default_profile_file = get_default_profile_path();
    profile = anjuta_profile_new(USER_PROFILE_NAME, plugin_manager);
    session_profile = g_file_new_for_path(default_profile_file);
    anjuta_profile_add_plugins_from_xml(profile, session_profile, TRUE, &error);
    if (error) {
        anjuta_util_dialog_error(GTK_WINDOW(win), "%s", error->message);
        g_error_free(error);
        error = NULL;
    }
    g_object_unref(session_profile);
    g_free(default_profile_file);

    /* Load user session profile */
    user_profile_file = get_user_profile_path();
    session_profile = g_file_new_for_path(user_profile_file);
    if (g_file_query_exists(session_profile, NULL)) {
        anjuta_profile_add_plugins_from_xml(profile, session_profile, FALSE, &error);
        if (error) {
            anjuta_util_dialog_error(GTK_WINDOW(win), "%s", error->message);
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
        anjuta_util_dialog_error(GTK_WINDOW(win), "%s", error->message);
        g_error_free(error);
        error = NULL;
    }

    /* Prepare for session save and load on profile change */
#if (ANJUTA_CHECK_VERSION(3, 9, 3))
    g_signal_connect (profile, "scoped", G_CALLBACK (on_profile_scoped), win);
#else
    g_signal_connect (profile_manager, "profile-scoped", G_CALLBACK (on_profile_scoped), win);
#endif

    anjuta_profile_manager_thaw(profile_manager, &error);

    if (error) {
        anjuta_util_dialog_error(GTK_WINDOW(win), "%s", error->message);
        g_error_free(error);
        error = NULL;
    }

#if (ANJUTA_CHECK_VERSION(3, 9, 3))
    g_signal_connect (profile, "descoped", G_CALLBACK (on_profile_descoped), win);
#else
    g_signal_connect (profile_manager, "profile-descoped", G_CALLBACK (on_profile_descoped), win);
#endif

    gdk_threads_add_idle(gp_init_itdbs, NULL);

    /* Load layout.*/
    session_dir = get_user_session_dir();
    if (!g_file_test(session_dir, G_FILE_TEST_IS_DIR))
        session_dir = g_strdup(get_data_dir());

    /* Restore session */
    anjuta_shell_session_load(ANJUTA_SHELL(win), session_dir, NULL);
    g_free(session_dir);

    anjuta_status_progress_tick(status, NULL, _("Loaded Session..."));
    anjuta_status_disable_splash(status, TRUE);

    gtk_window_set_default_icon_name("gtkpod");
    gtk_window_set_auto_startup_notification(TRUE);

    gtk_window_set_role(GTK_WINDOW(win), "gtkpod-app");
    gtk_widget_show(GTK_WIDGET(win));

    /*
     * Indicate to user that although the UI is up, the itdbs are still loading.
     * The message will be overriden by the import of
     */
    gtkpod_statusbar_message(_("Importing configured ipods ... "));
}

/* callback for gtkpod window's close button */
static gboolean on_gtkpod_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data) {

    if (!ok_to_close_gtkpod())
        return TRUE;

#if (ANJUTA_CHECK_VERSION(3, 8, 0))
    AnjutaProfileManager *profile_manager;
#endif

    AnjutaPluginManager *plugin_manager;
    AnjutaWindow *win;
    gchar *remembered_plugins;
    gchar *session_dir;

    win = ANJUTA_WINDOW(widget);
    plugin_manager = anjuta_shell_get_plugin_manager(ANJUTA_SHELL(win), NULL);

    /* Save remembered plugins */
    remembered_plugins = anjuta_plugin_manager_get_remembered_plugins(plugin_manager);
    g_settings_set_string(win->settings, GTKPOD_REMEMBERED_PLUGINS, remembered_plugins);
    g_free(remembered_plugins);

    session_dir = get_user_session_dir();
    anjuta_shell_session_save(ANJUTA_SHELL(win), session_dir, NULL);
    g_free(session_dir);

#if (ANJUTA_CHECK_VERSION(3, 8, 0))
    /* Close the profile manager which will emit "profile-descoped" and release
     * all previous profiles. */
    profile_manager = anjuta_shell_get_profile_manager(ANJUTA_SHELL(win), NULL);
    anjuta_profile_manager_close (profile_manager);

    /* Hide the window while unloading all the plugins. */
    gtk_widget_hide (widget);
    anjuta_plugin_manager_unload_all_plugins (plugin_manager);
#else
    anjuta_shell_notify_exit(ANJUTA_SHELL(win), NULL);
#endif

    /** Release any blocked widgets in order to cleanup correctly */
    release_widgets();

    if (!gtkpod_cleanup_quit()) {
        // Dont want to quit so avoid signalling any destroy event
        return TRUE;
    }

    return FALSE;
}

static void on_gtkpod_destroy(GtkWidget * w, gpointer data) {
    dispose_directories();

    gtk_widget_hide(w);
    gtk_main_quit();
}

#if (ANJUTA_CHECK_VERSION(3, 9, 3))
static void on_profile_scoped(AnjutaProfile *profile, AnjutaWindow *win) {
#else
static void on_profile_scoped(AnjutaProfileManager *profile_manager, AnjutaProfile *profile, AnjutaWindow *win) {
#endif
    gchar *session_dir;
    static gboolean first_time = TRUE;

    if (strcmp(anjuta_profile_get_name(profile), USER_PROFILE_NAME) != 0)
        return;

    /* If profile scoped to "user", restore user session */
    if (system_restore_session) {
        session_dir = system_restore_session;
        system_restore_session = NULL;
    }
    else {
        session_dir = get_user_session_dir();
    }

    if (first_time) {
        first_time = FALSE;
    }
    else {
        AnjutaSession *session;
        session = anjuta_session_new(session_dir);
        anjuta_session_sync(session);
        g_object_unref(session);
    }

    /* Restore session */
    anjuta_shell_session_load(ANJUTA_SHELL (win), session_dir, NULL);
    g_free(session_dir);
}

#if (ANJUTA_CHECK_VERSION(3, 9, 3))
static void on_profile_descoped (AnjutaProfile *profile, AnjutaWindow *win) {
#else
static void on_profile_descoped(AnjutaProfileManager *profile_manager, AnjutaProfile *profile, AnjutaWindow *win) {
#endif
    gchar *session_dir;

    if (strcmp(anjuta_profile_get_name(profile), USER_PROFILE_NAME) != 0)
        return;

    /* If profile descoped from is "user", save user session */
    session_dir = get_user_session_dir();

    /* Save current session */
    anjuta_shell_session_save(ANJUTA_SHELL (win), session_dir, NULL);
    g_free(session_dir);
}
