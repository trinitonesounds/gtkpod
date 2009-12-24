/*
 |  Copyright (C) 2002-2010 Jorg Schuler <jcsjcs at users sourceforge net>
 |                                          Paul Richardson <phantom_sf at users.sourceforge.net>
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

#include "playlist_display_actions.h"
#include "libgtkpod/file.h"
#include "libgtkpod/gtkpod_app_iface.h"
#include "libgtkpod/gp_itdb.h"
#include "libgtkpod/prefs.h"
#include "libgtkpod/misc_track.h"

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

/* OK Button */
static void fileselection_add_playlists(GSList* names, iTunesDB *itdb) {
    GSList* gsl;

    /* Get the names of the playlist(s) and add them */

    g_return_if_fail (itdb);

    g_warning("fileselection_add_playlists - block widgets commented out");
//    block_widgets();

    for (gsl = names; gsl; gsl = gsl->next) {
        add_playlist_by_filename(itdb, gsl->data, NULL, -1, NULL, NULL);
    }

    g_warning("fileselection_add_playlists - release widgets commented out");
//    release_widgets();

    /* clear log of non-updated tracks */
    display_non_updated((void *) -1, NULL);

    /* display log of updated tracks */
    display_updated(NULL, NULL);

    /* display log of detected duplicates */
    gp_duplicate_remove(NULL, NULL);

    gtkpod_tracks_statusbar_update();
}

/* Open a modal file selection dialog with multiple selction enabled */
static GSList* fileselection_get_files(const gchar *title) {
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

/* Open a modal file selection dialog for adding playlist files */
static void create_add_playlists_dialog(iTunesDB *itdb) {
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

static void fileselection_add_files(GSList* names, Playlist *playlist) {
    GSList* gsl; /* Current node in list */
    gboolean result = TRUE; /* Result of file adding */

    /* If we don't have a playlist to add to, don't add anything */
    g_return_if_fail (playlist);

    g_warning("fileselection_add_files - block widgets commented out");
//    block_widgets();

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

    g_warning("fileselection_add_playlists - release widgets commented out");
//    release_widgets();
}

/* Open a modal file selection dialog for adding individual files */
static void create_add_files_dialog(Playlist *pl) {
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

void on_load_ipods_mi(GtkAction* action, PlaylistDisplayPlugin* plugin) {
    gp_load_ipods();
}

void on_save_changes(GtkAction *action, PlaylistDisplayPlugin* plugin) {
    handle_export();
}

void on_create_add_files(GtkAction *action, PlaylistDisplayPlugin* plugin) {
    Playlist *pl;
    pl = gtkpod_get_current_playlist();
    create_add_files_dialog(pl);
}

void on_create_add_directory(GtkAction *action, PlaylistDisplayPlugin* plugin) {
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

void on_create_add_playlists(GtkAction *action, PlaylistDisplayPlugin* plugin) {
    iTunesDB *itdb;
    itdb = gp_get_selected_itdb();
    create_add_playlists_dialog(itdb);
}
