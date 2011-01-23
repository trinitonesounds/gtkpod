/*
 |  Copyright (C) 2002-2005 Jorg Schuler <jcsjcs at users.sourceforge.net>
 |  Part of the gtkpod project.
 |
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

/***********************************************************************
 * Functions for file chooser dialogs provided by:
 *
 *  Fri May 27 22:13:20 2005
 *  Copyright  2005  James Liggett
 *  Email jrliggett@cox.net
 ***********************************************************************/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <glib/gi18n-lib.h>
#include "charset.h"
#include "file.h"
#include "prefs.h"
#include "misc.h"
#include "misc_track.h"
#include "fileselection.h"

/*
 * Add Cover Art
 */
gchar *fileselection_get_cover_filename(void) {
    GtkWidget* fc; /* The file chooser dialog */
    gint response; /* The response of the filechooser */
    gchar *filename = NULL; /* The chosen file */
    gchar *last_dir, *new_dir;

    /* Create the file chooser, and handle the response */
    fc
            = gtk_file_chooser_dialog_new(_("Set Cover"), GTK_WINDOW (gtkpod_app), GTK_FILE_CHOOSER_ACTION_OPEN,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER (fc), FALSE);

    last_dir = prefs_get_string("last_dir_browsed");
    if (last_dir) {
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER (fc), last_dir);
        g_free(last_dir);
    }

    response = gtk_dialog_run(GTK_DIALOG(fc));

    switch (response) {
    case GTK_RESPONSE_ACCEPT:
        new_dir = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER (fc));
        prefs_set_string("last_dir_browsed", new_dir);
        g_free(new_dir);
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (fc));
        break;
    case GTK_RESPONSE_CANCEL:
        break;
    default: /* Fall through */
        break;
    }
    gtk_widget_destroy(fc);
    return filename;
}

/* BY JCS */

/* Get a file or directory
 *
 * @title:    title for the file selection dialog
 * @cur_file: initial file to be selected. If NULL, then use
 *            last_dir_browse.
 * @action:
 *   GTK_FILE_CHOOSER_ACTION_OPEN   Indicates open mode. The file chooser
 *                                  will only let the user pick an
 *                                  existing file.
 *   GTK_FILE_CHOOSER_ACTION_SAVE   Indicates save mode. The file
 *                                  chooser will let the user pick an
 *                                  existing file, or type in a new
 *                                  filename.
 *   GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER
 *                                  Indicates an Open mode for
 *                                  selecting folders. The file
 *                                  chooser will let the user pick an
 *                                  existing folder.
 *   GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER
 *                                  Indicates a mode for creating a
 *                                  new folder. The file chooser will
 *                                  let the user name an existing or
 *                                  new folder.
 */
gchar *fileselection_get_file_or_dir(const gchar *title, const gchar *cur_file, GtkFileChooserAction action) {
    GtkWidget* fc; /* The file chooser dialog */
    gint response; /* The response of the filechooser */
    gchar *new_file = NULL; /* The chosen file */
    gchar *new_dir; /* The new dir to remember */

    g_return_val_if_fail (title, NULL);

    /* Create the file chooser, and handle the response */
    fc
            = gtk_file_chooser_dialog_new(title, GTK_WINDOW (gtkpod_app), action,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);

    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER (fc), FALSE);

    if (cur_file) {
        /* Sanity checks: must exist and be absolute */
        if (g_path_is_absolute(cur_file))
            gtk_file_chooser_set_filename(GTK_FILE_CHOOSER (fc), cur_file);
        else
            cur_file = NULL;
    }
    if (cur_file == NULL) {
        gchar *filename = prefs_get_string("last_dir_browsed");
        if (filename) {
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER (fc), filename);
            g_free(filename);
        }
    }

    response = gtk_dialog_run(GTK_DIALOG(fc));

    switch (response) {
    case GTK_RESPONSE_ACCEPT:
        new_dir = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER (fc));
        prefs_set_string("last_dir_browsed", new_dir);
        g_free(new_dir);
        new_file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (fc));
        break;
    case GTK_RESPONSE_CANCEL:
        break;
    default: /* Fall through */
        break;
    }
    gtk_widget_destroy(fc);
    return new_file;
}


