/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/* 
 * mainmenu_callbacks.h
 * Copyright (C) 2003 Naba Kumar  <naba@gnome.org>
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along with 
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#ifndef _MAINMENU_CALLBACKS_H_
#define _MAINMENU_CALLBACKS_H_

#include <gnome.h>

void on_exit1_activate (GtkAction * action, AnjutaApp *app);
void on_fullscreen_toggle (GtkAction *action, AnjutaApp *app);
void on_layout_lock_toggle (GtkAction *action, AnjutaApp *app);
void on_reset_layout_activate (GtkAction *action, AnjutaApp *app);
void on_toolbar_view_toggled (GtkAction *action,  AnjutaApp *app);
void on_set_preferences1_activate (GtkAction * action, AnjutaApp *app);

/* Help actions */
void on_help_manual_activate (GtkAction *action, gpointer data);
void on_help_tutorial_activate (GtkAction *action, gpointer data);
void on_help_advanced_tutorial_activate (GtkAction *action, gpointer data);
void on_help_faqs_activate (GtkAction *action, gpointer data);

void on_url_home_activate (GtkAction * action, gpointer user_data);
void on_url_bugs_activate (GtkAction * action, gpointer user_data);
void on_url_faqs_activate (GtkAction * action, gpointer user_data);
void on_url_activate (GtkAction * action, gpointer url);
void on_about_activate (GtkAction * action, gpointer user_data);

#endif
