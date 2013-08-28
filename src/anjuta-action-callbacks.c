/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
  * mainmenu_callbacks.c
  * Copyright (C) 2003  Naba Kumar  <naba@gnome.org>
  *
  * This program is free software; you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation; either version 2 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program; if not, write to the Free Software
  * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
  */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sched.h>
#include <sys/wait.h>
#include <errno.h>

#include <libanjuta/anjuta-utils.h>
#include <libanjuta/resources.h>
#include <libanjuta/interfaces/ianjuta-preferences.h>
#include <libanjuta/anjuta-version.h>

#include "anjuta-window.h"
#include "anjuta-about.h"
#include "anjuta-action-callbacks.h"
#include "../libgtkpod/directories.h"

#define TOOLBAR_VISIBLE "toolbar-visible"

void
on_exit1_activate (GtkAction * action, AnjutaWindow *win)
{
	GdkEvent *event = gdk_event_new (GDK_DELETE);

	event->any.window = g_object_ref (gtk_widget_get_window (GTK_WIDGET(win)));
	event->any.send_event = TRUE;

	gtk_main_do_event (event);
	gdk_event_free (event);
}

void
on_fullscreen_toggle (GtkAction *action, AnjutaWindow *win)
{
	if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
		gtk_window_fullscreen (GTK_WINDOW(win));
	else
		gtk_window_unfullscreen (GTK_WINDOW(win));
}

void
on_layout_lock_toggle (GtkAction *action, AnjutaWindow *win)
{
	if (win->layout_manager)
#if (ANJUTA_CHECK_VERSION(3, 6, 2))
	    g_object_set (gdl_dock_layout_get_master (win->layout_manager), "locked",
#else
		g_object_set (win->layout_manager->master, "locked",
#endif
					  gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)),
					  NULL);
}

void
on_reset_layout_activate(GtkAction *action, AnjutaWindow *win)
{
	anjuta_window_layout_reset (win);
}

void
on_toolbar_view_toggled (GtkAction *action, AnjutaWindow *win)
{
	gboolean status = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action));
	if (status)
	{
		gtk_widget_show (win->toolbar);
	}
	else
	{
		gtk_widget_hide (win->toolbar);
	}
	g_settings_set_boolean (win->settings,
	                            TOOLBAR_VISIBLE,
	                            status);
}

void
on_set_preferences1_activate (GtkAction * action, AnjutaWindow *win)
{

	GtkWidget *preferences_dialog;

	if (anjuta_preferences_is_dialog_created (win->preferences))
	{
		gtk_window_present (GTK_WINDOW (anjuta_preferences_get_dialog (win->preferences)));
		return;
	}
	preferences_dialog = anjuta_preferences_get_dialog (win->preferences);
	gtk_window_set_title (GTK_WINDOW (preferences_dialog), _("GtkPod Preferences"));

	/* Install main application preferences */
	anjuta_window_install_preferences (win);

	g_signal_connect_swapped (G_OBJECT (preferences_dialog),
					  		  "response",
					  		  G_CALLBACK (gtk_widget_destroy),
					  		  preferences_dialog);

	gtk_window_set_transient_for (GTK_WINDOW (preferences_dialog),
								  GTK_WINDOW (win));

	gtk_widget_show (preferences_dialog);
}

void
on_help_manual_activate (GtkAction *action, gpointer data) {
    gchar *helpurl = g_build_filename("file://", get_doc_dir(), "gtkpod.html", NULL);
    anjuta_res_url_show(helpurl);
    g_free(helpurl);
}

void
on_url_home_activate (GtkAction * action, gpointer user_data)
{
    anjuta_res_url_show("http://www.gtkpod.org");
}

void
on_url_bugs_activate (GtkAction * action, gpointer user_data)
{
    anjuta_res_url_show("http://gtkpod.org/bugs/");
}

void
on_url_faqs_activate (GtkAction * action, gpointer user_data)
{
    anjuta_res_url_show("mailto:gtkpod-questions@lists.sourceforge.net");
}

void
on_about_activate (GtkAction * action, AnjutaWindow *win)
{
	GtkWidget *about_dlg = about_box_new (GTK_WINDOW(win));

	g_signal_connect_swapped(about_dlg, "response",
		G_CALLBACK(gtk_widget_destroy), about_dlg);

	gtk_widget_show (about_dlg);
}
