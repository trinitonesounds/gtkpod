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

#ifndef _ANJUTA_APP_H_
#define _ANJUTA_APP_H_

#include <gmodule.h>
#include <gdl/gdl-dock-layout.h>
#include <libanjuta/anjuta-status.h>
#include <libanjuta/anjuta-ui.h>
#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/anjuta-plugin-manager.h>
#include <libanjuta/anjuta-profile-manager.h>
#include "libgtkpod/gtkpod_app_iface.h"

G_BEGIN_DECLS

#define ANJUTA_TYPE_APP        (anjuta_app_get_type ())
#define ANJUTA_APP(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_APP, AnjutaApp))
#define ANJUTA_APP_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), ANJUTA_TYPE_APP, AnjutaAppClass))
#define ANJUTA_IS_APP(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_APP))
#define ANJUTA_IS_APP_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_APP))

typedef struct _AnjutaApp AnjutaApp;
typedef struct _AnjutaAppClass AnjutaAppClass;

struct _AnjutaApp
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

struct _AnjutaAppClass
{
	GtkWindowClass klass;
};

GType      anjuta_app_get_type (void);
GtkWidget* anjuta_app_new (void);

void       anjuta_app_set_geometry (AnjutaApp *app, const gchar *geometry);
gchar*     anjuta_app_get_geometry (AnjutaApp *app);
void       anjuta_app_layout_reset (AnjutaApp *app);
void	   anjuta_app_install_preferences (AnjutaApp *app);
void anjuta_set_ui_file_path (gchar * path);

G_END_DECLS

#endif
