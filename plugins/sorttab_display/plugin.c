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
#include <libanjuta/interfaces/ianjuta-preferences.h>
#include "libgtkpod/stock_icons.h"
#include "libgtkpod/gtkpod_app_iface.h"
#include "libgtkpod/prefs.h"
#include "libgtkpod/gp_private.h"
#include "plugin.h"
#include "display_sorttabs.h"
#include "sorttab_display_actions.h"
#include "sorttab_display_preferences.h"

#define ICON_FILE "sorttab_display-sorttab-category.png"
#define TAB_NAME "Track Filter"

/* Parent class. Part of standard class definition */
static gpointer parent_class;

static GtkActionEntry sorttab_actions[] =
    {
        {
            "ActionDeleteSelectedEntry", GTK_STOCK_DELETE, N_("Selected Filter Tab Entry from Playlist"), NULL, NULL,
            G_CALLBACK (on_delete_selected_entry_from_playlist) },
        {
            "ActionDeleteSelectedEntryFromDb", GTK_STOCK_DELETE, N_("Selected Filter Tab Entry from Database"), NULL,
            NULL, G_CALLBACK (on_delete_selected_entry_from_database) },
        {
            "ActionDeleteSelectedEntryFromDev", GTK_STOCK_DELETE, N_("Selected Filter Tab Entry from Device"), NULL,
            NULL, G_CALLBACK (on_delete_selected_entry_from_device) },
        {
            "ActionUpdateTabEntry", GTK_STOCK_REFRESH, N_("Selected Tab Entry"), NULL, NULL,
            G_CALLBACK (on_update_selected_tab_entry) },
        {
            "ActionUpdateMservTabEntry", GTK_STOCK_REFRESH, N_("Selected Tab Entry"), NULL, NULL,
            G_CALLBACK (on_update_mserv_selected_tab_entry) } };

static void set_default_preferences() {
    gint i;
    gint int_buf;

    /* Set sorting tab defaults */
    for (i = 0; i < SORT_TAB_MAX; i++) {
        /* sp_created_cond renamed to sp_added_cond */
        if (prefs_get_int_value_index("sp_created_cond", i, &int_buf)) {
            prefs_set_int_index("sp_added_cond", i, int_buf);
            prefs_set_string("sp_created_cond", NULL);
        }

        /* sp_created_state renamed to sp_added_state */
        if (prefs_get_int_value_index("sp_created_state", i, &int_buf)) {
            prefs_set_int_index("sp_added_state", i, int_buf);
            prefs_set_string("sp_created_state", NULL);
        }

        if (!prefs_get_int_value_index("st_autoselect", i, NULL))
            prefs_set_int_index("st_autoselect", i, TRUE);

        if (!prefs_get_int_value_index("st_category", i, NULL))
            prefs_set_int_index("st_category", i, (i < ST_CAT_NUM ? i : 0));

        if (!prefs_get_int_value_index("sp_or", i, NULL))
            prefs_set_int_index("sp_or", i, FALSE);

        if (!prefs_get_int_value_index("sp_rating_cond", i, NULL))
            prefs_set_int_index("sp_rating_cond", i, FALSE);

        if (!prefs_get_int_value_index("sp_playcount_cond", i, NULL))
            prefs_set_int_index("sp_playcount_cond", i, FALSE);

        if (!prefs_get_int_value_index("sp_played_cond", i, NULL))
            prefs_set_int_index("sp_played_cond", i, FALSE);

        if (!prefs_get_int_value_index("sp_modified_cond", i, NULL))
            prefs_set_int_index("sp_modified_cond", i, FALSE);

        if (!prefs_get_int_value_index("sp_added_cond", i, NULL))
            prefs_set_int_index("sp_added_cond", i, FALSE);

        if (!prefs_get_int_value_index("sp_rating_state", i, NULL))
            prefs_set_int_index("sp_rating_state", i, 0);

        if (!prefs_get_string_value_index("sp_played_state", i, NULL))
            prefs_set_string_index("sp_played_state", i, ">4w");

        if (!prefs_get_string_value_index("sp_modified_state", i, NULL))
            prefs_set_string_index("sp_modified_state", i, "<1d");

        if (!prefs_get_string_value_index("sp_added_state", i, NULL))
            prefs_set_string_index("sp_added_state", i, "<1d");

        if (!prefs_get_int_value_index("sp_playcount_low", i, NULL))
            prefs_set_int_index("sp_playcount_low", i, 0);

        if (!prefs_get_int_value_index("sp_playcount_high", i, NULL))
            prefs_set_int_index("sp_playcount_high", i, -1);

        if (!prefs_get_int_value_index("sp_autodisplay", i, NULL))
            prefs_set_int_index("sp_autodisplay", i, FALSE);
    }

    if (prefs_get_int_value("sort_tab_num", NULL))
        prefs_set_int("sort_tab_num", 2);

    if (prefs_get_int_value("st_sort", NULL))
        prefs_set_int("st_sort", SORT_NONE);

}

static gboolean activate_sorttab_display_plugin(AnjutaPlugin *plugin) {
    AnjutaUI *ui;
    SorttabDisplayPlugin *sorttab_display_plugin;
    GtkActionGroup* action_group;

    /* Prepare the icons for the sorttab */

    sorttab_display_plugin = (SorttabDisplayPlugin*) plugin;
    ui = anjuta_shell_get_ui(plugin->shell, NULL);

    /* Add our sorttab_actions */
    action_group
            = anjuta_ui_add_action_group_entries(ui, "ActionGroupSorttabDisplay", _("Sorttab Display"), sorttab_actions, G_N_ELEMENTS (sorttab_actions), GETTEXT_PACKAGE, TRUE, plugin);
    sorttab_display_plugin->action_group = action_group;

    sorttab_display_plugin->more_filtertabs_action
            = gtk_action_new("ActionViewMoreFilterTabs", _("More Filter Tabs"), NULL, GTK_STOCK_GO_UP);
    g_signal_connect(sorttab_display_plugin->more_filtertabs_action, "activate", G_CALLBACK(on_more_sort_tabs_activate), sorttab_display_plugin);
    gtk_action_group_add_action(sorttab_display_plugin->action_group, sorttab_display_plugin->more_filtertabs_action);

    sorttab_display_plugin->fewer_filtertabs_action
            = gtk_action_new("ActionViewFewerFilterTabs", _("Fewer Filter Tabs"), NULL, GTK_STOCK_GO_DOWN);
    g_signal_connect(sorttab_display_plugin->fewer_filtertabs_action, "activate", G_CALLBACK(on_fewer_sort_tabs_activate), sorttab_display_plugin);
    gtk_action_group_add_action(sorttab_display_plugin->action_group, sorttab_display_plugin->fewer_filtertabs_action);

    /* Merge UI */
    sorttab_display_plugin->uiid = anjuta_ui_merge(ui, UI_FILE);

    /* Set preferences */
    set_default_preferences();

    /* Add widget in Shell. Any number of widgets can be added */
    sorttab_display_plugin->st_paned = gtk_hpaned_new();
    st_create_tabs(GTK_PANED(sorttab_display_plugin->st_paned));
    gtk_widget_show(sorttab_display_plugin->st_paned);

    g_signal_connect (gtkpod_app, SIGNAL_PLAYLIST_SELECTED, G_CALLBACK (sorttab_display_select_playlist_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_TRACK_REMOVED, G_CALLBACK (sorttab_display_track_removed_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_TRACK_UPDATED, G_CALLBACK (sorttab_display_track_updated_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_PREFERENCE_CHANGE, G_CALLBACK (sorttab_display_preference_changed_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_TRACKS_REORDERED, G_CALLBACK (sorttab_display_tracks_reordered_cb), NULL);

    anjuta_shell_add_widget(plugin->shell, sorttab_display_plugin->st_paned, "SorttabDisplayPlugin", "Track Filter", NULL, ANJUTA_SHELL_PLACEMENT_CENTER, NULL);
    return TRUE; /* FALSE if activation failed */
}

static gboolean deactivate_sorttab_display_plugin(AnjutaPlugin *plugin) {
    AnjutaUI *ui;
    SorttabDisplayPlugin *sorttab_display_plugin;

    sorttab_display_plugin = (SorttabDisplayPlugin*) plugin;
    ui = anjuta_shell_get_ui(plugin->shell, NULL);

    sorttab_display_plugin->more_filtertabs_action = NULL;
    sorttab_display_plugin->fewer_filtertabs_action = NULL;

    /* Remove widgets from Shell */
    anjuta_shell_remove_widget(plugin->shell, sorttab_display_plugin->st_paned, NULL);

    /* Destroy the paned */
    sorttab_display_plugin->st_paned = NULL;

    /* Unmerge UI */
    anjuta_ui_unmerge(ui, sorttab_display_plugin->uiid);

    /* Remove Action groups */
    anjuta_ui_remove_action_group(ui, sorttab_display_plugin->action_group);

    /* FALSE if plugin doesn't want to deactivate */
    return TRUE;
}

static void sorttab_display_plugin_instance_init(GObject *obj) {
    SorttabDisplayPlugin *plugin = (SorttabDisplayPlugin*) obj;
    plugin->uiid = 0;
    plugin->st_paned = NULL;
    plugin->action_group = NULL;
}

static void sorttab_display_plugin_class_init(GObjectClass *klass) {
    AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

    parent_class = g_type_class_peek_parent(klass);

    plugin_class->activate = activate_sorttab_display_plugin;
    plugin_class->deactivate = deactivate_sorttab_display_plugin;
}

static void ipreferences_merge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e) {
    gchar *file;
    GdkPixbuf *pixbuf;

    SorttabDisplayPlugin* plugin = SORTTAB_DISPLAY_PLUGIN(ipref);
    plugin->prefs = init_sorttab_preferences();
    if (plugin->prefs == NULL)
        return;

    file = g_build_filename(GTKPOD_IMAGE_DIR, "hicolor/48x48/places", ICON_FILE, NULL);
    pixbuf = gdk_pixbuf_new_from_file(file, NULL);
    anjuta_preferences_dialog_add_page(ANJUTA_PREFERENCES_DIALOG (anjuta_preferences_get_dialog (prefs)), "gtkpod-sorttab-settings", _(TAB_NAME), pixbuf, plugin->prefs);
    g_free(file);
    g_object_unref(pixbuf);
}

static void ipreferences_unmerge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e) {
    anjuta_preferences_remove_page(prefs, _(TAB_NAME));
    SorttabDisplayPlugin* plugin = SORTTAB_DISPLAY_PLUGIN(ipref);
    gtk_widget_destroy(plugin->prefs);
}

static void ipreferences_iface_init(IAnjutaPreferencesIface* iface) {
    iface->merge = ipreferences_merge;
    iface->unmerge = ipreferences_unmerge;
}

ANJUTA_PLUGIN_BEGIN (SorttabDisplayPlugin, sorttab_display_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE(ipreferences, IANJUTA_TYPE_PREFERENCES);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (SorttabDisplayPlugin, sorttab_display_plugin)
;
