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

#include "gtkpod.h"
#include "directories.h"
#include "anjuta-app.h"

static gboolean on_gtkpod_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data);
static void on_gtkpod_destroy(GtkWidget * w, gpointer data);

void gtkpod_init(int argc, char *argv[]) {
    AnjutaPluginManager *plugin_manager;
    AnjutaProfileManager *profile_manager;
    AnjutaApp *app;
    AnjutaStatus *status;
    AnjutaProfile *profile;
    GFile *session_profile;
    gchar *profile_name = NULL;
    GError *error = NULL;
    gchar *plugin_profile_file = NULL;
    gchar *ui_file = NULL;

    // Initialise important directories
    init_directories(argv);

    GTKPOD_GLADE_XML_FILE = g_build_filename(get_glade_dir(), "gtkpod.glade", NULL);
    plugin_profile_file = g_build_filename(get_data_dir(), "default.profile", NULL);
    ui_file = g_build_filename(get_ui_dir(), "gtkpod.ui", NULL);

    anjuta_set_ui_file_path(ui_file);

    /* Initialize application */
    app = ANJUTA_APP(anjuta_app_new());
    status = anjuta_shell_get_status(ANJUTA_SHELL(app), NULL);
    anjuta_status_progress_add_ticks(status, 1);

    anjuta_status_disable_splash(status, TRUE);

    g_object_set_data(G_OBJECT(app), "__proper_shutdown", "1");

    g_signal_connect(G_OBJECT(app), "delete_event", G_CALLBACK(
                    on_gtkpod_delete_event), NULL);
    g_signal_connect(G_OBJECT(app), "destroy", G_CALLBACK(on_gtkpod_destroy),
            NULL);

    plugin_manager = anjuta_shell_get_plugin_manager(ANJUTA_SHELL(app), NULL);
    profile_manager = anjuta_shell_get_profile_manager(ANJUTA_SHELL(app), NULL);

    //        /* Restore remembered plugins */
    //        remembered_plugins =
    //anjuta_preferences_get (app->preferences, ANJUTA_REMEMBERED_PLUGINS);
    //        if (remembered_plugins)
    //anjuta_plugin_manager_set_remembered_plugins (plugin_manager,
    //          remembered_plugins);
    //        g_free (remembered_plugins);

    /* Prepare profile */
    profile = anjuta_profile_new(USER_PROFILE_NAME, plugin_manager);
    session_profile = g_file_new_for_path(plugin_profile_file);
    anjuta_profile_add_plugins_from_xml(profile, session_profile, TRUE, &error);
    if (error)
    {
        anjuta_util_dialog_error(GTK_WINDOW(app), "%s", error->message);
        g_error_free(error);
        error = NULL;
    }
    g_object_unref(session_profile);

    /* Load user session profile */
    profile_name = g_path_get_basename(plugin_profile_file);
    session_profile = anjuta_util_get_user_cache_file(profile_name, NULL);
    if (g_file_query_exists(session_profile, NULL))
    {
        anjuta_profile_add_plugins_from_xml(profile, session_profile, FALSE,
                &error);
        if (error)
        {
            anjuta_util_dialog_error(GTK_WINDOW(app), "%s", error->message);
            g_error_free(error);
            error = NULL;
        }
    }
    anjuta_profile_set_sync_file(profile, session_profile);
    g_object_unref(session_profile);
    g_free(profile_name);

    /* Load profile */
    anjuta_profile_manager_freeze(profile_manager);
    anjuta_profile_manager_push(profile_manager, profile, &error);
    if (error)
    {
        anjuta_util_dialog_error(GTK_WINDOW(app), "%s", error->message);
        g_error_free(error);
        error = NULL;
    }

    //        gchar *session_dir;
    //        AnjutaSession *session;
    //
    //        /* Load user session */
    //        session_dir = USER_SESSION_PATH_NEW;
    //
    //        /* If preferences is set to not load last session, clear it */
    //        if (no_session ||
    //    anjuta_preferences_get_int (app->preferences,
    //ANJUTA_SESSION_SKIP_LAST))
    //        {
    ///* Reset default session */
    //session = anjuta_session_new (session_dir);
    //anjuta_session_clear (session);
    //g_object_unref (session);
    //        }
    //        /* If preferences is set to not load last project, clear it */
    //        else if (no_files ||
    //    anjuta_preferences_get_int (app->preferences,
    //ANJUTA_SESSION_SKIP_LAST_FILES))
    //        {
    //session = anjuta_session_new (session_dir);
    //anjuta_session_set_string_list (session, "File Loader",
    //        "Files", NULL);
    //anjuta_session_sync (session);
    //g_object_unref (session);
    //        }
    //        /* Otherwise, load session normally */
    //        else
    //        {
    //project_file = extract_project_from_session (session_dir);
    //        }
    //        g_free (session_dir);
    //
    //        /* Prepare for session save and load on profile change */
    //        g_signal_connect (profile_manager, "profile-scoped",
    //    G_CALLBACK (on_profile_scoped), app);
    //        /* Load project file */
    //        if (project_file)
    //        {
    //GFile* file = g_file_new_for_commandline_arg (project_file);
    //IAnjutaFileLoader *loader;
    //loader = anjuta_shell_get_interface (ANJUTA_SHELL (app),
    //        IAnjutaFileLoader, NULL);
    //ianjuta_file_loader_load (loader, file, FALSE, NULL);
    //g_free (project_file);
    //g_object_unref (file);
    //        }
    anjuta_profile_manager_thaw(profile_manager, &error);

    if (error)
    {
        anjuta_util_dialog_error(GTK_WINDOW(app), "%s", error->message);
        g_error_free(error);
        error = NULL;
    }
    //        g_signal_connect (profile_manager, "profile-descoped",
    //    G_CALLBACK (on_profile_descoped), app);

    anjuta_status_progress_tick(status, NULL, _("Loaded Session..."));
    anjuta_status_disable_splash(status, TRUE);

    g_set_application_name(_("GtkPod"));
    gtk_window_set_default_icon_name("gtkpod");
    gtk_window_set_auto_startup_notification(FALSE);

    gtk_window_set_role(GTK_WINDOW(app), "gtkpod-app");
    gtk_widget_show(GTK_WIDGET(app));
    gtk_window_maximize(GTK_WINDOW(app));

    GList *plugins = anjuta_plugin_manager_get_active_plugins(plugin_manager);
    g_printf("Number of active plugins: %d\n", g_list_length(plugins));


}

/* callback for gtkpod window's close button */
static gboolean on_gtkpod_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
//    if (!widgets_blocked) {
//        if (ok_to_close_gtkpod()) {
//            gtkpod_shutdown();
//            /* returning FALSE to continue calling other handlers
//             causes tons of errors. */
//        }
//    }

    AnjutaPluginManager *plugin_manager;
    AnjutaProfileManager *profile_manager;
    AnjutaApp *app;
    //    AnjutaProfile *current_profile;
    //    AnjutaSavePrompt *save_prompt;
    //    gchar *remembered_plugins;

    app = ANJUTA_APP(widget);
    plugin_manager = anjuta_shell_get_plugin_manager(ANJUTA_SHELL (app), NULL);
    profile_manager = anjuta_shell_get_profile_manager (ANJUTA_SHELL (app), NULL);

    //    /* Save remembered plugins */
    //    remembered_plugins =
    //        anjuta_plugin_manager_get_remembered_plugins (plugin_manager);
    //    anjuta_preferences_set (app->preferences,
    //    ANJUTA_REMEMBERED_PLUGINS,
    //    remembered_plugins);
    //    g_free (remembered_plugins);

    /* Check for unsaved data */
    //    save_prompt = anjuta_save_prompt_new (GTK_WINDOW (app));
    //    anjuta_shell_save_prompt (ANJUTA_SHELL (app), save_prompt, NULL);
    //
    //    if (anjuta_save_prompt_get_items_count (save_prompt) > 0)
    //    {
    //        switch (gtk_dialog_run (GTK_DIALOG (save_prompt)))
    //        {
    //case ANJUTA_SAVE_PROMPT_RESPONSE_CANCEL:
    //    gtk_widget_destroy (GTK_WIDGET (save_prompt));
    //    /* Do not exit now */
    //    return TRUE;
    //case ANJUTA_SAVE_PROMPT_RESPONSE_DISCARD:
    //case ANJUTA_SAVE_PROMPT_RESPONSE_SAVE_CLOSE:
    //    /* exit now */
    //    break;
    //        }
    //    }
    //    /* Wait for files to be really saved (asyncronous operation) */
    //    if (app->save_count > 0)
    //    {
    //        g_message ("Waiting for %d file(s) to be saved!", app->save_count);
    //        while (app->save_count > 0)
    //        {
    //g_main_context_iteration (NULL, TRUE);
    //        }
    //    }

    /* If current active profile is "user", save current session as
     * default session
     */
    //    current_profile = anjuta_profile_manager_get_current (profile_manager);
    //    if (strcmp (anjuta_profile_get_name (current_profile), "user") == 0)
    //    {
    //        gchar *session_dir;
    //        session_dir = USER_SESSION_PATH_NEW;
    //        anjuta_shell_session_save (ANJUTA_SHELL (app), session_dir, NULL);
    //        g_free (session_dir);
    //    }

    anjuta_shell_notify_exit (ANJUTA_SHELL (app), NULL);

    //    gtk_widget_destroy (GTK_WIDGET (save_prompt));

    /* Shutdown */
    if (g_object_get_data (G_OBJECT (app), "__proper_shutdown"))
    {
        gtk_widget_hide (GTK_WIDGET (app));
        anjuta_plugin_manager_unload_all_plugins
        (anjuta_shell_get_plugin_manager (ANJUTA_SHELL (app), NULL));
    }
    return FALSE;
}

static void on_gtkpod_destroy(GtkWidget * w, gpointer data) {
    gtk_widget_hide(w);
    gtk_main_quit();
}
