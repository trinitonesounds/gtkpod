/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-app.h
 * Copyright (C) 2003 Naba Kumar  <naba@gnome.org>
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
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _ANJUTA_WINDOW_H_
#define _ANJUTA_WINDOW_H_

#include <gmodule.h>
#include <gdl/gdl-dock-layout.h>
#include <libanjuta/anjuta-status.h>
#include <libanjuta/anjuta-ui.h>
#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/anjuta-plugin-manager.h>
#include <libanjuta/anjuta-profile-manager.h>
#include "libgtkpod/gtkpod_app_iface.h"

G_BEGIN_DECLS

#define ANJUTA_TYPE_WINDOW        (anjuta_window_get_type ())
#define ANJUTA_WINDOW(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_WINDOW, AnjutaWindow))
#define ANJUTA_WINDOW_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), ANJUTA_TYPE_WINDOW, AnjutaWindowClass))
#define ANJUTA_IS_WINDOW(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_WINDOW))
#define ANJUTA_IS_WINDOW_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_WINDOW))

typedef struct _AnjutaWindow AnjutaWindow;
typedef struct _AnjutaWindowClass AnjutaWindowClass;

struct _AnjutaWindow
{
	GtkWindow parent;
	GtkWidget *toolbar;
	GtkWidget *menubar;
	GtkWidget *view_menu;
	GtkWidget *dock;
 	GdlDockLayout *layout_manager;

	GHashTable *values;
	GHashTable *widgets;
	gboolean maximized;

	GtkAccelGroup *accel_group;

	AnjutaStatus *status;
	AnjutaUI *ui;
	AnjutaPreferences *preferences;
	GSettings* settings;
	AnjutaPluginManager *plugin_manager;
	AnjutaProfileManager *profile_manager;

	gint save_count;
};

struct _AnjutaWindowClass
{
	GtkWindowClass klass;
};

GType      anjuta_window_get_type (void);
GtkWidget* anjuta_window_new (void);

void       anjuta_window_set_geometry (AnjutaWindow *app, const gchar *geometry);
gchar*     anjuta_window_get_geometry (AnjutaWindow *app);
void       anjuta_window_layout_reset (AnjutaWindow *app);
void	   anjuta_window_install_preferences (AnjutaWindow *app);
void anjuta_set_ui_file_path (gchar * path);

G_END_DECLS

#endif
