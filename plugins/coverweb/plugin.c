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
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/interfaces/ianjuta-preferences.h>
#include "libgtkpod/gtkpod_app_iface.h"
#include "libgtkpod/prefs.h"
#include "libgtkpod/directories.h"
#include "libgtkpod/stock_icons.h"
#include "plugin.h"
#include "coverweb.h"
#include "coverweb_preferences.h"

#define PREFERENCE_ICON "cover_web-gtkpod-category"
#define PREFERENCE_ICON_STOCK_ID "cover_web-preference-icon"
#define TAB_NAME "Cover Web"

/* Parent class. Part of standard class definition */
static gpointer parent_class;

static GtkActionEntry cover_actions[] =
    { };

static void set_default_preferences() {
    if (!prefs_get_string_value_index("coverweb_bookmark_", 0, NULL)) {
        prefs_set_string_index("coverweb_bookmark_", 0, "http://images.google.com");
        prefs_set_string_index("coverweb_bookmark_", 1, "http://www.allcdcovers.com");
        prefs_set_string_index("coverweb_bookmark_", 2, LIST_END_MARKER);
    }
}

static gboolean activate_plugin(AnjutaPlugin *plugin) {
    AnjutaUI *ui;
    CoverWebPlugin *cover_web_plugin;
    GtkActionGroup* action_group;

    register_icon_path(get_plugin_dir(), "coverweb");
    register_stock_icon(PREFERENCE_ICON, PREFERENCE_ICON_STOCK_ID);

    cover_web_plugin = (CoverWebPlugin*) plugin;
    ui = anjuta_shell_get_ui(plugin->shell, NULL);

    /* Add our cover_actions */
    action_group
            = anjuta_ui_add_action_group_entries(ui, "ActionGroupCoverWeb", _("Cover Display"), cover_actions, G_N_ELEMENTS (cover_actions), GETTEXT_PACKAGE, TRUE, plugin);
    cover_web_plugin->action_group = action_group;

    /* Merge UI */
    gchar *uipath = g_build_filename(get_ui_dir(), "coverweb.ui", NULL);
    cover_web_plugin->uiid = anjuta_ui_merge(ui, uipath);
    g_free(uipath);

    /* Set preferences */
    set_default_preferences();

    /* Add widget in Shell. Any number of widgets can be added */
    cover_web_plugin->coverweb_window = gtk_scrolled_window_new(NULL, NULL);
    g_object_ref(cover_web_plugin->coverweb_window);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (cover_web_plugin->coverweb_window), GTK_POLICY_NEVER, GTK_POLICY_NEVER);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW (cover_web_plugin->coverweb_window), GTK_SHADOW_IN);

    init_web_browser(cover_web_plugin->coverweb_window);
    gtk_widget_show_all(cover_web_plugin->coverweb_window);
    anjuta_shell_add_widget(plugin->shell, cover_web_plugin->coverweb_window, "CoverWebPlugin", "Cover Browser", NULL, ANJUTA_SHELL_PLACEMENT_CENTER, NULL);

    return TRUE; /* FALSE if activation failed */
}

static gboolean deactivate_plugin(AnjutaPlugin *plugin) {
    AnjutaUI *ui;
    CoverWebPlugin *cover_web_plugin;

    cover_web_plugin = (CoverWebPlugin*) plugin;
    ui = anjuta_shell_get_ui(plugin->shell, NULL);

    /* Remove widgets from Shell */
    anjuta_shell_remove_widget(plugin->shell, cover_web_plugin->coverweb_window, NULL);

    /* Destroy the browser */
    destroy_cover_web();

    /* Unmerge UI */
    anjuta_ui_unmerge(ui, cover_web_plugin->uiid);

    /* Remove Action groups */
    anjuta_ui_remove_action_group(ui, cover_web_plugin->action_group);

    /* FALSE if plugin doesn't want to deactivate */
    return TRUE;
}

static void cover_web_plugin_instance_init(GObject *obj) {
    CoverWebPlugin *plugin = (CoverWebPlugin*) obj;
    plugin->uiid = 0;
    plugin->coverweb_window = NULL;
    plugin->action_group = NULL;
    plugin->glade_path = g_build_filename(get_glade_dir(), "coverweb.glade", NULL);
}

static void cover_web_plugin_class_init(GObjectClass *klass) {
    AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

    parent_class = g_type_class_peek_parent(klass);

    plugin_class->activate = activate_plugin;
    plugin_class->deactivate = deactivate_plugin;
}

static void ipreferences_merge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e) {
    GdkPixbuf *pixbuf;
    GError *error = NULL;

    CoverWebPlugin* plugin = COVER_WEB_PLUGIN(ipref);
    plugin->prefs = init_coverweb_preferences(plugin->glade_path);
    if (plugin->prefs == NULL)
        return;

    pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), PREFERENCE_ICON, 48, 0, &error);

    if (!pixbuf) {
        g_warning ("Couldn't load icon: %s", error->message);
        g_error_free(error);
    }
    anjuta_preferences_dialog_add_page(ANJUTA_PREFERENCES_DIALOG (anjuta_preferences_get_dialog (prefs)), "gtkpod-coverweb-settings", _(TAB_NAME), pixbuf, plugin->prefs);
    g_object_unref(pixbuf);
}

static void ipreferences_unmerge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e) {
    anjuta_preferences_remove_page(prefs, _(TAB_NAME));
    CoverWebPlugin* plugin = COVER_WEB_PLUGIN(ipref);
    gtk_widget_destroy(plugin->prefs);
}

static void ipreferences_iface_init(IAnjutaPreferencesIface* iface) {
    iface->merge = ipreferences_merge;
    iface->unmerge = ipreferences_unmerge;
}

ANJUTA_PLUGIN_BEGIN (CoverWebPlugin, cover_web_plugin);
        ANJUTA_PLUGIN_ADD_INTERFACE(ipreferences, IANJUTA_TYPE_PREFERENCES);ANJUTA_PLUGIN_END
;

ANJUTA_SIMPLE_PLUGIN (CoverWebPlugin, cover_web_plugin)
;
