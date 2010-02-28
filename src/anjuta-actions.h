/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta.c
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "anjuta-action-callbacks.h"

static GtkActionEntry menu_entries_music[] = {
  {
      "ActionMenuMusic",
      NULL,
      N_("_Music")
  },
  {
      "ActionExit",
      GTK_STOCK_QUIT,
      N_("_Quit"),
      "<control>q",
      N_("Quit GtkPod"),
      G_CALLBACK (on_exit1_activate)}
  };

static GtkActionEntry menu_entries_edit[] = {
    {
        "ActionMenuEdit",
        NULL,
        N_("_Edit")
    },
    {
        "ActionEditDeleteMenu",
        GTK_STOCK_DELETE,
        N_("_Delete"),
        NULL,
        NULL,
        NULL
    },
    {
        "ActionEditPreferences",
        GTK_STOCK_PROPERTIES,
        N_("_Preferences"), NULL,
        N_("Do you prefer coffee to tea? Check it out."),
        G_CALLBACK (on_set_preferences1_activate)
    },
};

static GtkActionEntry menu_entries_view[] = {
  { "ActionMenuView", NULL, N_("_View")},
  { "ActionViewResetLayout", NULL,
	N_("_Reset Dock Layout"), NULL,
	N_("Reset the widgets docking layout to default"),
    G_CALLBACK (on_reset_layout_activate)}
};

static GtkToggleActionEntry menu_entries_toggle_view[] = {
  { "ActionViewFullscreen", GTK_STOCK_FULLSCREEN,
    N_("_Full Screen"), "F11",
    N_("Toggle fullscreen mode"),
	G_CALLBACK (on_fullscreen_toggle)},
  { "ActionViewLockLayout", NULL,
    N_("_Lock Dock Layout"), NULL,
    N_("Lock the current dock layout so that widgets cannot be moved"),
	G_CALLBACK (on_layout_lock_toggle)},
  { "ActionViewToolbar", NULL,
	N_("_Toolbar"), NULL,
    N_("Show or hide the toolbar"),
    G_CALLBACK (on_toolbar_view_toggled)}
};

static GtkActionEntry menu_entries_tools[] = {
  {
      "ActionMenuTools",
      NULL,
      N_("_Tools")
  }
};

static GtkActionEntry menu_entries_help[] = {
  { "ActionMenuHelp", NULL, N_("_Help")},
  { "ActionHelpUserManual", GTK_STOCK_HELP,
    N_("_User's Manual"), "F1",
	N_("Anjuta user's manual"),
    G_CALLBACK (on_help_manual_activate)},
  { "ActionHelpTutorial", NULL,
    N_("Kick start _tutorial"), NULL,
	N_("Anjuta Kick start tutorial"),
    G_CALLBACK (on_help_tutorial_activate)},
  { "ActionHelpAdvancedTutorial", NULL,
    N_("_Advanced tutorial"), NULL,
	N_("Anjuta advanced tutorial"),
    G_CALLBACK (on_help_advanced_tutorial_activate)},
  { "ActionHelpFaqManual", NULL,
    N_("_Frequently Asked Questions"), NULL,
	N_("Anjuta frequently asked questions"),
    G_CALLBACK (on_help_faqs_activate)},
  { "ActionHelpAnjutaHome", GTK_STOCK_HOME,
    N_("Anjuta _Home Page"), NULL,
	N_("Online documentation and resources"),
    G_CALLBACK (on_url_home_activate)},
  { "ActionHelpBugReport", NULL,
    N_("Report _Bugs/Patches/Requests"), NULL,
	N_("Submit a bug report, patch or feature request for Anjuta"),
    G_CALLBACK (on_url_bugs_activate)},
  { "ActionHelpFaq", NULL,
    N_("Ask a _Question"), NULL,
	N_("Submit a question for FAQs"),
    G_CALLBACK (on_url_faqs_activate)},
  { "ActionAboutAnjuta", GTK_STOCK_ABOUT,
    N_("_About"), NULL,
	N_("About Anjuta"),
    G_CALLBACK (on_about_activate)},
  { "ActionAboutPlugins", GTK_STOCK_ABOUT,
    N_("About External _Plugins"), NULL,
	N_("About third party Anjuta plugins"),
    NULL}
};
