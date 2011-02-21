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
 */

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "plugin.h"
#include "repository.h"
#include "libgtkpod/fileselection.h"
#include "libgtkpod/gtkpod_app_iface.h"
#include "libgtkpod/gp_itdb.h"
#include "libgtkpod/misc.h"
#include "libgtkpod/syncdir.h"
#include "libgtkpod/directories.h"

typedef enum {
    IPOD_SYNC_CONTACTS, IPOD_SYNC_CALENDAR, IPOD_SYNC_NOTES
} iPodSyncType;

/* string constants for window widgets used more than once */
#define SYNC_FRAME "sync_frame"
#define PLAYLIST_COMBO "playlist_combo"
#define REPOSITORY_TYPE_LABEL "repository_type_label"
#define REPOSITORY_COMBO "repository_combo"
#define MOUNTPOINT_LABEL "mountpoint_label"
#define MOUNTPOINT_CHOOSER "mountpoint_chooser"
#define BACKUP_LABEL "backup_label"
#define BACKUP_CHOOSER "backup_chooser"
#define IPOD_MODEL_LABEL "ipod_model_label"
#define LOCAL_PATH_LABEL "local_path_label"
#define LOCAL_PATH_CHOOSER "local_path_chooser"
#define STANDARD_PLAYLIST_VBOX "standard_playlist_vbox"
#define SPL_LIVE_UPDATE_TOGGLE "spl_live_update_toggle"
#define SYNC_PLAYLIST_MODE_NONE_RADIO "sync_playlist_mode_none_radio"
#define SYNC_PLAYLIST_MODE_AUTOMATIC_RADIO "sync_playlist_mode_automatic_radio"
#define SYNC_PLAYLIST_MODE_MANUAL_RADIO "sync_playlist_mode_manual_radio"
#define MANUAL_SYNCDIR_CHOOSER "manual_syncdir_chooser"
#define DELETE_REPOSITORY_BUTTON "delete_repository_button"
#define APPLY_BUTTON "apply_button"
#define REPOSITORY_VBOX "repository_vbox"
#define IPOD_SYNC_CONTACTS_ENTRY "ipod_sync_contacts_entry"
#define IPOD_SYNC_CONTACTS_BUTTON "ipod_sync_contacts_button"
#define IPOD_SYNC_CALENDAR_ENTRY "ipod_sync_calendar_entry"
#define IPOD_SYNC_CALENDAR_BUTTON "ipod_sync_calendar_button"
#define IPOD_SYNC_NOTES_ENTRY "ipod_sync_notes_entry"
#define IPOD_SYNC_NOTES_BUTTON "ipod_sync_notes_button"
#define IPOD_CONCAL_AUTOSYNC_TOGGLE "ipod_concal_autosync_toggle"
#define SYNC_OPTIONS_HBOX "sync_options_hbox"
#define PLAYLIST_TYPE_LABEL "playlist_type_label"
#define PLAYLIST_SYNC_DELETE_TRACKS_TOGGLE "playlist_sync_delete_tracks_toggle"
#define PLAYLIST_SYNC_CONFIRM_DELETE_TOGGLE "playlist_sync_confirm_delete_toggle"
#define PLAYLIST_SYNC_SHOW_SUMMARY_TOGGLE "playlist_sync_show_summary_toggle"
#define UPDATE_PLAYLIST_BUTTON "update_playlist_button"
#define UPDATE_ALL_PLAYLISTS_BUTTON "update_all_playlists_button"
#define NEW_REPOSITORY_BUTTON "new_repository_button"
#define GENERAL_FRAME "general_frame"
#define PLAYLIST_TAB_LABEL "playlist_tab_label"
#define PLAYLIST_TAB_CONTENTS "playlist_tab_contents"

/* some key names used several times */
#define KEY_CONCAL_AUTOSYNC "concal_autosync"
#define KEY_SYNC_DELETE_TRACKS "sync_delete_tracks"
#define KEY_SYNC_CONFIRM_DELETE "sync_confirm_delete"
#define KEY_SYNC_CONFIRM_DIRS "sync_confirm_dirs"
#define KEY_SYNC_SHOW_SUMMARY "sync_show_summary"
#define KEY_MOUNTPOINT "mountpoint"
#define KEY_IPOD_MODEL "ipod_model"
#define KEY_FILENAME "filename"
#define KEY_PATH_SYNC_CONTACTS "path_sync_contacts"
#define KEY_PATH_SYNC_CALENDAR "path_sync_calendar"
#define KEY_PATH_SYNC_NOTES "path_sync_notes"
#define KEY_SYNCMODE "syncmode"
#define KEY_MANUAL_SYNCDIR "manual_syncdir"
#define KEY_LIVEUPDATE "liveupdate"

static RepositoryView *repository_view = NULL;

/* Declarations */
static void init_repository_combo();
static void init_playlist_combo();
static void select_repository(iTunesDB *itdb, Playlist *playlist);
static void display_repository_info();
static void display_playlist_info();
static void update_buttons();
static void repository_combo_changed_cb(GtkComboBox *cb);

/* callbacks */
/**
 * Called when an itdb is updated / replaced. Whatever the reason for calling it,
 * both combo boxes should be reinitialised since they would otherwise point to
 * defunct playlists and itdbs.
 */
static void repository_update_itdb_cb(GtkPodApp *app, gpointer olditdb, gpointer newitdb, gpointer data) {
    gint index;

    index = gtk_combo_box_get_active(repository_view->repository_combo_box);

    init_repository_combo();

    if (index >= 0) {
        gtk_combo_box_set_active(repository_view->repository_combo_box, index);
    }
}

/**
 * Called when a playlist is added or removed. Whatever the reason for calling it,
 * both combo boxes should be reinitialised since they would otherwise point to
 * defunct playlists and itdbs.
 */
static void repository_playlist_changed_cb(GtkPodApp *app, gpointer pl, gint32 pos, gpointer data) {
    if (repository_view->itdb == gp_get_selected_itdb()) {
        // repository changed will not be called if the current
        // repository is selected so need to init the playlist combo
        // manually. Need to do this before init repository combo
        // since the latter set the repository_view-> itdb to NULL
        init_playlist_combo();
    }
    init_repository_combo();
}

/**
 * Called when a playlist is selected elsewhere in the UI. This should respond
 * and update the combo boxes with the new playlist selected.
 */
static void repository_playlist_selected_cb(GtkPodApp *app, gpointer pl, gpointer data) {
    Playlist *playlist = pl;
    iTunesDB *itdb = NULL;

    if (!playlist) {
        return;
    }

    itdb = playlist->itdb;

    /* Select the repository of the playlist */
    select_repository(itdb, playlist);
}

/****** New repository was selected */
static void repository_combo_changed_cb(GtkComboBox *cb) {
    struct itdbs_head *itdbs_head;
    iTunesDB *itdb;
    gint index;

#   if LOCAL_DEBUG
    printf ("Repository changed (%p)\n", repwin);
#   endif

    g_return_if_fail (repository_view);

    index = gtk_combo_box_get_active(cb);

    itdbs_head = gp_get_itdbs_head();
    g_return_if_fail (itdbs_head);

    itdb = g_list_nth_data(itdbs_head->itdbs, index);

    if (repository_view->itdb != itdb) {
        repository_view->itdb = itdb;
        repository_view->itdb_index = index;
        display_repository_info();
        init_playlist_combo();
        update_buttons();
    }
}

/****** New playlist was selected */
static void playlist_combo_changed_cb(GtkComboBox *cb) {
    GtkTreeModel *model;
    Playlist *playlist;
    GtkTreeIter iter;
    gint index;

#   if LOCAL_DEBUG
    printf ("Playlist changed (%p)\n", repwin);
#   endif

    g_return_if_fail (repository_view);

    index = gtk_combo_box_get_active(cb);
    /* We can't just use the index to find the right playlist in
     itdb->playlists because they might have been reordered. Instead
     use the playlist pointer stored in the model. */
    model = gtk_combo_box_get_model(cb);
    g_return_if_fail (model);
    g_return_if_fail (gtk_tree_model_iter_nth_child (model, &iter,
                    NULL, index));
    gtk_tree_model_get(model, &iter, 0, &playlist, -1);

    if (repository_view->playlist != playlist) {
        g_return_if_fail (playlist->itdb == repository_view->itdb);
        repository_view->playlist = playlist;
        display_playlist_info();
    }
}

/* ------------------------------------------------------------
 *
 *        Helper functions for prefs interfacing.
 *
 * ------------------------------------------------------------ */

/* Get prefs string -- either from repository_view->temp_prefs or from the main
 prefs system. Return an empty string if no value was set. */
/* Free string after use */
static gchar *get_current_prefs_string(const gchar *key) {
    gchar *value;

    g_return_val_if_fail (repository_view && key, NULL);

    value = temp_prefs_get_string(repository_view->temp_prefs, key);
    if (value == NULL) {
        value = prefs_get_string(key);
    }
    if (value == NULL) {
        value = g_strdup("");
    }
    return value;
}

/* Get integer prefs value -- either from repository_view->temp_prefs or from
 the main prefs system. Return 0 if no value was set. */
/* Free string after use */
static gint get_current_prefs_int(const gchar *key) {
    gint value;

    g_return_val_if_fail (repository_view && key, 0);

    if (!temp_prefs_get_int_value(repository_view->temp_prefs, key, &value)) {
        value = prefs_get_int(key);
    }
    return value;
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

/* Render apply insensitive when no changes were made.
 When an itdb is marked for deletion, make entries insensitive */
static void update_buttons() {
    gboolean apply, ok, deleted;
    gchar *key;

    g_return_if_fail (repository_view);
    g_return_if_fail (repository_view->temp_prefs);
    g_return_if_fail (repository_view->extra_prefs);

    if ((temp_prefs_size(repository_view->temp_prefs) > 0) || (temp_prefs_size(repository_view->extra_prefs) > 0)) {
        apply = TRUE;
        ok = TRUE;
    }
    else {
        apply = FALSE;
        ok = TRUE;
    }

    gtk_widget_set_sensitive(GET_WIDGET (repository_view->xml, APPLY_BUTTON), apply);

    if (repository_view->itdb) {
        gtk_widget_set_sensitive(GET_WIDGET (repository_view->xml, REPOSITORY_VBOX), TRUE);

        /* Check if this itdb is marked for deletion */
        key = get_itdb_prefs_key(repository_view->itdb_index, "deleted");
        deleted = temp_prefs_get_int(repository_view->extra_prefs, key);
        g_free(key);

        gtk_widget_set_sensitive(GET_WIDGET (repository_view->xml, GENERAL_FRAME), !deleted);
        gtk_widget_set_sensitive(GET_WIDGET (repository_view->xml, SYNC_FRAME), !deleted);
        gtk_widget_set_sensitive(GET_WIDGET (repository_view->xml, UPDATE_ALL_PLAYLISTS_BUTTON), !deleted);
        gtk_widget_set_sensitive(GET_WIDGET (repository_view->xml, PLAYLIST_TAB_LABEL), !deleted);
        gtk_widget_set_sensitive(GET_WIDGET (repository_view->xml, PLAYLIST_TAB_CONTENTS), !deleted);
        gtk_widget_set_sensitive(GET_WIDGET (repository_view->xml, DELETE_REPOSITORY_BUTTON), !deleted);

        if (repository_view->playlist) {
            gboolean sens = FALSE;
            if (repository_view->playlist->is_spl) {
                sens = TRUE;
            }
            else {
                gint val;
                key = get_playlist_prefs_key(repository_view->itdb_index, repository_view->playlist, KEY_SYNCMODE);
                val = get_current_prefs_int(key);
                g_free(key);
                if (val != SYNC_PLAYLIST_MODE_NONE) {
                    sens = TRUE;
                }
                gtk_widget_set_sensitive(GET_WIDGET (repository_view->xml, SYNC_OPTIONS_HBOX), sens);

                key
                        = get_playlist_prefs_key(repository_view->itdb_index, repository_view->playlist, KEY_SYNC_DELETE_TRACKS);
                val = get_current_prefs_int(key);
                g_free(key);
                gtk_widget_set_sensitive(GET_WIDGET (repository_view->xml, PLAYLIST_SYNC_CONFIRM_DELETE_TOGGLE), val);
            }
            gtk_widget_set_sensitive(GET_WIDGET (repository_view->xml, UPDATE_PLAYLIST_BUTTON), sens);
        }
    }
    else { /* no itdb loaded */
        gtk_widget_set_sensitive(GET_WIDGET (repository_view->xml, REPOSITORY_VBOX), FALSE);
    }
}

/* ------------------------------------------------------------
 *
 *        Callback functions (buttons, entries...)
 *
 * ------------------------------------------------------------ */

/* Compare the value of @str with the value stored for @key in the
 prefs system. If values differ, store @str for @key in

 @repository_view->temp_prefs, otherwise remove a possibly existing entry
 @key in @repository_view->temp_prefs.

 Return value: TRUE if a new string was set, FALSE if no new string
 was set, or the new string was identical to the one stored in the
 prefs system. */

/* Attention: g_frees() @key and @str for you */
static gboolean finish_string_storage(gchar *key, gchar *str) {
    gchar *prefs_str;
    gboolean result;

    g_return_val_if_fail (repository_view && key && str, FALSE);

    prefs_str = prefs_get_string(key);

    if ((!prefs_str && (strlen(str) > 0)) || (prefs_str && (strcmp(str, prefs_str) != 0))) { /* value has changed with respect to the value stored in the
     prefs */
#       if LOCAL_DEBUG
        printf ("setting '%s' to '%s'\n", key, str);
#       endif
        temp_prefs_set_string(repository_view->temp_prefs, key, str);
        result = TRUE;
    }
    else { /* value has not changed -- remove key from temp prefs (in
     case it exists */
#       if LOCAL_DEBUG
        printf ("removing '%s'.\n", key);
#       endif
        temp_prefs_remove_key(repository_view->temp_prefs, key);
        result = FALSE;
    }
    update_buttons(repository_view);
    g_free(key);
    g_free(str);
    g_free(prefs_str);
    return result;
}

/* Retrieve the current text in @editable and call
 finish_string_storage()

 Return value: see finish_string_storage() */
static gboolean finish_editable_storage(gchar *key, GtkEditable *editable) {
    gchar *str;

    g_return_val_if_fail (repository_view && key && editable, FALSE);

    str = gtk_editable_get_chars(editable, 0, -1);
    return finish_string_storage(key, str);
}

/* Compare the value of @val with the value stored for @key in the
 prefs system. If values differ, store @val for @key in
 @repository_view->temp_prefs, otherwise remove a possibly existing entry
 @key in @repository_view->temp_prefs. */
static void finish_int_storage(gchar *key, gint val) {
    gint prefs_val;

    g_return_if_fail (repository_view && key);

    /* defaults to '0' if not set */
    prefs_val = prefs_get_int(key);
    if (prefs_val != val) { /* value has changed with respect to the value stored in the
     prefs */
#       if LOCAL_DEBUG
        printf ("setting '%s' to '%d'\n", key, val);
#       endif
        temp_prefs_set_int(repository_view->temp_prefs, key, val);
    }
    else { /* value has not changed -- remove key from temp prefs (in
     case it exists */
#       if LOCAL_DEBUG
        printf ("removing '%s'.\n", key);
#       endif
        temp_prefs_remove_key(repository_view->temp_prefs, key);
    }
    update_buttons();
}

/* text in standard text entry has changed */
static void standard_itdb_entry_changed(GtkEditable *editable) {
    const gchar *keybase;
    gchar *key;

    g_return_if_fail (repository_view);

    keybase = g_object_get_data(G_OBJECT (editable), "key");
    g_return_if_fail (keybase);

    key = get_itdb_prefs_key(repository_view->itdb_index, keybase);

    finish_editable_storage(key, editable);
}

static void standard_itdb_chooser_button_updated (GtkWidget *chooser, gpointer user_data) {
    const gchar *keybase;
    gchar *key;

    g_return_if_fail (repository_view);

    keybase = g_object_get_data(G_OBJECT (chooser), "key");
    g_return_if_fail (keybase);

    key = get_itdb_prefs_key(repository_view->itdb_index, keybase);

    gchar *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(chooser));
    if (! filename)
        return;

    finish_string_storage(key, filename);
}

static void standard_playlist_chooser_button_updated (GtkWidget *chooser, gpointer user_data) {
    const gchar *keybase;
    gchar *key;

    g_return_if_fail (repository_view);

    keybase = g_object_get_data(G_OBJECT (chooser), "key");
    g_return_if_fail (keybase);

    key = get_playlist_prefs_key(repository_view->itdb_index, repository_view->playlist, KEY_MANUAL_SYNCDIR);

    gchar *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(chooser));
    if (! filename)
        return;

    g_warning("file %s", filename);
    finish_string_storage(key, filename);
}

/* sync_playlist_mode_none was toggled */
static void sync_playlist_mode_none_toggled(GtkToggleButton *togglebutton) {
    gchar *key;

    g_return_if_fail (repository_view);

    key = get_playlist_prefs_key(repository_view->itdb_index, repository_view->playlist, KEY_SYNCMODE);

    if (gtk_toggle_button_get_active(togglebutton)) {
        finish_int_storage(key, SYNC_PLAYLIST_MODE_NONE);
        gtk_widget_set_sensitive(GET_WIDGET (repository_view->xml, MANUAL_SYNCDIR_CHOOSER), FALSE);
        update_buttons();
    }

    g_free(key);
}

/* sync_playlist_mode_none was toggled */
static void sync_playlist_mode_manual_toggled(GtkToggleButton *togglebutton) {
    gchar *key;

    g_return_if_fail (repository_view);

    key = get_playlist_prefs_key(repository_view->itdb_index, repository_view->playlist, KEY_SYNCMODE);

    if (gtk_toggle_button_get_active(togglebutton)) {
        finish_int_storage(key, SYNC_PLAYLIST_MODE_MANUAL);
        gtk_widget_set_sensitive(GET_WIDGET (repository_view->xml, MANUAL_SYNCDIR_CHOOSER), TRUE);
        update_buttons(repository_view);
    }

    g_free(key);
}

/* sync_playlist_mode_none was toggled */
static void sync_playlist_mode_automatic_toggled(GtkToggleButton *togglebutton) {
    gchar *key;

    g_return_if_fail (repository_view);

    key = get_playlist_prefs_key(repository_view->itdb_index, repository_view->playlist, KEY_SYNCMODE);

    if (gtk_toggle_button_get_active(togglebutton)) {
        finish_int_storage(key, SYNC_PLAYLIST_MODE_AUTOMATIC);
        gtk_widget_set_sensitive(GET_WIDGET (repository_view->xml, MANUAL_SYNCDIR_CHOOSER), FALSE);
        update_buttons(repository_view);
    }

    g_free(key);
}

static void standard_itdb_checkbutton_toggled(GtkToggleButton *togglebutton) {
    const gchar *keybase;
    gchar *key;

    g_return_if_fail (repository_view);

    keybase = g_object_get_data(G_OBJECT (togglebutton), "key");
    g_return_if_fail (keybase);
    key = get_itdb_prefs_key(repository_view->itdb_index, keybase);
    finish_int_storage(key, gtk_toggle_button_get_active(togglebutton));

    g_free(key);
}

static void standard_playlist_checkbutton_toggled(GtkToggleButton *togglebutton) {
    const gchar *keybase;
    gboolean active;
    gchar *key;

    g_return_if_fail (repository_view);
    g_return_if_fail (repository_view->playlist);

    keybase = g_object_get_data(G_OBJECT (togglebutton), "key");
    g_return_if_fail (keybase);
    key = get_playlist_prefs_key(repository_view->itdb_index, repository_view->playlist, keybase);
    active = gtk_toggle_button_get_active(togglebutton);

    /* Check if this is the liveupdate toggle which needs special
     * treatment. */
    if (strcmp(keybase, KEY_LIVEUPDATE) == 0) {
        if (active == repository_view->playlist->splpref.liveupdate)
            temp_prefs_remove_key(repository_view->extra_prefs, key);
        else
            temp_prefs_set_int(repository_view->extra_prefs, key, active);

        update_buttons(repository_view);
        g_free(key);
        return;
    }

    finish_int_storage(key, active);
    g_free(key);
}

/* delete_repository_button was clicked */
static void delete_repository_button_clicked(GtkButton *button) {
    Playlist *mpl;
    gchar *message;
    gchar *key;
    gint response;

    g_return_if_fail (repository_view);
    mpl = itdb_playlist_mpl(repository_view->itdb);
    message
            = g_strdup_printf(_("Are you sure you want to delete repository \"%s\"? This action cannot be undone!"), mpl->name);

    response = gtkpod_confirmation_simple(GTK_MESSAGE_WARNING, _("Delete repository?"), message, GTK_STOCK_DELETE);

    g_free(message);

    if (response == GTK_RESPONSE_CANCEL)
        return;

    key = get_itdb_prefs_key(repository_view->itdb_index, "deleted");

    temp_prefs_set_int(repository_view->extra_prefs, key, TRUE);
    g_free(key);
    update_buttons(repository_view);
}

/* new repository button was clicked */
static void new_repository_button_clicked(GtkButton *button) {
    g_return_if_fail (repository_view);

    display_create_repository_dialog();
}

/* ------------------------------------------------------------
 *
 *        Callback functions (windows control)
 *
 * ------------------------------------------------------------ */

static void edit_apply_clicked(GtkButton *button) {
    gint i, itdb_num, del_num;
    struct itdbs_head *itdbs_head;
    GList *deaditdbs = NULL;
    GList *gl = NULL;

    g_return_if_fail (repository_view);

    itdbs_head = gp_get_itdbs_head();
    g_return_if_fail (itdbs_head);
    itdb_num = g_list_length(itdbs_head->itdbs);

    temp_prefs_apply(repository_view->temp_prefs);

    del_num = 0;
    for (i = 0; i < itdb_num; ++i) {
        gchar *key, *subkey;
        gboolean deleted = FALSE;

        iTunesDB *itdb = g_list_nth_data(itdbs_head->itdbs, i - del_num);
        g_return_if_fail (itdb);

        subkey = get_itdb_prefs_key(i, "");

        if (temp_prefs_subkey_exists(repository_view->extra_prefs, subkey)) {
            gboolean deleted;
            GList *gl;

            key = get_itdb_prefs_key(i, "deleted");
            deleted = temp_prefs_get_int(repository_view->extra_prefs, key);
            g_free(key);
            if (deleted) {
                iTunesDB *itdb;
                gint j;

                /* flush all keys relating to the deleted itdb */
                key = get_itdb_prefs_key(i - del_num, "");
                prefs_flush_subkey(key);
                g_free(key);

                for (j = i - del_num; j < itdb_num - del_num - 1; ++j) {
                    gchar *from_key = get_itdb_prefs_key(j + 1, "");
                    gchar *to_key = get_itdb_prefs_key(j, "");
                    prefs_rename_subkey(from_key, to_key);
                    g_free(from_key);
                    g_free(to_key);
                }

                itdb = g_list_nth_data(itdbs_head->itdbs, i - del_num);
                gp_itdb_remove(itdb);
                deaditdbs = g_list_append(deaditdbs, itdb);

                /* keep itdb_index of currently displayed repository
                 updated in case we need to select a new one */
                if (repository_view->itdb_index > i - del_num) {
                    --repository_view->itdb_index;
                }
                ++del_num;
            }

            if (!deleted) {
                /* apply the "live update flag", which is kept inside
                 the playlist struct */
                for (gl = itdb->playlists; gl; gl = gl->next) {
                    Playlist *pl = gl->data;
                    gint val;
                    g_return_if_fail (pl);
                    key = get_playlist_prefs_key(i, pl, KEY_LIVEUPDATE);
                    if (temp_prefs_get_int_value(repository_view->extra_prefs, key, &val)) {
                        pl->splpref.liveupdate = val;
                        data_changed(itdb);
                    }
                    g_free(key);
                }
            }
        }

        if (!deleted && temp_prefs_subkey_exists(repository_view->temp_prefs, subkey)) {
            gchar *str;
            /* check if mountpoint has changed */
            key = get_itdb_prefs_key(i, KEY_MOUNTPOINT);
            str = temp_prefs_get_string(repository_view->temp_prefs, key);
            g_free(key);
            if (str) { /* have to set mountpoint */
                itdb_set_mountpoint(itdb, str);
                g_free(str);
            }

            /* check if model_number has changed */
            key = get_itdb_prefs_key(i, KEY_IPOD_MODEL);
            str = temp_prefs_get_string(repository_view->temp_prefs, key);
            g_free(key);
            if (str) { /* set model */
                if (itdb->usertype && GP_ITDB_TYPE_IPOD) {
                    itdb_device_set_sysinfo(itdb->device, "ModelNumStr", str);
                }
                g_free(str);
            }

            /* this repository was changed */
            data_changed(itdb);
        }
        g_free(subkey);
    }

    temp_prefs_destroy(repository_view->temp_prefs);
    temp_prefs_destroy(repository_view->extra_prefs);

    repository_view->temp_prefs = temp_prefs_create();
    repository_view->extra_prefs = temp_prefs_create();

    if (g_list_length(itdbs_head->itdbs) < itdb_num) { /* at least one repository has been removed */
        iTunesDB *new_itdb = g_list_nth_data(itdbs_head->itdbs, repository_view->itdb_index);
        iTunesDB *old_itdb = repository_view->itdb;
        Playlist *old_playlist = repository_view->playlist;

        init_repository_combo(repository_view);
        if (new_itdb == old_itdb) {
            select_repository(new_itdb, old_playlist);
        }
        else {
            select_repository(new_itdb, NULL);
        }
    }
#   if LOCAL_DEBUG
    printf ("index: %d\n", repository_view->itdb_index);
#   endif

    update_buttons(repository_view);

    /*
     * Free the deleted itdbs. Need to do this at the end
     * due to the combo boxes having to be re-initialised first.
     */
    for (gl = deaditdbs; gl; gl = gl->next) {
        gp_itdb_free(gl->data);
    }
    gl = NULL;
    g_list_free(deaditdbs);
}

static void ipod_sync_button_clicked(iPodSyncType type) {
    const gchar *title;
    const gchar *entry;
    gchar *text, *key, *oldpath, *newpath;

    g_return_if_fail (repository_view);

    switch (type) {
    case IPOD_SYNC_CONTACTS:
        title = _("Please select command to sync contacts");
        entry = IPOD_SYNC_CONTACTS_ENTRY;
        key = get_itdb_prefs_key(repository_view->itdb_index, KEY_PATH_SYNC_CONTACTS);
        break;
    case IPOD_SYNC_CALENDAR:
        title = _("Please select command to sync calendar");
        entry = IPOD_SYNC_CALENDAR_ENTRY;
        key = get_itdb_prefs_key(repository_view->itdb_index, KEY_PATH_SYNC_CALENDAR);
        break;
    case IPOD_SYNC_NOTES:
        title = _("Please select command to sync notes");
        entry = IPOD_SYNC_NOTES_ENTRY;
        key = get_itdb_prefs_key(repository_view->itdb_index, KEY_PATH_SYNC_NOTES);
        break;
    default:
        g_return_if_reached ();
    }

    oldpath = prefs_get_string(key);
    g_free(key);

    text
            = g_markup_printf_escaped(_("<i>Have a look at the scripts provided in '%s'. If you write a new script or improve an existing one, please send it to jcsjcs at users.sourceforge.net for inclusion into the next release.</i>"), get_script_dir());

    newpath = fileselection_select_script(oldpath, get_script_dir(), title, text);
    g_free(oldpath);
    g_free(text);

    if (newpath) {
        gtk_entry_set_text(GTK_ENTRY (GET_WIDGET (repository_view->xml, entry)), newpath);
        g_free(newpath);
    }
}

/* Callback */
static void ipod_sync_contacts_button_clicked(GtkButton *button) {
    ipod_sync_button_clicked(IPOD_SYNC_CONTACTS);
}

/* Callback */
static void ipod_sync_calendar_button_clicked(GtkButton *button) {
    ipod_sync_button_clicked(IPOD_SYNC_CALENDAR);

}

/* Callback */
static void ipod_sync_notes_button_clicked(GtkButton *button) {
    ipod_sync_button_clicked(IPOD_SYNC_NOTES);

}

/**
 * sync_or_update_playlist:
 *
 * Sync (normal playlist) or update (spl) @playlist (in repository
 * @itdb_index) using the currently displayed options.
 */
static void sync_or_update_playlist(Playlist *playlist) {
    g_return_if_fail (repository_view);
    g_return_if_fail (playlist);

    gint itdb_index = repository_view->itdb_index;

    if (playlist->is_spl) {
        itdb_spl_update(playlist);

        if (gtkpod_get_current_playlist() == playlist) { /* redisplay */
            gtkpod_set_current_playlist(playlist);
        }

        gtkpod_statusbar_message(_("Smart playlist updated."));
    }
    else {
        gchar *key_sync_delete_tracks, *key_sync_confirm_delete;
        gchar *key_sync_show_summary, *key_manual_sync_dir, *key_syncmode;
        gchar *sync_delete_tracks_orig, *sync_confirm_delete_orig;
        gchar *sync_show_summary_orig;
        gint sync_delete_tracks_current, sync_confirm_delete_current;
        gint sync_show_summary_current;
        gchar *manual_sync_dir = NULL;
        gint value_new;

        /* create needed prefs keys */
        key_sync_delete_tracks = get_playlist_prefs_key(itdb_index, playlist, KEY_SYNC_DELETE_TRACKS);
        key_sync_confirm_delete = get_playlist_prefs_key(itdb_index, playlist, KEY_SYNC_CONFIRM_DELETE);
        key_sync_show_summary = get_playlist_prefs_key(itdb_index, playlist, KEY_SYNC_SHOW_SUMMARY);
        key_manual_sync_dir = get_playlist_prefs_key(itdb_index, playlist, KEY_MANUAL_SYNCDIR);
        key_syncmode = get_playlist_prefs_key(itdb_index, playlist, KEY_SYNCMODE);

        /* retrieve original settings for prefs strings */
        sync_delete_tracks_orig = prefs_get_string(key_sync_delete_tracks);
        sync_confirm_delete_orig = prefs_get_string(key_sync_confirm_delete);
        sync_show_summary_orig = prefs_get_string(key_sync_show_summary);

        /* retrieve current settings for prefs_strings */
        sync_delete_tracks_current = get_current_prefs_int(key_sync_delete_tracks);
        sync_confirm_delete_current = get_current_prefs_int(key_sync_confirm_delete);
        sync_show_summary_current = get_current_prefs_int(key_sync_show_summary);

        /* temporarily apply current settings */
        prefs_set_int(key_sync_delete_tracks, sync_delete_tracks_current);
        prefs_set_int(key_sync_confirm_delete, sync_confirm_delete_current);
        prefs_set_int(key_sync_show_summary, sync_show_summary_current);

        /* sync directory or directories */
        switch (get_current_prefs_int(key_syncmode)) {
        case SYNC_PLAYLIST_MODE_NONE:
            break; /* should never happen */
        case SYNC_PLAYLIST_MODE_MANUAL:
            manual_sync_dir = get_current_prefs_string(key_manual_sync_dir);
            /* no break;! we continue calling sync_playlist() */
        case SYNC_PLAYLIST_MODE_AUTOMATIC:
            sync_playlist(playlist, manual_sync_dir, NULL, FALSE, key_sync_delete_tracks, 0, key_sync_confirm_delete, 0, NULL, sync_show_summary_current);
            break;
        }

        /* Update temporary prefs in case some settings were changed
         * (currently only 'key_sync_confirm_delete' can be changed
         * by sync_playlist()) */
        value_new = prefs_get_int(key_sync_confirm_delete);
        if (value_new != sync_confirm_delete_current) {
            if (playlist == repository_view->playlist) { /* currently displayed --> adjust toggle button */
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (GET_WIDGET (repository_view->xml, PLAYLIST_SYNC_CONFIRM_DELETE_TOGGLE)), value_new);
            }
            else { /* not currently displayed --> copy to temp_prefs */
                temp_prefs_set_int(repository_view->temp_prefs, key_sync_confirm_delete, value_new);
            }
        }

        /* Copy original values back to prefs */
        prefs_set_string(key_sync_delete_tracks, sync_delete_tracks_orig);
        prefs_set_string(key_sync_confirm_delete, sync_confirm_delete_orig);
        prefs_set_string(key_sync_show_summary, sync_show_summary_orig);

        /* Free memory used by all strings */
        g_free(key_sync_delete_tracks);
        g_free(key_sync_confirm_delete);
        g_free(key_sync_show_summary);
        g_free(key_manual_sync_dir);
        g_free(sync_delete_tracks_orig);
        g_free(sync_confirm_delete_orig);
        g_free(sync_show_summary_orig);
        g_free(manual_sync_dir);
    }
}

/* Callback */
static void update_all_playlists_button_clicked(GtkButton *button) {
    GList *gl;

    g_return_if_fail (repository_view);
    g_return_if_fail (repository_view->itdb);

    for (gl = repository_view->itdb->playlists; gl; gl = gl->next) {
        Playlist *pl = gl->data;
        g_return_if_fail (pl);
        sync_or_update_playlist(pl);
    }
}

/* Callback */
static void update_playlist_button_clicked(GtkButton *button) {
    g_return_if_fail (repository_view);
    sync_or_update_playlist(repository_view->playlist);
}

/* Used by select_playlist() to find the new playlist. If found, select it */
static gboolean select_playlist_find(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data) {
    Playlist *playlist;
    g_return_val_if_fail (repository_view, TRUE);

    gtk_tree_model_get(model, iter, 0, &playlist, -1);
    if (playlist == repository_view->next_playlist) {
        gtk_combo_box_set_active_iter(GTK_COMBO_BOX (gtkpod_xml_get_widget (repository_view->xml,
                        PLAYLIST_COMBO)), iter);
        return TRUE;
    }
    return FALSE;
}

/* Select @playlist

 If @playlist == NULL, select first playlist (MPL);
 */
static void select_playlist(Playlist *playlist) {
    GtkTreeModel *model;

    g_return_if_fail (repository_view);
    g_return_if_fail (repository_view->itdb);

    if (!playlist)
        playlist = itdb_playlist_mpl(repository_view->itdb);
    g_return_if_fail (playlist);

    g_return_if_fail (playlist->itdb == repository_view->itdb);

    model = gtk_combo_box_get_model(GTK_COMBO_BOX (gtkpod_xml_get_widget (repository_view->xml,
                    PLAYLIST_COMBO)));
    g_return_if_fail (model);

    repository_view->next_playlist = playlist;
    gtk_tree_model_foreach(model, select_playlist_find, repository_view);
    repository_view->next_playlist = NULL;
}

/* Select @itdb and playlist @playlist

 If @itdb == NULL, only change to @playlist.

 If @playlist == NULL, select first playlist.
 */
static void select_repository(iTunesDB *itdb, Playlist *playlist) {
    g_return_if_fail (repository_view);

    if (repository_view->itdb != itdb) {
        gint index;
        repository_view->next_playlist = playlist;
        index = get_itdb_index(itdb);
        gtk_combo_box_set_active(GTK_COMBO_BOX (gtkpod_xml_get_widget (repository_view->xml,
                        REPOSITORY_COMBO)), index);
    }
    else {
        if (repository_view->itdb)
            select_playlist(playlist);
    }
}

/* set @widget with value of @key */
static void set_widget_index(gint itdb_index, const gchar *subkey, const gchar *name) {
    gchar *buf;
    gchar *key;
    GtkWidget *w;

    g_return_if_fail (repository_view && subkey && name);

    key = get_itdb_prefs_key(itdb_index, subkey);

    buf = get_current_prefs_string(key);
    w = GET_WIDGET (repository_view->xml, name);

    if (buf) {
        if (GTK_IS_ENTRY(w))
            gtk_entry_set_text(GTK_ENTRY(w), buf);
        else if (GTK_IS_FILE_CHOOSER(w)) {
            if (g_file_test(buf, G_FILE_TEST_IS_DIR))
                gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(w), buf);
            else
                gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(w), buf);
        }
    }
    else {
        if (GTK_IS_ENTRY(w))
            gtk_entry_set_text(GTK_ENTRY(w), "");
        else if (GTK_IS_FILE_CHOOSER(w))
            gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(w), "");
    }
    g_free(buf);
    g_free(key);
}

/****** Fill in info about selected repository *****/
static void display_repository_info() {
    iTunesDB *itdb;
    gint index;
    gchar *buf, *key;

    g_return_if_fail (repository_view);
    g_return_if_fail (repository_view->itdb);

    itdb = repository_view->itdb;
    index = repository_view->itdb_index;

    /* Repository type */
    if (itdb->usertype & GP_ITDB_TYPE_IPOD) {
        buf = g_markup_printf_escaped("<i>%s</i>", _("iPod"));
    }
    else if (itdb->usertype & GP_ITDB_TYPE_PODCASTS) {
        buf = g_markup_printf_escaped("<i>%s</i>", _("Podcasts Repository"));
    }
    else if (itdb->usertype & GP_ITDB_TYPE_LOCAL) {
        buf = g_markup_printf_escaped("<i>%s</i>", _("Local Repository"));
    }
    else {
        buf = g_markup_printf_escaped("<b>Unknown -- please report bug</b>");
    }
    gtk_label_set_markup(GTK_LABEL(GET_WIDGET (repository_view->xml, REPOSITORY_TYPE_LABEL)), buf);
    g_free(buf);

    /* Hide/show corresponding widgets in table */
    if (itdb->usertype & GP_ITDB_TYPE_IPOD) {
        const gchar *widgets_show[] =
            {
                MOUNTPOINT_LABEL,
                MOUNTPOINT_CHOOSER,
                BACKUP_LABEL,
                BACKUP_CHOOSER,
                IPOD_MODEL_LABEL,
                IPOD_MODEL_COMBO,
                LOCAL_PATH_CHOOSER,
                SYNC_FRAME,
                NULL };
        const gchar *widgets_hide[] =
            {
                LOCAL_PATH_LABEL,
                LOCAL_PATH_CHOOSER,
                NULL
            };
        const gchar **widget;

        for (widget = widgets_show; *widget; ++widget) {
            gtk_widget_show(GET_WIDGET (repository_view->xml, *widget));
        }
        for (widget = widgets_hide; *widget; ++widget) {
            gtk_widget_hide(GET_WIDGET (repository_view->xml, *widget));
        }

        set_widget_index(index, KEY_MOUNTPOINT, MOUNTPOINT_CHOOSER);

        set_widget_index(index, KEY_FILENAME, BACKUP_CHOOSER);

        set_widget_index(index, KEY_PATH_SYNC_CONTACTS, IPOD_SYNC_CONTACTS_ENTRY);

        set_widget_index(index, KEY_PATH_SYNC_CALENDAR, IPOD_SYNC_CALENDAR_ENTRY);

        set_widget_index(index, KEY_PATH_SYNC_NOTES, IPOD_SYNC_NOTES_ENTRY);

        set_widget_index(index, KEY_IPOD_MODEL, IPOD_MODEL_ENTRY);

        key = get_itdb_prefs_key(index, KEY_CONCAL_AUTOSYNC);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (GET_WIDGET (repository_view->xml, IPOD_CONCAL_AUTOSYNC_TOGGLE)), get_current_prefs_int(key));
        g_free(key);
    }
    else if (itdb->usertype & GP_ITDB_TYPE_LOCAL) {
        const gchar *widgets_show[] =
            {
                LOCAL_PATH_LABEL,
                LOCAL_PATH_CHOOSER,
                NULL
            };
        const gchar *widgets_hide[] =
            {
                MOUNTPOINT_LABEL,
                MOUNTPOINT_CHOOSER,
                BACKUP_LABEL,
                BACKUP_CHOOSER,
                IPOD_MODEL_LABEL,
                IPOD_MODEL_COMBO,
                SYNC_FRAME,
                NULL
            };
        const gchar **widget;

        for (widget = widgets_show; *widget; ++widget) {
            gtk_widget_show(GET_WIDGET (repository_view->xml, *widget));
        }
        for (widget = widgets_hide; *widget; ++widget) {
            gtk_widget_hide(GET_WIDGET (repository_view->xml, *widget));
        }

        set_widget_index(index, KEY_FILENAME, LOCAL_PATH_CHOOSER);
    }
    else {
        g_return_if_reached ();
    }
}

/****** Fill in info about selected playlist *****/
static void display_playlist_info() {
    gchar *buf, *key;
    Playlist *playlist;
    iTunesDB *itdb;
    gint i, index;
    const gchar *widget_names[] =
        {
            PLAYLIST_SYNC_DELETE_TRACKS_TOGGLE,
            PLAYLIST_SYNC_CONFIRM_DELETE_TOGGLE,
            PLAYLIST_SYNC_SHOW_SUMMARY_TOGGLE,
            NULL
        };
    const gchar *key_names[] =
        {
            KEY_SYNC_DELETE_TRACKS,
            KEY_SYNC_CONFIRM_DELETE,
            KEY_SYNC_SHOW_SUMMARY,
            NULL
        };

    g_return_if_fail (repository_view);
    g_return_if_fail (repository_view->itdb);
    g_return_if_fail (repository_view->playlist);

    /* for convenience */
    itdb = repository_view->itdb;
    index = repository_view->itdb_index;
    playlist = repository_view->playlist;

    /* Playlist type */
    if (itdb_playlist_is_mpl(playlist)) {
        buf = g_markup_printf_escaped("<i>%s</i>", _("Master Playlist"));
    }
    else if (itdb_playlist_is_podcasts(playlist)) {
        buf = g_markup_printf_escaped("<i>%s</i>", _("Podcasts Playlist"));
    }
    else if (playlist->is_spl) {
        buf = g_markup_printf_escaped("<i>%s</i>", _("Smart Playlist"));
    }
    else {
        buf = g_markup_printf_escaped("<i>%s</i>", _("Regular Playlist"));
    }
    gtk_label_set_markup(GTK_LABEL(GET_WIDGET (repository_view->xml, PLAYLIST_TYPE_LABEL)), buf);
    g_free(buf);

    /* Hide/show corresponding widgets in table */
    if (playlist->is_spl) {
        gint liveupdate;

        gtk_widget_show(GET_WIDGET (repository_view->xml, PLAYLIST_SYNC_DELETE_TRACKS_TOGGLE));
        gtk_widget_hide(GET_WIDGET (repository_view->xml, STANDARD_PLAYLIST_VBOX));

        key = get_playlist_prefs_key(index, playlist, KEY_LIVEUPDATE);
        if (!temp_prefs_get_int_value(repository_view->extra_prefs, key, &liveupdate))
            liveupdate = playlist->splpref.liveupdate;
        g_free(key);

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (GET_WIDGET (repository_view->xml, SPL_LIVE_UPDATE_TOGGLE)), liveupdate);
    }
    else {
        gint syncmode;
        gtk_widget_show(GET_WIDGET (repository_view->xml, STANDARD_PLAYLIST_VBOX));

        key = get_playlist_prefs_key(index, playlist, KEY_SYNCMODE);
        syncmode = get_current_prefs_int(key);
        g_free(key);

        switch (syncmode) {
        case SYNC_PLAYLIST_MODE_NONE:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (GET_WIDGET (repository_view->xml, SYNC_PLAYLIST_MODE_NONE_RADIO)), TRUE);
            break;
        case SYNC_PLAYLIST_MODE_MANUAL:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (GET_WIDGET (repository_view->xml, SYNC_PLAYLIST_MODE_MANUAL_RADIO)), TRUE);
            /* Need to set manual_syncdir_entry here as it may set the
            syncmode to 'MANUAL' -- this will be corrected by setting
            the radio button with the original syncmode setting further
            down. */
            key = get_playlist_prefs_key(index, playlist, KEY_MANUAL_SYNCDIR);
            gchar *dir = get_current_prefs_string(key);
            gtk_file_chooser_set_filename(GTK_FILE_CHOOSER (GET_WIDGET (repository_view->xml, MANUAL_SYNCDIR_CHOOSER)), dir);
            g_free(key);
            g_free(dir);
            break;
        case SYNC_PLAYLIST_MODE_AUTOMATIC:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (GET_WIDGET (repository_view->xml, SYNC_PLAYLIST_MODE_AUTOMATIC_RADIO)), TRUE);
            break;
        default:
            /* repair broken prefs */
            prefs_set_int(key, SYNC_PLAYLIST_MODE_NONE);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (GET_WIDGET (repository_view->xml, SYNC_PLAYLIST_MODE_NONE_RADIO)), TRUE);
            break;
        }
        /* make options available where appropriate */
        gtk_widget_set_sensitive(GET_WIDGET (repository_view->xml, SYNC_OPTIONS_HBOX), syncmode
                != SYNC_PLAYLIST_MODE_NONE);
        /* set standard toggle buttons */
        for (i = 0; widget_names[i]; ++i) {
            key = get_playlist_prefs_key(index, playlist, key_names[i]);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (GET_WIDGET (repository_view->xml, widget_names[i])), get_current_prefs_int(key));
            if (strcmp(key_names[i], KEY_SYNC_DELETE_TRACKS) == 0) {
                gtk_widget_set_sensitive(GET_WIDGET (repository_view->xml, PLAYLIST_SYNC_CONFIRM_DELETE_TOGGLE), get_current_prefs_int(key));
            }
            g_free(key);
        }
    }
}

/****** Initialize the repository combo *****/
static void init_repository_combo() {

    g_return_if_fail (repository_view);

    if (!repository_view->repository_combo_box) {
        repository_view->repository_combo_box = GTK_COMBO_BOX (GET_WIDGET (repository_view->xml, REPOSITORY_COMBO));
    }

    repository_combo_populate(repository_view->repository_combo_box);

    if (g_object_get_data(G_OBJECT (repository_view->repository_combo_box), "combo_set") == NULL) { /* the combo has not yet been initialized */
        g_signal_connect (repository_view->repository_combo_box, "changed", G_CALLBACK (repository_combo_changed_cb), NULL);
        g_object_set_data(G_OBJECT (repository_view->repository_combo_box), "combo_set", "set");
    }

    repository_view->itdb = NULL;
    repository_view->playlist = NULL;
}

/****** Initialize the playlist combo *****/
static void init_playlist_combo() {
    GtkCellRenderer *cell;
    GtkListStore *store;
    GList *gl;

    g_return_if_fail (repository_view);
    g_return_if_fail (repository_view->itdb);

    if (!repository_view->playlist_combo_box) {
        repository_view->playlist_combo_box = GTK_COMBO_BOX (gtkpod_xml_get_widget (repository_view->xml,
                        PLAYLIST_COMBO));
    }

    if (g_object_get_data(G_OBJECT (repository_view->playlist_combo_box), "combo_set") == NULL) {
        /* Cell for graphic indicator */
        cell = gtk_cell_renderer_pixbuf_new();
        gtk_cell_layout_pack_start(GTK_CELL_LAYOUT (repository_view->playlist_combo_box), cell, FALSE);
        gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT (repository_view->playlist_combo_box), cell, playlist_cb_cell_data_func_pix, NULL, NULL);
        /* Cell for playlist name */
        cell = gtk_cell_renderer_text_new();
        gtk_cell_layout_pack_start(GTK_CELL_LAYOUT (repository_view->playlist_combo_box), cell, FALSE);
        gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT (repository_view->playlist_combo_box), cell, playlist_cb_cell_data_func_text, NULL, NULL);

        g_object_set(G_OBJECT (cell), "editable", FALSE, NULL);

        g_signal_connect (repository_view->playlist_combo_box, "changed", G_CALLBACK (playlist_combo_changed_cb), NULL);

        g_object_set_data(G_OBJECT (repository_view->playlist_combo_box), "combo_set", "set");
    }

    store = gtk_list_store_new(1, G_TYPE_POINTER);

    if (repository_view->itdb) {
        for (gl = repository_view->itdb->playlists; gl; gl = gl->next) {
            GtkTreeIter iter;
            Playlist *pl = gl->data;
            g_return_if_fail (pl);

            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, 0, pl, -1);
        }
    }

    gtk_combo_box_set_model(repository_view->playlist_combo_box, GTK_TREE_MODEL (store));
    g_object_unref(store);

    if (repository_view->itdb) {
        select_playlist(repository_view->next_playlist);
        repository_view->next_playlist = NULL;
    }
}

static void create_repository_editor_view() {
    GtkWidget *repo_window;
    GtkWidget *viewport;
    GtkComboBox *model_number_combo;
    gint i;

    repository_view = g_malloc0(sizeof(RepositoryView));

    gchar *glade_path = g_build_filename(get_glade_dir(), "repository_editor.glade", NULL);
    repository_view->xml = gtkpod_xml_new(glade_path, "repository_window");
    repo_window = gtkpod_xml_get_widget(repository_view->xml, "repository_window");
    viewport = gtkpod_xml_get_widget(repository_view->xml, "repository_viewport");
    g_free(glade_path);

    /* according to GTK FAQ: move a widget to a new parent */
    g_object_ref(viewport);
    gtk_container_remove(GTK_CONTAINER (repo_window), viewport);

    /* Add widget in Shell. Any number of widgets can be added */
    repository_editor_plugin->repo_window = gtk_scrolled_window_new(NULL, NULL);
    g_object_ref(repository_editor_plugin->repo_window);
    repository_editor_plugin->repo_view = viewport;
    g_object_ref(repository_editor_plugin->repo_view);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (repository_editor_plugin->repo_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW (repository_editor_plugin->repo_window), GTK_SHADOW_IN);

    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(repository_editor_plugin->repo_window), GTK_WIDGET (repository_editor_plugin->repo_view));
    anjuta_shell_add_widget(ANJUTA_PLUGIN(repository_editor_plugin)->shell, repository_editor_plugin->repo_window, "RepositoryEditorPlugin", _("  Edit iPod Repositories"), NULL, ANJUTA_SHELL_PLACEMENT_CENTER, NULL);

    repository_view->window = repository_editor_plugin->repo_window;

    /* we don't need these any more */
    g_object_unref(viewport);
    gtk_widget_destroy(repo_window);

    /* widget names and corresponding keys for 'standard' toggle
     options */
    const gchar *playlist_widget_names_toggle[] =
        {
            PLAYLIST_SYNC_DELETE_TRACKS_TOGGLE,
            PLAYLIST_SYNC_CONFIRM_DELETE_TOGGLE,
            PLAYLIST_SYNC_SHOW_SUMMARY_TOGGLE,
            SPL_LIVE_UPDATE_TOGGLE, NULL
        };
    const gchar *playlist_key_names_toggle[] =
        {
            KEY_SYNC_DELETE_TRACKS,
            KEY_SYNC_CONFIRM_DELETE,
            KEY_SYNC_SHOW_SUMMARY,
            KEY_LIVEUPDATE,
            NULL
        };
    const gchar *itdb_widget_names_toggle[] =
        {
            IPOD_CONCAL_AUTOSYNC_TOGGLE,
            NULL
        };
    const gchar *itdb_key_names_toggle[] =
        {
            KEY_CONCAL_AUTOSYNC,
            NULL
        };
    /* widget names and corresponding keys for 'standard' strings */
    const gchar *itdb_widget_names_entry[] =
        {
            MOUNTPOINT_CHOOSER,
            BACKUP_CHOOSER,
            IPOD_MODEL_ENTRY,
            LOCAL_PATH_CHOOSER,
            IPOD_SYNC_CONTACTS_ENTRY,
            IPOD_SYNC_CALENDAR_ENTRY,
            IPOD_SYNC_NOTES_ENTRY,
            NULL
        };
    const gchar *itdb_key_names_entry[] =
        {
            KEY_MOUNTPOINT,
            KEY_BACKUP,
            KEY_IPOD_MODEL,
            KEY_FILENAME,
            KEY_PATH_SYNC_CONTACTS,
            KEY_PATH_SYNC_CALENDAR,
            KEY_PATH_SYNC_NOTES,
            NULL
        };

    /* Setup model number combo */
    model_number_combo = GTK_COMBO_BOX (GET_WIDGET (repository_view->xml, IPOD_MODEL_COMBO));
    repository_init_model_number_combo(model_number_combo);

    /* connect standard text entries */
    for (i = 0; itdb_widget_names_entry[i]; ++i) {
        GtkWidget *w = GET_WIDGET (repository_view->xml, itdb_widget_names_entry[i]);

        if (GTK_IS_ENTRY(w)) {
            g_signal_connect (w, "changed",
                G_CALLBACK (standard_itdb_entry_changed),
                repository_view);
        } else if (GTK_IS_FILE_CHOOSER_BUTTON(w)) {
            g_signal_connect(w, "selection_changed",
                    G_CALLBACK (standard_itdb_chooser_button_updated),
                    repository_view);
        }

        g_object_set_data(G_OBJECT (w), "key", (gpointer) itdb_key_names_entry[i]);
    }

    /* Togglebutton callbacks */
    g_signal_connect (GET_WIDGET (repository_view->xml, SYNC_PLAYLIST_MODE_NONE_RADIO),
            "toggled",
            G_CALLBACK (sync_playlist_mode_none_toggled),
            repository_view);

    g_signal_connect (GET_WIDGET (repository_view->xml, SYNC_PLAYLIST_MODE_MANUAL_RADIO),
            "toggled",
            G_CALLBACK (sync_playlist_mode_manual_toggled),
            repository_view);

    g_signal_connect (GET_WIDGET (repository_view->xml, SYNC_PLAYLIST_MODE_AUTOMATIC_RADIO),
            "toggled",
            G_CALLBACK (sync_playlist_mode_automatic_toggled),
            repository_view);

    /* connect standard toggle buttons */
    for (i = 0; playlist_widget_names_toggle[i]; ++i) {
        GtkWidget *w = GET_WIDGET (repository_view->xml, playlist_widget_names_toggle[i]);

        g_signal_connect (w, "toggled",
                G_CALLBACK (standard_playlist_checkbutton_toggled),
                repository_view);
        g_object_set_data(G_OBJECT (w), "key", (gpointer) playlist_key_names_toggle[i]);
    }

    for (i = 0; itdb_widget_names_toggle[i]; ++i) {
        GtkWidget *w = GET_WIDGET (repository_view->xml, itdb_widget_names_toggle[i]);

        g_signal_connect (w, "toggled",
                G_CALLBACK (standard_itdb_checkbutton_toggled),
                repository_view);
        g_object_set_data(G_OBJECT (w), "key", (gpointer) itdb_key_names_toggle[i]);
    }

    /* Button callbacks */
    g_signal_connect (GET_WIDGET (repository_view->xml, DELETE_REPOSITORY_BUTTON), "clicked",
            G_CALLBACK (delete_repository_button_clicked), repository_view);

    g_signal_connect (GET_WIDGET (repository_view->xml, IPOD_SYNC_CONTACTS_BUTTON), "clicked",
            G_CALLBACK (ipod_sync_contacts_button_clicked), repository_view);

    g_signal_connect (GET_WIDGET (repository_view->xml, IPOD_SYNC_CALENDAR_BUTTON), "clicked",
            G_CALLBACK (ipod_sync_calendar_button_clicked), repository_view);

    g_signal_connect (GET_WIDGET (repository_view->xml, IPOD_SYNC_NOTES_BUTTON), "clicked",
            G_CALLBACK (ipod_sync_notes_button_clicked), repository_view);

    g_signal_connect (GET_WIDGET (repository_view->xml, UPDATE_PLAYLIST_BUTTON), "clicked",
            G_CALLBACK (update_playlist_button_clicked), repository_view);

    g_signal_connect (GET_WIDGET (repository_view->xml, UPDATE_ALL_PLAYLISTS_BUTTON), "clicked",
            G_CALLBACK (update_all_playlists_button_clicked), repository_view);

    g_signal_connect (GET_WIDGET (repository_view->xml, NEW_REPOSITORY_BUTTON), "clicked",
            G_CALLBACK (new_repository_button_clicked), repository_view);

    g_signal_connect (GET_WIDGET (repository_view->xml, APPLY_BUTTON), "clicked",
            G_CALLBACK (edit_apply_clicked), repository_view);

    g_signal_connect (GET_WIDGET (repository_view->xml, MANUAL_SYNCDIR_CHOOSER), "selection_changed",
            G_CALLBACK (standard_playlist_chooser_button_updated), repository_view);

    init_repository_combo();

    /* Set up temp_prefs struct */
    repository_view->temp_prefs = temp_prefs_create();
    repository_view->extra_prefs = temp_prefs_create();

    g_signal_connect (gtkpod_app, SIGNAL_PLAYLIST_SELECTED, G_CALLBACK (repository_playlist_selected_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_PLAYLIST_ADDED, G_CALLBACK (repository_playlist_changed_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_PLAYLIST_REMOVED, G_CALLBACK (repository_playlist_changed_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_ITDB_UPDATED, G_CALLBACK (repository_update_itdb_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_ITDB_ADDED, G_CALLBACK (repository_update_itdb_cb), NULL);
    g_signal_connect (gtkpod_app, SIGNAL_ITDB_REMOVED, G_CALLBACK (repository_update_itdb_cb), NULL);

}

void destroy_repository_editor() {
    if (!repository_view)
        return;

    /* Remove widgets from Shell */
    anjuta_shell_remove_widget(ANJUTA_PLUGIN(repository_editor_plugin)->shell, repository_editor_plugin->repo_window, NULL);

    g_object_unref(repository_view->xml);

    if (repository_view->window) {
        gtk_widget_destroy(repository_view->window);
        repository_view->window = NULL;
    }

    temp_prefs_destroy(repository_view->temp_prefs);
    temp_prefs_destroy(repository_view->extra_prefs);
    g_free(repository_view);
}

/**
 * open_repository_editor:
 *
 * Open the repository options window and display repository @itdb and
 * playlist @playlist.
 *
 * If @itdb and @playlist is NULL, display first repository and playlist.
 *
 * If @itdb is NULL and @playlist is not NULL, display @playlist and
 * assume repository is playlist->itdb.
 */
void open_repository_editor(iTunesDB *itdb, Playlist *playlist) {
    if (!repository_view || !repository_view->window)
        create_repository_editor_view();
    else
        gtkpod_display_widget(repository_view->window);

    if (!itdb && playlist)
        itdb = playlist->itdb;
    if (!itdb) {
        struct itdbs_head *itdbs_head = gp_get_itdbs_head();
        itdb = g_list_nth_data(itdbs_head->itdbs, 0);
    }
    g_return_if_fail (itdb);

    gtk_widget_show_all(repository_view->window);

    select_repository(itdb, playlist);
    display_repository_info();
    update_buttons();
}
