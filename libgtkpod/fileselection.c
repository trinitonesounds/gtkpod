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

/* Open a modal file selection dialog with multiple selction enabled */
GSList* fileselection_get_files(const gchar *title) {
    GtkWidget* fc; /* The file chooser dialog */
    gint response; /* The response of the filechooser */
    gchar *last_dir, *new_dir;
    GSList * files = NULL;

    fc
            = gtk_file_chooser_dialog_new(title, NULL, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

    /* allow multiple selection of files */
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER (fc), TRUE);

    /* set same directory as last time */
    last_dir = prefs_get_string("last_dir_browsed");
    if (last_dir) {
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER (fc), last_dir);
        g_free(last_dir);
    }

    /* Run the dialog */
    response = gtk_dialog_run(GTK_DIALOG(fc));

    /* Handle the response */
    switch (response) {
    case GTK_RESPONSE_ACCEPT:
        new_dir = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER (fc));
        prefs_set_string("last_dir_browsed", new_dir);
        g_free(new_dir);
        files = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER (fc));
        break;
    case GTK_RESPONSE_CANCEL:
        break;
    default: /* Fall through */
        break;
    }
    gtk_widget_destroy(fc);

    return files;
}

static void fileselection_add_files(GSList* names, Playlist *playlist) {
    GSList* gsl; /* Current node in list */
    gboolean result = TRUE; /* Result of file adding */

    /* If we don't have a playlist to add to, don't add anything */
    g_return_if_fail (playlist);

    block_widgets();

    /* Get the filenames and add them */
    for (gsl = names; gsl; gsl = gsl->next) {
        result
                &= add_track_by_filename(playlist->itdb, gsl->data, playlist, prefs_get_int("add_recursively"), NULL, NULL);
    }

    /* clear log of non-updated tracks */
    display_non_updated((void *) -1, NULL);

    /* display log of updated tracks */
    display_updated(NULL, NULL);

    /* display log of detected duplicates */
    gp_duplicate_remove(NULL, NULL);

    /* Were all files successfully added? */
    if (result == TRUE)
        gtkpod_statusbar_message(_("Successfully added files"));
    else
        gtkpod_statusbar_message(_("Some files were not added successfully"));

    release_widgets();
}

/*
 * Add Files Dialog
 */
/* ATTENTION: directly used as callback in gtkpod.glade -- if you
 change the arguments of this function make sure you define a
 separate callback for gtkpod.glade */
G_MODULE_EXPORT void create_add_files_callback(void) {
    Playlist *pl;

    pl = gtkpod_get_current_playlist();

    create_add_files_dialog(pl);
}

/* Open a modal file selection dialog for adding individual files */
void create_add_files_dialog(Playlist *pl) {
    gchar *str;
    GSList *names;
    iTunesDB *itdb;
    ExtraiTunesDBData *eitdb;
    Playlist *mpl;

    if (!pl) {
        gtkpod_warning_simple(_("Please select a playlist or repository before adding tracks."));
        return;
    }

    itdb = pl->itdb;
    g_return_if_fail (itdb);
    eitdb = itdb->userdata;
    g_return_if_fail (eitdb);

    if (!eitdb->itdb_imported) {
        gtkpod_warning_simple(_("Please load the iPod before adding tracks."));
        return;
    }

    mpl = itdb_playlist_mpl(itdb);
    g_return_if_fail (mpl);

    /* Create window title */
    if (mpl == pl) {
        str = g_strdup_printf(_("Add files to '%s'"), mpl->name);
    }
    else {
        str = g_strdup_printf(_("Add files to '%s/%s'"), mpl->name, pl->name);
    }

    names = fileselection_get_files(str);
    g_free(str);

    if (!names)
        return;

    fileselection_add_files(names, pl);

    g_slist_foreach(names, (GFunc) g_free, NULL);
    g_slist_free(names);
}

/* OK Button */
static void fileselection_add_playlists(GSList* names, iTunesDB *itdb) {
    GSList* gsl;

    /* Get the names of the playlist(s) and add them */

    g_return_if_fail (itdb);

    block_widgets();

    for (gsl = names; gsl; gsl = gsl->next) {
        add_playlist_by_filename(itdb, gsl->data, NULL, -1, NULL, NULL);
    }

    release_widgets();

    /* clear log of non-updated tracks */
    display_non_updated((void *) -1, NULL);

    /* display log of updated tracks */
    display_updated(NULL, NULL);

    /* display log of detected duplicates */
    gp_duplicate_remove(NULL, NULL);

    gtkpod_tracks_statusbar_update();
}

/*
 * Add Playlist Dialog
 */
/* ATTENTION: directly used as callback in gtkpod.glade -- if you
 change the arguments of this function make sure you define a
 separate callback for gtkpod.glade */
G_MODULE_EXPORT void create_add_playlists_callback(void) {
    iTunesDB *itdb;

    itdb = gp_get_selected_itdb();

    create_add_playlists_dialog(itdb);
}

/* Open a modal file selection dialog for adding playlist files */
void create_add_playlists_dialog(iTunesDB *itdb) {
    gchar *str;
    GSList *names;
    ExtraiTunesDBData *eitdb;
    Playlist *mpl;

    if (!itdb) {
        gtkpod_warning_simple(_("Please select a playlist or repository before adding tracks."));
        return;
    }

    eitdb = itdb->userdata;
    g_return_if_fail (eitdb);

    if (!eitdb->itdb_imported) {
        gtkpod_warning_simple(_("Please load the iPod before adding tracks."));
        return;
    }

    mpl = itdb_playlist_mpl(itdb);
    g_return_if_fail (mpl);

    /* Create window title */
    str = g_strdup_printf(_("Add playlist files to '%s'"), mpl->name);

    names = fileselection_get_files(str);
    g_free(str);

    if (!names)
        return;

    fileselection_add_playlists(names, itdb);

    g_slist_foreach(names, (GFunc) g_free, NULL);
    g_slist_free(names);
}

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
            = gtk_file_chooser_dialog_new(_("Set Cover"), NULL, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

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
            = gtk_file_chooser_dialog_new(title, NULL, action, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);

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

/* BY JCS */

/* Used by the prefs system (prefs_windows.c, repository.c) when a
 * script should be selected. Takes into account that command line
 * arguments can be present
 *
 * @opath: the current path to the script including command line
 *         arguments. May be NULL.
 * @fallback: default dir in case @opath is not set.
 * @title: title of the file selection window.
 * @additional_text: additional explanotary text to be displayed
 *
 * Return value: The new script including command line arguments. NULL
 * if the selection was aborted.
 */
gchar *fileselection_select_script(const gchar *opath, const gchar *fallback, const gchar *title, const gchar *additional_text) {
    gchar *npath = NULL;
    gchar *buf, *fbuf;
    const gchar *opathp;
    GtkFileChooser *fc;
    gint response; /* The response of the filechooser */

    fc = GTK_FILE_CHOOSER (gtk_file_chooser_dialog_new (
                    title,
                    NULL,
                    GTK_FILE_CHOOSER_ACTION_OPEN,
                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                    GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                    NULL));

    /* find first whitespace separating path from command line
     * arguments */

    if (opath)
        opathp = strchr(opath, ' ');
    else
        opathp = NULL;

    if (opathp)
        buf = g_strndup(opath, opathp - opath);
    else
        buf = g_strdup(opath);

    /* get full path -- if the file cannot be found it can't be
     * selected in the filechooser */
    if (buf) {
        fbuf = g_find_program_in_path(buf);
        g_free(buf);
    }
    else {
        fbuf = NULL;
    }

    if (!fbuf) { /* set default */
        fbuf = g_strdup(fallback);
    }

    if (fbuf && *fbuf) {
        gchar *fbuf_utf8 = g_filename_from_utf8(fbuf, -1, NULL, NULL, NULL);
        if (g_file_test(fbuf, G_FILE_TEST_IS_DIR)) {
            gtk_file_chooser_set_current_folder(fc, fbuf_utf8);
        }
        else {
            gtk_file_chooser_set_filename(fc, fbuf_utf8);
        }
        g_free(fbuf_utf8);
    }
    g_free(fbuf);

    response = gtk_dialog_run(GTK_DIALOG(fc));

    switch (response) {
    case GTK_RESPONSE_ACCEPT:
        buf = gtk_file_chooser_get_filename(fc);
        /* attach command line arguments if present */
        if (opathp)
            npath = g_strdup_printf("%s%s", buf, opathp);
        else
            npath = g_strdup(buf);
        g_free(buf);
        break;
    case GTK_RESPONSE_CANCEL:
        break;
    default: /* Fall through */
        break;
    }
    gtk_widget_destroy(GTK_WIDGET (fc));

    return npath;
}

/* Callback after directories to add have been selected */
static void add_selected_dirs(GSList *names, Playlist *db_active_pl) {
    gboolean result = TRUE;

    g_return_if_fail (names);
    g_return_if_fail (db_active_pl);

    if (names) {
        GSList* currentnode;
        for (currentnode = names; currentnode; currentnode = currentnode->next) {
            result
                    &= add_directory_by_name(db_active_pl->itdb, currentnode->data, db_active_pl, prefs_get_int("add_recursively"), NULL, NULL);
            g_free(currentnode->data);
            gtkpod_tracks_statusbar_update();
        }
        g_slist_free(names);
        names = NULL;

        /* clear log of non-updated tracks */
        display_non_updated((void *) -1, NULL);
        /* display log of updated tracks */
        display_updated(NULL, NULL);
        /* display log of detected duplicates */
        gp_duplicate_remove(NULL, NULL);

        if (result == TRUE)
            gtkpod_statusbar_message(_("Successfully added files"));
        else
            gtkpod_statusbar_message(_("Some files were not added successfully"));
    }
}

/* ATTENTION: directly used as callback in gtkpod.glade -- if you
 change the arguments of this function make sure you define a
 separate callback for gtkpod.glade */
G_MODULE_EXPORT void dirbrowser_create_callback(void) {
    GSList* names = NULL; /* List of selected items */
    Playlist *pl = gtkpod_get_current_playlist();
    GtkWidget *dialog;

    if (!pl) {
        gtkpod_warning_simple(_("Please select a playlist or repository before adding tracks."));
        return;
    }

    dialog
            = gtk_file_chooser_dialog_new(_("Add Folder"), GTK_WINDOW (gtkpod_app), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_ADD, GTK_RESPONSE_ACCEPT, NULL);

    /* Allow multiple selection of directories. */
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);

    /* Set same directory as the last browsed directory. */
    gchar *last_dir = prefs_get_string("last_dir_browsed");
    if (last_dir) {
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER (dialog), last_dir);
        g_free(last_dir);
    }

    if (gtk_dialog_run(GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
        names = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER (dialog));

    if (names) {
        add_selected_dirs(names, pl);
        prefs_set_string("last_dir_browsed", gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER (dialog)));
    }

    gtk_widget_destroy(dialog);
}
