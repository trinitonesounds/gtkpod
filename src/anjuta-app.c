/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta.c Copyright (C) 2003 Naba Kumar  <naba@gnome.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>

#include <glade/glade-xml.h>
#include <gtk/gtk.h>

#include <gdl/gdl.h>

#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-ui.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/resources.h>
#include <libanjuta/anjuta-plugin-manager.h>
#include <libanjuta/anjuta-debug.h>

#include "anjuta-app.h"
#include "anjuta-actions.h"
#include "anjuta-about.h"

#include "gtkpod.h"
#include "directories.h"

#define ICON_FILE "anjuta-preferences-general-48.png"

static void anjuta_app_layout_load (AnjutaApp *app,
									const gchar *layout_filename,
									const gchar *name);
static void anjuta_app_layout_save (AnjutaApp *app,
									const gchar *layout_filename,
									const gchar *name);

static gpointer parent_class = NULL;
static GtkToolbarStyle style = -1;
static gchar * uifile = NULL;

void anjuta_set_ui_file_path (gchar * path)
{
    uifile = path;
}

static void
on_toolbar_style_changed (GConfClient* client, guint id, GConfEntry* entry,
						  gpointer user_data)
{
	AnjutaApp* app = ANJUTA_APP (user_data);

	char* tb_style = anjuta_preferences_get (app->preferences, "anjuta.toolbar.style");

	if (tb_style)
	{
		if (strcasecmp (tb_style, "Default") == 0)
			style = -1;
		else if (strcasecmp (tb_style, "Both") == 0)
			style = GTK_TOOLBAR_BOTH;
		else if (strcasecmp (tb_style, "Horiz") == 0)
			style = GTK_TOOLBAR_BOTH_HORIZ;
		else if (strcasecmp (tb_style, "Icons") == 0)
			style = GTK_TOOLBAR_ICONS;
		else if (strcasecmp (tb_style, "Text") == 0)
			style = GTK_TOOLBAR_TEXT;

		DEBUG_PRINT ("Toolbar style: %s", tb_style);

		g_free(tb_style);
	}

	if (style != -1)
	{
		gtk_toolbar_set_style (GTK_TOOLBAR (app->toolbar), style);
	}
	else
	{
		gtk_toolbar_unset_style (GTK_TOOLBAR (app->toolbar));
	}
}

static void
on_gdl_style_changed (GConfClient* client, guint id, GConfEntry* entry,
					  gpointer user_data)
{
	AnjutaApp* app = ANJUTA_APP (user_data);
	GdlSwitcherStyle style = GDL_SWITCHER_STYLE_BOTH;

	gchar* pr_style = anjuta_preferences_get (app->preferences, "anjuta.gdl.style");

	if (pr_style)
	{
		if (strcasecmp (pr_style, "Text") == 0)
			style = GDL_SWITCHER_STYLE_TEXT;
		else if (strcasecmp (pr_style, "Icon") == 0)
			style = GDL_SWITCHER_STYLE_ICON;
		else if (strcasecmp (pr_style, "Both") == 0)
			style = GDL_SWITCHER_STYLE_BOTH;
		else if (strcasecmp (pr_style, "Toolbar") == 0)
			style = GDL_SWITCHER_STYLE_TOOLBAR;
		else if (strcasecmp (pr_style, "Tabs") == 0)
			style = GDL_SWITCHER_STYLE_TABS;

		DEBUG_PRINT ("Switcher style: %s", pr_style);

		g_free (pr_style);
	}
	g_object_set (G_OBJECT(app->layout_manager->master), "switcher-style",
				  style, NULL);
}

static void
on_toggle_widget_view (GtkCheckMenuItem *menuitem, GtkWidget *dockitem)
{
	gboolean state;
	state = gtk_check_menu_item_get_active (menuitem);
	if (state)
		gdl_dock_item_show_item (GDL_DOCK_ITEM (dockitem));
	else
		gdl_dock_item_hide_item (GDL_DOCK_ITEM (dockitem));
}

static void
on_update_widget_view_menuitem (gpointer key, gpointer wid, gpointer data)
{
	GtkCheckMenuItem *menuitem;
	GdlDockItem *dockitem;

	dockitem = g_object_get_data (G_OBJECT (wid), "dockitem");
	menuitem = g_object_get_data (G_OBJECT (wid), "menuitem");

	g_signal_handlers_block_by_func (menuitem,
									 G_CALLBACK (on_toggle_widget_view),
									 dockitem);

	if (GDL_DOCK_OBJECT_ATTACHED (dockitem))
		gtk_check_menu_item_set_active (menuitem, TRUE);
	else
		gtk_check_menu_item_set_active (menuitem, FALSE);

	g_signal_handlers_unblock_by_func (menuitem,
									   G_CALLBACK (on_toggle_widget_view),
									   dockitem);
}

static void
on_layout_dirty_notify (GObject *object, GParamSpec *pspec, gpointer user_data)
{
	if (!strcmp (pspec->name, "dirty")) {
		gboolean dirty;
		g_object_get (object, "dirty", &dirty, NULL);
		if (dirty) {
			/* Update UI toggle buttons */
			g_hash_table_foreach (ANJUTA_APP (user_data)->widgets,
								  on_update_widget_view_menuitem,
								  NULL);
		}
	}
}

static void
on_layout_locked_notify (GdlDockMaster *master, GParamSpec *pspec,
						 AnjutaApp     *app)
{
	AnjutaUI *ui;
	GtkAction *action;
	gint locked;

	ui = app->ui;
	action = anjuta_ui_get_action (ui, "ActionGroupToggleView",
								   "ActionViewLockLayout");

	g_object_get (master, "locked", &locked, NULL);
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
								  (locked == 1));
}

static void
on_session_save (AnjutaShell *shell, AnjutaSessionPhase phase,
				 AnjutaSession *session, AnjutaApp *app)
{
	gchar *geometry, *layout_file;
	GdkWindowState state;

	if (phase != ANJUTA_SESSION_PHASE_NORMAL)
		return;

	/* Save geometry */
	state = gdk_window_get_state (GTK_WIDGET (app)->window);
	if (state & GDK_WINDOW_STATE_MAXIMIZED) {
		anjuta_session_set_int (session, "Anjuta", "Maximized", 1);
	}
	if (state & GDK_WINDOW_STATE_FULLSCREEN) {
		anjuta_session_set_int (session, "Anjuta", "Fullscreen", 1);
	}

	/* Save geometry only if window is not maximized or fullscreened */
	if (!(state & GDK_WINDOW_STATE_MAXIMIZED) ||
		!(state & GDK_WINDOW_STATE_FULLSCREEN))
	{
		geometry = anjuta_app_get_geometry (app);
		if (geometry)
			anjuta_session_set_string (session, "Anjuta", "Geometry",
									   geometry);
		g_free (geometry);
	}

	/* Save layout */
	layout_file = g_build_filename (anjuta_session_get_session_directory (session),
									"dock-layout.xml", NULL);
	anjuta_app_layout_save (app, layout_file, NULL);
	g_free (layout_file);
}

static void
on_session_load (AnjutaShell *shell, AnjutaSessionPhase phase,
				 AnjutaSession *session, AnjutaApp *app)
{
	/* We load layout at last so that all plugins would have loaded by now */
	if (phase == ANJUTA_SESSION_PHASE_LAST)
	{
		gchar *geometry;
		gchar *layout_file;

		/* Restore geometry */
		geometry = anjuta_session_get_string (session, "Anjuta", "Geometry");
		anjuta_app_set_geometry (app, geometry);

		/* Restore window state */
		if (anjuta_session_get_int (session, "Anjuta", "Fullscreen"))
		{
			/* bug #304495 */
			AnjutaUI* ui = anjuta_shell_get_ui(shell, NULL);
			GtkAction* action = anjuta_ui_get_action (ui, "ActionGroupToggleView",
								   "ActionViewFullscreen");
			gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
								  TRUE);

			gtk_window_fullscreen (GTK_WINDOW (shell));

		}
		else if (anjuta_session_get_int (session, "Anjuta", "Maximized"))
		{
			gtk_window_maximize (GTK_WINDOW (shell));
		}

		/* Restore layout */
		layout_file = g_build_filename (anjuta_session_get_session_directory (session),
										"dock-layout.xml", NULL);
		anjuta_app_layout_load (app, layout_file, NULL);
		g_free (layout_file);
	}
}

static void
anjuta_app_dispose (GObject *widget)
{
	AnjutaApp *app;

	g_return_if_fail (ANJUTA_IS_APP (widget));

	app = ANJUTA_APP (widget);

	if (app->widgets)
	{
		if (g_hash_table_size (app->widgets) > 0)
		{
			/*
			g_warning ("Some widgets are still inside shell (%d widgets), they are:",
					   g_hash_table_size (app->widgets));
			g_hash_table_foreach (app->widgets, (GHFunc)puts, NULL);
			*/
		}
		g_hash_table_destroy (app->widgets);
		app->widgets = NULL;
	}

	if (app->values)
	{
		if (g_hash_table_size (app->values) > 0)
		{
			/*
			g_warning ("Some Values are still left in shell (%d Values), they are:",
					   g_hash_table_size (app->values));
			g_hash_table_foreach (app->values, (GHFunc)puts, NULL);
			*/
		}
		g_hash_table_destroy (app->values);
		app->values = NULL;
	}

	if (app->layout_manager) {
		g_object_unref (app->layout_manager);
		app->layout_manager = NULL;
	}
	if (app->profile_manager) {
		g_object_unref (G_OBJECT (app->profile_manager));
		app->profile_manager = NULL;
	}
	if (app->plugin_manager) {
		g_object_unref (G_OBJECT (app->plugin_manager));
		app->plugin_manager = NULL;
	}
	if (app->status) {
		g_object_unref (G_OBJECT (app->status));
		app->status = NULL;
	}

	G_OBJECT_CLASS (parent_class)->dispose (widget);
}

static void
anjuta_app_finalize (GObject *widget)
{
	AnjutaApp *app;

	g_return_if_fail (ANJUTA_IS_APP (widget));

	app = ANJUTA_APP (widget);

	gtk_widget_destroy (GTK_WIDGET (app->ui));
	gtk_widget_destroy (GTK_WIDGET (app->preferences));

	G_OBJECT_CLASS (parent_class)->finalize (widget);
}

static void
anjuta_app_instance_init (AnjutaApp *app)
{
	gint merge_id;
	GtkWidget *menubar, *about_menu;
	GtkWidget *view_menu, *hbox;
	GtkWidget *main_box;
	GtkWidget *dockbar;
	GtkAction* action;
	GList *plugins_dirs = NULL;
	GdkGeometry size_hints = {
    	100, 100, 0, 0, 100, 100, 1, 1, 0.0, 0.0, GDK_GRAVITY_NORTH_WEST
  	};

	DEBUG_PRINT ("%s", "Initializing Anjuta...");

	gtk_window_set_geometry_hints (GTK_WINDOW (app), GTK_WIDGET (app),
								   &size_hints, GDK_HINT_RESIZE_INC);
	gtk_window_set_resizable (GTK_WINDOW (app), TRUE);

	/*
	 * Main box
	 */
	main_box = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (app), main_box);
	gtk_widget_show (main_box);

	app->values = NULL;
	app->widgets = NULL;

	/* Status bar */
	app->status = ANJUTA_STATUS (anjuta_status_new ());
	anjuta_status_set_title_window (app->status, GTK_WIDGET (app));
	gtk_widget_show (GTK_WIDGET (app->status));
	gtk_box_pack_end (GTK_BOX (main_box),
					  GTK_WIDGET (app->status), FALSE, TRUE, 0);
	g_object_ref (G_OBJECT (app->status));
	g_object_add_weak_pointer (G_OBJECT (app->status), (gpointer)&app->status);

	/* configure dock */
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox);
	app->dock = gdl_dock_new ();
	gtk_widget_show (app->dock);
	gtk_box_pack_end(GTK_BOX (hbox), app->dock, TRUE, TRUE, 0);

	dockbar = gdl_dock_bar_new (GDL_DOCK(app->dock));
	gtk_widget_show (dockbar);
	gtk_box_pack_start(GTK_BOX (hbox), dockbar, FALSE, FALSE, 0);

	app->layout_manager = gdl_dock_layout_new (GDL_DOCK (app->dock));
	g_signal_connect (app->layout_manager, "notify::dirty",
					  G_CALLBACK (on_layout_dirty_notify), app);
	g_signal_connect (app->layout_manager->master, "notify::locked",
					  G_CALLBACK (on_layout_locked_notify), app);

	/* UI engine */
	app->ui = anjuta_ui_new ();
	g_object_add_weak_pointer (G_OBJECT (app->ui), (gpointer)&app->ui);

	/* Plugin Manager */
	plugins_dirs = g_list_prepend (plugins_dirs, get_plugin_dir());
	app->plugin_manager = anjuta_plugin_manager_new (G_OBJECT (app),
													 app->status,
													 plugins_dirs);
	app->profile_manager = anjuta_profile_manager_new (app->plugin_manager);
	g_list_free (plugins_dirs);

	/* Preferences */
	app->preferences = anjuta_preferences_new (app->plugin_manager);
	g_object_add_weak_pointer (G_OBJECT (app->preferences),
							   (gpointer)&app->preferences);

	anjuta_preferences_notify_add (app->preferences, "anjuta.gdl.style",
								   on_gdl_style_changed, app, NULL);

	on_gdl_style_changed (NULL, 0, NULL, app);

	/* Register actions */
	anjuta_ui_add_action_group_entries (app->ui, "ActionGroupFile", _("File"),
										menu_entries_file,
										G_N_ELEMENTS (menu_entries_file),
										GETTEXT_PACKAGE, TRUE, app);
	anjuta_ui_add_action_group_entries (app->ui, "ActionGroupEdit", _("Edit"),
										menu_entries_edit,
										G_N_ELEMENTS (menu_entries_edit),
										GETTEXT_PACKAGE, TRUE, app);
	anjuta_ui_add_action_group_entries (app->ui, "ActionGroupView", _("View"),
										menu_entries_view,
										G_N_ELEMENTS (menu_entries_view),
										GETTEXT_PACKAGE, TRUE, app);
	anjuta_ui_add_toggle_action_group_entries (app->ui, "ActionGroupToggleView",
										_("View"),
										menu_entries_toggle_view,
										G_N_ELEMENTS (menu_entries_toggle_view),
										GETTEXT_PACKAGE, TRUE, app);
	anjuta_ui_add_action_group_entries (app->ui, "ActionGroupHelp", _("Help"),
										menu_entries_help,
										G_N_ELEMENTS (menu_entries_help),
										GETTEXT_PACKAGE, TRUE, app);

	/* Merge UI */
	merge_id = anjuta_ui_merge (app->ui, uifile);

	/* Adding accels group */
	gtk_window_add_accel_group (GTK_WINDOW (app),
								gtk_ui_manager_get_accel_group (GTK_UI_MANAGER (app->ui)));

	/* create main menu */
	menubar = gtk_ui_manager_get_widget (GTK_UI_MANAGER (app->ui),
										 "/MenuMain");
	gtk_box_pack_start (GTK_BOX (main_box), menubar, FALSE, FALSE, 0);
	gtk_widget_show (menubar);

	/* create toolbar */
	app->toolbar = gtk_ui_manager_get_widget (GTK_UI_MANAGER (app->ui),
										 "/ToolbarMain");
    if (!anjuta_preferences_get_int (app->preferences, "anjuta.toolbar.visible"))
		gtk_widget_hide (app->toolbar);
	gtk_box_pack_start (GTK_BOX (main_box), app->toolbar, FALSE, FALSE, 0);
	action = gtk_ui_manager_get_action (GTK_UI_MANAGER (app->ui),
										"/MenuMain/MenuView/Toolbar");
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION(action),
								  anjuta_preferences_get_int_with_default (app->preferences,
																		   "anjuta.toolbar.visible",
																		   TRUE));
	anjuta_preferences_notify_add (app->preferences, "anjuta.toolbar.style",
								   on_toolbar_style_changed, app, NULL);
	on_toolbar_style_changed (NULL, 0, NULL, app);

	/* Create widgets menu */
	view_menu =
		gtk_ui_manager_get_widget (GTK_UI_MANAGER(app->ui),
								  "/MenuMain/MenuView");
	app->view_menu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (view_menu));

	/* Disable unavailible tutorials */
	action = anjuta_ui_get_action(app->ui, "ActionGroupHelp", "ActionHelpTutorial");
	g_object_set(G_OBJECT(action), "visible", FALSE, NULL);
	action = anjuta_ui_get_action(app->ui, "ActionGroupHelp", "ActionHelpAdvancedTutorial");
	g_object_set(G_OBJECT(action), "visible", FALSE, NULL);

	/* Create about plugins menu */
	about_menu =
		gtk_ui_manager_get_widget (GTK_UI_MANAGER(app->ui),
								   "/MenuMain/PlaceHolderHelpMenus/MenuHelp/"
								   "PlaceHolderHelpAbout/AboutPlugins");
	about_create_plugins_submenu (ANJUTA_SHELL (app), about_menu);

	/* Add main view */
	gtk_box_pack_start (GTK_BOX (main_box), hbox, TRUE, TRUE, 0);

	/* Connect to session */
	g_signal_connect (G_OBJECT (app), "save_session",
					  G_CALLBACK (on_session_save), app);
	g_signal_connect (G_OBJECT (app), "load_session",
					  G_CALLBACK (on_session_load), app);

	/* Loading accels */
	anjuta_ui_load_accels (NULL);

	app->save_count = 0;
}

static void
anjuta_app_class_init (AnjutaAppClass *class)
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;
	widget_class = (GtkWidgetClass*) class;
	object_class->finalize = anjuta_app_finalize;
	object_class->dispose = anjuta_app_dispose;
}

GtkWidget *
anjuta_app_new (void)
{
	AnjutaApp *app;

	app = ANJUTA_APP (g_object_new (ANJUTA_TYPE_APP,
									"title", "GtkPod",
									NULL));
	return GTK_WIDGET (app);
}

gchar*
anjuta_app_get_geometry (AnjutaApp *app)
{
	gchar *geometry;
	gint width, height, posx, posy;

	g_return_val_if_fail (ANJUTA_IS_APP (app), NULL);

	geometry = NULL;
	width = height = posx = posy = 0;
	if (GTK_WIDGET(app)->window)
	{
		gtk_window_get_size (GTK_WINDOW (app), &width, &height);
		gtk_window_get_position (GTK_WINDOW(app), &posx, &posy);

		geometry = g_strdup_printf ("%dx%d+%d+%d", width, height, posx, posy);
	}
	return geometry;
}

void
anjuta_app_set_geometry (AnjutaApp *app, const gchar *geometry)
{
	gint width, height, posx, posy;
	gboolean geometry_set = FALSE;

	if (geometry && strlen (geometry) > 0)
	{
		DEBUG_PRINT ("Setting geometry: %s", geometry);

		if (sscanf (geometry, "%dx%d+%d+%d", &width, &height,
					&posx, &posy) == 4)
		{
			if (GTK_WIDGET_REALIZED (app))
			{
				gtk_window_resize (GTK_WINDOW (app), width, height);
			}
			else
			{
				gtk_window_set_default_size (GTK_WINDOW (app), width, height);
				gtk_window_move (GTK_WINDOW (app), posx, posy);
			}
			geometry_set = TRUE;
		}
		else
		{
			g_warning ("Failed to parse geometry: %s", geometry);
		}
	}
	if (!geometry_set)
	{
		posx = 10;
		posy = 10;
		width = gdk_screen_width () - 10;
		height = gdk_screen_height () - 25;
		width = (width < 790)? width : 790;
		height = (height < 575)? width : 575;
		if (GTK_WIDGET_REALIZED (app) == FALSE)
		{
			gtk_window_set_default_size (GTK_WINDOW (app), width, height);
			gtk_window_move (GTK_WINDOW (app), posx, posy);
		}
	}
}

static void
anjuta_app_layout_save (AnjutaApp *app, const gchar *filename,
						const gchar *name)
{
	g_return_if_fail (ANJUTA_IS_APP (app));
	g_return_if_fail (filename != NULL);

	gdl_dock_layout_save_layout (app->layout_manager, name);
	if (!gdl_dock_layout_save_to_file (app->layout_manager, filename))
		g_warning ("Saving dock layout to '%s' failed!", filename);

	/* This is a good place to save the accels too */
	anjuta_ui_save_accels (NULL);
}

static void
anjuta_app_layout_load (AnjutaApp *app, const gchar *layout_filename,
						const gchar *name)
{
	g_return_if_fail (ANJUTA_IS_APP (app));

	if (!layout_filename ||
		!gdl_dock_layout_load_from_file (app->layout_manager, layout_filename))
	{
		gchar *datadir, *filename;
		datadir = anjuta_res_get_data_dir();

		filename = g_build_filename (datadir, "layout.xml", NULL);
		DEBUG_PRINT ("Layout = %s", filename);
		g_free (datadir);
		if (!gdl_dock_layout_load_from_file (app->layout_manager, filename))
			g_warning ("Loading layout from '%s' failed!!", filename);
		g_free (filename);
	}

	if (!gdl_dock_layout_load_layout (app->layout_manager, name))
		g_warning ("Loading layout failed!!");
}

void
anjuta_app_layout_reset (AnjutaApp *app)
{
	anjuta_app_layout_load (app, NULL, NULL);
}

void
anjuta_app_install_preferences (AnjutaApp *app)
{
	GladeXML *gxml;
	GtkWidget *notebook, *plugins;

	/* Create preferences page */
#ifdef ENABLE_NLS
    gxml = glade_xml_new(GTKPOD_GLADE_XML_FILE,  "gtkpod_preferences_window", GETTEXT_PACKAGE);
#else
    gxml = glade_xml_new (GTKPOD_GLADE_XML_FILE,  "gtkpod_preferences_window", NULL);
#endif

	anjuta_preferences_add_page (app->preferences, gxml,
								 "General", _("General"), ICON_FILE);
	notebook = 	glade_xml_get_widget (gxml, "General");
	plugins = anjuta_plugin_manager_get_plugins_page (app->plugin_manager);
	gtk_widget_show (plugins);

	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), plugins,
							  gtk_label_new (_("Installed plugins")));

	g_object_unref (gxml);
}

/* AnjutaShell Implementation */

static void
on_value_removed_from_hash (gpointer value)
{
	g_value_unset ((GValue*)value);
	g_free (value);
}

static void
anjuta_app_add_value (AnjutaShell *shell, const char *name,
					  const GValue *value, GError **error)
{
	GValue *copy;
	AnjutaApp *app;

	g_return_if_fail (ANJUTA_IS_APP (shell));
	g_return_if_fail (name != NULL);
	g_return_if_fail (G_IS_VALUE(value));

	app = ANJUTA_APP (shell);

	if (app->values == NULL)
	{
		app->values = g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
											 on_value_removed_from_hash);
	}
	anjuta_shell_remove_value (shell, name, error);

	copy = g_new0 (GValue, 1);
	g_value_init (copy, value->g_type);
	g_value_copy (value, copy);

	g_hash_table_insert (app->values, g_strdup (name), copy);
	g_signal_emit_by_name (shell, "value_added", name, copy);
}

static void
anjuta_app_get_value (AnjutaShell *shell, const char *name, GValue *value,
					  GError **error)
{
	GValue *val;
	AnjutaApp *app;

	g_return_if_fail (ANJUTA_IS_APP (shell));
	g_return_if_fail (name != NULL);
	/* g_return_if_fail (G_IS_VALUE (value)); */

	app = ANJUTA_APP (shell);

	val = NULL;
	if (app->values)
		val = g_hash_table_lookup (app->values, name);
	if (val)
	{
		if (!value->g_type)
		{
			g_value_init (value, val->g_type);
		}
		g_value_copy (val, value);
	}
	else
	{
		if (error)
		{
			*error = g_error_new (ANJUTA_SHELL_ERROR,
								  ANJUTA_SHELL_ERROR_DOESNT_EXIST,
								  _("Value doesn't exist"));
		}
	}
}

static void
anjuta_app_remove_value (AnjutaShell *shell, const char *name, GError **error)
{
	AnjutaApp *app;
	GValue *value;
	char *key;

	g_return_if_fail (ANJUTA_IS_APP (shell));
	g_return_if_fail (name != NULL);

	app = ANJUTA_APP (shell);

	/*
	g_return_if_fail (app->values != NULL);
	if (app->widgets && g_hash_table_lookup_extended (app->widgets, name,
													  (gpointer*)&key,
													  (gpointer*)&w)) {
		GtkWidget *item;
		item = g_object_get_data (G_OBJECT (w), "dockitem");
		gdl_dock_item_hide_item (GDL_DOCK_ITEM (item));
		gdl_dock_object_unbind (GDL_DOCK_OBJECT (item));
		g_free (key);
	}
	*/

	if (app->values && g_hash_table_lookup_extended (app->values, name,
													 (gpointer)&key,
													 (gpointer)&value)) {
		g_signal_emit_by_name (app, "value_removed", name);
		g_hash_table_remove (app->values, name);
	}
}

static void
anjuta_app_saving_push (AnjutaShell* shell)
{
	AnjutaApp* app = ANJUTA_APP (shell);
	app->save_count++;
}

static void
anjuta_app_saving_pop (AnjutaShell* shell)
{
	AnjutaApp* app = ANJUTA_APP (shell);
	app->save_count--;
}

static gboolean
remove_from_widgets_hash (gpointer name, gpointer hash_widget, gpointer widget)
{
	if (hash_widget == widget)
		return TRUE;
	return FALSE;
}

static void
on_widget_destroy (GtkWidget *widget, AnjutaApp *app)
{
	DEBUG_PRINT ("%s", "Widget about to be destroyed");
	g_hash_table_foreach_remove (app->widgets, remove_from_widgets_hash,
								 widget);
}

static void
on_widget_remove (GtkWidget *container, GtkWidget *widget, AnjutaApp *app)
{
	GtkWidget *dock_item;

	dock_item = g_object_get_data (G_OBJECT (widget), "dockitem");
	if (dock_item)
	{
		gchar* unique_name = g_object_get_data(G_OBJECT(dock_item), "unique_name");
		g_free(unique_name);
		g_signal_handlers_disconnect_by_func (G_OBJECT (dock_item),
					G_CALLBACK (on_widget_remove), app);
		gdl_dock_item_unbind (GDL_DOCK_ITEM(dock_item));
	}
	if (g_hash_table_foreach_remove (app->widgets,
									 remove_from_widgets_hash,
									 widget)){
		DEBUG_PRINT ("%s", "Widget removed from container");
	}
}

static void
on_widget_removed_from_hash (gpointer widget)
{
	AnjutaApp *app;
	GtkWidget *menuitem;
	GdlDockItem *dockitem;

	DEBUG_PRINT ("%s", "Removing widget from hash");

	app = g_object_get_data (G_OBJECT (widget), "app-object");
	dockitem = g_object_get_data (G_OBJECT (widget), "dockitem");
	menuitem = g_object_get_data (G_OBJECT (widget), "menuitem");

	gtk_widget_destroy (menuitem);

	g_object_set_data (G_OBJECT (widget), "dockitem", NULL);
	g_object_set_data (G_OBJECT (widget), "menuitem", NULL);

	g_signal_handlers_disconnect_by_func (G_OBJECT (widget),
				G_CALLBACK (on_widget_destroy), app);
	g_signal_handlers_disconnect_by_func (G_OBJECT (dockitem),
				G_CALLBACK (on_widget_remove), app);

	g_object_unref (G_OBJECT (widget));
}

static void
anjuta_app_add_widget_full (AnjutaShell *shell,
					   GtkWidget *widget,
					   const char *name,
					   const char *title,
					   const char *stock_id,
					   AnjutaShellPlacement placement,
					   gboolean locked,
					   GError **error)
{
	AnjutaApp *app;
	GtkWidget *item;
	GtkCheckMenuItem* menuitem;

	g_return_if_fail (ANJUTA_IS_APP (shell));
	g_return_if_fail (GTK_IS_WIDGET (widget));
	g_return_if_fail (name != NULL);
	g_return_if_fail (title != NULL);

	app = ANJUTA_APP (shell);

	/*
	anjuta_shell_add (shell, name, G_TYPE_FROM_INSTANCE (widget),
					  widget, NULL);
	*/

	/* Add the widget to hash */
	if (app->widgets == NULL)
	{
		app->widgets = g_hash_table_new_full (g_str_hash, g_str_equal,
											  g_free,
											  on_widget_removed_from_hash);
	}
	g_hash_table_insert (app->widgets, g_strdup (name), widget);
	g_object_ref (widget);

	/* Add the widget to dock */
	if (stock_id == NULL)
		item = gdl_dock_item_new (name, title, GDL_DOCK_ITEM_BEH_NORMAL);
	else
		item = gdl_dock_item_new_with_stock (name, title, stock_id,
											 GDL_DOCK_ITEM_BEH_NORMAL);
	if (locked)
	{
		guint flags = 0;
		flags |= GDL_DOCK_ITEM_BEH_NEVER_FLOATING;
		flags |= GDL_DOCK_ITEM_BEH_CANT_CLOSE;
		flags |= GDL_DOCK_ITEM_BEH_CANT_ICONIFY;
		flags |= GDL_DOCK_ITEM_BEH_NO_GRIP;
		g_object_set(G_OBJECT(item), "behavior", flags, NULL);
	}

	gtk_container_add (GTK_CONTAINER (item), widget);
    gdl_dock_add_item (GDL_DOCK (app->dock),
                       GDL_DOCK_ITEM (item), placement);

	if (locked)
		gdl_dock_item_set_default_position(GDL_DOCK_ITEM(item), GDL_DOCK_OBJECT(app->dock));

	gtk_widget_show_all (item);

	/* Add toggle button for the widget */
	menuitem = GTK_CHECK_MENU_ITEM (gtk_check_menu_item_new_with_label (title));
	gtk_widget_show (GTK_WIDGET (menuitem));
	gtk_check_menu_item_set_active (menuitem, TRUE);
	gtk_menu_append (GTK_MENU (app->view_menu), GTK_WIDGET (menuitem));

	if (locked)
		g_object_set( G_OBJECT(menuitem), "visible", FALSE, NULL);


	g_object_set_data (G_OBJECT (widget), "app-object", app);
	g_object_set_data (G_OBJECT (widget), "menuitem", menuitem);
	g_object_set_data (G_OBJECT (widget), "dockitem", item);

	/* For toggling widget view on/off */
	g_signal_connect (G_OBJECT (menuitem), "toggled",
					  G_CALLBACK (on_toggle_widget_view), item);

	/*
	  Watch for widget removal/destruction so that it could be
	  removed from widgets hash.
	*/
	g_signal_connect (G_OBJECT (item), "remove",
					  G_CALLBACK (on_widget_remove), app);
	g_signal_connect_after (G_OBJECT (widget), "destroy",
					  G_CALLBACK (on_widget_destroy), app);
}

static void
anjuta_app_remove_widget (AnjutaShell *shell, GtkWidget *widget,
						  GError **error)
{
	AnjutaApp *app;
	GtkWidget *dock_item;

	g_return_if_fail (ANJUTA_IS_APP (shell));
	g_return_if_fail (GTK_IS_WIDGET (widget));

	app = ANJUTA_APP (shell);

	g_return_if_fail (app->widgets != NULL);

	dock_item = g_object_get_data (G_OBJECT (widget), "dockitem");
	g_return_if_fail (dock_item != NULL);

	/* Remove the widget from container */
	g_object_ref (widget);
	/* It should call on_widget_remove() and clean up should happen */
	gtk_container_remove (GTK_CONTAINER (dock_item), widget);
	g_object_unref (widget);
}

static void
anjuta_app_present_widget (AnjutaShell *shell, GtkWidget *widget,
						   GError **error)
{
	AnjutaApp *app;
	GdlDockItem *dock_item;
	GtkWidget *parent;

	g_return_if_fail (ANJUTA_IS_APP (shell));
	g_return_if_fail (GTK_IS_WIDGET (widget));

	app = ANJUTA_APP (shell);

	g_return_if_fail (app->widgets != NULL);

	dock_item = g_object_get_data (G_OBJECT(widget), "dockitem");
	g_return_if_fail (dock_item != NULL);

	/* Hack to present the dock item if it's in a notebook dock item */
	parent = gtk_widget_get_parent (GTK_WIDGET(dock_item) );
	if (GTK_IS_NOTEBOOK (parent))
	{
		gint pagenum;
		pagenum = gtk_notebook_page_num (GTK_NOTEBOOK (parent), GTK_WIDGET (dock_item));
		gtk_notebook_set_current_page (GTK_NOTEBOOK (parent), pagenum);
	}
	else if (!GDL_DOCK_OBJECT_ATTACHED (dock_item))
	{
	    gdl_dock_item_show_item (GDL_DOCK_ITEM (dock_item));
	}

	/* FIXME: If the item is floating, present the window */
	/* FIXME: There is no way to detect if a widget was floating before it was
	detached since it no longer has a parent there is no way to access the
	floating property of the GdlDock structure.*/
}

static GObject*
anjuta_app_get_object  (AnjutaShell *shell, const char *iface_name,
					    GError **error)
{
	g_return_val_if_fail (ANJUTA_IS_APP (shell), NULL);
	g_return_val_if_fail (iface_name != NULL, NULL);
	return anjuta_plugin_manager_get_plugin (ANJUTA_APP (shell)->plugin_manager,
											 iface_name);
}

static AnjutaStatus*
anjuta_app_get_status (AnjutaShell *shell, GError **error)
{
	g_return_val_if_fail (ANJUTA_IS_APP (shell), NULL);
	return ANJUTA_APP (shell)->status;
}

static AnjutaUI *
anjuta_app_get_ui  (AnjutaShell *shell, GError **error)
{
	g_return_val_if_fail (ANJUTA_IS_APP (shell), NULL);
	return ANJUTA_APP (shell)->ui;
}

static AnjutaPreferences *
anjuta_app_get_preferences  (AnjutaShell *shell, GError **error)
{
	g_return_val_if_fail (ANJUTA_IS_APP (shell), NULL);
	return ANJUTA_APP (shell)->preferences;
}

static AnjutaPluginManager *
anjuta_app_get_plugin_manager  (AnjutaShell *shell, GError **error)
{
	g_return_val_if_fail (ANJUTA_IS_APP (shell), NULL);
	return ANJUTA_APP (shell)->plugin_manager;
}

static AnjutaProfileManager *
anjuta_app_get_profile_manager (AnjutaShell *shell, GError **error)
{
	g_return_val_if_fail (ANJUTA_IS_APP (shell), NULL);
	return ANJUTA_APP (shell)->profile_manager;
}

static void
anjuta_shell_iface_init (AnjutaShellIface *iface)
{
	iface->add_widget_full = anjuta_app_add_widget_full;
	iface->remove_widget = anjuta_app_remove_widget;
	iface->present_widget = anjuta_app_present_widget;
	iface->add_value = anjuta_app_add_value;
	iface->get_value = anjuta_app_get_value;
	iface->remove_value = anjuta_app_remove_value;
	iface->get_object = anjuta_app_get_object;
	iface->get_status = anjuta_app_get_status;
	iface->get_ui = anjuta_app_get_ui;
	iface->get_preferences = anjuta_app_get_preferences;
	iface->get_plugin_manager = anjuta_app_get_plugin_manager;
	iface->get_profile_manager = anjuta_app_get_profile_manager;
	iface->saving_push = anjuta_app_saving_push;
	iface->saving_pop = anjuta_app_saving_pop;
}

ANJUTA_TYPE_BEGIN(AnjutaApp, anjuta_app, GTK_TYPE_WINDOW);
ANJUTA_TYPE_ADD_INTERFACE(anjuta_shell, ANJUTA_TYPE_SHELL);
ANJUTA_TYPE_END;
