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
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include "directories.h"

static int USING_LOCAL = 0;

static gchar * init_dir(char *argv[], gchar *filename, gchar *installdir);
static void debug_print_directories();

static gchar *datadir = NULL;
static gchar *icondir = NULL;
static gchar *plugindir = NULL;
static gchar *uidir = NULL;

void init_directories(char *argv[]) {
//    g_printf("argv[0] = %s\n", argv[0]);
    datadir = init_dir(argv, "data", GTKPOD_DATA_DIR);
    icondir = init_dir(argv, "data/icons", GTKPOD_IMAGE_DIR);
    uidir = init_dir(argv, "data/ui", GTKPOD_UI_DIR);
    plugindir = init_dir(argv, "plugins", GTKPOD_PLUGIN_DIR);

    gtk_icon_theme_append_search_path(gtk_icon_theme_get_default(), icondir);
    gtk_icon_theme_append_search_path(gtk_icon_theme_get_default(), GTKPOD_IMAGE_DIR);

//    debug_print_directories();
}

static void debug_print_directories() {
    g_printf("data directory: %s\n", get_data_dir());
    g_printf("ui directory: %s\n", get_ui_dir());
    g_printf("glade directory: %s\n", get_glade_dir());
    g_printf("icon directory: %s\n", get_icon_dir());
    g_printf("plugin directory: %s\n", get_plugin_dir());
}

static gchar * init_dir(char *argv[], gchar *localdir, gchar *fullinstalldir) {
    gchar *progname;
    gchar *newdir;

    progname = g_find_program_in_path(argv[0]);
    if (progname) {
        static const gchar *gtkpodSEPsrcSEP = "gtkpod" G_DIR_SEPARATOR_S "src" G_DIR_SEPARATOR_S;

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

        gchar *substr = g_strrstr(progname, gtkpodSEPsrcSEP);
        if (substr) {
            gchar** tokens = g_strsplit(progname, substr, -1);
            newdir = g_build_filename(tokens[0], "gtkpod", localdir, NULL);
            g_strfreev(tokens);
        }
        g_free(progname);

        if (newdir && !g_file_test(newdir, G_FILE_TEST_EXISTS)) {
            g_free(newdir);
            newdir = NULL;
        }
    }

    if (!newdir)
        newdir = fullinstalldir;
    else {
        USING_LOCAL = 1;
        g_printf("Using local %s file since program was started from source directory:\n%s\n", localdir, newdir);
    }

    return newdir;
}

gchar * get_data_dir() {
    return datadir;
}

gchar * get_glade_dir() {
    return datadir;
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
