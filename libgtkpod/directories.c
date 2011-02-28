/* -*- coding: utf-8; -*-
 |
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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
#include "directories.h"

static gboolean USING_LOCAL = FALSE;

static gchar * init_dir(char *argv[], gchar *filename, gchar *installdir);
#if LOCALDEBUG
static void debug_print_directories();
#endif

static gchar *datadir = NULL;
static gchar *docdir = NULL;
static gchar *icondir = NULL;
static gchar *plugindir = NULL;
static gchar *uidir = NULL;
static gchar *gladedir = NULL;
static gchar *scriptdir = NULL;

void init_directories(char *argv[]) {
//    g_printf("argv[0] = %s\n", argv[0]);
    datadir = init_dir(argv, "data", GTKPOD_DATA_DIR);
    docdir = init_dir(argv, "doc", GTKPOD_DOC_DIR);
    icondir = init_dir(argv, "icons", GTKPOD_IMAGE_DIR);
    uidir = init_dir(argv, "data/ui", GTKPOD_UI_DIR);
    gladedir = init_dir(argv, "data/glade", GTKPOD_GLADE_DIR);
    plugindir = init_dir(argv, "plugins", GTKPOD_PLUGIN_DIR);
    scriptdir = init_dir(argv, "scripts", GTKPOD_SCRIPT_DIR);

    gtk_icon_theme_append_search_path(gtk_icon_theme_get_default(), icondir);
    gtk_icon_theme_append_search_path(gtk_icon_theme_get_default(), GTKPOD_IMAGE_DIR);

//    debug_print_directories();
}

#if LOCALDEBUG
static void debug_print_directories() {
    g_printf("data directory: %s\n", get_data_dir());
    g_printf("doc directory: %s\n", get_doc_dir());
    g_printf("ui directory: %s\n", get_ui_dir());
    g_printf("glade directory: %s\n", get_glade_dir());
    g_printf("icon directory: %s\n", get_icon_dir());
    g_printf("plugin directory: %s\n", get_plugin_dir());
    g_printf("script directory: %s\n", get_script_dir());
}
#endif

static gchar * init_dir(char *argv[], gchar *localdir, gchar *fullinstalldir) {
    gchar *progname;
    gchar *newdir = NULL;
    GRegex *regex;
    GMatchInfo *match_info;

    progname = g_find_program_in_path(argv[0]);
    if (progname) {
        if (!g_path_is_absolute(progname)) {
            gchar *cur_dir = g_get_current_dir();
            gchar *prog_absolute;

            if (g_str_has_prefix(progname, "." G_DIR_SEPARATOR_S))
                prog_absolute = g_build_filename(cur_dir, progname + 2, NULL);
            else
                prog_absolute = g_build_filename(cur_dir, progname, NULL);
            g_free(progname);
            g_free(cur_dir);
            progname = prog_absolute;
        }

        static const gchar *gtkpodSEPsrcSEP = "(gtkpod[\x20-\x7E]*)([\\\\/]src[\\\\/])";

        regex = g_regex_new (gtkpodSEPsrcSEP, 0, 0, NULL);
        if (g_regex_match (regex, progname, 0, &match_info)) {
            /* Find the gtkpod* parathesis pattern */
            gchar *gtkpoddir = g_match_info_fetch (match_info, 1);

            /* Get the base directory by splitting the regex on the pattern */
            gchar **tokens = g_regex_split (regex, progname, 0);
            newdir = g_build_filename(tokens[0], gtkpoddir, localdir, NULL);
            g_free(gtkpoddir);
            g_strfreev(tokens);
        }

        g_match_info_free (match_info);
        g_regex_unref (regex);
        g_free(progname);

        if (newdir && !g_file_test(newdir, G_FILE_TEST_EXISTS)) {
            g_free(newdir);
            newdir = NULL;
        }
    }

    if (!newdir)
        newdir = fullinstalldir;
    else {
        USING_LOCAL = TRUE;
        g_printf(_("Using local %s directory since program was started from source directory:\n%s\n"), localdir, newdir);
    }

    return newdir;
}

gboolean using_local() {
    return USING_LOCAL;
}

gchar * get_data_dir() {
    return datadir;
}

gchar * get_doc_dir() {
    return docdir;
}

gchar * get_glade_dir() {
    return gladedir;
}

gchar * get_icon_dir() {
    return icondir;
}

gchar * get_image_dir() {
    return icondir;
}

gchar * get_ui_dir() {
    return uidir;
}

gchar * get_plugin_dir() {
    return plugindir;
}

gchar * get_script_dir() {
    return scriptdir;
}
